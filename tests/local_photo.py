"""Rigtig Edge Impulse-inferens paa RGB-data, uden ESP32 eller tredjeparts Python-pakker."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "KalorieApp/preview"))
from native_classifier import LocalClassifier
from server import PreviewEngine

classifier = LocalClassifier(ROOT / "KalorieApp")
classifier._prepare()
assert classifier.binary, classifier.error
engine = PreviewEngine(ROOT / "KalorieApp", classifier)
try:
    black = bytes(96 * 96 * 3)
    yellow = bytes((255, 230, 40)) * (96 * 96)
    first = engine.scan(black)
    prediction = first["prediction"]
    assert {item["label"] for item in prediction["predictions"]} == {"Background", "Banan", "Æble"}
    assert all(0 <= item["confidence"] <= 1 for item in prediction["predictions"])
    assert abs(sum(item["confidence"] for item in prediction["predictions"]) - 1) < .02
    assert first["state"]["today"] == 0 and not first["state"]["pending"]
    second = engine.scan(yellow)
    assert second["prediction"]["predictions"] != prediction["predictions"]  # Pixeldata bruges af den rigtige model.
    assert second["state"]["today"] == 0
    engine.frame("back")
    engine.frame("banana")
    try:
        engine.scan(black)
        raise AssertionError("En ny scanning erstattede et ventende forslag")
    except ValueError:
        pass
    assert engine.frame()["state"]["fruit"] == "Banan"
    engine.frame("press")  # Afvis er stadig standard.
    assert engine.frame()["state"]["today"] == 0
    for data in (b"", black[:-1], black + b"x"):
        result = subprocess.run([str(classifier.binary)], input=data, capture_output=True, timeout=10)
        assert result.returncode == 2
    print("PASS: real model predictions depend on pixels; no automatic registration; pending scan protected; invalid RGB rejected")
finally:
    engine.close()
