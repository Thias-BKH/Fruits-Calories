# Fruits & Calories

Medialogy-projekt med frugtbilleder og en kalorie-app til ESP32.

## Projektets dele

| Mappe / fil | Indhold |
| --- | --- |
| [KalorieApp](KalorieApp/) | Skærmdesign, navigation, touchslider og lokal forhåndsvisning |
| [Edge_impulse_WIP](Edge_impulse_WIP/) | Eksisterende ESP32-prototype til modtagelse af frugtbilleder via Wi-Fi |
| `ei-image-classification-(fruits)-arduino-1.0.5-impulse-#2.zip` | Eksporteret Edge Impulse-bibliotek |

KalorieApp er et selvstændigt Arduino-program til ESP32-WROOM-32 med en
128 × 160-skærm og fem touchfelter. Det er endnu ikke koblet sammen med
billedmodtagelsen eller Edge Impulse-modellen.

## Se og rediger designet på computeren

Fra repositoryets rodmappe:

```sh
cd KalorieApp
python3 preview/server.py
```

Åbn **http://127.0.0.1:8000** i browseren. Stop serveren med **Ctrl+C**.
På Mac kan du også dobbeltklikke på `KalorieApp/Start-preview.command`.
Previewet kræver Python 3 og en C++17-compiler (`clang++`, `g++` eller `c++`).

- `app_indstillinger.h`: farver, tekststørrelser, placeringer og startværdier.
- `app_visning.h`: skærmenes indhold og layout.
- `app_model.h`: navigation og appens adfærd.

Gem ændringerne for at opdatere previewet automatisk. Brug knapperne i browseren
eller **↑ / ↓ / Enter / Escape**. **+** tilføjer 100 kcal som testdata.
Previewet bruger samme C++-designkode som ESP32-programmet og kører lokalt;
det kan ikke afvikles alene som en statisk GitHub Pages-side.

Se [previewvejledningen](KalorieApp/PREVIEW.md) for mere om betjening og fejlfinding.

## Kør på ESP32

Åbn **[KalorieApp/KalorieApp.ino](KalorieApp/KalorieApp.ino)** i Arduino IDE,
og behold alle appens filer i samme mappe. Vælg **ESP32 Dev Module** til den
klassiske **ESP32-WROOM-32**. Programmet bruger Arduino-ESP32 3.x samt
**Adafruit GFX**, **Adafruit ST7735 and ST7789** og **Adafruit BusIO**.

Se [hardware- og touchvejledningen](KalorieApp/LAES_MIG.md) for ledninger,
kalibrering og upload. `KalorieApp/KONTROL.txt` indeholder tidligere kontrolresultater;
en ny upload og fysisk hardwaretest er stadig nødvendig ved ændringer.
