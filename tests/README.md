# Softwaretests

Kør fra repositoryets rod med en C++17-compiler og Python 3:

```sh
clang++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I KalorieApp tests/meal_confirmation.cpp -o /tmp/kalorie-meal-test
/tmp/kalorie-meal-test
python3 tests/preview_reload.py
python3 tests/local_photo.py
```

Testene kontrollerer, at scanning og afvisning ikke registrerer noget; at kun
Accepter tilføjer kalorier; og at gentagne tryk, nye billeder og hot reload ikke
giver dobbeltregistrering. De dækker også vedvarende data, usikre/ukendte klasser,
et trin pr. fysisk swipe, RGB-chunks, buffergrænser og rendering. `local_photo.py` bygger/kører den rigtige model
på forskellige RGB-input, kontrollerer at pixeldata ændrer modellens scores,
og at et ventende forslag ikke overskrives. Første modelbygning kan tage et par minutter.

## Samlet ESP32-build

Installer modelzip 1.0.8 fra `models/`, Arduino-ESP32 3.3.12 og Adafruit-bibliotekerne,
og kør med Arduino CLI:

```sh
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app KalorieApp
```

Verificeret 17. november 2026. Standardpartitionen er for lille til programmet;
Huge APP er brugt til 4 MB flash. Ingen fysisk upload/test er udført.

## På udstyret

1. Åbn telefonens side, scan en frugt, og kontrollér at dagens tal er uændret.
2. Afvis på slideren, scan igen, og accepter. Kontrollér, at kun den accepterede portion tilføjes.
3. Tryk igen og genstart kortet. Kontrollér, at den accepterede total bevares én gang.
4. Scan baggrund og et uklart billede. Kontrollér ingen registrering.
5. Gentag med begge frugter og flere fotos. Kontrollér Serial Monitor for
   RAM og modelfejl; bekræft, at touch virker efter hver scanning.
