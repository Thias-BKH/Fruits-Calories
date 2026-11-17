// Manuel visuel kontrol: skriv de faktiske RGB565-skaermbilleder til en valgt mappe.
#include <filesystem>
#include <fstream>
#include <string>
#include "app_visning.h"
#include "frugt_kalorier.h"

int main(int argc, char** argv) {
  if (argc != 2) return 1;
  std::filesystem::create_directories(argv[1]);
  auto save = [&](const char* name, const KalorieAppModel& app) {
    uint16_t pixels[128*160];
    tegnApp(pixels,app);
    std::ofstream output(std::filesystem::path(argv[1])/(std::string(name)+".rgb565"),std::ios::binary);
    for (auto pixel:pixels) {output.put(pixel&255);output.put(pixel>>8);}
  };
  KalorieAppModel base;
  base.data.kalorier={14000,1000,0};base.data.lysstyrke=75;
  save("01-forside",base);
  auto app=base;app.input(Input::Tryk,0);
  save("02-menu-maal",app);
  app.input(Input::SwipeNed,1);save("03-menu-lys",app);
  app.input(Input::SwipeNed,2);save("04-menu-nulstil",app);
  app=base;app.side=Side::Ugemaal;app.kladde=14000;save("05-ugemaal",app);
  app=base;app.side=Side::Lysstyrke;app.kladde=75;save("06-lysstyrke",app);
  app=base;app.side=Side::Nulstil;app.fokus=Fokus::Fortryd;save("07-fortryd",app);
  app.input(Input::SwipeNed,3);save("08-nulstil",app);
  app=base;FrugtKalorier::modtag(app,"Æble",.95f,4);save("09-aeble-afvis",app);
  app=base;FrugtKalorier::modtag(app,"Banan",.95f,5);app.input(Input::SwipeNed,6);save("10-banan-accepter",app);
  app=base;FrugtKalorier::modtag(app,"Background",.95f,7);save("11-ingen-frugt",app);
  app=base;FrugtKalorier::modtag(app,"Æble",.4f,8);save("12-usikker",app);
}
