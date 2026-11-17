// Kun til computer-preview. Denne fil ligger uden for Arduinos sketch/src-mapper.
#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include "app_overgang.h"
#include "frugt_kalorier.h"

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
      unsigned active = 0, gram = 0, confidence = 0;
      std::string fruit, error;
      input >> active >> app.forslag.kcal >> gram >> confidence >> std::quoted(fruit) >> std::quoted(error);
      app.forslag.aktiv = active;
      app.forslag.gram = gram;
      app.forslag.sikkerhed = confidence;
      snprintf(app.forslag.frugt, sizeof(app.forslag.frugt), "%s", fruit.c_str());
      snprintf(app.scanFejl, sizeof(app.scanFejl), "%s", error.c_str());
    } else if (action == "restart") app = KalorieAppModel();
    else if (action == "up") app.input(Input::SwipeOp, now);
    else if (action == "down") app.input(Input::SwipeNed, now);
    else if (action == "press") app.input(Input::Tryk, now);
    else if (action == "back") app.input(Input::Tilbage, now);
    else if (action == "add" || action == "apple") FrugtKalorier::modtag(app, "Æble", .95f, now);
    else if (action == "banana") FrugtKalorier::modtag(app, "Banan", .91f, now);
    else if (action == "background") FrugtKalorier::modtag(app, "Background", .98f, now);
    else if (action == "uncertain") FrugtKalorier::modtag(app, "Æble", .4f, now);
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
              << ",\"pending\":" << (app.forslag.aktiv ? "true" : "false")
              << ",\"pendingKcal\":" << app.forslag.kcal
              << ",\"pendingGrams\":" << app.forslag.gram
              << ",\"confidence\":" << unsigned(app.forslag.sikkerhed)
              << ",\"fruit\":" << std::quoted(app.forslag.frugt)
              << ",\"scanError\":" << std::quoted(app.scanFejl)
              << ",\"sideDepth\":" << unsigned(AppMotion::niveau(app.side))
              << ",\"transitionMs\":" << AppConfig::OVERGANG_MS
              << "}\n";
    std::cout.write(reinterpret_cast<const char*>(bytes), sizeof(bytes));
    std::cout.flush();
  }
}
