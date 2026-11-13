#pragma once
// USB (UVC) webcam on the ESP32-S3's native USB port, e.g. the XC-CAMERA.

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

// Installs the USB host + UVC driver. Call once at boot.
esp_err_t camera_init(void);

// Blocks until a camera with a usable MJPEG mode is plugged in.
void camera_wait_until_ready(void);

// Opens the camera, takes one picture, closes it again, and decodes it to RGB888 into `rgb` (capacity `rgb_capacity` bytes).
// The decoder downscales by 2/4/8 as needed to fit. Returns the decoded size.
esp_err_t camera_capture_rgb(uint8_t *rgb, size_t rgb_capacity, int *width, int *height);
