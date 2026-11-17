#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "kalorie_aktiver.h"

constexpr int SKAERM_BREDDE = 128;
constexpr int SKAERM_HOEJDE = 160;

#include "app_typer.h"

inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

inline uint16_t blandPixel(uint16_t farve, uint16_t baggrund, uint8_t alpha) {
  const uint16_t modsat = 255 - alpha;
  const uint16_t r = (((farve >> 11) & 31) * alpha
                    + ((baggrund >> 11) & 31) * modsat + 127) / 255;
  const uint16_t g = (((farve >> 5) & 63) * alpha
                    + ((baggrund >> 5) & 63) * modsat + 127) / 255;
  const uint16_t b = ((farve & 31) * alpha
                    + (baggrund & 31) * modsat + 127) / 255;
  return (r << 11) | (g << 5) | b;
}

inline uint16_t naesteTegn(const char*& tekst) {
  const uint8_t foerste = static_cast<uint8_t>(*tekst++);
  if (foerste < 128) return foerste;
  if ((foerste & 0xE0) == 0xC0 && *tekst) {
    const uint8_t andet = static_cast<uint8_t>(*tekst++);
    return ((foerste & 0x1F) << 6) | (andet & 0x3F);
  }
  return '?';
}

inline const AAGlyph* findTegn(const AAFont& font, uint16_t tegn) {
  for (uint8_t i = 0; i < font.antal; ++i) {
    if (font.tegn[i].kode == tegn) return &font.tegn[i];
  }
  return nullptr;
}

inline int32_t tekstBredde(const char* tekst, const AAFont& font) {
  int32_t bredde = 0;
  while (*tekst) {
    const AAGlyph* tegn = findTegn(font, naesteTegn(tekst));
    if (tegn) bredde += tegn->fremQ8;
  }
  return bredde;
}

inline int32_t tegnTekst(uint16_t* pixels, const char* tekst, const AAFont& font,
                        int32_t xQ8, int grundlinje, uint16_t farve) {
  while (*tekst) {
    const AAGlyph* tegn = findTegn(font, naesteTegn(tekst));
    if (!tegn) continue;
    const int venstre = (xQ8 + 128) / 256 - 1;
    const int top = grundlinje - font.grundlinje;
    for (int y = 0; y < font.hoejde; ++y) {
      for (int x = 0; x < tegn->bredde; ++x) {
        const int px = venstre + x;
        const int py = top + y;
        if (px < 0 || px >= SKAERM_BREDDE || py < 0 || py >= SKAERM_HOEJDE) continue;
        const size_t tekstPixel = tegn->offset + y * tegn->bredde + x;
        const uint8_t pakket = font.pixels[tekstPixel / 2];
        const uint8_t alpha = ((tekstPixel & 1) ? (pakket & 15) : (pakket >> 4)) * 17;
        if (!alpha) continue;
        uint16_t& pixel = pixels[py * SKAERM_BREDDE + px];
        pixel = blandPixel(farve, pixel, alpha);
      }
    }
    xQ8 += tegn->fremQ8;
  }
  return xQ8;
}

inline void tegnCentreret(uint16_t* pixels, const char* lyst, const char* graat,
                          const AAFont& font, int grundlinje) {
  const int32_t bredde = tekstBredde(lyst, font) + tekstBredde(graat, font);
  const int32_t x = (SKAERM_BREDDE * 256 - bredde) / 2;
  const int32_t efter = tegnTekst(pixels, lyst, font, x, grundlinje, rgb565(255, 255, 255));
  tegnTekst(pixels, graat, font, efter, grundlinje, rgb565(170, 170, 170));
}

inline void formaterDansk(uint32_t tal, char* tekst, size_t kapacitet) {
  char omvendt[20];
  size_t antal = 0;
  uint8_t gruppe = 0;
  do {
    if (gruppe == 3) {
      omvendt[antal++] = '.';
      gruppe = 0;
    }
    omvendt[antal++] = '0' + tal % 10;
    tal /= 10;
    ++gruppe;
  } while (tal && antal < sizeof(omvendt));
  if (kapacitet == 0) return;
  size_t skrevet = 0;
  while (antal > 0 && skrevet + 1 < kapacitet) tekst[skrevet++] = omvendt[--antal];
  tekst[skrevet] = '\0';
}
