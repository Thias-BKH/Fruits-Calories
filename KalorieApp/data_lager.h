#pragma once
#include <Preferences.h>
#include "data_format.h"

// NVS/Preferences ligger paa ESP32-kortet og bevarer data uden stroem.
// Kun bekraeftede aendringer gemmes; swipe skriver ikke loebende til flash.
class DataLager {
 public:
  bool start() { klar = prefs.begin("kalorieui", false); return klar; }
  bool laes(AppData& data) {
    uint32_t ord[DataFormat::ANTAL_ORD] = {};
    if (!klar || prefs.getBytesLength("data") != sizeof(ord)) return false;
    if (prefs.getBytes("data", ord, sizeof(ord)) != sizeof(ord)) return false;
    return DataFormat::pakUd(ord, data);
  }
  bool gem(const AppData& data) {
    if (!klar) return false;
    uint32_t ord[DataFormat::ANTAL_ORD] = {};
    DataFormat::pak(data, ord);
    return prefs.putBytes("data", ord, sizeof(ord)) == sizeof(ord);
  }
 private:
  Preferences prefs;
  bool klar = false;
};
