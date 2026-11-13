# Fruit classifier firmware (ESP32-S3 + USB camera)

Press the button → the USB webcam (XC-CAMERA or any UVC camera with MJPEG) takes a picture →
the Edge Impulse model (`Background` / `Banan` / `Æble`, 96×96 RGB) classifies it on the chip →
result on the serial log and the RGB LED. No Wi-Fi, no phone.

Written for the **M5Stack AtomS3 Lite** (ESP32-S3, 8 MB flash, no PSRAM). **Not tested on hardware yet.**

## Hardware you need

- The ESP32-S3 board. Its USB-C port becomes a **USB host** for the camera.
- A **USB-C OTG adapter** to plug the camera into the board.
- **5 V for the camera.** The board does not power devices on its USB port. Use a powered
  OTG hub / Y-cable, or feed 5 V into the cable's VBUS line yourself.
- A **3.3 V USB-UART adapter** to read the serial log, because the board's USB port is busy
  with the camera. Wire adapter RX → G1, adapter GND → GND (115200 baud). You can skip this
  and just watch the LED.

Pins are in `main/board_config.h` (button, LED) and `sdkconfig.defaults` (log TX/RX).

## LED

| Colour  | Meaning                              |
|---------|--------------------------------------|
| blue    | waiting for camera to be plugged in  |
| green   | ready, press the button              |
| white   | taking picture / classifying         |
| yellow  | Banan                                |
| red     | Æble                                 |
| off     | Background                           |
| purple  | unsure (confidence < 60%)            |
| magenta | camera error, press again            |

## Build and flash

With Docker, no install needed:

```sh
cd firmware
docker run --rm -v "$PWD":/project -w /project espressif/idf:v5.4.1 idf.py build
```

Flash it with [esptool](https://docs.espressif.com/projects/esptool/) (`pip install esptool`),
with the board on USB and **no camera attached**:

```sh
cd build && python -m esptool --chip esp32s3 write_flash @flash_args
```

Or install ESP-IDF v5.4 natively and run `idf.py build flash`.

**Reflashing:** once this firmware runs, the board's USB port is a host and no longer shows up
as a serial port. Hold the reset button for about 2 seconds until the LED turns green (download mode), then flash.

## Tests

The image crop/resize code is plain C++ and tested on the PC:

```sh
cd firmware/test && c++ -std=c++11 -I../main test_image_prep.cpp && ./a.out
```

## Updating the model

Export a new **Arduino library** from Edge Impulse, unzip it, and replace
`components/ei_model/src` with the zip's `src` folder. If the labels change, update
`show_result()` in `main/main.cpp`.

## Known risks (hardware not tested)

- The ESP32-S3 only does USB **full speed** (12 Mbit/s). Some high-resolution webcams only
  stream at high speed and won't work. The serial log lists the camera's modes on connect.
  The camera must offer an MJPEG mode of at least 96 px on its short side. The smallest one is used.
- The AtomS3 Lite pin numbers (button G41, LED G35, log on G1) were not checked on a real board.
