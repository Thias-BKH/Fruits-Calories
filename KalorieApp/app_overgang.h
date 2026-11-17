#pragma once
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "app_visning.h"

// Preview og ESP32 bruger samme varighed, retning og bevaegelseskurve.
namespace AppMotion {
  inline uint8_t niveau(Side side) {
    if (side == Side::Forside) return 0;
    if (side == Side::Indstillinger) return 1;
    return 2;
  }

  inline int8_t retning(Side fra, Side til) {
    return niveau(til) > niveau(fra) ? 1 : -1;
  }

  // Smoothstep giver en rolig start og opbremsning uden ventende delay().
  inline float fremgang(uint32_t forloebet) {
    if (AppConfig::OVERGANG_MS == 0 || forloebet >= AppConfig::OVERGANG_MS) return 1;
    const float t = static_cast<float>(forloebet) / AppConfig::OVERGANG_MS;
    return t * t * (3 - 2 * t);
  }

  // out indeholder den nye side. Flyt dens synlige del FOER den gamle kopieres.
  inline void skub(uint16_t* out, const uint16_t* foer, int skridt, int8_t vej) {
    constexpr int w = SKAERM_BREDDE;
    if (skridt >= w) return;
    if (skridt <= 0) {
      memcpy(out, foer, w * SKAERM_HOEJDE * sizeof(uint16_t));
      return;
    }
    const size_t nyeBytes = skridt * sizeof(uint16_t);
    const size_t gamleBytes = (w - skridt) * sizeof(uint16_t);
    for (int y = 0; y < SKAERM_HOEJDE; ++y) {
      uint16_t* row = out + y * w;
      const uint16_t* oldRow = foer + y * w;
      if (vej > 0) {
        memmove(row + w - skridt, row, nyeBytes);
        memcpy(row, oldRow + skridt, gamleBytes);
      } else {
        memmove(row, row + w - skridt, nyeBytes);
        memcpy(row + skridt, oldRow, gamleBytes);
      }
    }
  }
}

// Bufferen bruges kun under sideskift og frigives inden billedgenkendelse.
class AppOvergang {
 public:
  ~AppOvergang() { free(foer); }
  bool erAktiv() const { return aktiv; }

  void nulstil() { klar = false; aktiv = false; free(foer); foer = nullptr; }

  void tegn(uint16_t* pixels, const KalorieAppModel& app, uint32_t nu) {
    if (!klar) {
      tegnApp(pixels, app);
      vistSide = app.side;
      klar = true;
      return;
    }
    if (app.side != vistSide) {
      // Ogsaa ved hurtige tryk starter vi fra det billede, brugeren faktisk ser.
      if (!foer && AppConfig::OVERGANG_MS > 0)
        foer = static_cast<uint16_t*>(malloc(SKAERM_BYTES));
      if (foer) memcpy(foer, pixels, SKAERM_BYTES);
      vej = AppMotion::retning(vistSide, app.side);
      vistSide = app.side;
      start = nu;
      aktiv = foer && AppConfig::OVERGANG_MS > 0;
    }
    tegnApp(pixels, app);
    if (!aktiv) return;
    const uint32_t forloebet = nu - start;
    if (forloebet >= AppConfig::OVERGANG_MS) {
      aktiv = false;
      free(foer); foer = nullptr;
      return;
    }
    const int skridt = static_cast<int>(AppMotion::fremgang(forloebet) * SKAERM_BREDDE + 0.5f);
    AppMotion::skub(pixels, foer, skridt, vej);
  }

 private:
  static constexpr size_t SKAERM_BYTES = SKAERM_BREDDE * SKAERM_HOEJDE * sizeof(uint16_t);
  uint16_t* foer = nullptr;
  Side vistSide = Side::Forside;
  uint32_t start = 0;
  int8_t vej = 1;
  bool klar = false;
  bool aktiv = false;
};
