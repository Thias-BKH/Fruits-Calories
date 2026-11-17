# Se KalorieApp uden ESP32

Åbn hele **KalorieApp-mappen** i VS Code. Vælg **Terminal → New Terminal**, og kør:

```sh
python3 preview/server.py
```

Åbn **http://127.0.0.1:8000**. Lad terminalen køre, mens du arbejder.
Du kan også dobbeltklikke på `Start-preview.command` på din Mac.
Stop serveren med **Ctrl+C** i terminalen.

## Rediger designet

- `app_indstillinger.h`: farver, tekststørrelser, placeringer og ringens størrelse.
- `app_visning.h`: sidernes indhold og layout.
- `app_model.h`: navigation og appens adfærd.

Gem filen i VS Code (**Cmd+S**). Previewet opdager ændringen, bygger C++-koden
og viser resultatet automatisk. Den aktuelle side, kladde og testtal bevares
ved en genbygning. Korte statusbeskeder og trykfeedback starter ikke forfra.
Hvis der er en fejl i koden, viser siden fejlbeskeden og det senest fungerende
design. Ret fejlen og gem igen.

Browserens knapper og **↑ / ↓ / Enter / Escape** efterligner sliderens hændelser.
**+** simulerer et æbleforslag; tallet registreres først efter **Accepter**. Musen bruges til previewets betjeningsknapper; der er
ikke tilføjet berøringsstyring direkte på den virtuelle skærm.

## Det er den samme designkode

Previewet kalder den eksisterende `tegnApp()` og viser dens 128 × 160 pixels
med de eksisterende skrifttyper og grafik. Ændringer i designets C++-filer bruges
også på ESP32, når du senere uploader som normalt fra Arduino IDE.

Previewets egen C++-fil ligger
i `preview/`, så den ikke bliver en del af Arduino-programmet.

Alle tal og indstillinger i previewet er **testdata i hukommelsen**. De overføres
ikke til ESP32. Browsergenindlæsning bevarer testdata, så længe serveren kører.
Knappen **Genstart test** eller genstart af serveren indlæser standardværdierne
fra `app_indstillinger.h`. Brug dette efter ændring af `START_KALORIER`.

Den fysiske touchfølsomhed, skærmens lysstyrke, farvegengivelse og opdateringshastighed
skal stadig testes på udstyret. Skærmvisningen i browseren dæmpes ikke af lysstyrketallet.

## Live Server på port 5500

Live Server kan vise HTML, men kan ikke bygge og køre C++-delen. Brug derfor
Python-kommandoen ovenfor og port **8000** til dette preview. En `index.html`
i projektmappen viser også denne vejledning, hvis du besøger port 5500.

## Hvis det ikke starter

- På denne Mac kræves Python 3 og en C++-compiler. Hvis compileren mangler,
  kan Apples værktøjer installeres med `xcode-select --install`.
- Er port 8000 optaget, brug `python3 preview/server.py --port 8001` og åbn
  http://127.0.0.1:8001.
- Åbn `preview/index.html` via previewserveren, ikke direkte eller gennem Live Server.


## Scan dit eget foto eller brug kameraet

1. Åbn **http://127.0.0.1:8000** på computeren, hvor serveren kører.
2. Vent på **Modellen er klar**. Første gang bygges den medfølgende model fra
   `models/`-zippen. Det kræver C++-compileren og kan tage et par minutter.
3. Vælg et foto af ét æble eller én banan, og tryk **Scan foto**. Alternativt:
   **Åbn kamera → tillad kameraadgang → Tag billede og scan**.
4. Hold frugten midt i billedet. Det viste kvadratiske udsnit er dét, modellen ser.
5. Kontrollér frugt og kcal på den virtuelle skærm. Brug **↓ → Enter** for Accepter,
   **Enter** direkte for Afvis eller **Escape** for at afvise.

Det er **jeres rigtige RGB-model 1.0.8**, som kører på computeren. Billedet
beskæres i midten og skaleres til 96 × 96 RGB-pixels, ligesom på telefonens
ESP32-side. Resultatet bruger samme kalorieberegning og bekræftelseskode.
**Vis modellens vurdering** viser sandsynlighederne for alle tre klasser.

Kameraet åbnes kun, når du trykker Åbn kamera, og kan slukkes med **Luk kamera**.
Billeder behandles i hukommelsen og gemmes ikke af serveren. Ingen fotos sendes
til Edge Impulse eller andre eksterne tjenester.

Modellens kildefiler og program caches i `.preview-cache/`, som Git ignorerer.
Der installeres ingen Python-pakker eller Arduino-biblioteker til denne test.

### Simulerede resultater til layouttest

Knapperne **Simulér scanning: Æble** og **Banan** åbner samme bekræftelsesside
med faste testresultater. Foto/kamera-sektionen kører den rigtige model.

- **Afvis** er valgt fra start. Enter afviser og ændrer ikke dagens kalorier.
- Swipe én gang (↑ eller ↓) til **Accepter**, og tryk Enter for at registrere.
- Escape afviser også. Resultatet kan kun accepteres én gang.
- Standardportionens gram og kcal står på skærmen. Kameraet måler ingen vægt.
- **Baggrund** og **Usikkert resultat** viser fejlflowet uden at tilføje kalorier.
- Et ventende forslag bliver ikke erstattet af endnu et simuleret resultat.
- Hot reload bevarer frugt, portion, kcal og valgt knap uden at registrere forslaget.

Lokal foto-/kameratest kræver ingen tilsluttet ESP32 eller skærm. Den tester
modellens genkendelse og appens bekræftelsesflow. Fysisk touch, RAM og hastighed
på ESP32 skal stadig afprøves på kortet. Ved senere hardwaretest åbner telefonen
ESP32's side på **http://192.168.4.1**.
