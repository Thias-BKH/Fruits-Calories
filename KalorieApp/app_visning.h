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

  // Linjer og symboler tegnes i samme RGB565-buffer som resten af designet.
  inline void linje(uint16_t* p, float x1, float y1, float x2, float y2,
                    uint16_t farve, float tykkelse = 1.0f) {
    const float dx = x2 - x1, dy = y2 - y1;
    const float laengde2 = dx * dx + dy * dy;
    for (int y = int(floorf(fminf(y1, y2) - tykkelse)); y <= int(ceilf(fmaxf(y1, y2) + tykkelse)); ++y)
      for (int x = int(floorf(fminf(x1, x2) - tykkelse)); x <= int(ceilf(fmaxf(x1, x2) + tykkelse)); ++x) {
        if (x < 0 || x >= 128 || y < 0 || y >= 160) continue;
        const float t = laengde2 ? klip(((x-x1)*dx + (y-y1)*dy) / laengde2, 0, 1) : 0;
        const float afstand = hypotf(x-x1-t*dx, y-y1-t*dy);
        const float d = klip(tykkelse / 2 + .5f - afstand, 0, 1);
        p[y*128+x] = blandPixel(farve, p[y*128+x], alpha(d));
      }
  }

  inline void sidebetjening(uint16_t* p, bool op, bool ned, int midteY = 80) {
    const uint16_t opFarve = op ? AppConfig::HVID : AppConfig::DEMPET;
    const uint16_t nedFarve = ned ? AppConfig::HVID : AppConfig::DEMPET;
    linje(p, 119, midteY-8, 121.5f, midteY-10, opFarve);
    linje(p, 121.5f, midteY-10, 124, midteY-8, opFarve);
    kort(p, 119, midteY-2.5f, 5, 5, 2.5f, AppConfig::GROEN);
    linje(p, 119, midteY+8, 121.5f, midteY+10, nedFarve);
    linje(p, 121.5f, midteY+10, 124, midteY+8, nedFarve);
  }

  inline void knap(uint16_t* p, int x, int y, int w, const char* tekst,
                   bool primaer, bool valgt, bool trykket, bool farlig = false) {
    const uint16_t fyld = primaer ? AppConfig::GROEN : AppConfig::KORT;
    const uint16_t kant = primaer ? AppConfig::HVID : AppConfig::GROEN;
    kort(p, x, y, w, AppConfig::KNAP_HOEJDE, 11, fyld, kant, valgt ? 1.0f : 0);
    const AAFont& f = tilpas(tekst, "", AppConfig::FONT_KNAP, w - 8);
    const int32_t tx = x*256 + (w*256 - tekstBredde(tekst, f))/2;
    const uint16_t tekstFarve = primaer ? AppConfig::SORT : farlig ? AppConfig::FEJL : AppConfig::HVID;
    tegnTekst(p, tekst, f, tx, y + AppConfig::KNAP_HOEJDE/2 + 4, tekstFarve);
    (void)trykket;
  }

  inline void skraldespand(uint16_t* p, int x, int y) {
    kort(p, x+1, y+3, 6, 8, 1, AppConfig::FEJL);
    linje(p, x, y+1, x+8, y+1, AppConfig::FEJL, 1.2f);
    linje(p, x+3, y-1, x+5, y-1, AppConfig::FEJL, 1.2f);
  }

  // Cirkulaer bue med afrundede ender og en aabning oeverst, som i designet.
  inline void ring(uint16_t* p, uint8_t laengde, uint16_t farve) {
    constexpr float pi = 3.14159265359f;
    constexpr float start = -70 * pi / 180;
    constexpr float helBue = 320 * pi / 180;
    const float radius = AppConfig::RING_RADIUS;
    const float aktivBue = helBue * laengde / 255;
    const float startX = radius*cosf(start), startY = radius*sinf(start);
    const float slutX=radius*cosf(start+helBue), slutY=radius*sinf(start+helBue);
    const float aktivX=radius*cosf(start+aktivBue), aktivY=radius*sinf(start+aktivBue);
    auto afstand = [&](float x, float y, float vinkel, float bue, float endeX, float endeY) {
      if (vinkel <= bue) return fabsf(hypotf(x,y)-radius);
      return fminf(hypotf(x-startX,y-startY), hypotf(x-endeX,y-endeY));
    };
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 128; ++x) {
      const float dx=x-64.0f, dy=y-AppConfig::RING_MIDTE_Y;
      if (fabsf(dx)>radius+3 || fabsf(dy)>radius+3) continue;
      const float bredde=AppConfig::RING_TYKKELSE/2+.5f;
      if (fabsf(hypotf(dx,dy)-radius)>bredde) continue;
      float vinkel=atan2f(dy,dx)-start;
      if (vinkel<0) vinkel+=2*pi;
      uint16_t& pixel=p[y*128+x];
      pixel=blandPixel(AppConfig::GRAA_BUE,pixel,alpha(klip(bredde-afstand(dx,dy,vinkel,helBue,slutX,slutY),0,1)));
      if (laengde) pixel=blandPixel(farve,pixel,alpha(klip(bredde-afstand(dx,dy,vinkel,aktivBue,aktivX,aktivY),0,1)));
    }
  }

  inline void forside(uint16_t* p, const KalorieAppModel& app) {
    const KalorieTal tal = beregnKalorier(app.data.kalorier);
    ring(p, tal.groentAfBaren, AppConfig::GROEN);
    char spist[24], maal[24], uge[64], ugeMaal[24];
    snprintf(spist, sizeof(spist), "%lu/", static_cast<unsigned long>(app.data.kalorier.spistIDag));
    snprintf(maal, sizeof(maal), "%lu", static_cast<unsigned long>(tal.dagligtMaal));
    centrer(p, spist, maal, AppConfig::FONT_FORSIDE.dagensTal, AppConfig::DAGSTAL_Y,
            AppConfig::HVID, AppConfig::GRAA_TEKST, 92);
    const bool fejl = strcmp(app.besked, "Kunne ikke gemme") == 0;
    centrer(p, fejl ? "Gemmefejl" : "kcal spist", "", AppConfig::FONT_FORSIDE.kcalTekst,
            AppConfig::KCAL_TEKST_Y, fejl ? AppConfig::FEJL : AppConfig::HVID, AppConfig::HVID, 86);
    formaterDansk(app.data.kalorier.ugentligtMaal, ugeMaal, sizeof(ugeMaal));
    snprintf(uge, sizeof(uge), "Ugentligt mål: %lu/", static_cast<unsigned long>(tal.spistDenneUge));
    const AAFont& ugeFont=findSkrift(AppConfig::FONT_FORSIDE.ugensTal);
    if (tekstBredde(uge,ugeFont)+tekstBredde(ugeMaal,ugeFont)<=124*256) {
      centrer(p, uge, ugeMaal, AppConfig::FONT_FORSIDE.ugensTal, AppConfig::UGETAL_Y,
              AppConfig::HVID, AppConfig::GRAA_TEKST, 124);
    } else {
      // Ekstreme testtal faar to linjer frem for at blive klippet i kanten.
      centrer(p, "Ugentligt mål:", "", 9, AppConfig::UGETAL_Y-12);
      snprintf(uge,sizeof(uge),"%lu/",static_cast<unsigned long>(tal.spistDenneUge));
      centrer(p, uge, ugeMaal, 9, AppConfig::UGETAL_Y, AppConfig::HVID, AppConfig::GRAA_TEKST,124);
    }
    sidebetjening(p, true, true);
  }

  inline void sidetitel(uint16_t* p, const KalorieAppModel& app, const char* tekst) {
    const bool fejl = strcmp(app.besked, "Kunne ikke gemme") == 0;
    centrer(p, fejl ? "Kunne ikke gemme" : tekst, "", AppConfig::FONT_TITEL,
            AppConfig::SIDETITEL_Y, fejl ? AppConfig::FEJL : AppConfig::GROEN);
  }

  inline void menu(uint16_t* p, const KalorieAppModel& app) {
    sidetitel(p, app, "Indstillinger");
    for (int i = 0; i < 3; ++i) {
      const int y = AppConfig::MENU_START_Y + i*(AppConfig::MENU_RAEKKE_HOEJDE + AppConfig::MENU_AFSTAND);
      const bool valgt = app.valgtRaekke == i;
      kort(p, AppConfig::MENU_X, y, AppConfig::MENU_BREDDE, AppConfig::MENU_RAEKKE_HOEJDE,
           7, AppConfig::KORT, AppConfig::GROEN, valgt ? 1.0f : 0);
      const char* tekst = AppConfig::MENU_TEKSTER[i];
      const AAFont& font = tilpas(tekst, "", AppConfig::FONT_MENU, AppConfig::MENU_BREDDE - (i==2?24:12));
      tegnTekst(p, tekst, font, (AppConfig::MENU_X+6)*256, y+AppConfig::MENU_RAEKKE_HOEJDE/2+4,
                i==2 ? AppConfig::FEJL : AppConfig::HVID);
      if (i==2) skraldespand(p, 100, y+5);
    }
    centrer(p, "Tilbage", "", 10, AppConfig::MENU_TILBAGE_Y,
            app.valgtRaekke==3 ? AppConfig::GROEN : AppConfig::GRAA_TEKST);
    sidebetjening(p, app.valgtRaekke>0, app.valgtRaekke<3);
  }

  inline void rediger(uint16_t* p, const KalorieAppModel& app) {
    const bool maal = app.side == Side::Ugemaal;
    const uint32_t minimum = maal ? AppConfig::MIN_UGEMAAL : AppConfig::MIN_LYS;
    const uint32_t maksimum = maal ? AppConfig::MAX_UGEMAAL : AppConfig::MAX_LYS;
    char tal[32], dag[32], beskrivelse[48];
    sidetitel(p, app, maal ? "Ugentligt mål" : "Lysstyrke");
    if (maal) {
      formaterDansk(app.kladde, tal, sizeof(tal));
      centrer(p, tal, "", 20, 55);
      centrer(p, "kcal pr. uge", "", 10, 75, AppConfig::GRAA_TEKST);
      formaterDansk(app.kladde/7, dag, sizeof(dag));
      snprintf(beskrivelse, sizeof(beskrivelse), "%s %s kcal pr. dag", app.kladde%7 ? "ca." : "=", dag);
      centrer(p, beskrivelse, "", 10, 95);
    } else {
      const unsigned lys = app.kladde;
      if (lys < AppConfig::MAX_LYS) {
        snprintf(tal, sizeof(tal), "%u%%", lys+AppConfig::LYS_TRIN);
        centrer(p, tal, "", 13, 48, AppConfig::DEMPET);
      }
      kort(p, 15, 56, 98, 27, 7, AppConfig::VAERDI_FELT);
      snprintf(tal, sizeof(tal), "%u%%", lys);
      centrer(p, tal, "", 20, 77);
      if (lys > AppConfig::MIN_LYS) {
        snprintf(tal, sizeof(tal), "%u%%", lys-AppConfig::LYS_TRIN);
        centrer(p, tal, "", 13, 99, AppConfig::DEMPET);
      }
    }
    knap(p, 15, 118, 98, "Gem", true, false, app.trykFeedback);
    centrer(p, "Swipe: Ændr | Tryk: Gem", "", 9, 155, AppConfig::GRAA_TEKST, AppConfig::GRAA_TEKST, 124);
    sidebetjening(p, app.kladde<maksimum, app.kladde>minimum);
  }

  inline void nulstil(uint16_t* p, const KalorieAppModel& app) {
    const bool fortryd = app.fokus == Fokus::Fortryd;
    sidetitel(p, app, "Nulstil data");
    centrer(p, "Nulstil dagens og", "", 11, 57);
    centrer(p, "ugens kalorier", "", 11, 73);
    centrer(p, "Ugentligt mål bevares", "", 9, 93, AppConfig::GRAA_TEKST);
    knap(p, 15, 118, 44, "Fortryd", fortryd, fortryd, app.trykFeedback);
    knap(p, 69, 118, 44, "Nulstil", false, !fortryd, app.trykFeedback, true);
    centrer(p, fortryd ? "Swipe: Vælg | Tryk: Fortryd" : "Swipe: Vælg | Tryk: Nulstil", "", 9, 155,
            AppConfig::GRAA_TEKST, AppConfig::GRAA_TEKST, 124);
    sidebetjening(p, !fortryd, fortryd);
  }

  inline void maaltid(uint16_t* p, const KalorieAppModel& app) {
    const bool accepter = app.fokus == Fokus::Gem;
    sidetitel(p, app, "Bekræft måltid");
    centrer(p, app.forslag.frugt, "", 16, 48);
    char tal[32];
    snprintf(tal, sizeof(tal), "%lu kcal", static_cast<unsigned long>(app.forslag.kcal));
    centrer(p, tal, "", 20, 76);
    snprintf(tal, sizeof(tal), "Standard: %u g", unsigned(app.forslag.gram));
    centrer(p, tal, "", 10, 98, AppConfig::GRAA_TEKST);
    knap(p, 15, 118, 43, "Afvis", false, !accepter, app.trykFeedback);
    knap(p, 66, 118, 53, "Accepter", true, accepter, app.trykFeedback);
    centrer(p, accepter ? "Tryk: Accepter" : "Tryk: Afvis", "", 10, 155, AppConfig::GRAA_TEKST);
    sidebetjening(p, accepter, !accepter);
  }

  inline void scanFejl(uint16_t* p, const KalorieAppModel& app) {
    const bool ingenFrugt = strcmp(app.scanFejl, "Ingen kendt frugt")==0 || strcmp(app.scanFejl, "Ingen frugt fundet")==0;
    const bool usikker = strcmp(app.scanFejl, "Usikker genkendelse")==0;
    kort(p, 50, 13, 28, 28, 14, AppConfig::KORT);
    if (ingenFrugt) {
      // Forbudssymbol med tynd, hvid cirkel og diagonal.
      for (int y=19; y<=35; ++y) for (int x=56; x<=72; ++x) {
        const float d=fabsf(hypotf(x-64.0f, y-27.0f)-7.5f);
        const float a=klip(1.2f-d,0,1);
        p[y*128+x]=blandPixel(AppConfig::HVID,p[y*128+x],alpha(a));
      }
      linje(p, 59, 22, 69, 32, AppConfig::HVID, 1.4f);
    } else centrer(p, usikker ? "?" : "!", "", 20, 35);
    centrer(p, ingenFrugt ? "Ingen frugt fundet" : app.scanFejl, "", 11, 79, AppConfig::HVID,
            AppConfig::HVID, 108);
    centrer(p, "Tag et nyt billede", "", 10, 99, AppConfig::GRAA_TEKST);
    knap(p, 26, 118, 76, "Prøv igen", true, true, app.trykFeedback);
    centrer(p, "Tryk: Prøv igen", "", 10, 155, AppConfig::GRAA_TEKST);
    sidebetjening(p, false, false);
  }
}

inline void tegnApp(uint16_t* pixels, const KalorieAppModel& app) {
  memset(pixels, 0, 128*160*sizeof(uint16_t));
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
