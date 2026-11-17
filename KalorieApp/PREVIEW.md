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
**+** tilføjer 100 kcal. Musen bruges til previewets betjeningsknapper; der er
ikke tilføjet berøringsstyring direkte på den virtuelle skærm.

## Det er den samme designkode

Previewet kalder den eksisterende `tegnApp()` og viser dens 128 × 160 pixels
med de eksisterende skrifttyper og grafik. Ændringer i designets C++-filer bruges
også på ESP32, når du senere uploader som normalt fra Arduino IDE.

`KalorieApp.ino` og hardwarefilerne er uændrede. Previewets egen C++-fil ligger
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
