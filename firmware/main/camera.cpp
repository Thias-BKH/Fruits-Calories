#include "camera.h"

#include <stdlib.h>
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "jpeg_decoder.h"
#include "usb/usb_host.h"
#include "usb/uvc_host.h"

static const char *TAG = "camera";

static const int USB_HOST_PRIORITY = 15;
static const int FRAME_BUFFERS = 2;
// Auto-exposure needs a few frames after the stream starts before the picture is usable.
static const int WARMUP_FRAMES = 8;
static const int FRAME_TIMEOUT_MS = 3000;
static const int OPEN_TIMEOUT_MS = 5000;
// Use the smallest MJPEG mode whose short side is at least this. The model is 96x96, so
// anything bigger only costs RAM (the AtomS3 Lite has no PSRAM).
static const unsigned MIN_SHORT_SIDE = 96;
// One compressed frame is ~1/6 of its raw YUY2 size; never go below this.
static const size_t MIN_FRAME_BUFFER = 32 * 1024;

static QueueHandle_t s_frames;               // uvc_host_frame_t* handed over by the driver
static SemaphoreHandle_t s_connected;        // given when a usable camera is plugged in
static volatile bool s_disconnected;
static uvc_host_frame_info_t s_mode;         // chosen resolution

static bool on_frame(const uvc_host_frame_t *frame, void *)
{
    // Queue the frame for camera_capture_rgb(), which gives it back with uvc_host_frame_return().
    // If the queue is full, returning true hands it straight back to the driver.
    return xQueueSend(s_frames, &frame, 0) != pdPASS;
}

static void on_stream_event(const uvc_host_stream_event_data_t *event, void *)
{
    switch (event->type) {
    case UVC_HOST_DEVICE_DISCONNECTED:
        ESP_LOGW(TAG, "camera unplugged");
        s_disconnected = true;  // camera_capture_rgb() closes the stream
        break;
    case UVC_HOST_TRANSFER_ERROR:
        ESP_LOGE(TAG, "USB transfer error %d", event->transfer_error.error);
        break;
    case UVC_HOST_FRAME_BUFFER_OVERFLOW:
        ESP_LOGW(TAG, "frame bigger than buffer, dropped");
        break;
    default:
        break;
    }
}

static bool pick_mode(uint8_t dev_addr, uint8_t stream_index, size_t count)
{
    uvc_host_frame_info_t *modes = (uvc_host_frame_info_t *)calloc(count, sizeof(*modes));
    if (!modes) {
        return false;
    }
    uvc_host_get_frame_list(dev_addr, stream_index, (uvc_host_frame_info_t (*)[])modes, &count);

    bool found = false;
    for (size_t i = 0; i < count; i++) {
        const uvc_host_frame_info_t &m = modes[i];
        ESP_LOGI(TAG, "camera mode %u: format %d %ux%u", (unsigned)i, m.format, m.h_res, m.v_res);
        const unsigned short_side = m.h_res < m.v_res ? m.h_res : m.v_res;
        if (m.format != UVC_VS_FORMAT_MJPEG || short_side < MIN_SHORT_SIDE) {
            continue;
        }
        if (!found || m.h_res * m.v_res < s_mode.h_res * s_mode.v_res) {
            s_mode = m;
            found = true;
        }
    }
    free(modes);
    return found;
}

static void on_driver_event(const uvc_host_driver_event_data_t *event, void *)
{
    if (event->type != UVC_HOST_DRIVER_EVENT_DEVICE_CONNECTED || event->device_connected.uvc_stream_index != 0) {
        return;
    }
    if (!pick_mode(event->device_connected.dev_addr, 0, event->device_connected.frame_info_num)) {
        ESP_LOGE(TAG, "camera has no MJPEG mode we can use");
        return;
    }
    ESP_LOGI(TAG, "camera connected, using MJPEG %ux%u", s_mode.h_res, s_mode.v_res);
    xSemaphoreGive(s_connected);
}

static void usb_lib_task(void *)
{
    while (true) {
        uint32_t flags;
        usb_host_lib_handle_events(portMAX_DELAY, &flags);
        if (flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
    }
}

esp_err_t camera_init(void)
{
    s_frames = xQueueCreate(FRAME_BUFFERS, sizeof(uvc_host_frame_t *));
    s_connected = xSemaphoreCreateBinary();
    if (!s_frames || !s_connected) {
        return ESP_ERR_NO_MEM;
    }

    usb_host_config_t host_config = {};
    host_config.intr_flags = ESP_INTR_FLAG_LOWMED;
    ESP_RETURN_ON_ERROR(usb_host_install(&host_config), TAG, "usb_host_install");
    if (xTaskCreate(usb_lib_task, "usb_lib", 4096, NULL, USB_HOST_PRIORITY, NULL) != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    uvc_host_driver_config_t uvc_config = {};
    uvc_config.driver_task_stack_size = 4096;
    uvc_config.driver_task_priority = USB_HOST_PRIORITY + 1;
    uvc_config.xCoreID = tskNO_AFFINITY;
    uvc_config.create_background_task = true;
    uvc_config.event_cb = on_driver_event;
    return uvc_host_install(&uvc_config);
}

void camera_wait_until_ready(void)
{
    ESP_LOGI(TAG, "waiting for USB camera...");
    xSemaphoreTake(s_connected, portMAX_DELAY);
}

static esp_err_t decode(const uvc_host_frame_t *frame, uint8_t *rgb, size_t capacity, int *width, int *height)
{
    esp_jpeg_image_cfg_t cfg = {};
    cfg.indata = frame->data;
    cfg.indata_size = frame->data_len;
    cfg.outbuf = rgb;
    cfg.outbuf_size = capacity;
    cfg.out_format = JPEG_IMAGE_FORMAT_RGB888;

    // Use the least downscaling that still fits the output buffer.
    const esp_jpeg_image_scale_t scales[] = {JPEG_IMAGE_SCALE_0, JPEG_IMAGE_SCALE_1_2,
                                             JPEG_IMAGE_SCALE_1_4, JPEG_IMAGE_SCALE_1_8};
    for (esp_jpeg_image_scale_t scale : scales) {
        cfg.out_scale = scale;
        esp_jpeg_image_cfg_t probe = cfg;  // the decoder keeps internal state in cfg; use a fresh copy per call
        esp_jpeg_image_output_t info;
        ESP_RETURN_ON_ERROR(esp_jpeg_get_image_info(&probe, &info), TAG, "not a valid JPEG frame");
        if (info.output_len > capacity) {
            continue;
        }
        esp_jpeg_image_cfg_t run = cfg;
        ESP_RETURN_ON_ERROR(esp_jpeg_decode(&run, &info), TAG, "JPEG decode failed");
        *width = info.width;
        *height = info.height;
        return ESP_OK;
    }
    ESP_LOGE(TAG, "frame too large to decode even at 1/8 scale");
    return ESP_ERR_NO_MEM;
}

static esp_err_t open_stream(uvc_host_stream_hdl_t *stream)
{
    const size_t compressed = s_mode.h_res * s_mode.v_res * 2 / 6;
    uvc_host_stream_config_t config = {
        .event_cb = on_stream_event,
        .frame_cb = on_frame,
        .user_ctx = NULL,
        .usb = {.dev_addr = UVC_HOST_ANY_DEV_ADDR, .vid = UVC_HOST_ANY_VID, .pid = UVC_HOST_ANY_PID, .uvc_stream_index = 0},
        .vs_format = {.h_res = s_mode.h_res, .v_res = s_mode.v_res, .fps = 0, .format = UVC_VS_FORMAT_MJPEG},
        .advanced = {
            .number_of_frame_buffers = FRAME_BUFFERS,
            .frame_size = compressed > MIN_FRAME_BUFFER ? compressed : MIN_FRAME_BUFFER,
            .frame_heap_caps = 0,
            .number_of_urbs = 3,
            .urb_size = 0,
            .user_frame_buffers = NULL,
        },
    };
    xQueueReset(s_frames);
    s_disconnected = false;
    return uvc_host_stream_open(&config, pdMS_TO_TICKS(OPEN_TIMEOUT_MS), stream);
}

static esp_err_t grab_and_decode(uvc_host_stream_hdl_t stream, uint8_t *rgb, size_t capacity, int *width, int *height)
{
    for (int i = 0; i <= WARMUP_FRAMES; i++) {
        uvc_host_frame_t *frame;
        if (xQueueReceive(s_frames, &frame, pdMS_TO_TICKS(FRAME_TIMEOUT_MS)) != pdPASS) {
            ESP_LOGE(TAG, "no frame from camera");
            return ESP_ERR_TIMEOUT;
        }
        esp_err_t err = ESP_OK;
        if (i == WARMUP_FRAMES) {
            err = decode(frame, rgb, capacity, width, height);
        }
        uvc_host_frame_return(stream, frame);
        if (i == WARMUP_FRAMES || s_disconnected) {
            return s_disconnected ? ESP_ERR_INVALID_STATE : err;
        }
    }
    return ESP_FAIL;  // unreachable
}

esp_err_t camera_capture_rgb(uint8_t *rgb, size_t rgb_capacity, int *width, int *height)
{
    // The stream (and its frame buffers) only exists during a capture, so that RAM is free
    // again while the model runs.
    uvc_host_stream_hdl_t stream = NULL;
    esp_err_t err = open_stream(&stream);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "could not open camera: %s", esp_err_to_name(err));
        return err;
    }
    err = uvc_host_stream_start(stream);
    if (err == ESP_OK) {
        err = grab_and_decode(stream, rgb, rgb_capacity, width, height);
        if (!s_disconnected) {
            uvc_host_stream_stop(stream);
        }
    }
    uvc_host_stream_close(stream);  // also frees any frames still queued
    return err;
}
