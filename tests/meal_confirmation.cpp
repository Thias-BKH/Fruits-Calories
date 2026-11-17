#include <assert.h>
#include <limits>
#include <vector>
#include <iostream>
#include "frugt_kalorier.h"
#include "data_format.h"
#include "touch_routing.h"
#include "rgb_billede.h"
#include "app_overgang.h"

static bool scan(KalorieAppModel& app, const char* label = "Æble", float confidence = .95f) {
  return FrugtKalorier::modtag(app, label, confidence, 1000);
}

int main() {
  KalorieAppModel app;
  uint32_t saved[DataFormat::ANTAL_ORD], pending[DataFormat::ANTAL_ORD];
  DataFormat::pak(app.data, saved);
  assert(scan(app));
  assert(app.side == Side::Maaltid && app.fokus == Fokus::Fortryd);
  assert(app.forslag.kcal == 69 && app.forslag.gram == 150);
  assert(app.data.kalorier.spistIDag == 0 && !app.skalGemmes);
  DataFormat::pak(app.data, pending);
  assert(memcmp(saved, pending, sizeof(saved)) == 0);  // Preview maa ikke gemmes som spist.

  assert(!scan(app, "Banan"));  // Et ventende resultat maa ikke blive udskiftet under fingeren.
  assert(strcmp(app.forslag.frugt, "Æble") == 0);
  app.input(Input::Tryk, 1100);  // Et utilsigtet tryk paa standardsiden afviser.
  assert(!app.forslag.aktiv && app.data.kalorier.spistIDag == 0 && !app.skalGemmes);
  assert(scan(app));
  app.input(Input::Tilbage, 1200);
  assert(app.side == Side::Forside && !app.forslag.aktiv && !app.skalGemmes);

  assert(scan(app, "Banan"));
  TouchRouting routing;
  routing.handle({Input::SwipeNed, 1}, app, 1300);
  routing.handle({Input::SwipeNed, 1}, app, 1310);  // Et langt swipe maa kun flytte en knap.
  assert(app.fokus == Fokus::Gem);
  routing.handle({Input::Tryk, 2}, app, 1400);
  assert(app.data.kalorier.spistIDag == 106 && app.skalGemmes && !app.forslag.aktiv);
  app.input(Input::Tryk, 1401);
  app.input(Input::Tryk, 1402);
  assert(app.data.kalorier.spistIDag == 106);  // Gentagne tryk maa ikke registrere igen.
  app.gemningFaerdig(true, 1500);
  app.input(Input::Tilbage, 1501);
  app.input(Input::Tilbage, 1502);
  assert(app.side == Side::Forside);
  DataFormat::pak(app.data, saved);
  KalorieAppModel restarted;
  assert(DataFormat::pakUd(saved, restarted.data));
  assert(restarted.data.kalorier.spistIDag == 106 && !restarted.forslag.aktiv);

  for (const char* label : {"Background", "Ukendt", "", static_cast<const char*>(nullptr)}) {
    assert(!scan(app, label));
    assert(!app.forslag.aktiv && app.data.kalorier.spistIDag == 106 && !app.skalGemmes);
  }
  for (float confidence : {.59f, -1.f, 1.1f, std::numeric_limits<float>::quiet_NaN()}) {
    assert(!scan(app, "Æble", confidence));
    assert(!app.forslag.aktiv && !app.skalGemmes);
  }
  assert(scan(app, "Æble", .60f));
  app.nyDag(1600);
  assert(!app.forslag.aktiv && app.data.kalorier.spistIDag == 0);
  assert(app.data.kalorier.spistTidligereDage == 106);
  app.gemningFaerdig(true, 1700);
  assert(scan(app));
  app.nyUge(1800);
  assert(!app.forslag.aktiv && app.data.kalorier.spistTidligereDage == 0);

  // Indstillingskladder maa ikke overskrives af en scanning.
  app.input(Input::Tryk, 1900);
  app.input(Input::SwipeNed, 1901);
  app.input(Input::Tryk, 1902);
  assert(app.side == Side::Lysstyrke && !scan(app));

  // RGB-chunks maa ikke skrive forbi skaermbufferen eller genbruge en gammel upload.
  std::vector<uint8_t> pixels(RGBBillede::BYTES + 1, 0);
  pixels.back() = 123;
  RGBBillede rgb;
  assert(!rgb.start(pixels.data(), 1));
  rgb.nulstil();
  assert(rgb.start(pixels.data(), RGBBillede::BYTES));
  const uint8_t first[] = {0x12, 0x34, 0x56, 0xAB, 0xCD, 0xEF};
  assert(rgb.tilfoej(first, 5));
  assert(rgb.tilfoej(first + 5, 1));
  std::vector<uint8_t> rest(RGBBillede::BYTES - 6, 0);
  assert(rgb.tilfoej(rest.data(), rest.size()));
  assert(!rgb.klar());
  rgb.afslut();
  assert(rgb.klar());
  float signal[2];
  assert(rgb.signal(0, 2, signal) == 0 && signal[0] == 0x123456 && signal[1] == 0xABCDEF);
  assert(rgb.signal(RGBBillede::PIXELS, 1, signal) == -1);
  assert(!rgb.tilfoej(first, 1) && !rgb.klar() && pixels.back() == 123);
  rgb.nulstil();
  assert(!rgb.klar() && rgb.signal(0, 1, signal) == -1);
  assert(rgb.start(pixels.data(), RGBBillede::BYTES));
  assert(!rgb.start(pixels.data(), RGBBillede::BYTES));
  rgb.afslut();
  assert(!rgb.klar());

  // Alle nye sider skal kunne renderes med den rigtige 128x160-buffer.
  KalorieAppModel rendered;
  std::vector<uint16_t> screen(128 * 160);
  AppOvergang animation;
  animation.tegn(screen.data(), rendered, 0);
  assert(scan(rendered));
  animation.tegn(screen.data(), rendered, 100);
  animation.tegn(screen.data(), rendered, 500);
  assert(!animation.erAktiv());
  rendered.input(Input::Tilbage, 600);
  rendered.visScanFejl("Ingen kendt frugt");
  animation.nulstil();
  animation.tegn(screen.data(), rendered, 700);
  std::cout << "PASS: confirmation, cancel, single registration, persistence, scan policy, touch, RGB bounds, rendering\n";
}
