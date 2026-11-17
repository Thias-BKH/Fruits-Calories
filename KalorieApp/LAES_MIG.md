# KalorieApp – ESP32-WROOM-32 med skærm og sidebetjening

Denne mappe er **ét samlet program til ét ESP32-WROOM-32-kort**.
Det samler skærmappen, swipe/tryk-betjeningen og Edge Impulse RGB-model 1.0.8.
Skærmen har ikke touch. Den eksisterende sensordriver læser fem kapacitive
felter i sidebetjeningen; kontrollér ledninger og følsomhed på jeres opstilling.
Et foto giver først et forslag; kun **Accepter** registrerer kalorier.

## 1. De bekræftede ledninger

Numrene nedenfor er **GPIO-numre**, som på jeres board står fx P23 og P32.

### Skærmen

| Skærm | ESP32-WROOM-32 |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SDI/MOSI | GPIO 23 |
| SCK | GPIO 18 |
| RS/DC | GPIO 25 |
| CS | GPIO 27 |
| RST | GPIO 22 |
| LED/baggrundslys | GPIO 21 |

### Sliderens felter fra øverst til nederst

| Felt | ESP32-WROOM-32 |
| --- | --- |
| 1 – øverst | GPIO 32 |
| 2 | GPIO 33 |
| 3 – midten | **GPIO 4** |
| 4 | GPIO 14 |
| 5 – nederst | GPIO 13 |

**Midterfeltet er flyttet fra knapfilens GPIO 27 til GPIO 4**, som på billedet.
GPIO 27 bruges allerede af skærmens CS. De to skal være separate.
Der er også kontrol af pin-konflikter i koden, hvis I senere ændrer benene.

## 2. Åbn og overfør

1. Åbn **KalorieApp.ino** i Arduino IDE. Behold alle filerne i mappen sammen.
2. Vælg **Tools → Board → esp32 → ESP32 Dev Module**.
3. Vælg USB-porten til WROOM-32-kortet. På et kort med 4 MB flash: vælg
   **Flash Size → 4MB (32Mb)** og
   **Partition Scheme → Huge APP (3MB No OTA/1MB SPIFFS)**.
4. Brug den installerede **esp32 by Espressif Systems 3.3.12** og de eksisterende
   biblioteker **Adafruit GFX**, **Adafruit ST7735 and ST7789** og **Adafruit BusIO**.
5. Installer RGB-modelbibliotek **1.0.8** via **Add .ZIP Library…** fra zip-filen i
   repositoryets `models/`-mappe. Klik derefter **Upload**.
6. Hold fingrene væk fra touchfelterne, mens skærmen viser **Kalibrerer...**.
   Kalibreringen tager cirka 1–2 sekunder og sker ved hver genstart.

Der skal kun være denne ene `setup()` og `loop()` i projektet. Den gamle knapfil
er allerede flettet ind i filerne nedenfor og skal ikke indsættes som en ekstra
`.ino`-fane i samme mappe.

## 3. Scan og bekræft

Forbind telefonen til **FruitClassifier**, kode **fruit1234**, og åbn
**http://192.168.4.1**. Tag/vælg et foto af ét æble eller én banan og tryk **Scan frugt**.
ESP32 kører modellen og viser frugt, kcal og den faste standardportion.

**Afvis** er valgt fra start. Swipe ned til **Accepter** og tryk kort for at registrere.
Swipe op markerer Afvis igen. Gentagne swipes i samme retning ændrer ikke valget.
Afvis eller langt tilbage-tryk tilføjer intet. Et nyt billede kan ikke erstatte
et forslag, mens du er ved at bekræfte det. Forslaget gemmes ikke ved genstart.

Portioner: æble **150 g → 69 kcal**, banan **120 g → 106 kcal**, spiselig del.
De er prototypevalg, ikke målte vægte. Energikilder og tilpasning står i
[README](../README.md#standardportioner) og `frugt_kalorier.h`.
Baggrund, ukendte klasser og sikkerhed under 60 % giver besked om at prøve igen.

### Øvrig betjening

- **Kort tryk:** Vælg/åbn den markerede handling.
- **Swipe op eller ned:** Flyt markeringen i menuen eller ændr tallet på en indstillingsside.
- **Langt, roligt tryk i cirka 0,8 sekund:** Tilbage/annuller.
- **På forsiden:** Et tryk eller swipe åbner indstillingerne.
- **I menuen:** Ét fysisk swipe flytter én række. Der er tre menupunkter og en lille
  Tilbage-mulighed nederst.
- **I et talfelt:** Et langt swipe kan give flere værditrin. Det gør det lettere
  at indstille ugemålet og lysstyrken.
- **Gem:** På ugemål og lysstyrke gemmer ét kort tryk kladden. Et langt tryk
  annullerer. Der er kun én Gem-knap på disse sider.
- **Nulstil:** Op vælger Fortryd, ned vælger Nulstil. Kort tryk udfører valget.
- **Prøv igen:** Kort tryk vender tilbage, så et nyt billede kan tages; swipe ignoreres.

Et swipe udløser ikke samtidig et klik ved slip. Et langt tilbage-tryk udløser
heller ikke et ekstra klik, når fingeren løftes.

## 4. Hvordan knapkode og skærmkode er samlet

| Fil | Indhold |
| --- | --- |
| **KalorieApp.ino** | Samlet opstart og hovedløkke; skærm, lysstyrke, gemning og input |
| **app_indstillinger.h** | Skærmben, tekststørrelser, farver, måltrin og placeringer |
| **touch_indstillinger.h** | Touchben, følsomhed, støjfilter, smoothing, tryktider og scrolltrin |
| **touch_model.h** | Jeres vægtede fingerposition og registrering af tryk/swipe |
| **touch_input.h** | Kalibrering og de faktiske `touchRead()`-aflæsninger |
| **touch_routing.h** | Kobling til appen: ét menutrin pr. swipe, flere trin i talfelter |
| **app_model.h** | Sider, navigation, Gem/Fortryd, kalorier og nulstilling |
| **app_visning.h** | Tegning af ring, menu, knapper og indstillingssider |
| **data_lager.h / data_format.h** | Gemning af mål, tællere og lysstyrke på kortet |
| **ui_faelles.h / kalorie_aktiver.h** | Skrifttegning og genererede skrifter |

Touchaflæsningen kører i en separat FreeRTOS-opgave på kerne 0. Skærmtegning og
appdata håndteres i hovedløkken. Dermed bliver touch ikke afhængig af, at hele
TFT-billedet først skal være overført. Events går gennem en kø; touchopgaven
tegner ikke og ændrer ikke appens data direkte.

## 5. Jeres følsomhedsindstillinger er bevaret

I **touch_indstillinger.h** står bl.a.:

```cpp
constexpr float TOUCH_THRESHOLD = 0.12f;
constexpr float NOISE_FLOOR = 0.04f;
constexpr float SCROLL_STEP = 0.40f;
constexpr float SMOOTHING = 0.40f;
```

- Lavere `TOUCH_THRESHOLD` gør sensoren mere følsom.
- `NOISE_FLOOR` filtrerer små ændringer fra.
- Lavere `SCROLL_STEP` giver flere værditrin pr. bevægelse.
- Større `SMOOTHING` giver hurtigere respons på den nye fingerposition.

`MENU_ET_TRIN_PR_SWIPE = true` forhindrer, at en fuld fingerbevægelse hopper
gennem alle menurækkerne. Sæt den til `false`, hvis I vil bruge hvert scrolltrin
til menuen, ligesom den oprindelige knapfil udskriver hvert trin.

`BRUG_LANGT_TRYK` slår tilbagegestussen til/fra, og `LANGT_TRYK_MS` ændrer dens tid.
Slip-forsinkelsen medregnes ikke i fingerens tryktid, så korte tryk bliver vurderet
ud fra den faktiske kontakt.

## 6. Hvis I vil se, hvad sensoren registrerer

Åbn Serial Monitor med **115200 baud**. Opstarten viser baseline for alle fem felter.
Sæt `DEBUG_POSITION = true` i touchindstillingerne, hvis I vil se fingerpositionen.

Den tidligere computerbetjening fungerer også:

| Tast | Testhandling |
| --- | --- |
| `p` | Tryk |
| `u` / `d` | Swipe op/ned |
| `b` | Tilbage |
| `s` / `+` | Simulér æbleforslag; kræver Accepter |
| `f` | Simulér bananforslag; kræver Accepter |
| `x` | Simulér baggrund; intet registreres |
| `n` | Manuel test af ny dag |
| `v` | Manuel test af ny uge |
| `?` | Vis hjælp |

Hvis computerbetjening virker, men slideren ikke reagerer, kan I kontrollere
felternes rækkefølge, baselineværdierne og kalibrere igen ved at trykke RESET
med fingrene væk fra felterne.

## 7. Data og de eksisterende sider

Appens menu, ugemål, lysstyrke, nulstilling, grøn ring og gemte valg er med.
Startdata er **14.000 kcal pr. uge, 0 kcal spist i dag, 0 fra tidligere dage og 75 % lysstyrke**.
Forsidens tal og ring viser kalorier tilbage i dag. Ugens tal viser spiste kcal.
På det nye kort bruges startværdierne første gang; derefter læses dets egne gemte data.
Data på et andet ESP32-kort overføres ikke automatisk mellem kortene.

Telefon/foto og modellen er koblet til. Scanningsresultatet går gennem
`FrugtKalorier::modtag(...)` til et midlertidigt forslag i appmodellen.
Kalorier gemmes først efter Accepter. Dato er endnu ikke koblet til; dag/uge
skifter fortsat manuelt med `nyDag()` og `nyUge()` i denne prototype.

## 8. Kontrol af den samlede version

Det nye design er kompileret med **ESP32 Dev Module, Arduino-ESP32 3.3.12** og
**Huge APP** den 17. november 2026:

- 1.472.899 bytes program, cirka 46 % af 3 MB-programpartitionen.
- 101.716 bytes globale data, cirka 31 % af RAM-grænsen i buildrapporten.

Tallene er fra kompileringen. Modelkørslens midlertidige RAM og hastighed skal
måles på kortet. De tidligere hardwaremålinger findes i `KONTROL.txt`.

Softwaretests dækker: ingen registrering før accept, standardvalget Afvis,
tilbage/afvis, gentagne tryk, nyt billede mens et forslag venter, baggrund,
ukendte klasser, usikre resultater, gemmeformat, RGB-buffergrænser og hot reload.
Det samlede program er kompileret med ESP32 Dev Module og Huge APP.

Fysisk test af denne version mangler. Kontrollér især gentagne scanninger og
modelkørslens RAM på det faktiske kort. En modelfejl vises på skærmen og i
Serial Monitor; den kan ikke registrere kalorier. Skærm/navigation venter under
billedmodtagelse og modelkørsel, mens touch-sampling fortsætter. Gamle events
fra ventetiden kasseres, før resultatet betjenes.
