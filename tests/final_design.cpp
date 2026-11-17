#include <assert.h>
#include <vector>
#include <iostream>
#include "app_visning.h"
#include "frugt_kalorier.h"
#include "data_format.h"

int main() {
  KalorieAppModel app;
  assert(app.data.kalorier.ugentligtMaal == 14000);
  assert(app.data.kalorier.spistIDag == 0 && app.data.lysstyrke == 75);
  assert(beregnKalorier(app.data.kalorier).tilbageIDag == 2000);
  assert(beregnKalorier(app.data.kalorier).groentAfBaren == 0);
  app.input(Input::Tryk, 0);
  app.input(Input::SwipeOp, 1);
  assert(app.valgtRaekke == 0);  // Den graa pil maa ikke flytte forbi foerste raekke.
  app.input(Input::Tryk, 2);
  app.input(Input::SwipeOp, 3);
  assert(app.kladde == 14100 && app.data.kalorier.ugentligtMaal == 14000);
  app.input(Input::Tryk, 4);  // Gem skal kun kraeve et tryk.
  assert(app.side == Side::Indstillinger && app.data.kalorier.ugentligtMaal == 14100 && app.skalGemmes);
  app.gemningFaerdig(true, 5);
  app.input(Input::Tryk, 6);
  app.input(Input::SwipeNed, 7);
  app.input(Input::Tilbage, 8);
  assert(app.data.kalorier.ugentligtMaal == 14100 && !app.skalGemmes);

  app.input(Input::SwipeNed, 9);
  app.input(Input::Tryk, 10);
  assert(app.side == Side::Lysstyrke && app.kladde == 75);
  app.input(Input::SwipeNed, 11);
  assert(app.aktivLysstyrke() == 70 && app.data.lysstyrke == 75);
  app.input(Input::Tilbage, 12);
  assert(app.aktivLysstyrke() == 75 && !app.skalGemmes);
  app.input(Input::Tryk, 13);
  app.input(Input::SwipeOp, 14);
  app.input(Input::Tryk, 15);
  assert(app.data.lysstyrke == 80 && app.side == Side::Indstillinger && app.skalGemmes);
  app.gemningFaerdig(true, 16);
  for (int i=0;i<8;++i) app.input(Input::SwipeNed, 17+i);
  assert(app.valgtRaekke == 3);
  app.input(Input::Tryk, 26);
  assert(app.side == Side::Forside);

  assert(FrugtKalorier::modtag(app, "Æble", .95f, 30));
  app.input(Input::SwipeNed, 31);
  app.input(Input::SwipeNed, 32);
  assert(app.fokus == Fokus::Gem);  // Samme retning vender ikke tilbage til Afvis.
  app.input(Input::SwipeOp, 33);
  app.input(Input::SwipeOp, 34);
  assert(app.fokus == Fokus::Fortryd);
  app.input(Input::Tryk, 35);
  assert(app.data.kalorier.spistIDag == 0 && !app.skalGemmes);
  assert(FrugtKalorier::modtag(app, "Banan", .95f, 36));
  app.input(Input::SwipeNed, 37);
  app.input(Input::Tryk, 38);
  assert(app.data.kalorier.spistIDag == 106);
  assert(beregnKalorier(app.data.kalorier).groentAfBaren > 0);
  assert(beregnKalorier(app.data.kalorier).tilbageIDag == 1908);  // 14100/7 = 2014.
  app.gemningFaerdig(true, 39);
  app.data.kalorier.spistIDag = 3000;
  assert(beregnKalorier(app.data.kalorier).tilbageIDag == 0);
  assert(beregnKalorier(app.data.kalorier).groentAfBaren == 255);
  assert(beregnKalorier({14000,1000,0}).groentAfBaren == 128);
  assert(beregnKalorier({14000,2000,0}).groentAfBaren == 255);
  assert(beregnKalorier({14000,1000000,0}).groentAfBaren == 255);
  assert(beregnKalorier({0,0,0}).groentAfBaren == 0);

  app.input(Input::Tryk, 40);
  app.input(Input::SwipeNed, 41);
  app.input(Input::SwipeNed, 42);
  app.input(Input::Tryk, 43);
  assert(app.side == Side::Nulstil && app.fokus == Fokus::Fortryd);
  app.input(Input::Tryk, 44);
  assert(app.data.kalorier.spistIDag == 3000 && !app.skalGemmes);
  app.input(Input::Tryk, 45);
  app.input(Input::SwipeNed, 46);
  app.input(Input::Tryk, 47);
  assert(app.data.kalorier.spistIDag == 0 && app.data.kalorier.ugentligtMaal == 14100);

  app.visScanFejl("Usikker genkendelse");
  app.input(Input::SwipeNed, 48);
  assert(app.side == Side::ScanFejl);
  app.input(Input::Tryk, 49);
  assert(app.side == Side::Forside && app.kanScanne());
  std::vector<uint16_t> pixels(128*160+2, 0x1234);
  for (Side side : {Side::Forside,Side::Indstillinger,Side::Ugemaal,Side::Lysstyrke,
                    Side::Nulstil,Side::Maaltid,Side::ScanFejl}) {
    app.side = side;
    app.kladde = side == Side::Ugemaal ? 70000 : 100;
    app.data.kalorier.spistIDag = 1000000;
    app.data.kalorier.spistTidligereDage = 1000000;
    tegnApp(pixels.data()+1,app);
    assert(pixels.front() == 0x1234 && pixels.back() == 0x1234);
  }
  std::cout << "PASS: single-press save, cancellation, bounded selection, increasing calorie progress, retry and rendering\n";
}
