#pragma once
#include "app_model.h"

// Et versionsnummer og en kontrolsum opdager gamle/ufuldstaendige gemte data.
// Hele posten gemmes samlet, frem for at skrive hvert felt separat.
namespace DataFormat {
  constexpr size_t ANTAL_ORD = 7;
  constexpr uint32_t MAGISK = 0x4B43414C;
  constexpr uint32_t VERSION = 1;
  inline uint32_t kontrolsum(const uint32_t* ord, size_t antal) {
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < antal; ++i) {
      for (int b = 0; b < 4; ++b) { hash ^= (ord[i] >> (b * 8)) & 255; hash *= 16777619u; }
    }
    return hash;
  }
  inline void pak(const AppData& data, uint32_t* ord) {
    ord[0] = MAGISK; ord[1] = VERSION;
    ord[2] = data.kalorier.ugentligtMaal;
    ord[3] = data.kalorier.spistIDag;
    ord[4] = data.kalorier.spistTidligereDage;
    ord[5] = data.lysstyrke;
    ord[6] = kontrolsum(ord, 6);
  }
  inline bool pakUd(const uint32_t* ord, AppData& data) {
    if (ord[0] != MAGISK || ord[1] != VERSION || ord[6] != kontrolsum(ord, 6)) return false;
    if (ord[2] < AppConfig::MIN_UGEMAAL || ord[2] > AppConfig::MAX_UGEMAAL) return false;
    if (ord[3] > 1000000 || ord[4] > 1000000) return false;
    if (ord[5] < AppConfig::MIN_LYS || ord[5] > AppConfig::MAX_LYS) return false;
    data.kalorier = {ord[2], ord[3], ord[4]};
    data.lysstyrke = static_cast<uint8_t>(ord[5]);
    return true;
  }
}
