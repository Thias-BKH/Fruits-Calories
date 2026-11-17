#pragma once
#include <stdint.h>

// Datatyper og kalorieberegning fra jeres fungerende prototype.
struct KalorieData {
  uint32_t ugentligtMaal;
  uint32_t spistIDag;
  uint32_t spistTidligereDage;
};

struct TekstStoerrelser {
  uint8_t dagensTal; // Tallene i midten
  uint8_t kcalTekst; // "Kalorier"
  uint8_t ugeOverskrift; // "Ugentlig mål:"
  uint8_t ugensTal; // Ugens tal
};

struct KalorieTal {
  uint32_t dagligtMaal;
  uint32_t tilbageIDag;
  uint32_t spistDenneUge;
  uint8_t groentAfBaren;
};

inline KalorieTal beregnKalorier(const KalorieData& data) {
  const uint32_t dagligt = data.ugentligtMaal / 7;
  const uint32_t tilbage = data.spistIDag < dagligt
      ? dagligt - data.spistIDag : 0;
  // Ringen vokser med accepterede kcal og stopper ved fuld, ogsaa over dagsmaalet.
  const uint32_t spistOpTilMaal = data.spistIDag < dagligt ? data.spistIDag : dagligt;
  const uint8_t andel = dagligt > 0
      ? static_cast<uint8_t>((static_cast<uint64_t>(spistOpTilMaal) * 255
                             + dagligt / 2) / dagligt) : 0;
  return {dagligt, tilbage, data.spistTidligereDage + data.spistIDag, andel};
}
