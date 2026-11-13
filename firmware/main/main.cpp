// Fruit classifier: press button -> USB camera takes a picture -> Edge Impulse model
// classifies it on-device -> result on the serial log and the RGB LED.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "board_config.h"
#include "camera.h"
#include "image_prep.h"

static const char *TAG = "fruit";

// Room for the decoded (downscaled) camera picture: 160x120 RGB888. 320x240 and 640x480
// cameras decode to exactly this; bigger modes are scaled by 1/8 at most.
static const size_t DECODE_BUFFER_BYTES = 160 * 120 * 3;
static const int DEBOUNCE_MS = 30;

static RgbImage s_picture;
static led_strip_handle_t s_led;

static int get_feature_data(size_t offset, size_t length, float *out)
{
    image_to_features(s_picture, EI_CLASSIFIER_INPUT_WIDTH, offset, length, out);
    return 0;
}

static void led_init(void)
{
    if (STATUS_LED_GPIO < 0) {
        return;
    }
    led_strip_config_t strip = {};
    strip.strip_gpio_num = STATUS_LED_GPIO;
    strip.max_leds = 1;
    led_strip_rmt_config_t rmt = {};
    rmt.resolution_hz = 10 * 1000 * 1000;
    if (led_strip_new_rmt_device(&strip, &rmt, &s_led) != ESP_OK) {
        ESP_LOGW(TAG, "status LED not available");
        s_led = NULL;
    }
}

static void led_set(uint8_t r, uint8_t g, uint8_t b)
{
    if (s_led) {
        led_strip_set_pixel(s_led, 0, r, g, b);
        led_strip_refresh(s_led);
    }
}

static void show_result(const char *label, float confidence)
{
    if (confidence < MIN_CONFIDENCE) {
        printf("RESULT: unsure (best guess %s, %.0f%%)\n", label, confidence * 100);
        led_set(20, 0, 20);      // purple
    } else if (strcmp(label, "Banan") == 0) {
        printf("RESULT: Banan (%.0f%%)\n", confidence * 100);
        led_set(40, 30, 0);      // yellow
    } else if (strcmp(label, "\xC3\x86" "ble") == 0) {  // "Æble" in UTF-8
        printf("RESULT: Aeble (%.0f%%)\n", confidence * 100);
        led_set(40, 0, 0);       // red
    } else {
        printf("RESULT: %s (%.0f%%)\n", label, confidence * 100);
        led_set(0, 0, 0);        // background: off
    }
}

static bool classify(void)
{
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
    signal.get_data = &get_feature_data;

    ei_impulse_result_t result = {};
    EI_IMPULSE_ERROR err = run_classifier(&signal, &result, false);
    if (err != EI_IMPULSE_OK) {
        ESP_LOGE(TAG, "run_classifier failed: %d", err);
        return false;
    }

    size_t best = 0;
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        printf("  %-12s %5.1f%%\n", result.classification[i].label, result.classification[i].value * 100);
        if (result.classification[i].value > result.classification[best].value) {
            best = i;
        }
    }
    ESP_LOGI(TAG, "inference took %d ms", result.timing.classification);
    show_result(result.classification[best].label, result.classification[best].value);
    return true;
}

static void wait_for_button_press(void)
{
    while (gpio_get_level((gpio_num_t)BUTTON_GPIO) == 0) {  // wait for release first
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
    }
    while (true) {
        if (gpio_get_level((gpio_num_t)BUTTON_GPIO) == 0) {
            vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
            if (gpio_get_level((gpio_num_t)BUTTON_GPIO) == 0) {
                return;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "fruit classifier starting, model input %dx%d, %d labels",
             EI_CLASSIFIER_INPUT_WIDTH, EI_CLASSIFIER_INPUT_HEIGHT, EI_CLASSIFIER_LABEL_COUNT);

    gpio_config_t button = {};
    button.pin_bit_mask = 1ULL << BUTTON_GPIO;
    button.mode = GPIO_MODE_INPUT;
    button.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&button));
    led_init();

    uint8_t *rgb = (uint8_t *)malloc(DECODE_BUFFER_BYTES);
    if (!rgb) {
        ESP_LOGE(TAG, "out of memory for picture buffer");
        abort();
    }
    ESP_ERROR_CHECK(camera_init());

    led_set(0, 0, 20);  // blue: waiting for camera
    camera_wait_until_ready();
    led_set(0, 20, 0);  // green: ready
    ESP_LOGI(TAG, "ready - press the button to classify (free heap %u bytes)",
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT));

    while (true) {
        wait_for_button_press();
        led_set(20, 20, 20);  // white: working
        int w = 0, h = 0;
        if (camera_capture_rgb(rgb, DECODE_BUFFER_BYTES, &w, &h) != ESP_OK) {
            printf("RESULT: error (camera)\n");
            led_set(40, 0, 40);  // magenta: camera error, press again to retry
            continue;
        }
        ESP_LOGI(TAG, "picture %dx%d", w, h);
        s_picture = {rgb, w, h};
        if (!classify()) {
            led_set(40, 0, 40);
        }
    }
}
