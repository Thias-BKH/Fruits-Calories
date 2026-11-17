#include <Arduino.h>

// Denne version bruger touch-periferien i den KLASSISKE ESP32-WROOM-32.
// Den er ikke til C6, S2 eller S3, som har andre/ingen touch-egenskaber.
#if !defined(CONFIG_IDF_TARGET_ESP32)
#error "Vaelg ESP32 Dev Module til ESP32-WROOM-32, ikke ESP32C6 Dev Module."
#endif

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "app_indstillinger.h"
#include "app_model.h"
#include "app_visning.h"
#include "app_overgang.h"
#include "data_lager.h"
#include "touch_input.h"
#include "touch_routing.h"
#include "frugt_scanner.h"

// SAMLET PROGRAM: Skaerm og jeres fem touchfelter sidder paa samme ESP32.
// Skift skaermben/farver/layout i app_indstillinger.h.
// Skift touchben/foelsomhed/timing i touch_indstillinger.h.
Adafruit_ST7735 tft(&SPI, AppConfig::PIN_CS, AppConfig::PIN_RS, AppConfig::PIN_RST);
KalorieAppModel app;
AppOvergang overgang;
TouchRouting touchRouting;
DataLager lager;
uint16_t skaermPixels[128 * 160];
bool pwmKlar = false;
uint8_t sidsteLys = 255;
uint32_t sidsteFrame = 0;

// Rigtig regulering af baggrundslys paa GPIO 21. Gem/Fortryd styres af appmodellen.
void opdaterLys() {
  uint8_t procent = app.aktivLysstyrke();
  if (procent == sidsteLys) return;
  sidsteLys = procent;
  if (pwmKlar) ledcWrite(AppConfig::PIN_LED, (procent * 255u + 50) / 100);
  else digitalWrite(AppConfig::PIN_LED, HIGH);
}

// Samme 40 KB bruges til skaermpixels og RGB-billede paa skift, aldrig samtidig.
void visScanning() {
  overgang.nulstil();
  app.skalTegnes = true;  // Gendan app-pixels efter upload, ogsaa hvis forbindelsen afbrydes.
  memset(skaermPixels, 0, sizeof(skaermPixels));
  AppDraw::centrer(skaermPixels, "Scanner...", "", 16, 65, AppConfig::GROEN);
  AppDraw::centrer(skaermPixels, "Vent på resultatet", "", 10, 90);
  tft.drawRGBBitmap(0, 0, skaermPixels, 128, 160);
}

// Computerinput er bevaret som test og som hjaelp under justering af sensoren.
void laesComputer() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'u' || c == 'U') app.input(Input::SwipeOp, millis());
    if (c == 'd' || c == 'D') app.input(Input::SwipeNed, millis());
    if (c == 'p' || c == 'P') app.input(Input::Tryk, millis());
    if (c == 'b' || c == 'B') app.input(Input::Tilbage, millis());
    if (c == '+' || c == 's') FrugtKalorier::modtag(app, "Æble", .95f, millis());
    if (c == 'f') FrugtKalorier::modtag(app, "Banan", .91f, millis());
    if (c == 'x') FrugtKalorier::modtag(app, "Background", .98f, millis());
    // Manuelle testkommandoer, indtil telefon/ur giver datoen.
    if (c == 'n') app.nyDag(millis());
    if (c == 'v') app.nyUge(millis());
    if (c == '?') Serial.println("u=op, d=ned, p=tryk, b=tilbage, s/+=test-aeble, f=test-banan, x=baggrund, n=ny dag, v=ny uge");
  }
}

// Vis en tydelig besked, mens baseline maales paa de fem felter.
void visKalibrering() {
  memset(skaermPixels, 0, sizeof(skaermPixels));
  AppDraw::centrer(skaermPixels, "Kalibrerer...", "", 13, 52, AppConfig::GROEN);
  AppDraw::centrer(skaermPixels, "Hold fingrene væk", "", 10, 81);
  AppDraw::centrer(skaermPixels, "fra touchfelterne", "", 10, 97);
  tft.drawRGBBitmap(0, 0, skaermPixels, 128, 160);
}

void setup() {
  Serial.begin(AppConfig::SERIAL_HASTIGHED);
  pinMode(AppConfig::PIN_LED, OUTPUT);
  digitalWrite(AppConfig::PIN_LED, HIGH);

  // ESP32-WROOM-32: SCK=18, SDI=23, CS=27, RS=25 og RST=22.
  SPI.begin(AppConfig::PIN_SCK, -1, AppConfig::PIN_SDI, AppConfig::PIN_CS);
  tft.initR(INITR_BLACKTAB);
  tft.setSPISpeed(AppConfig::SPI_HASTIGHED);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  // Gemte maal, kalorier og lysstyrke genbruges paa DETTE kort.
  bool lagerKlar = lager.start();
  AppData gemt;
  if (lagerKlar && AppConfig::BRUG_GEMTE_DATA && lager.laes(gemt)) app.indlaes(gemt);
  else if (lagerKlar && AppConfig::BRUG_GEMTE_DATA) lager.gem(app.data);
  pwmKlar = ledcAttach(AppConfig::PIN_LED, 5000, 8);
  opdaterLys();

  // Kalibrering koerer kun ved opstart; roer ikke slideren foer "Slider klar".
  visKalibrering();
  bool touchKlar = TouchInput::start();
  app.visBesked(touchKlar ? "Slider klar" : "Slider kunne ikke starte", millis());
  Serial.println(touchKlar ? "Touch og skaerm klar paa samme ESP32-WROOM-32."
                          : "Touchopgaven kunne ikke starte. Serial-test er stadig mulig.");
  Serial.println("Serial Monitor: 115200 baud. Send ? for hjaelp.");
  const bool scannerKlar = FrugtScanner::start(app, skaermPixels, sizeof(skaermPixels), visScanning);
  if (!scannerKlar) { app.visScanFejl("Wi-Fi kunne ikke starte"); Serial.println("Scanner kunne ikke starte."); }

  overgang.tegn(skaermPixels, app, millis());
  tft.drawRGBBitmap(0, 0, skaermPixels, 128, 160);
  app.skalTegnes = false;
  sidsteFrame = millis();
}

void loop() {
  // Billedmodtagelse og inferens er synkron. Drop gamle touch-events fra ventetiden,
  // saa de ikke kan vaelge Accepter, naar resultatet foerst bliver synligt.
  if (FrugtScanner::opdater()) {
    TouchEvent gammel;
    while (TouchInput::laes(gammel)) {}
  }
  // Touch-opgaven leverer events via en koe. Kun denne loekke aendrer appen.
  TouchEvent event;
  while (TouchInput::laes(event)) touchRouting.handle(event, app, millis());
  laesComputer();
  app.tick(millis());
  opdaterLys();
  if (app.skalGemmes) app.gemningFaerdig(lager.gem(app.data), millis());

  // Animationen tager et enkelt skridt ad gangen; input og touch fortsaetter imens.
  const uint32_t nu = millis();
  if ((app.skalTegnes || overgang.erAktiv()) && nu - sidsteFrame >= AppConfig::FRAME_MS) {
    sidsteFrame = nu;  // Maal fra billedets start, saa SPI-tiden ikke laegges oveni.
    overgang.tegn(skaermPixels, app, nu);
    tft.drawRGBBitmap(0, 0, skaermPixels, 128, 160);
    app.skalTegnes = false;
  }
  delay(1);
}
