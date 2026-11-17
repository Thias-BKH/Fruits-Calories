# Fruits & Calories

Kalorie-app til **ét ESP32-WROOM-32-kort** med 128 × 160-skærm, touchslider
og Edge Impulse-genkendelse af æble og banan fra et telefonfoto.

## Scan → kontrollér → accepter

1. Telefonen sender et beskåret **96 × 96 RGB-billede** til ESP32.
2. Edge Impulse-modellen genkender frugten på ESP32.
3. Skærmen viser **frugt, beregnede kcal og standardportion i gram**.
4. **Afvis er valgt fra start.** Swipe til **Accepter**, og bekræft med et kort tryk.
5. Først ved accept opdateres dagens kalorier, ugens total og ringen, og data gemmes.

Afvis eller langt tilbage-tryk lukker forslaget uden at registrere noget.
Baggrund, ukendte klasser og resultater under 60 % giver intet registrerbart forslag.
Et ventende forslag skal afsluttes, før et nyt billede kan scannes.

![Bekræftelsesside i simulatoren](docs/bekraeft-frugt.jpg)

## Prøv flowet på computeren

```sh
cd KalorieApp
python3 preview/server.py
```

Åbn **http://127.0.0.1:8000**. Under **Scan en rigtig frugt** kan du:

- vælge et foto og trykke **Scan foto**, eller
- trykke **Åbn kamera**, tillade kameraadgang og vælge **Tag billede og scan**.

Den medfølgende **Edge Impulse RGB-model 1.0.8 kører lokalt på computeren**.
Første opstart bygger modellen og viser status; senere starter den fra en lokal cache.
Fotos behandles i hukommelsen og gemmes ikke. Der bruges ingen skytjeneste eller ESP32.

Resultatet åbner samme bekræftelsesside som på hardwaren. Brug **↓** for at vælge
**Accepter**, og tryk **Enter** for at registrere. Enter direkte efter scanningen
vælger standardhandlingen **Afvis**. **Escape** afviser også.

Knapperne under **Simulerede testdata** er fortsat tilgængelige til layouttests.
Python 3 og en C++17-compiler er nødvendige; ingen ekstra Python-pakker.
Se [PREVIEW.md](KalorieApp/PREVIEW.md).

## Kør hele løsningen på ESP32

1. Installer **esp32 by Espressif Systems 3.3.12** og bibliotekerne **Adafruit GFX**,
   **Adafruit ST7735 and ST7789** og **Adafruit BusIO** i Arduino IDE.
2. Vælg **Sketch → Include Library → Add .ZIP Library…**, og installer
   **[RGB-model 1.0.8](models/ei-image-classification-(fruits)-arduino-1.0.8-impulse-3.zip)**.
   Den gamle modelzip i repositoryets rod er ikke den model, det samlede program bruger.
3. Åbn **[KalorieApp/KalorieApp.ino](KalorieApp/KalorieApp.ino)**. Behold alle `.h`-filer i mappen.
4. Vælg **ESP32 Dev Module**, **Flash Size: 4MB (32Mb)** og
   **Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)** til et kort med 4 MB flash.
   Standardens 1,3 MB programpartition er for lille til modellen og Wi-Fi sammen med appen.
5. Upload til kortet, og hold fingrene væk fra slideren under kalibrering.
6. Forbind telefonen til **FruitClassifier**, kode **fruit1234**. Åbn
   **http://192.168.4.1** i telefonens browser. Det er ESP32's lokale netværk uden internet.
7. Tag eller vælg et foto og tryk **Scan frugt**. Kontrollér og accepter/afvis
   på den **fysiske ESP32-skærm** med slideren.

Telefonen sender kun billedet til ESP32. Den har ingen knap, som kan registrere
kalorier uden bekræftelsen på kortet.
Se [LAES_MIG.md](KalorieApp/LAES_MIG.md) for ledninger og betjening.

## Standardportioner

| Modelklasse | Standardportion, spiselig del | kcal pr. 100 g | Beregnede kcal |
| --- | ---: | ---: | ---: |
| Æble | 150 g | 46 | 69 |
| Banan | 120 g | 88 | 106 |

Portionsstørrelserne er **faste prototypevalg** og bliver ikke målt af kameraet.
Bananvægten er uden skræl; æblevægten er den del, der spises. Kcal afrundes til hele tal.
Værdier og portionsstørrelser kan ændres i
[frugt_kalorier.h](KalorieApp/frugt_kalorier.h).
Energikilder: Mattilsynets Matvaretabellen for
[æble](https://www.matvaretabellen.no/eple-uspesifisert-ra/) og
[banan](https://www.matvaretabellen.no/en/banana-raw/), tilgået 17. november 2026.

## Kode og model

| Mappe / fil | Rolle |
| --- | --- |
| `KalorieApp/` | Samlet aktivt program: skærm, touch, Wi-Fi, model og bekræftelse |
| `KalorieApp/frugt_scanner.h` | Billedmodtagelse og kald til Edge Impulse |
| `KalorieApp/frugt_kalorier.h` | Klasser, portioner, energi og sikkerhedsgrænse |
| `KalorieApp/app_model.h` | Midlertidigt forslag og registrering efter accept |
| `KalorieApp/telefon_side.h` | Telefonens foto- og scanningsside |
| `models/` | RGB-model 1.0.8 fra branch `Classifier-with-RGB` |
| `Edge_impulse_WIP/` | Tidligere selvstændig prototype, bevaret som reference |
| `tests/` | Tests af bekræftelse, RGB-data og hot reload |

Billedet bruger midlertidigt skærmbufferens 40 KB; der tegnes ikke samtidigt.
Animationsbufferen allokeres kun under sideskift og frigives før scanning.
Touch-sampling fortsætter, men skærm/navigation venter under upload og inferens.
Events fra ventetiden kasseres, før bekræftelsessiden betjenes.

## Verifikation

Softwareflowet og RGB-bufferen er testet med address/undefined-sanitizer;
previewets hot reload og bekræftelsesside er kontrolleret. Lokal inferens med den
rigtige model og foto → Banan → Accepter er afprøvet i browseren med et testfoto. Det samlede program
er kompileret til ESP32 Dev Module med Arduino-ESP32 3.3.12 og Huge APP.

**Der er ikke foretaget upload eller fysisk test af den samlede version.**
Modelkørsel, faktisk fri RAM, touch under Wi-Fi og gentagne fotos skal afprøves
på jeres kort. Serial Monitor (115200 baud) viser modelresultater, fejlkoder,
fri heap og største ledige hukommelsesblok. En modelfejl giver ingen registrering.
Dato og automatisk dag-/ugeskift er fortsat ikke tilkoblet.

Kør softwaretests som beskrevet i [tests/README.md](tests/README.md).
`KalorieApp/KONTROL.txt` er historiske testresultater fra versionen før integrationen.
