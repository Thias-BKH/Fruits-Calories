#pragma once
#include <WiFi.h>
#include <WebServer.h>
#include <Image_classification_Fruits__inferencing.h>
#include "frugt_kalorier.h"
#include "rgb_billede.h"
#include "telefon_side.h"

static_assert(EI_CLASSIFIER_INPUT_WIDTH == 96 && EI_CLASSIFIER_INPUT_HEIGHT == 96,
              "Installer RGB-modellen 1.0.8 fra models-mappen.");
static_assert(EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE == RGBBillede::PIXELS,
              "Modellen skal modtage 96x96 RGB-pixels.");
static_assert(EI_CLASSIFIER_PROJECT_DEPLOY_VERSION == 8 &&
              EI_CLASSIFIER_NN_INPUT_FRAME_SIZE == RGBBillede::PIXELS * 3,
              "Brug RGB-modelbibliotek 1.0.8 fra models, ikke den gamle graatone-model.");

// WebServer og appmodellen koerer paa samme task. Der er ingen parallelle app-skrivninger.
namespace FrugtScanner {
  static WebServer server(80);
  static KalorieAppModel* app = nullptr;
  static uint8_t* buffer = nullptr;
  static size_t bufferBytes = 0;
  static void (*visScanner)() = nullptr;
  static RGBBillede billede;
  static int uploadFejl = 0;
  static bool rydInput = false;

  inline int signalData(size_t offset, size_t length, float* out) {
    return billede.signal(offset, length, out);
  }

  inline String statusJson() {
    String json = "{\"pending\":";
    json += app->forslag.aktiv ? "true" : "false";
    json += ",\"can_scan\":";
    json += app->kanScanne() ? "true" : "false";
    json += ",\"fruit\":\""; json += app->forslag.frugt;
    json += "\",\"kcal\":"; json += app->forslag.kcal;
    json += ",\"grams\":"; json += app->forslag.gram;
    json += ",\"confidence\":"; json += app->forslag.sikkerhed;
    json += ",\"today\":"; json += app->data.kalorier.spistIDag;
    json += ",\"message\":\"";
    json += app->side == Side::ScanFejl ? app->scanFejl : "Kontrollér resultatet på skærmen";
    json += "\"}";
    return json;
  }

  inline void uploadData() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      if (!app->kanScanne()) { uploadFejl = 409; return; }
      if (uploadFejl) return;
      if (upload.name != "image" || !billede.start(buffer, bufferBytes)) {
        uploadFejl = 400; return;
      }
      visScanner();  // Frigiver animationen og tegner status FOER bufferens pixels overskrives.
      rydInput = true;
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (!uploadFejl && !billede.tilfoej(upload.buf, upload.currentSize)) uploadFejl = 413;
    } else if (upload.status == UPLOAD_FILE_END) {
      billede.afslut();
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
      billede.nulstil(); uploadFejl = 0;
      app->visScanFejl("Upload afbrudt");
    }
  }

  inline void uploadFaerdig() {
    server.sendHeader("Cache-Control", "no-store");
    if (uploadFejl || !billede.klar()) {
      const int status = uploadFejl ? uploadFejl : 400;
      billede.nulstil(); uploadFejl = 0;
      if (status == 409) {
        server.send(409, "application/json; charset=utf-8",
                    "{\"error\":\"Afslut dit valg på ESP32-skærmen før en ny scanning.\"}");
      } else {
        app->visScanFejl("Ugyldigt billede");
        server.send(status, "application/json; charset=utf-8",
                    "{\"error\":\"Send ét billede på 96x96 RGB888-pixels fra denne side.\"}");
      }
      return;
    }
    signal_t signal;
    signal.total_length = RGBBillede::PIXELS;
    signal.get_data = &signalData;
    ei_impulse_result_t resultat = {};
    Serial.printf("Scanner: heap=%u, stoerste blok=%u\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
    const EI_IMPULSE_ERROR fejl = run_classifier(&signal, &resultat, false);
    billede.nulstil();
    if (fejl != EI_IMPULSE_OK) {
      Serial.printf("Modelfejl: %d\n", int(fejl));
      app->visScanFejl("Model kunne ikke køre");
      server.send(500, "application/json; charset=utf-8",
                  "{\"error\":\"Modellen kunne ikke køre. Se Serial Monitor for fejlkode og fri hukommelse.\"}");
      return;
    }
    const char* label = nullptr;
    float sikkerhed = -1;
    for (size_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; ++i) {
      const auto& prediction = resultat.classification[i];
      Serial.printf("%s: %.1f%%\n", prediction.label, prediction.value * 100);
      if (prediction.value > sikkerhed) { sikkerhed = prediction.value; label = prediction.label; }
    }
    FrugtKalorier::modtag(*app, label, sikkerhed, millis());
    server.send(200, "application/json; charset=utf-8", statusJson());
  }

  inline bool start(KalorieAppModel& model, uint16_t* skaermBuffer, size_t bytes, void (*vis)()) {
    app = &model; buffer = reinterpret_cast<uint8_t*>(skaermBuffer); bufferBytes = bytes; visScanner = vis;
    if (bytes < RGBBillede::BYTES) return false;
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP("FruitClassifier", "fruit1234")) return false;
    server.on("/", HTTP_GET, []() { server.send_P(200, "text/html; charset=utf-8", TELEFON_SIDE); });
    server.on("/api/status", HTTP_GET, []() {
      server.sendHeader("Cache-Control", "no-store");
      server.send(200, "application/json; charset=utf-8", statusJson());
    });
    server.on("/upload", HTTP_POST, uploadFaerdig, uploadData);
    server.begin();
    Serial.print("Telefon: forbind til FruitClassifier og aabn http://");
    Serial.println(WiFi.softAPIP());
    return true;
  }

  inline bool opdater() {
    server.handleClient();
    const bool ryd = rydInput;
    rydInput = false;
    return ryd;
  }
}
