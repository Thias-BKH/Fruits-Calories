#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "touch_model.h"

// Hardwaredelen kalibrerer touchRead() og aflaeser slideren paa en separat kerne.
// Den tegner aldrig paa skaermen og gemmer aldrig app-data.
namespace TouchInput {
  static TouchSliderModel slider;
  static QueueHandle_t inputKoe = nullptr;
  static TaskHandle_t taskHandle = nullptr;

  inline void kalibrer() {
    float sum[TouchConfig::FELTER] = {};
    Serial.println("Kalibrerer touchslider. Hold fingrene vaek fra felterne.");
    delay(TouchConfig::KALIBRERING_START_MS);
    for (int sample = 0; sample < TouchConfig::KALIBRERING_PROEVER; ++sample) {
      for (int i = 0; i < TouchConfig::FELTER; ++i) sum[i] += touchRead(TouchConfig::PINS[i]);
      delay(TouchConfig::KALIBRERING_PAUSE_MS);
    }
    for (int i = 0; i < TouchConfig::FELTER; ++i) {
      sum[i] /= TouchConfig::KALIBRERING_PROEVER;
      Serial.printf("Felt %d, GPIO %u, baseline %.1f\n", i + 1,
                    static_cast<unsigned>(TouchConfig::PINS[i]), sum[i]);
    }
    slider.setBaseline(sum);
    Serial.println("Slider klar.");
  }

  inline void sensorTask(void*) {
    TickType_t sidste = xTaskGetTickCount();
    const TickType_t interval = pdMS_TO_TICKS(TouchConfig::SAMPLE_MS) > 0
        ? pdMS_TO_TICKS(TouchConfig::SAMPLE_MS) : 1;
    uint32_t debugSidst = 0;
    for (;;) {
      float raw[TouchConfig::FELTER];
      for (int i = 0; i < TouchConfig::FELTER; ++i) raw[i] = touchRead(TouchConfig::PINS[i]);
      uint32_t now = millis();
      slider.update(raw, now, [](const TouchEvent& event) { xQueueSend(inputKoe, &event, 0); });
      if (TouchConfig::DEBUG_POSITION && slider.harFinger() && now - debugSidst > 120) {
        Serial.printf("Fingerposition: %.2f\n", slider.getPosition());
        debugSidst = now;
      }
      // Fast samplingstid selv mens TFT-overfoerslen blokerer hovedloekken.
      vTaskDelayUntil(&sidste, interval);
    }
  }

  inline bool start() {
    inputKoe = xQueueCreate(32, sizeof(TouchEvent));
    if (!inputKoe) return false;
    kalibrer();
    return xTaskCreatePinnedToCore(sensorTask, "touchslider", 4096, nullptr, 2,
                                  &taskHandle, 0) == pdPASS;
  }
  inline bool laes(TouchEvent& event) {
    return inputKoe && xQueueReceive(inputKoe, &event, 0) == pdTRUE;
  }
}
