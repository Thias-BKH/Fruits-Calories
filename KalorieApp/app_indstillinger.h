#pragma once
#include <stdint.h>
#include "app_typer.h"

// HER TILPASSER I PROGRAMMET. De andre filer bruger disse vaerdier.
// Farver, tekst, trin og placeringer kan aendres uden at redigere skriftdata.
namespace AppConfig {
  // STARTDATA: Bruges foerste gang. Gemte indstillinger har derefter forrang.
  constexpr KalorieData START_KALORIER{16000, 0, 0};
  constexpr uint8_t START_LYSSTYRKE = 100;
  constexpr bool BRUG_GEMTE_DATA = true;

  // SKAERMENS LEDNINGER: Jeres bekraeftede GPIO-numre paa ESP32-WROOM-32.
  // SDI=23, SCK=18, RS=25, CS=27, RST=22, LED=21. VCC=3V3 og GND=GND.
  constexpr int PIN_CS = 27;
  constexpr int PIN_RS = 25;
  constexpr int PIN_RST = 22;
  constexpr int PIN_SCK = 18;
  constexpr int PIN_SDI = 23;
  constexpr int PIN_LED = 21;
  // 8 MHz giver flere billeder i overgangen; ved ustabile ledninger kan 4000000 bruges.
  constexpr uint32_t SPI_HASTIGHED = 8000000;

  // TEKST: Vaelg 10..20. Automatisk tilpasning gaar aldrig under FONT_MIN.
  constexpr uint8_t FONT_MIN = 10;
  constexpr TekstStoerrelser FONT_FORSIDE{
    16, // Tallene i midten.
    10, // "Kalorier".
    10, // Overskriften "Ugentlig maal".
    10  // Ugens kalorier og maal paa en separat linje.
  };
  constexpr uint8_t FONT_TITEL = 13;
  constexpr uint8_t FONT_MENU = 10;
  constexpr uint8_t FONT_STOR_VAERDI = 20;
  constexpr uint8_t FONT_BESKRIVELSE = FONT_MIN;
  constexpr uint8_t FONT_KNAP = 10;
  constexpr uint8_t FONT_HINT = FONT_MIN;

  // FORSIDENS PLACERINGER: Stoerre Y flytter indholdet laengere ned.
  constexpr float RING_SKALA = 1.10f;
  constexpr int RING_MIDTE_Y = 85;
  constexpr int DAGSTAL_Y = 68;
  constexpr int KCAL_TEKST_Y = 84;
  constexpr int UGEOVERSKRIFT_Y = 136;
  constexpr int UGETAL_Y = 150;
  // false: Ringen fyldes efter dagens SPISTE kalorier. true: Vis kalorier tilbage.
  // Ringen er altid groen; tallet i midten viser dagens spiste kalorier.
  constexpr bool RING_VISER_TILBAGE = false;
  // En hel groen ring markerer, at dagsmaalet er naaet.
  constexpr bool HEL_RING_VED_MAAL = true;

  // MENU: Tre raekker og en lille, separat "Tilbage"-mulighed nederst.
  constexpr const char* MENU_TEKSTER[3] = {
    "Registrer kalorier", "Lysstyrke", "Nulstil data"
  };
  constexpr int MENU_X = 9;
  constexpr int MENU_BREDDE = 109;
  constexpr int MENU_START_Y = 40;
  constexpr int MENU_RAEKKE_HOEJDE = 21;
  constexpr int MENU_AFSTAND = 7;
  constexpr int MENU_TILBAGE_Y = 149;

  // FAELLES LAYOUT: Luft mellem elementerne og faste positioner uden popups.
  constexpr int SIDETITEL_Y = 22;
  constexpr int SKYDER_Y = 104;
  constexpr int KNAP_Y = 117;
  constexpr int KNAP_HOEJDE = 22;
  constexpr int KNAP_BREDDE = 48;
  constexpr int KNAP_VENSTRE_X = 10;
  constexpr int KNAP_HOEJRE_X = 70;
  constexpr int HINT_Y = 155;

  // UGEMAAL: Swipe aendrer med dette antal kcal. Dagsmaal = ugemaal / 7.
  constexpr uint32_t MAAL_TRIN = 100;
  constexpr uint32_t MIN_UGEMAAL = 700;
  constexpr uint32_t MAX_UGEMAAL = 70000;
  // LYSSTYRKE: 10 % som minimum holder skaermen synlig under indstilling.
  constexpr uint8_t MIN_LYS = 10;
  constexpr uint8_t MAX_LYS = 100;
  constexpr uint8_t LYS_TRIN = 5;

  // FEEDBACK: Varighed i millisekunder. Ingen ventende delay() i navigationen.
  constexpr uint32_t TRYK_FEEDBACK_MS = 140;
  constexpr uint32_t BESKED_MS = 1500;
  // Korte sideskift med bloe start/stop. Saet OVERGANG_MS til 0 for at slaa fra.
  constexpr uint32_t OVERGANG_MS = 320;
  constexpr uint32_t FRAME_MS = 25;

  // BETJENING: Skaerm og femfelts-touchslider sidder nu paa SAMME ESP32-WROOM-32.
  constexpr uint32_t SERIAL_HASTIGHED = 115200;

  // FARVER: R, G og B er tal fra 0..255. Sort baggrund giver hoej kontrast.
  constexpr uint16_t rgb(int r, int g, int b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
  }
  constexpr uint16_t SORT = rgb(0, 0, 0);
  constexpr uint16_t HVID = rgb(255, 255, 255);
  constexpr uint16_t GRAA_TEKST = rgb(170, 170, 170);
  constexpr uint16_t GRAA_BUE = rgb(72, 72, 72);
  constexpr uint16_t KORT = rgb(55, 55, 55);
  constexpr uint16_t VALGT_KORT = rgb(66, 66, 66);
  constexpr uint16_t GROEN = rgb(0, 225, 0);
  constexpr uint16_t FEJL = rgb(255, 95, 75);
}
