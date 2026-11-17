#pragma once
#include <math.h>
#include <stdio.h>
#include "app_model.h"
#include "ui_faelles.h"

// Denne fil tegner siderne. Den aendrer ikke kalorier eller gemte indstillinger.
// Alle positioner er pixels paa en 128 x 160 skaerm.
namespace AppDraw {
  inline float klip(float n, float minimum, float maksimum) {
    return n < minimum ? minimum : (n > maksimum ? maksimum : n);
  }
  inline uint8_t alpha(float d) { return static_cast<uint8_t>(klip(d, 0, 1) * 255 + 0.5f); }

  // Afrundede kort/knapper med glatte kanter. Kanttykkelse 0 giver kun fyld.
  inline void kort(uint16_t* p, float x, float y, float w, float h, float radius,
                   uint16_t fyld, uint16_t kant = 0, float kantTykkelse = 0) {
    const int x0 = static_cast<int>(fmaxf(0, floorf(x - 1)));
    const int y0 = static_cast<int>(fmaxf(0, floorf(y - 1)));
    const int x1 = static_cast<int>(fminf(127, ceilf(x + w)));
    const int y1 = static_cast<int>(fminf(159, ceilf(y + h)));
    radius = fminf(radius, fminf(w, h) / 2);
    for (int py = y0; py <= y1; ++py) {
      for (int px = x0; px <= x1; ++px) {
        float qx = fabsf(px + 0.5f - x - w / 2) - (w / 2 - radius);
        float qy = fabsf(py + 0.5f - y - h / 2) - (h / 2 - radius);
        float afstand = hypotf(fmaxf(qx, 0), fmaxf(qy, 0))
                      + fminf(fmaxf(qx, qy), 0) - radius;
        float udenfor = klip(0.5f - afstand, 0, 1);
        if (udenfor <= 0) continue;
        uint16_t farve = fyld;
        if (kantTykkelse > 0) {
          float indenfor = klip(0.5f - afstand - kantTykkelse, 0, 1);
          farve = blandPixel(kant, fyld, alpha((udenfor - indenfor) / udenfor));
        }
        uint16_t& pixel = p[py * 128 + px];
        pixel = blandPixel(farve, pixel, alpha(udenfor));
      }
    }
  }

  // Lange tekster tilpasses, men goeres aldrig mindre end den laesbare FONT_MIN.
  inline const AAFont& tilpas(const char* a, const char* b, uint8_t oensket, int bredde) {
    uint8_t stoerrelse = oensket < AppConfig::FONT_MIN ? AppConfig::FONT_MIN
        : (oensket > 20 ? 20 : oensket);
    while (stoerrelse > AppConfig::FONT_MIN) {
      const AAFont& f = findSkrift(stoerrelse);
      if (tekstBredde(a, f) + tekstBredde(b, f) <= bredde * 256) break;
      --stoerrelse;
    }
    return findSkrift(stoerrelse);
  }

  // Centreret tekst, eventuelt i to farver, som "1500/" og det graa dagsmaal.
  inline void centrer(uint16_t* p, const char* a, const char* b, uint8_t stoerrelse,
                      int y, uint16_t farve = AppConfig::HVID,
                      uint16_t andenFarve = AppConfig::GRAA_TEKST, int maxBredde = 120) {
    const AAFont& f = tilpas(a, b, stoerrelse, maxBredde);
    int32_t x = (128 * 256 - tekstBredde(a, f) - tekstBredde(b, f)) / 2;
    x = tegnTekst(p, a, f, x, y, farve);
    tegnTekst(p, b, f, x, y, andenFarve);
  }

  // Vandret skyder. Groent fyld viser vaerdien; prikken markerer fokus.
  inline void skyder(uint16_t* p, float andel, bool valgt) {
    andel = klip(andel, 0, 1);
    constexpr float x = 10, y = AppConfig::SKYDER_Y, w = 108, h = 4;
    kort(p, x, y, w, h, 2, AppConfig::GRAA_BUE);
    if (andel > 0) kort(p, x, y, fmaxf(4, w * andel), h, 2, AppConfig::GROEN);
    if (valgt) kort(p, x + w * andel - 4, y - 2, 8, 8, 4, AppConfig::GROEN);
  }

  // Knaptekst placeres efter skrifttypens synlige hoejde, ikke blot dens billedfelt.
  inline void knap(uint16_t* p, int x, int y, int w, const char* tekst,
                   bool primaer, bool valgt, bool trykket) {
    constexpr int h = AppConfig::KNAP_HOEJDE;
    uint16_t fyld = primaer ? AppConfig::GROEN : AppConfig::KORT;
    uint16_t kant = valgt && (primaer || trykket) ? AppConfig::HVID : AppConfig::GROEN;
    kort(p, x, y, w, h, 11, fyld, kant, valgt ? 1.4f : 0);
    const AAFont& f = tilpas(tekst, "", AppConfig::FONT_KNAP, w - 8);
    int32_t tx = (x * 256) + (w * 256 - tekstBredde(tekst, f)) / 2;
    int baseline = y + h / 2 + 4;
    tegnTekst(p, tekst, f, tx, baseline, primaer ? AppConfig::SORT : AppConfig::HVID);
  }

  // Den eksisterende ring forstoerres med blanding af nabopixels, som i jeres seneste version.
  inline void ring(uint16_t* p, uint8_t laengde, uint16_t farve) {
    const int top = (160 - RING_HOEJDE) / 2;
    const int midtX = 64;
    const int midtY = AppConfig::RING_MIDTE_Y;
    auto hent = [&](int x, int y) -> uint16_t {
      const size_t i = y * RING_BREDDE + x;
      bool aktiv = laengde > 0 && ringPosition[i] <= laengde;
      return blandPixel(aktiv ? farve : AppConfig::GRAA_BUE, 0, ringAlpha[i]);
    };
    for (int y = 0; y < 160; ++y) {
      float sy = (y - midtY) / AppConfig::RING_SKALA + midtY - top;
      if (sy < 0 || sy >= RING_HOEJDE - 1) continue;
      int y0 = static_cast<int>(sy);
      uint8_t wy = static_cast<uint8_t>((sy - y0) * 255 + 0.5f);
      for (int x = 0; x < 128; ++x) {
        float sx = (x - midtX) / AppConfig::RING_SKALA + midtX;
        if (sx < 0 || sx >= RING_BREDDE - 1) continue;
        int x0 = static_cast<int>(sx);
        uint8_t wx = static_cast<uint8_t>((sx - x0) * 255 + 0.5f);
        uint16_t oeverst = blandPixel(hent(x0 + 1, y0), hent(x0, y0), wx);
        uint16_t nederst = blandPixel(hent(x0 + 1, y0 + 1), hent(x0, y0 + 1), wx);
        p[y * 128 + x] = blandPixel(nederst, oeverst, wy);
      }
    }
  }

  inline void forside(uint16_t* p, const KalorieAppModel& app) {
    // Den groenne ring og tallet viser dagens spiste kalorier.
    const KalorieTal tal = beregnKalorier(app.data.kalorier);
    const float fremgang = tal.dagligtMaal > 0
        ? klip(static_cast<float>(app.data.kalorier.spistIDag) / tal.dagligtMaal, 0, 1) : 0;
    uint8_t laengde = AppConfig::RING_VISER_TILBAGE ? tal.groentAfBaren : alpha(fremgang);
    if (AppConfig::HEL_RING_VED_MAAL && tal.dagligtMaal > 0 && tal.tilbageIDag == 0) laengde = 255;
    ring(p, laengde, AppConfig::GROEN);

    // Store, hvide dagstal og graat maal, som i jeres prototype.
    char venstre[64], hoejre[24], uge[24];
    snprintf(venstre, sizeof(venstre), "%lu/", static_cast<unsigned long>(app.data.kalorier.spistIDag));
    snprintf(hoejre, sizeof(hoejre), "%lu", static_cast<unsigned long>(tal.dagligtMaal));
    centrer(p, venstre, hoejre, AppConfig::FONT_FORSIDE.dagensTal, AppConfig::DAGSTAL_Y,
            AppConfig::HVID, AppConfig::GRAA_TEKST, 84);
    // En gemmefejl bruger det eksisterende tekstfelt, aldrig et banner.
    const bool fejl = strcmp(app.besked, "Kunne ikke gemme") == 0;
    centrer(p, fejl ? "Gemmefejl" : "Kalorier", "", AppConfig::FONT_FORSIDE.kcalTekst,
            AppConfig::KCAL_TEKST_Y, fejl ? AppConfig::FEJL : AppConfig::HVID, AppConfig::HVID, 78);
    // To laesbare linjer: fuld overskrift og praecise kalorietal uden afkortning.
    centrer(p, "Ugentlig mål", "", AppConfig::FONT_FORSIDE.ugeOverskrift,
            AppConfig::UGEOVERSKRIFT_Y);
    formaterDansk(tal.spistDenneUge, uge, sizeof(uge));
    formaterDansk(app.data.kalorier.ugentligtMaal, hoejre, sizeof(hoejre));
    snprintf(venstre, sizeof(venstre), "%s/", uge);
    centrer(p, venstre, hoejre, AppConfig::FONT_FORSIDE.ugensTal, AppConfig::UGETAL_Y);
    // Den groenne markering ved siden af ringen peger paa betjeningen, som i designet.
    kort(p, 123, 51, 2, 49, 1, AppConfig::GROEN);
  }

  // Overskriften flytter sig aldrig. Kun fejl erstatter teksten i samme felt.
  inline void sidetitel(uint16_t* p, const KalorieAppModel& app, const char* tekst) {
    const bool fejl = strcmp(app.besked, "Kunne ikke gemme") == 0;
    centrer(p, fejl ? "Kunne ikke gemme" : tekst, "", AppConfig::FONT_TITEL,
            AppConfig::SIDETITEL_Y, fejl ? AppConfig::FEJL : AppConfig::GROEN);
  }

  inline void menu(uint16_t* p, const KalorieAppModel& app) {
    // Sidetitel og tre menuraekker. Placeringerne findes i app_indstillinger.h.
    sidetitel(p, app, "Indstillinger");
    for (int i = 0; i < 3; ++i) {
      int y = AppConfig::MENU_START_Y + i * (AppConfig::MENU_RAEKKE_HOEJDE + AppConfig::MENU_AFSTAND);
      bool valgt = app.valgtRaekke == i;
      uint16_t kant = app.trykFeedback && valgt ? AppConfig::HVID : AppConfig::GROEN;
      kort(p, AppConfig::MENU_X, y, AppConfig::MENU_BREDDE,
           AppConfig::MENU_RAEKKE_HOEJDE, 8,
           AppConfig::KORT, kant, valgt ? 1.2f : 0);
      const char* tekst = AppConfig::MENU_TEKSTER[i];
      const AAFont& font = tilpas(tekst, "", AppConfig::FONT_MENU, AppConfig::MENU_BREDDE - 12);
      tegnTekst(p, tekst, font, (AppConfig::MENU_X + 6) * 256,
                y + AppConfig::MENU_RAEKKE_HOEJDE / 2 + 4, AppConfig::HVID);
      if (valgt) kort(p, 122, y, 2, AppConfig::MENU_RAEKKE_HOEJDE, 1, AppConfig::GROEN);
    }
    // Ekstra tilbagevej goer hele menuen brugbar med blot op, ned og tryk.
    centrer(p, "Tilbage", "", AppConfig::FONT_MIN, AppConfig::MENU_TILBAGE_Y,
            app.valgtRaekke == 3 ? AppConfig::GROEN : AppConfig::GRAA_TEKST);
    if (app.valgtRaekke == 3) kort(p, 122, AppConfig::MENU_TILBAGE_Y - 10, 2, 12, 1, AppConfig::GROEN);
  }

  // Samme hjaelp paa alle undersider: teksten foelger den valgte handling.
  inline void hjaelp(uint16_t* p, const KalorieAppModel& app) {
    const char* hint = "Swipe: vælg | Tryk: OK";
    if (app.fokus == Fokus::Fortryd) hint = "Tryk: fortryd";
    else if (app.fokus == Fokus::Gem) {
      hint = app.side == Side::Nulstil ? "Tryk: nulstil | Swipe: flyt"
                                      : "Tryk: gem | Swipe: flyt";
    }
    centrer(p, hint, "", AppConfig::FONT_HINT, AppConfig::HINT_Y, AppConfig::GRAA_TEKST);
  }

  inline void rediger(uint16_t* p, const KalorieAppModel& app) {
    bool maal = app.side == Side::Ugemaal;
    bool vaerdiValgt = app.fokus == Fokus::Vaerdi;
    char tal[32], beskrivelse[64], dag[24];
    sidetitel(p, app, maal ? "Ugentligt mål" : "Lysstyrke");
    // Den store vaerdi er en kladde. Gem bekraefter; Fortryd genskaber originalen.
    if (maal) formaterDansk(app.kladde, tal, sizeof(tal));
    else snprintf(tal, sizeof(tal), "%lu%%", static_cast<unsigned long>(app.kladde));
    centrer(p, tal, "", AppConfig::FONT_STOR_VAERDI, maal ? 52 : 65, AppConfig::HVID);
    // Skyderens prik viser fokus; talfeltet faar mere luft uden understregning.
    centrer(p, maal ? "kcal pr. uge" : "skærmens lys", "", AppConfig::FONT_BESKRIVELSE,
            maal ? 70 : 83, AppConfig::GRAA_TEKST);
    if (maal) {
      formaterDansk(app.kladde / 7, dag, sizeof(dag));
      snprintf(beskrivelse, sizeof(beskrivelse), "%s %s kcal pr. dag",
               app.kladde % 7 ? "ca." : "=", dag);
      centrer(p, beskrivelse, "", AppConfig::FONT_MIN, 90);
      skyder(p, static_cast<float>(app.kladde - AppConfig::MIN_UGEMAAL)
                  / (AppConfig::MAX_UGEMAAL - AppConfig::MIN_UGEMAAL), vaerdiValgt);
    } else {
      skyder(p, static_cast<float>(app.kladde - AppConfig::MIN_LYS)
                  / (AppConfig::MAX_LYS - AppConfig::MIN_LYS), vaerdiValgt);
    }
    // Knapperne har faelles position og ekstra luft over hjaelpeteksten.
    knap(p, AppConfig::KNAP_VENSTRE_X, AppConfig::KNAP_Y, AppConfig::KNAP_BREDDE,
         "Fortryd", false, app.fokus == Fokus::Fortryd, app.trykFeedback);
    knap(p, AppConfig::KNAP_HOEJRE_X, AppConfig::KNAP_Y, AppConfig::KNAP_BREDDE,
         "Gem", true, app.fokus == Fokus::Gem, app.trykFeedback);
    hjaelp(p, app);
  }

  inline void nulstil(uint16_t* p, const KalorieAppModel& app) {
    sidetitel(p, app, "Nulstil data");
    centrer(p, "Vil du nulstille dagens", "", 11, 57);
    centrer(p, "og ugens kalorier?", "", 11, 74);
    centrer(p, "Ugentlig mål bevares", "", AppConfig::FONT_MIN, 96, AppConfig::GRAA_TEKST);
    knap(p, AppConfig::KNAP_VENSTRE_X, AppConfig::KNAP_Y, AppConfig::KNAP_BREDDE,
         "Fortryd", false, app.fokus == Fokus::Fortryd, app.trykFeedback);
    knap(p, AppConfig::KNAP_HOEJRE_X, AppConfig::KNAP_Y, AppConfig::KNAP_BREDDE,
         "Nulstil", true, app.fokus == Fokus::Gem, app.trykFeedback);
    hjaelp(p, app);
  }

  inline void maaltid(uint16_t* p, const KalorieAppModel& app) {
    sidetitel(p, app, "Bekræft frugt");
    centrer(p, app.forslag.frugt, "", 16, 47);
    char tal[32];
    snprintf(tal, sizeof(tal), "%lu kcal", static_cast<unsigned long>(app.forslag.kcal));
    centrer(p, tal, "", 20, 76);
    snprintf(tal, sizeof(tal), "Standard: %u g", unsigned(app.forslag.gram));
    centrer(p, tal, "", 10, 96, AppConfig::GRAA_TEKST);
    centrer(p, "Ikke registreret endnu", "", 10, 110, AppConfig::GRAA_TEKST);
    knap(p, 6, 120, 46, "Afvis", false, app.fokus == Fokus::Fortryd, app.trykFeedback);
    knap(p, 57, 120, 65, "Accepter", true, app.fokus == Fokus::Gem, app.trykFeedback);
    centrer(p, app.fokus == Fokus::Gem ? "Tryk: accepter" : "Tryk: afvis", "", 10, 155,
            AppConfig::GRAA_TEKST);
  }

  inline void scanFejl(uint16_t* p, const KalorieAppModel& app) {
    sidetitel(p, app, "Prøv igen");
    centrer(p, app.scanFejl, "", 11, 60, AppConfig::HVID);
    centrer(p, "Tag et nyt billede", "", 10, 82, AppConfig::GRAA_TEKST);
    centrer(p, "Intet registreret", "", 10, 101, AppConfig::GRAA_TEKST);
    knap(p, 25, 120, 78, "Tilbage", true, true, app.trykFeedback);
  }
}

// Eneste tegnefunktion, som hovedprogrammet behoever at kalde.
inline void tegnApp(uint16_t* pixels, const KalorieAppModel& app) {
  memset(pixels, 0, 128 * 160 * sizeof(uint16_t));
  switch (app.side) {
    case Side::Forside: AppDraw::forside(pixels, app); break;
    case Side::Indstillinger: AppDraw::menu(pixels, app); break;
    case Side::Ugemaal:
    case Side::Lysstyrke: AppDraw::rediger(pixels, app); break;
    case Side::Nulstil: AppDraw::nulstil(pixels, app); break;
    case Side::Maaltid: AppDraw::maaltid(pixels, app); break;
    case Side::ScanFejl: AppDraw::scanFejl(pixels, app); break;
  }
}
