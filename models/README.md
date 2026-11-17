# RGB-model til det samlede program

Installer `ei-image-classification-(fruits)-arduino-1.0.8-impulse-3.zip`
i Arduino IDE via **Sketch → Include Library → Add .ZIP Library…**.
Bibliotekets navn er `Image_classification_Fruits__inferencing`, version **1.0.8**.

Zip-filen er kopieret uændret fra `Classifier-with-RGB` i dette repository
(commit `c096e77b1922c707a296e1ead8e78c4d0fc78848`).
Modellen er deployment-version **8**, projekt **1143951**, med input **96 × 96 RGB**
og klasserne **Background**, **Banan** og **Æble**.

Den gamle modelzip i repositoryets rod er bevaret som reference og bruges ikke
af det samlede program. Koden kontrollerer modelversion og inputdimensioner ved kompilering.
De genererede biblioteksfiler og deres licenstekster er uændrede.
