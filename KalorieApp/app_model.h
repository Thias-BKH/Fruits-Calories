#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "app_indstillinger.h"
#include "input_typer.h"

// Denne fil bestemmer navigation og data. Den kan testes uden skaermhardware.
enum class Side : uint8_t { Forside, Indstillinger, Ugemaal, Lysstyrke, Nulstil };
enum class Fokus : uint8_t { Vaerdi, Gem, Fortryd };

struct AppData {
  KalorieData kalorier = AppConfig::START_KALORIER;
  uint8_t lysstyrke = AppConfig::START_LYSSTYRKE;
};

class KalorieAppModel {
 public:
  AppData data;
  Side side = Side::Forside;
  Fokus fokus = Fokus::Vaerdi;
  uint8_t valgtRaekke = 0;  // 0..2 = menu, 3 = "Tilbage" nederst.
  uint32_t kladde = 0;     // Redigeres foer Gem. Originalen bevares ved Fortryd.
  bool skalTegnes = true;
  bool skalGemmes = false;
  bool trykFeedback = false;
  char besked[40] = {};

  // Laesning af gemte data aendrer ikke tekster, farver eller layoutindstillinger.
  void indlaes(const AppData& gemt) { data = gemt; skalTegnes = true; }

  // Denne vaerdi bruges til LED/PWM. Lysstyrke faar live-preview under redigering.
  uint8_t aktivLysstyrke() const {
    return side == Side::Lysstyrke ? static_cast<uint8_t>(kladde) : data.lysstyrke;
  }

  // Alle inputkilder kalder samme funktion: sensor, BOOT-knap og Serial Monitor.
  void input(Input event, uint32_t nu) {
    skalTegnes = true;
    if (event == Input::Tryk) {
      trykFeedback = true;
      feedbackSlut = nu + AppConfig::TRYK_FEEDBACK_MS;
    }
    if (event == Input::Tilbage) { tilbage(nu); return; }

    if (side == Side::Forside) {
      side = Side::Indstillinger;
      valgtRaekke = 0;
      return;
    }

    if (side == Side::Indstillinger) {
      if (event == Input::SwipeOp) valgtRaekke = (valgtRaekke + 3) % 4;
      if (event == Input::SwipeNed) valgtRaekke = (valgtRaekke + 1) % 4;
      if (event != Input::Tryk) return;
      fokus = Fokus::Vaerdi;
      if (valgtRaekke == 0) { side = Side::Ugemaal; kladde = data.kalorier.ugentligtMaal; }
      if (valgtRaekke == 1) { side = Side::Lysstyrke; kladde = data.lysstyrke; }
      if (valgtRaekke == 2) { side = Side::Nulstil; fokus = Fokus::Fortryd; }
      if (valgtRaekke == 3) side = Side::Forside;
      return;
    }

    // Nulstil har bekraeftelse og starter paa Fortryd, saa et enkelt tryk er ufarligt.
    if (side == Side::Nulstil) {
      if (event == Input::SwipeOp || event == Input::SwipeNed)
        fokus = fokus == Fokus::Gem ? Fokus::Fortryd : Fokus::Gem;
      if (event == Input::Tryk) {
        if (fokus == Fokus::Gem) {
          data.kalorier.spistIDag = 0;
          data.kalorier.spistTidligereDage = 0;
          side = Side::Forside;
          skalGemmes = true;
          visBesked("Data nulstillet", nu);
        } else tilbage(nu);
      }
      return;
    }

    // I talfeltet aendrer swipe vaerdien. Tryk flytter fokus til Gem-knappen.
    if (fokus == Fokus::Vaerdi) {
      const uint32_t trin = side == Side::Ugemaal ? AppConfig::MAAL_TRIN : AppConfig::LYS_TRIN;
      const uint32_t minimum = side == Side::Ugemaal ? AppConfig::MIN_UGEMAAL : AppConfig::MIN_LYS;
      const uint32_t maksimum = side == Side::Ugemaal ? AppConfig::MAX_UGEMAAL : AppConfig::MAX_LYS;
      if (event == Input::SwipeOp) kladde = kladde + trin > maksimum ? maksimum : kladde + trin;
      if (event == Input::SwipeNed) kladde = kladde < minimum + trin ? minimum : kladde - trin;
      if (event == Input::Tryk) fokus = Fokus::Gem;
      return;
    }

    // Paa knapperne skifter swipe fokus. Man kan ogsaa vende tilbage til talfeltet.
    if (event == Input::SwipeOp) fokus = fokus == Fokus::Gem ? Fokus::Vaerdi : Fokus::Gem;
    if (event == Input::SwipeNed) fokus = fokus == Fokus::Gem ? Fokus::Fortryd : Fokus::Vaerdi;
    if (event != Input::Tryk) return;
    if (fokus == Fokus::Fortryd) { tilbage(nu); return; }
    if (fokus == Fokus::Gem) {
      if (side == Side::Ugemaal) {
        data.kalorier.ugentligtMaal = kladde;
        visBesked("Ugemål gemt", nu);
      } else {
        data.lysstyrke = static_cast<uint8_t>(kladde);
        visBesked("Lysstyrke gemt", nu);
      }
      side = Side::Indstillinger;
      skalGemmes = true;
    }
  }

  // Fortryd forlader kladden uden at overskrive det gemte maal/lys.
  void tilbage(uint32_t nu) {
    if (side == Side::Forside) return;
    if (side == Side::Indstillinger) side = Side::Forside;
    else { side = Side::Indstillinger; visBesked("Annulleret", nu); }
    skalTegnes = true;
  }

  // Kald denne funktion fra den senere telefon/maaltidsintegration.
  void registrerMaaltid(uint32_t kcal, uint32_t nu) {
    uint64_t sum = static_cast<uint64_t>(data.kalorier.spistIDag) + kcal;
    data.kalorier.spistIDag = sum > 1000000 ? 1000000 : static_cast<uint32_t>(sum);
    skalGemmes = true;
    skalTegnes = true;
    visBesked("Kalorier tilføjet", nu);
  }

  // Kalenderen er endnu ikke tilkoblet. Disse funktioner kan senere kaldes af telefonen.
  void nyDag(uint32_t nu) {
    uint64_t sum = static_cast<uint64_t>(data.kalorier.spistTidligereDage) + data.kalorier.spistIDag;
    data.kalorier.spistTidligereDage = sum > 1000000 ? 1000000 : static_cast<uint32_t>(sum);
    data.kalorier.spistIDag = 0;
    skalGemmes = true; skalTegnes = true; visBesked("Ny dag", nu);
  }
  void nyUge(uint32_t nu) {
    data.kalorier.spistIDag = 0; data.kalorier.spistTidligereDage = 0;
    skalGemmes = true; skalTegnes = true; visBesked("Ny uge", nu);
  }

  // Beskeder/trykfeedback udloeber uden at blokere input med delay().
  void tick(uint32_t nu) {
    if (trykFeedback && static_cast<int32_t>(nu - feedbackSlut) >= 0) {
      trykFeedback = false; skalTegnes = true;
    }
    if (besked[0] && static_cast<int32_t>(nu - beskedSlut) >= 0) {
      besked[0] = 0; skalTegnes = true;
    }
  }
  void visBesked(const char* tekst, uint32_t nu) {
    snprintf(besked, sizeof(besked), "%s", tekst);
    beskedSlut = nu + AppConfig::BESKED_MS;
    skalTegnes = true;
  }
  void gemningFaerdig(bool ok, uint32_t nu) {
    skalGemmes = false;
    if (!ok) visBesked("Kunne ikke gemme", nu);
  }

 private:
  uint32_t feedbackSlut = 0;
  uint32_t beskedSlut = 0;
};
