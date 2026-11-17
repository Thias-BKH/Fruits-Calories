"""Byg og koer den medfoelgende Edge Impulse-model lokalt uden Python-pakker."""
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import threading
import zipfile

HERE = Path(__file__).resolve().parent
IMAGE_BYTES = 96 * 96 * 3
MODEL_NAME = "ei-image-classification-(fruits)-arduino-1.0.8-impulse-3.zip"
FLAGS = ["-std=c++17", "-O2", "-DNDEBUG", "-DEIDSP_USE_CMSIS_DSP=0", "-DEIDSP_USE_NEON=0",
         "-DEI_CLASSIFIER_TFLITE_ENABLE_CMSIS_NN=0", "-DTF_LITE_DISABLE_X86_NEON=1",
         "-Wno-deprecated-declarations", "-Wno-c++11-narrowing", "-Wno-c99-designator"]


class LocalClassifier:
    def __init__(self, project):
        self.project = Path(project).resolve()
        self.archive = self.project.parent / "models" / MODEL_NAME
        self.binary = None
        self.error = None
        self.message = "Forbereder billedgenkendelse…"
        self.thread = None

    def start(self):
        self.thread = threading.Thread(target=self._prepare, daemon=True)
        self.thread.start()

    def status(self):
        return {"ready": self.binary is not None, "error": self.error, "message": self.message}

    def _prepare(self):
        try:
            self.binary = self._build()
            self.message = "Modellen er klar · billeder behandles lokalt"
            print(self.message, flush=True)
        except (OSError, ValueError, RuntimeError, subprocess.SubprocessError, zipfile.BadZipFile) as exc:
            self.error = str(exc)
            self.message = "Billedgenkendelsen kunne ikke starte"
            print(self.message + ": " + self.error, flush=True)

    def _build(self):
        compiler = shutil.which("clang++") or shutil.which("g++") or shutil.which("c++")
        if not compiler:
            raise RuntimeError("C++-compiler mangler. På Mac: xcode-select --install.")
        if not self.archive.is_file():
            raise RuntimeError("RGB-model 1.0.8 mangler i repositoryets models-mappe.")
        digest = hashlib.sha256(self.archive.read_bytes())
        for path in (HERE / "model_worker.cpp", HERE / "model_porting.cpp", Path(__file__),
                     self.project / "rgb_billede.h"):
            digest.update(path.read_bytes())
        digest.update((compiler + sys.platform + platform.machine()).encode())
        cache = self.project.parent / ".preview-cache" / digest.hexdigest()[:20]
        binary = cache / "classifier"
        if binary.is_file() and os.access(binary, os.X_OK):
            return binary
        self.message = "Bygger billedmodellen første gang…"
        cache.mkdir(parents=True, exist_ok=True)
        source = cache / "src"
        with zipfile.ZipFile(self.archive) as archive:
            prefix = "Image_classification_Fruits__inferencing/src/"
            for entry in archive.infolist():
                if not entry.filename.startswith(prefix) or entry.is_dir():
                    continue
                relative = Path(entry.filename[len(prefix):])
                if relative.is_absolute() or ".." in relative.parts:
                    raise ValueError("Ugyldig sti i modelarkivet.")
                target = source / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(archive.read(entry))
        sources = [HERE / "model_worker.cpp", HERE / "model_porting.cpp"]
        sources += sorted((source / "tflite-model").glob("*.cpp"))
        # De portable TFLM-kilder fra modelarkivet; ingen Arduino/ESP32-adaptere.
        for folder in ("tensorflow/lite/kernels", "tensorflow/lite/kernels/internal",
                       "tensorflow/lite/micro/kernels", "tensorflow/lite/micro",
                       "tensorflow/lite/micro/memory_planner", "tensorflow/lite/core/api",
                       "tensorflow/lite/core/c", "tensorflow/lite/c", "dsp/kissfft", "dsp/dct", "dsp/image"):
            directory = source / "edge-impulse-sdk" / folder
            for suffix in ("*.cpp", "*.cc", "*.c"):
                sources += sorted(directory.glob(suffix))
        sources += [source / "edge-impulse-sdk/dsp/memory.cpp"]
        flags = FLAGS + ["-I" + str(source), "-I" + str(self.project),
                         "-I" + str(source / "edge-impulse-sdk/third_party/flatbuffers/include")]

        def compile_source(item):
            index, path = item
            obj = cache / (str(index) + ".o")
            if not obj.exists():
                result = subprocess.run([compiler, *flags, "-c", str(path), "-o", str(obj)],
                                        capture_output=True, text=True, timeout=90)
                if result.returncode:
                    obj.unlink(missing_ok=True)
                    raise RuntimeError(result.stderr[-8000:])
            return obj

        with concurrent.futures.ThreadPoolExecutor(max_workers=min(4, os.cpu_count() or 1)) as pool:
            objects = []
            for index, obj in enumerate(pool.map(compile_source, enumerate(sources)), 1):
                objects.append(obj)
                self.message = "Bygger billedmodellen (%d/%d)…" % (index, len(sources))
        candidate = cache / "classifier-next"
        result = subprocess.run([compiler, *map(str, objects), "-o", str(candidate)],
                                capture_output=True, text=True, timeout=90)
        if result.returncode:
            raise RuntimeError(result.stderr[-8000:])
        os.replace(candidate, binary)
        return binary

    def classify(self, rgb):
        if len(rgb) != IMAGE_BYTES:
            raise ValueError("Forventede et billede med 96 × 96 RGB-pixels.")
        if self.binary is None:
            raise RuntimeError(self.error or self.message)
        result = subprocess.run([str(self.binary)], input=rgb, capture_output=True, timeout=30)
        if result.returncode:
            raise RuntimeError(result.stderr.decode(errors="replace")[-2000:] or "Modellen kunne ikke køre.")
        return json.loads(result.stdout)
