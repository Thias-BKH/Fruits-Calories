#pragma once
#include <stdint.h>
#include "app_indstillinger.h"

// HER TILPASSER I SLIDEREN. Taerskler og filtrering er fra Kcaltracker-button.ino.
namespace TouchConfig {
  constexpr uint8_t FELTER = 5;
  // Raekkefoelge fra OEVERST til NEDERST. Felt 3 er GPIO 4, da GPIO 27 er skaermens CS.
  constexpr uint8_t PINS[FELTER] = {32, 33, 4, 14, 13};

  constexpr float TOUCH_THRESHOLD = 0.12f; // 12 % fald fra baseline registrerer en finger.
  constexpr float NOISE_FLOOR = 0.04f;    // Smaa aendringer filtreres fra.
  constexpr float SCROLL_STEP = 0.40f;    // Positionstrin mellem scroll-events; pad-indeks er 0..4.
  constexpr float SMOOTHING = 0.40f;      // Vaegt for ny position. Stoerre tal reagerer hurtigere.
  constexpr float TAP_MAX_TRAVEL = 0.60f;

  constexpr uint32_t TAP_MAX_TIME = 350;  // Kort tryk, maalt i ms med fingeren paa slideren.
  constexpr uint32_t RELEASE_DELAY = 60;  // Undgaa falsk slip ved kortvarigt signaludfald.
  constexpr uint32_t START_DELAY = 25;    // En ny finger skal vaere stabil lidt foer start.
  constexpr uint32_t SAMPLE_MS = 12;      // Aflaeses uafhaengigt af skaermens tegning.

  // Langt, roligt tryk giver Tilbage. Efter et swipe udloeses der ikke langt tryk.
  constexpr bool BRUG_LANGT_TRYK = true;
  constexpr uint32_t LANGT_TRYK_MS = 800;
  constexpr float LANGT_TRYK_MAX_TRAVEL = 0.30f;

  // Menuen flyttes en raekke pr. fysisk swipe. Vaerdiredigering bruger alle scrolltrin.
  constexpr bool MENU_ET_TRIN_PR_SWIPE = true;

  // Opstartskalibrering: Hold fingrene vaek fra alle fem felter.
  constexpr uint16_t KALIBRERING_PROEVER = 80;
  constexpr uint32_t KALIBRERING_PAUSE_MS = 12;
  constexpr uint32_t KALIBRERING_START_MS = 300;
  constexpr bool DEBUG_POSITION = false;

  // Fang pin-konflikter ved kompilering, saa CS=27 ikke samtidig kan blive et touchfelt.
  constexpr bool erTouchPin(int p) {
    return p == 0 || p == 2 || p == 4 || p == 12 || p == 13 || p == 14
        || p == 15 || p == 27 || p == 32 || p == 33;
  }
  constexpr bool pinoutErGyldigt() {
    const int skaerm[] = {AppConfig::PIN_CS, AppConfig::PIN_RS, AppConfig::PIN_RST,
                         AppConfig::PIN_SCK, AppConfig::PIN_SDI, AppConfig::PIN_LED};
    for (int i = 0; i < FELTER; ++i) {
      if (!erTouchPin(PINS[i])) return false;
      for (int j = i + 1; j < FELTER; ++j) if (PINS[i] == PINS[j]) return false;
      for (int p : skaerm) if (PINS[i] == p) return false;
    }
    for (int i = 0; i < 6; ++i) {
      if (skaerm[i] >= 6 && skaerm[i] <= 11) return false; // WROOM-32 bruger disse til flash.
      for (int j = i + 1; j < 6; ++j) if (skaerm[i] == skaerm[j]) return false;
    }
    return true;
  }
  static_assert(pinoutErGyldigt(), "GPIO-konflikt: kontroller skaermben og touchfelter");
  static_assert(SAMPLE_MS > 0 && KALIBRERING_PROEVER > 0, "Aflaesning og kalibrering skal have positive intervaller");
  static_assert(SCROLL_STEP > 0 && SMOOTHING > 0 && SMOOTHING <= 1,
                "Scrolltrin skal vaere positivt; smoothing skal vaere stoerre end 0 og hoejst 1");
}
