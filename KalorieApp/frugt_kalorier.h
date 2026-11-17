#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "app_model.h"

// Standardportioner er prototypevalg, ikke vaegte maalt af kameraet.
// Energi pr. 100 g: Matvaretabellen (Mattilsynet), raadata laest 17-11-2026.
// https://www.matvaretabellen.no/eple-uspesifisert-ra/ (46 kcal)
// https://www.matvaretabellen.no/en/banana-raw/ (88 kcal)
namespace FrugtKalorier {
  constexpr float MIN_SIKKERHED = 0.60f;
  struct Frugt { const char* label; uint16_t gram; uint16_t kcalPr100g; };
  constexpr Frugt FRUGTER[] = {{"Æble", 150, 46}, {"Banan", 120, 88}};

  inline const Frugt* find(const char* label) {
    if (label) for (const auto& frugt : FRUGTER)
      if (strcmp(label, frugt.label) == 0) return &frugt;
    return nullptr;
  }
  inline uint32_t kcal(const Frugt& frugt) {
    return (uint32_t(frugt.gram) * frugt.kcalPr100g + 50) / 100;
  }
  // Faelles politik for ESP32 og simulator: background/ukendt/usikkert giver aldrig et maaltid.
  inline bool modtag(KalorieAppModel& app, const char* label, float sikkerhed, uint32_t nu) {
    if (!app.kanScanne()) return false;
    if (!isfinite(sikkerhed) || sikkerhed < MIN_SIKKERHED || sikkerhed > 1) {
      app.visScanFejl("Usikker genkendelse");
      return false;
    }
    const Frugt* frugt = find(label);
    if (!frugt) {
      app.visScanFejl("Ingen frugt fundet");
      return false;
    }
    return app.foreslaaMaaltid(frugt->label, kcal(*frugt), frugt->gram,
                             uint8_t(sikkerhed * 100 + 0.5f), nu);
  }
}
