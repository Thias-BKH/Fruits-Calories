#pragma once
#include <math.h>
#include "touch_indstillinger.h"
#include "input_typer.h"

// Jeres slideralgoritme er samlet her, saa den kan testes uden touch-hardware.
// Kun touch-opgaven kalder denne klasse. Appens data aendres via en inputkoe.
class TouchSliderModel {
 public:
  void setBaseline(const float* vaerdier) {
    for (int i = 0; i < TouchConfig::FELTER; ++i) baseline[i] = vaerdier[i];
    touching = false; candidateStarted = false;
  }

  float getPosition() const { return position; }
  bool harFinger() const { return touching; }

  // Emit kaldes en gang pr. click/scrolltrin/langt tryk med samme gesture-ID under samme beroering.
  template<class Emit>
  void update(const float* raw, uint32_t now, Emit emit) {
    float totalWeight = 0, weightedPosition = 0, strongestChange = 0;
    for (int i = 0; i < TouchConfig::FELTER; ++i) {
      if (baseline[i] < 1) continue;
      float change = (baseline[i] - raw[i]) / baseline[i]; // Klassisk ESP32: touch saenker vaerdien.
      if (change > strongestChange) strongestChange = change;
      float weight = change - TouchConfig::NOISE_FLOOR;
      if (weight < 0) weight = 0;
      totalWeight += weight;
      weightedPosition += weight * i;
    }
    bool detected = strongestChange > TouchConfig::TOUCH_THRESHOLD && totalWeight > 0;

    if (detected) {
      float measured = weightedPosition / totalWeight;
      lastContact = now;
      if (!touching) {
        if (!candidateStarted) { candidateStart = now; candidateStarted = true; }
        if (now - candidateStart >= TouchConfig::START_DELAY) {
          touching = true; hasScrolled = false; hasHeld = false;
          touchStartTime = now;
          position = measured; startPosition = measured; scrollAnchor = measured; maxTravel = 0;
          ++gesture;
          if (!gesture) ++gesture;
        }
        return;
      }

      position = position * (1.0f - TouchConfig::SMOOTHING) + measured * TouchConfig::SMOOTHING;
      float travel = fabsf(position - startPosition);
      if (travel > maxTravel) maxTravel = travel;
      if (hasHeld) return;

      while (position < scrollAnchor - TouchConfig::SCROLL_STEP) {
        emit(TouchEvent{Input::SwipeOp, gesture});
        scrollAnchor -= TouchConfig::SCROLL_STEP; hasScrolled = true;
      }
      while (position > scrollAnchor + TouchConfig::SCROLL_STEP) {
        emit(TouchEvent{Input::SwipeNed, gesture});
        scrollAnchor += TouchConfig::SCROLL_STEP; hasScrolled = true;
      }

      if (TouchConfig::BRUG_LANGT_TRYK && !hasScrolled
          && maxTravel < TouchConfig::LANGT_TRYK_MAX_TRAVEL
          && now - touchStartTime >= TouchConfig::LANGT_TRYK_MS) {
        emit(TouchEvent{Input::Tilbage, gesture}); hasHeld = true;
      }
    } else {
      candidateStarted = false;
      if (touching && now - lastContact > TouchConfig::RELEASE_DELAY) {
        // Release-delay medregnes ikke i fingerens faktiske tryktid.
        uint32_t duration = lastContact - touchStartTime;
        if (!hasScrolled && !hasHeld && duration < TouchConfig::TAP_MAX_TIME
            && maxTravel < TouchConfig::TAP_MAX_TRAVEL) emit(TouchEvent{Input::Tryk, gesture});
        touching = false;
      }
    }
  }

 private:
  float baseline[TouchConfig::FELTER] = {};
  bool touching = false, candidateStarted = false, hasScrolled = false, hasHeld = false;
  float position = 0, startPosition = 0, scrollAnchor = 0, maxTravel = 0;
  uint32_t touchStartTime = 0, lastContact = 0, candidateStart = 0, gesture = 0;
};
