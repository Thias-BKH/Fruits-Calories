"""Kontroller at hot reload bevarer et ventende forslag uden at registrere det."""
import importlib.util
from pathlib import Path
import shutil
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="kalorie-reload-test-") as temp:
    project = Path(temp) / "KalorieApp"
    shutil.copytree(ROOT / "KalorieApp", project, ignore=shutil.ignore_patterns("__pycache__"))
    spec = importlib.util.spec_from_file_location("preview", project / "preview/server.py")
    preview = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(preview)
    engine = preview.PreviewEngine(project)
    try:
        first = engine.frame("apple")
        assert not first["error"] and first["state"]["pending"]
        engine.frame("down")
        (project / "app_indstillinger.h").touch()
        rebuilt = engine.frame()
        assert not rebuilt["error"] and rebuilt["revision"] == first["revision"] + 1
        state = rebuilt["state"]
        assert state["fruit"] == "Æble" and state["pendingKcal"] == 69
        assert state["today"] == 0 and state["pending"] and state["fokus"] == 1
        accepted = engine.frame("press")["state"]
        assert accepted["today"] == 69 and not accepted["pending"]
        (project / "app_indstillinger.h").touch()
        assert engine.frame()["state"]["today"] == 69
        engine.frame("press")
        assert engine.frame()["state"]["today"] == 69
        print("PASS: pending/result/focus survive hot reload; accepted total survives once")
    finally:
        engine.close()
