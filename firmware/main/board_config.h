#pragma once
// Pin assignments. Defaults are for the M5Stack AtomS3 Lite; change these for another ESP32-S3 board.
// The serial log pins are set in sdkconfig.defaults (CONFIG_ESP_CONSOLE_UART_TX/RX_GPIO).

#define BUTTON_GPIO       41   // AtomS3 Lite: the button on top of the screen-less case (active low)
#define STATUS_LED_GPIO   35   // AtomS3 Lite: single WS2812 RGB LED. Set to -1 if the board has none.

// Predictions below this confidence are reported as "unsure".
#define MIN_CONFIDENCE    0.60f
