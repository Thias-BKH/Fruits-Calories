#!/usr/bin/env python3
"""Lokal visning af KalorieApps faktiske C++-rendering. Ingen ekstra pakker."""
import argparse
import base64
import json
import os
from pathlib import Path
import select
import shutil
import subprocess
import tempfile
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

HERE = Path(__file__).resolve().parent
ACTIONS = {"up", "down", "press", "back", "add", "apple", "banana", "background", "uncertain", "day", "week", "restart"}


class PreviewEngine:
    def __init__(self, project):
        self.project = Path(project).resolve()
        self.temp = tempfile.TemporaryDirectory(prefix="kalorie-preview-")
        self.binary = Path(self.temp.name) / "runtime"
        self.process = None
        self.signature = None
        self.state = None
        self.error = None
        self.revision = 0
        self.started = time.monotonic()
        self.lock = threading.Lock()

    def stop_process(self):
        if self.process:
            self.process.terminate()
            try:
                self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
            self.process.stdin.close()
            self.process.stdout.close()
            self.process = None

    def close(self):
        self.stop_process()
        self.temp.cleanup()

    def check_build(self):
        paths = sorted(self.project.glob("*.h")) + [HERE / "runtime.cpp"]
        signature = tuple((str(p), p.stat().st_mtime_ns, p.stat().st_size) for p in paths)
        if signature == self.signature:
            return
        self.signature = signature
        compiler = shutil.which("clang++") or shutil.which("g++") or shutil.which("c++")
        if not compiler:
            self.error = "C++-compiler mangler. På Mac: kør xcode-select --install i Terminal."
            return
        candidate = Path(self.temp.name) / "runtime-next"
        try:
            build = subprocess.run(
                [compiler, "-std=c++17", "-O2", "-I", str(self.project),
                 str(HERE / "runtime.cpp"), "-o", str(candidate)],
                capture_output=True, text=True, timeout=45,
            )
        except (OSError, subprocess.TimeoutExpired) as exc:
            self.error = "Kunne ikke bygge preview: " + str(exc)
            return
        if build.returncode:
            self.error = build.stderr.strip() or "C++-koden kunne ikke bygges."
            print("\nFejl i koden – ret filen og gem igen:\n" + self.error, flush=True)
            return
        self.stop_process()
        os.replace(candidate, self.binary)
        self.process = subprocess.Popen(
            [str(self.binary)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, bufsize=0,
        )
        self.error = None
        self.revision += 1
        if self.state:
            s = self.state
            values = [s[k] for k in ("weekly", "today", "previous", "brightness",
                                     "side", "fokus", "row", "draft")]
            values += [int(s.get("pending", False)), s.get("pendingKcal", 0),
                       s.get("pendingGrams", 0), s.get("confidence", 0)]
            extra = " ".join(map(str, values)) + " " + json.dumps(s.get("fruit", ""), ensure_ascii=False)
            extra += " " + json.dumps(s.get("scanError", ""), ensure_ascii=False)
            self.exchange("restore", extra)
        print("Preview klar – designet er opdateret.", flush=True)

    def read_exact(self, count, deadline):
        result = bytearray()
        while len(result) < count:
            remaining = deadline - time.monotonic()
            if remaining <= 0 or not select.select([self.process.stdout], [], [], remaining)[0]:
                raise RuntimeError("Skærmvisningen svarede ikke. Genstart previewet.")
            chunk = os.read(self.process.stdout.fileno(), count - len(result))
            if not chunk:
                raise RuntimeError("Skærmvisningen stoppede. Genstart previewet.")
            result.extend(chunk)
        return bytes(result)

    def exchange(self, action, extra=""):
        now = int((time.monotonic() - self.started) * 1000) & 0xffffffff
        self.process.stdin.write(("%s %d %s\n" % (action, now, extra)).encode())
        self.process.stdin.flush()
        deadline = time.monotonic() + 5
        line = bytearray()
        while len(line) < 4096:
            char = self.read_exact(1, deadline)
            if char == b"\n":
                break
            line.extend(char)
        else:
            raise RuntimeError("Ugyldigt svar fra skærmvisningen.")
        state = json.loads(line)
        pixels = self.read_exact(128 * 160 * 2, deadline)
        self.state = state
        return {"pixels": base64.b64encode(pixels).decode(), "state": state,
                "error": self.error, "revision": self.revision}

    def frame(self, action="frame"):
        with self.lock:
            try:
                self.check_build()
                if self.process is None:
                    return {"pixels": None, "state": None, "error": self.error, "revision": self.revision}
                return self.exchange(action)
            except (OSError, ValueError, RuntimeError) as exc:
                self.stop_process()
                self.signature = None
                self.error = str(exc)
                return {"pixels": None, "state": self.state, "error": self.error,
                        "revision": self.revision}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8000)
    parser.add_argument("--project", type=Path, default=HERE.parent)
    args = parser.parse_args()
    if not (args.project / "app_visning.h").is_file():
        parser.error("Start fra KalorieApp-mappen, hvor app_visning.h ligger.")
    engine = PreviewEngine(args.project)

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_):
            pass

        def send_data(self, status, data, content_type):
            self.send_response(status)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            try:
                self.wfile.write(data)
            except (BrokenPipeError, ConnectionResetError):
                pass

        def send_json(self, status, data):
            self.send_data(status, json.dumps(data).encode(), "application/json; charset=utf-8")

        def do_GET(self):
            path = self.path.split("?", 1)[0]
            if path in ("/", "/index.html"):
                self.send_data(200, (HERE / "index.html").read_bytes(), "text/html; charset=utf-8")
            elif path == "/api/frame":
                self.send_json(200, engine.frame())
            elif path == "/health":
                self.send_json(200, {"app": "KalorieApp-preview", "project": str(engine.project)})
            else:
                self.send_json(404, {"error": "Siden findes ikke."})

        def do_POST(self):
            if self.path != "/api/input":
                return self.send_json(404, {"error": "Siden findes ikke."})
            origin = self.headers.get("Origin")
            allowed = {"http://127.0.0.1:%d" % args.port, "http://localhost:%d" % args.port}
            if origin and origin not in allowed:
                return self.send_json(403, {"error": "Åbn previewet på den lokale previewadresse."})
            try:
                length = int(self.headers.get("Content-Length", "0"))
                if not 0 < length <= 512:
                    raise ValueError()
                data = json.loads(self.rfile.read(length))
                action = data.get("action")
                if action not in ACTIONS:
                    raise ValueError()
            except (ValueError, AttributeError, TypeError):
                return self.send_json(400, {"error": "Ukendt handling."})
            self.send_json(200, engine.frame(action))

    try:
        server = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
    except OSError as exc:
        parser.exit(1, "Kan ikke bruge port %d: %s\nPrøv --port 8001.\n" % (args.port, exc))
    server.daemon_threads = True
    print("\nKalorieApp – lokal skærmvisning", flush=True)
    print("Åbn http://127.0.0.1:%d i browseren." % args.port, flush=True)
    print("Gem ændringer i .h-filerne for at opdatere designet. Stop med Ctrl+C.\n", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nPreview stoppet.")
    finally:
        server.server_close()
        engine.close()


if __name__ == "__main__":
    main()
