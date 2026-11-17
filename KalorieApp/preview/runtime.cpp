// Kun til computer-preview. Denne fil ligger uden for Arduinos sketch/src-mapper.
#include <iostream>
#include <sstream>
#include <string>
#include "app_overgang.h"

int main() {
  KalorieAppModel app;
  uint16_t pixels[128 * 160];
  unsigned char bytes[128 * 160 * 2];
  std::string line;
  while (std::getline(std::cin, line)) {
    std::istringstream input(line);
    std::string action;
    uint32_t now = 0;
    input >> action >> now;
    if (action == "restore") {
      unsigned brightness, side, focus, row;
      input >> app.data.kalorier.ugentligtMaal >> app.data.kalorier.spistIDag
            >> app.data.kalorier.spistTidligereDage >> brightness
            >> side >> focus >> row >> app.kladde;
      app.data.lysstyrke = static_cast<uint8_t>(brightness);
      app.side = static_cast<Side>(side);
      app.fokus = static_cast<Fokus>(focus);
      app.valgtRaekke = static_cast<uint8_t>(row);
    } else if (action == "restart") app = KalorieAppModel();
    else if (action == "up") app.input(Input::SwipeOp, now);
    else if (action == "down") app.input(Input::SwipeNed, now);
    else if (action == "press") app.input(Input::Tryk, now);
    else if (action == "back") app.input(Input::Tilbage, now);
    else if (action == "add") app.registrerMaaltid(100, now);
    else if (action == "day") app.nyDag(now);
    else if (action == "week") app.nyUge(now);
    app.tick(now);
    // Previewdata lever kun i denne proces. ESP32'ens lager bruges ikke.
    if (app.skalGemmes) app.gemningFaerdig(true, now);
    tegnApp(pixels, app);
    for (size_t i = 0; i < 128 * 160; ++i) {
      bytes[i * 2] = pixels[i] & 255;
      bytes[i * 2 + 1] = pixels[i] >> 8;
    }
    std::cout << "{\"side\":" << unsigned(app.side)
              << ",\"fokus\":" << unsigned(app.fokus)
              << ",\"row\":" << unsigned(app.valgtRaekke)
              << ",\"draft\":" << app.kladde
              << ",\"weekly\":" << app.data.kalorier.ugentligtMaal
              << ",\"today\":" << app.data.kalorier.spistIDag
              << ",\"previous\":" << app.data.kalorier.spistTidligereDage
              << ",\"brightness\":" << unsigned(app.data.lysstyrke)
              << ",\"activeBrightness\":" << unsigned(app.aktivLysstyrke())
              << ",\"sideDepth\":" << unsigned(AppMotion::niveau(app.side))
              << ",\"transitionMs\":" << AppConfig::OVERGANG_MS
              << "}\n";
    std::cout.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
    std::cout.flush();
  }
}
