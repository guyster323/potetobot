"""Loopback host GUI. Bind is 127.0.0.1 only. No COM port is opened.

From the repository root:

    C:\\Python313\\python.exe host\\gui_server.py
"""

import hashlib
import hmac
import json
import secrets
import sys
import threading
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, unquote, urlparse

import gui_characters
import gui_ports
import gui_providers
import gui_serial
import gui_session
import gui_settings
import gui_virtual

ROOT = Path(__file__).resolve().parents[1]
PAGE = Path(__file__).with_name("gui_page.html")
HOST = "127.0.0.1"
PORT = 8765
FRAME_PREFIX = "/frames/"

_ORCA_NAMES = {"names": None}


def known_orca_names(force=False):
    if force or _ORCA_NAMES["names"] is None:
        listed = gui_providers.list_orca_bots()
        if not listed["ok"]:
            return None, listed.get("error") or "orca list failed"
        _ORCA_NAMES["names"] = {bot["name"] for bot in listed["bots"]}
    return _ORCA_NAMES["names"], ""


class Handler(BaseHTTPRequestHandler):
    server_version = "PotetobotGUI"

    def log_message(self, fmt, *args):
        return

    def _send(self, status, body, content_type):
        data = body if isinstance(body, bytes) else body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    def _json(self, status, payload):
        self._send(status, json.dumps(payload, ensure_ascii=False), "application/json; charset=utf-8")

    def _read_json(self):
        length = int(self.headers.get("Content-Length", "0") or "0")
        if length < 0 or length > 8192:
            raise ValueError("body length")
        raw = self.rfile.read(length) if length else b"{}"
        data = json.loads(raw.decode("utf-8"))
        if not isinstance(data, dict):
            raise ValueError("json object")
        return data

    def _frame_file(self, path):
        relative = unquote(path[len(FRAME_PREFIX):])
        if relative.startswith("/") or "\\" in relative or ".." in relative.split("/"):
            return None
        parts = relative.split("/")
        if len(parts) != 3:
            return None
        character, state, name = parts
        allowed = {item["id"] for item in gui_characters.catalog()["characters"]}
        if character not in allowed or not gui_characters.valid_id(character):
            return None
        if not state.isalpha() or not name.startswith("f") or not name.endswith(".png"):
            return None
        file_path = (gui_characters.ASSETS / character / state / name).resolve()
        try:
            file_path.relative_to(gui_characters.ASSETS.resolve())
        except ValueError:
            return None
        if not file_path.is_file():
            return None
        return file_path

    def _expected_origin(self):
        port = self.server.server_address[1]
        return f"http://{HOST}:{port}"

    def _post_gate(self):
        """Explicit local writes only. A page load does not reach this."""
        host = self.headers.get("Host", "")
        origin = self.headers.get("Origin", "")
        sent = self.headers.get("X-Potetobot-Token", "")
        ctype = self.headers.get("Content-Type", "")
        expected_host = f"{HOST}:{self.server.server_address[1]}"
        if host != expected_host:
            return "host"
        if origin != self._expected_origin():
            return "origin"
        if not ctype.lower().startswith("application/json"):
            return "content type"
        expected = getattr(self.server, "token", "")
        if len(sent) != len(expected) or not hmac.compare_digest(sent, expected):
            return "token"
        return ""

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path in ("/", "/index.html"):
            page = PAGE.read_text(encoding="utf-8").replace("__POTETOBOT_TOKEN__", self.server.token)
            self._send(200, page, "text/html; charset=utf-8")
            return
        if parsed.path.startswith(FRAME_PREFIX):
            file_path = self._frame_file(parsed.path)
            if file_path is None:
                self._json(404, {"error": "frame missing"})
                return
            self._send(200, file_path.read_bytes(), "image/png")
            return
        if parsed.path == "/api/settings":
            try:
                self._json(200, gui_settings.load())
            except gui_settings.SettingsError as exc:
                self._json(500, {"error": str(exc)})
            return
        if parsed.path == "/api/session":
            self._json(200, self.server.supervisor.status())
            return
        if parsed.path == "/api/characters":
            self._json(200, gui_characters.catalog())
            return
        if parsed.path == "/api/ports":
            self._json(200, gui_ports.adapter_report())
            return
        if parsed.path == "/api/bots":
            query = parse_qs(parsed.query)
            provider = (query.get("provider") or ["orca_bots"])[0]
            needle = (query.get("q") or [""])[0]
            if provider == "grok_bot":
                self._json(200, gui_providers.grok_status())
                return
            if provider != "orca_bots":
                self._json(400, {"error": "provider"})
                return
            listed = gui_providers.list_orca_bots(needle)
            if listed["ok"] and not needle:
                _ORCA_NAMES["names"] = {bot["name"] for bot in listed["bots"]}
            self._json(200, listed)
            return
        self._json(404, {"error": "not found"})

    def _session_start(self, body):
        try:
            pinned = {
                "port": gui_session.clean_port(body.get("port")),
                "provider": body.get("provider"),
                "botId": gui_settings.normalize({"botId": body.get("botId", ""), "provider": "orca_bots"})["botId"],
                "grokBotId": gui_settings.normalize({"grokBotId": body.get("grokBotId", ""), "provider": "grok_bot"})["grokBotId"],
                "sttLanguage": body.get("sttLanguage"),
            }
        except ValueError as exc:
            self._json(400, {"error": str(exc)})
            return
        if pinned["provider"] not in gui_settings.PROVIDERS:
            self._json(400, {"error": "provider"})
            return
        if pinned["sttLanguage"] not in gui_settings.STT_LANGUAGES:
            self._json(400, {"error": "stt language"})
            return
        started = self.server.supervisor.start(pinned)
        if started.get("accepted") is False and started.get("error") == "session already running":
            self._json(409, {"error": "session already running", "state": started.get("state", "")})
            return
        self._json(200, started)

    def do_POST(self):
        parsed = urlparse(self.path)
        try:
            body = self._read_json()
            bad_json = False
        except (ValueError, json.JSONDecodeError):
            body = {}
            bad_json = True
        gate = self._post_gate()
        if gate:
            self._json(403, {"error": gate})
            return
        if bad_json:
            self._json(400, {"error": "bad json"})
            return
        if parsed.path == "/api/session/start":
            self._session_start(body)
            return
        if parsed.path == "/api/session/stop":
            self._json(200, self.server.supervisor.stop())
            return
        if parsed.path == "/api/settings":
            try:
                data = gui_settings.normalize(body)
            except ValueError as exc:
                self._json(400, {"error": str(exc)})
                return
            if data["provider"] == "orca_bots" and data["botId"]:
                names, err = known_orca_names()
                if names is None:
                    self._json(502, {"error": err})
                    return
                if data["botId"] not in names:
                    self._json(400, {"error": "bot is not in the Orca list"})
                    return
            try:
                saved = gui_settings.save(data)
            except gui_settings.SettingsError as exc:
                status = 500 if str(exc).startswith("settings ") else 400
                self._json(status, {"error": str(exc)})
                return
            self._json(200, saved)
            return
        if parsed.path == "/api/stt-probe":
            language = body.get("language", "ko")
            if language not in ("ko", "en"):
                self._json(400, {"error": "stt language"})
                return
            self._json(200, gui_virtual.probe_transcribe(language))
            return
        if parsed.path == "/api/virtual":
            mode = body.get("mode")
            if mode not in ("speak", "error"):
                self._json(400, {"error": "mode"})
                return
            settings = gui_settings.load()
            try:
                result = gui_virtual.run_virtual(mode, settings["sttLanguage"])
            except Exception as exc:
                self._json(500, {"error": str(exc), "openedPort": False})
                return
            self._json(200, result)
            return
        if parsed.path == "/api/grok-link":
            try:
                url = gui_providers.grok_prompt_link(body.get("botId", ""), body.get("text", ""))
            except ValueError as exc:
                self._json(400, {"error": str(exc)})
                return
            self._json(200, {
                "url": url,
                "sends": False,
                "note": "Opens the app. Does not send. HTTP 200 from a webhook is not an answer.",
            })
            return
        self._json(404, {"error": "not found"})


def make_server(port):
    server = ThreadingHTTPServer((HOST, port), Handler)
    bound = server.server_address
    if bound[0] != HOST:
        server.server_close()
        raise SystemExit(f"refusing bind {bound[0]}")
    server.token = secrets.token_hex(16)
    server.supervisor = gui_session.Supervisor()
    return server


def serve(port=PORT):
    server = make_server(port)
    print(f"gui http://{HOST}:{server.server_address[1]}/", flush=True)
    server.serve_forever()


def _file_hash(path):
    path = Path(path)
    if not path.exists():
        return None
    return hashlib.sha256(path.read_bytes()).hexdigest()


def self_test():
    import os
    import tempfile
    import urllib.request

    failures = []

    def check(cond, message):
        if not cond:
            failures.append(message)

    user_gui = gui_settings.settings_path()
    user_dest = gui_settings.destination_path()
    user_sender = gui_settings.sender_path()
    user_before = {
        "gui": _file_hash(user_gui),
        "destination": _file_hash(user_dest),
        "sender": _file_hash(user_sender),
    }
    frames_bin = ROOT / "firmware" / "main" / "frames.bin"
    frames_before = frames_bin.stat().st_mtime_ns if frames_bin.exists() else None

    folder = tempfile.TemporaryDirectory()
    os.environ["POTETOBOT_GUI_FILE"] = str(Path(folder.name) / "gui.json")
    gui_settings.save({"uiLanguage": "en", "sttLanguage": "ko", "provider": "orca_bots", "botId": "", "grokBotId": "", "characterId": "poteto"})
    loaded = gui_settings.load()
    check(loaded["uiLanguage"] == "en" and loaded["sttLanguage"] == "ko", "ui and stt stayed apart")
    gui_settings.save({"uiLanguage": "ko", "sttLanguage": "en", "provider": "orca_bots", "botId": "", "grokBotId": "", "characterId": "sunny"})
    loaded = gui_settings.load()
    check(loaded["uiLanguage"] == "ko" and loaded["sttLanguage"] == "en" and loaded["characterId"] == "sunny", "stt english saved")
    check(gui_settings.settings_path().name != "destination.json", "settings file name")
    stored_path = gui_settings.settings_path()
    stored_path.write_text(
        json.dumps({"uiLanguage": "ko", "botId": "Poteto", "note": "keep-me"}, ensure_ascii=False),
        encoding="utf-8",
    )
    gui_settings.save({"sttLanguage": "en"})
    kept = json.loads(stored_path.read_text(encoding="utf-8"))
    check(kept.get("note") == "keep-me" and kept.get("sttLanguage") == "en" and kept.get("uiLanguage") == "ko", f"merge {kept}")
    corrupt = b"{"
    stored_path.write_bytes(corrupt)
    try:
        gui_settings.load()
        check(False, "corrupt settings loaded")
    except gui_settings.SettingsError:
        pass
    try:
        gui_settings.save({"uiLanguage": "en"})
        check(False, "corrupt settings overwritten")
    except gui_settings.SettingsError:
        pass
    check(stored_path.read_bytes() == corrupt, "corrupt bytes changed")
    os.environ["POTETOBOT_GUI_FILE"] = str(Path(folder.name) / "destination.json")
    try:
        gui_settings.save({"uiLanguage": "ko"})
        check(False, "destination.json name accepted")
    except gui_settings.SettingsError as exc:
        check("destination.json" in str(exc), str(exc))
    check(not (Path(folder.name) / "destination.json").exists(), "destination.json created")
    os.environ["POTETOBOT_GUI_FILE"] = str(stored_path)
    stored_path.write_text("{}\n", encoding="utf-8")
    try:
        gui_settings.normalize({"uiLanguage": "ko", "sttLanguage": "ko", "provider": "orca_bots", "botId": "", "grokBotId": "", "characterId": "no-such"})
        check(False, "unknown character accepted")
    except ValueError:
        pass

    ko = gui_virtual.probe_transcribe("ko")
    en = gui_virtual.probe_transcribe("en")
    check(ko["language"] == "ko" and ko["fp16"] is False and ko["temperature"] == 0, f"ko kwargs {ko}")
    check(en["language"] == "en" and en["text"] == "virtual" and en["weightsLoaded"] is False, f"en kwargs {en}")
    default_model = gui_virtual.ProbeModel()
    gui_virtual.mvp_host.transcribe(b"\x00\x10" * 8, model=default_model)
    check(default_model.kwargs.get("language") == "ko", "omitted language stays ko")

    catalog = gui_characters.catalog()
    by_id = {item["id"]: item for item in catalog["characters"]}
    poteto = {state["id"]: state for state in by_id["poteto"]["states"]}
    sunny = {state["id"]: state for state in by_id["sunny"]["states"]}
    check(poteto["listening"]["boardFrames"] == 2 and poteto["listening"]["boardCompatible"], "poteto listening")
    check(poteto["thinking"]["boardFrames"] == 2, "poteto thinking")
    check(poteto["smile"]["boardFrames"] == 0 and poteto["smile"]["assetFrames"], "poteto smile preview only")
    check(poteto["emphasis"]["boardFrames"] == 0 and poteto["emphasis"]["assetFrames"], "poteto emphasis preview only")
    check(poteto["idle"]["boardFrames"] == 3 and poteto["speaking"]["boardFrames"] == 4, "poteto idle speaking")
    check(poteto["tilt"]["boardFrames"] == 4 and poteto["breath"]["boardFrames"] == 1, "poteto tilt breath")
    check(sunny["smile"]["boardFrames"] == 1 and sunny["idle"]["boardFrames"] == 2, "sunny packed")
    check(sunny["tilt"]["boardFrames"] == 0 and sunny["breath"]["boardFrames"] == 0, "sunny tilt breath off")
    check(poteto["happy"]["boardFrames"] == 0 and sunny["wink"]["boardFrames"] == 0, "unpacked names")
    preview = gui_characters.ASSETS / by_id["poteto"]["preview"]
    check(preview.is_file(), "poteto preview file")
    check("mint" in by_id, f"mint missing {sorted(by_id)}")
    mint = by_id["mint"]
    mint_states = {state["id"]: state for state in mint["states"]}
    check(mint["onBoard"] is False and mint["voiceCuesUseThisFace"] is False and mint["selectionSendsSel"] is False, f"mint role {mint}")
    check(mint_states["idle"]["boardFrames"] == 0 and mint_states["idle"]["assetFrames"], "mint preview frames")
    gui_settings.save({"characterId": "mint", "provider": "orca_bots", "botId": ""})
    check(gui_settings.load()["characterId"] == "mint", "mint saved")

    listed = gui_providers.list_orca_bots()
    names = {bot["name"] for bot in listed["bots"]}
    check(listed["ok"] and "Poteto" in names and "It'sMe" in names, f"orca list {listed.get('error')} {sorted(names)}")
    narrowed = gui_providers.list_orca_bots("Poteto")
    check(narrowed["ok"] and {bot["name"] for bot in narrowed["bots"]} == {"Poteto"}, "orca search")
    grok = gui_providers.grok_status()
    check(grok["bots"] == [] and grok["listApi"] is False and grok["roundTrip"] is False, "grok empty")
    check(grok["webhookHttp200IsAnswer"] is False and grok["deepLinkSends"] is False and grok["modelApiSubstitute"] is False, "grok boundary")
    link = gui_providers.grok_prompt_link("abc123", "hello")
    check(link.startswith("grokbot://app/v1/prompt?id=abc123&text=hello") and "send" not in link, link)

    ports = gui_ports.adapter_report()
    check(ports["windows"]["measured"] is True and ports["windows"]["opened"] is False, "windows listed closed")
    check(ports["mac"]["measured"] is False and ports["mac"]["status"] == "not_measured" and ports["mac"]["ports"] == [], "mac not measured")
    check(ports["linux"]["measured"] is False and ports["linux"]["status"] == "not_measured" and ports["linux"]["ports"] == [], "linux not measured")
    serial = ports["serial"]
    check(serial["macMeasured"] is False and serial["linuxMeasured"] is False and serial["posixExecutedHere"] is False, f"serial boundary {serial}")
    check(serial["pty"] == ("available" if __import__("os").name == "posix" else "unavailable"), f"pty {serial['pty']}")
    transfer = gui_serial.parameter_transfer()
    check(transfer.get("measuredDevice") is False and transfer.get("kind") == "socketpair" and transfer.get("payload") == "sock-byte" and transfer.get("closed") is True, f"parameter {transfer}")
    win_open = gui_serial.windows_parameter_open()
    check(win_open["baud"] == 115200 and win_open["byte"] == 8 and win_open["parity"] == 0 and win_open["stop"] == 0 and win_open["flags"] == 1, f"win open {win_open}")
    check(win_open["deviceOpened"] is False and win_open["closed"] is True and win_open["read"] == "ok", f"win loop {win_open}")

    os.environ.pop("POTETOBOT_PORT", None)
    spoken = gui_virtual.run_virtual("speak", "en")
    erred = gui_virtual.run_virtual("error", "ko")
    check(spoken["openedPort"] is False and spoken["sentToProvider"] is False and spoken["ask"] == "local_stub", "speak isolation")
    check(spoken["transcriptLanguage"] == "en" and spoken["samples"] == 2, f"speak language {spoken}")
    check(any("prepare" in line for line in spoken["cues"]) and any("speak virtual" in line for line in spoken["cues"]), f"speak cues {spoken['cues']}")
    check(spoken["ok"] == 1 and spoken["errors"] == 0, f"speak counts {spoken['ok']} {spoken['errors']}")
    check(erred["openedPort"] is False and any(line.endswith("error") for line in erred["cues"]), f"error cues {erred['cues']}")
    check(not any("speak" in line for line in erred["cues"]) and erred["errors"] >= 1, f"error session {erred}")

    server = make_server(0)
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    base = f"http://{HOST}:{server.server_address[1]}"

    def request(method, path, payload=None, origin=None, token=None, content_type="application/json"):
        import urllib.error

        data = None if payload is None else json.dumps(payload).encode("utf-8")
        req = urllib.request.Request(base + path, data=data, method=method)
        if data is not None:
            if content_type:
                req.add_header("Content-Type", content_type)
            if origin is None:
                origin = base
            if origin:
                req.add_header("Origin", origin)
            if token is None:
                token = server.token
            if token:
                req.add_header("X-Potetobot-Token", token)
        try:
            with urllib.request.urlopen(req, timeout=60) as response:
                return response.status, response.headers.get("Content-Type", ""), response.read()
        except urllib.error.HTTPError as exc:
            return exc.code, exc.headers.get("Content-Type", ""), exc.read()

    status, content_type, raw = request("GET", "/")
    check(status == 200 and b"ui-language" in raw and server.token.encode("ascii") in raw and b"__POTETOBOT_TOKEN__" not in raw, "page token")
    status, content_type, raw = request("GET", "/frames/poteto/idle/f00.png")
    check(status == 200 and content_type.startswith("image/png") and raw[:8] == b"\x89PNG\r\n\x1a\n", "preview bytes")
    status, _content_type, raw = request("GET", "/frames/../mvp_host.py")
    check(status == 404 and b"def transcribe" not in raw, f"traversal {status} {raw[:80]!r}")
    status, _ctype, raw = request("POST", "/api/stt-probe", {"language": "en"})
    probed = json.loads(raw.decode("utf-8"))
    check(status == 200 and probed["language"] == "en", f"http probe {probed}")
    status, _ctype, raw = request("POST", "/api/settings", {
        "uiLanguage": "en",
        "sttLanguage": "en",
        "provider": "orca_bots",
        "botId": "Poteto",
        "grokBotId": "",
        "characterId": "poteto",
    })
    saved = json.loads(raw.decode("utf-8"))
    check(status == 200 and saved["botId"] == "Poteto" and saved["sttLanguage"] == "en" and saved["uiLanguage"] == "en", f"http save {saved}")
    gui_file = Path(os.environ["POTETOBOT_GUI_FILE"])
    before_save = gui_file.read_bytes()
    status, _ctype, raw = request("POST", "/api/settings", {
        "uiLanguage": "ko",
        "sttLanguage": "ko",
        "provider": "grok_bot",
        "botId": "NotARealBot",
        "grokBotId": "fromshare",
        "characterId": "mint",
    }, origin="http://evil.example")
    rejected = json.loads(raw.decode("utf-8"))
    check(status == 403 and rejected.get("error") == "origin", f"settings origin {status} {rejected}")
    check(gui_file.read_bytes() == before_save, "evil origin wrote settings")
    status, _ctype, raw = request("POST", "/api/settings", {
        "uiLanguage": "ko",
        "sttLanguage": "ko",
        "provider": "grok_bot",
        "botId": "NotARealBot",
        "grokBotId": "fromshare",
        "characterId": "mint",
    })
    saved_grok = json.loads(raw.decode("utf-8"))
    check(status == 200 and saved_grok["provider"] == "grok_bot" and saved_grok["grokBotId"] == "fromshare" and saved_grok["characterId"] == "mint", f"grok save {status} {saved_grok}")
    status, _ctype, raw = request("POST", "/api/settings", {
        "uiLanguage": "ko",
        "sttLanguage": "ko",
        "provider": "orca_bots",
        "botId": "NotARealBot",
        "grokBotId": "",
        "characterId": "poteto",
    })
    rejected = json.loads(raw.decode("utf-8"))
    check(status == 400 and "Orca" in rejected.get("error", ""), f"fake orca bot {status} {rejected}")
    status, _content_type, raw = request("GET", "/frames/mint/idle/f00.png")
    check(status == 200 and raw[:8] == b"\x89PNG\r\n\x1a\n", "mint preview bytes")
    status, _ctype, raw = request("GET", "/api/bots?provider=grok_bot")
    grok_http = json.loads(raw.decode("utf-8"))
    check(status == 200 and grok_http["bots"] == [] and grok_http["listApi"] is False, "http grok empty")
    gui_bytes = gui_file.read_bytes()
    try:
        gui_session.prove_session(server, request, check)
    finally:
        try:
            server.supervisor.stop()
        except Exception as exc:
            check(False, f"supervisor stop {exc}")
    check(gui_file.read_bytes() == gui_bytes, "session start wrote gui.json")
    server.shutdown()
    folder.cleanup()

    user_after = {
        "gui": _file_hash(user_gui),
        "destination": _file_hash(user_dest),
        "sender": _file_hash(user_sender),
    }
    frames_after = frames_bin.stat().st_mtime_ns if frames_bin.exists() else None
    check(user_before == user_after, f"user files changed {user_before} {user_after}")
    check(frames_before == frames_after, "frames.bin changed")
    if failures:
        for message in failures:
            print(f"gui fail {message}")
        raise SystemExit(1)
    print(f"windows ports {','.join(ports['windows']['ports'])}")
    print(f"speak cues {' | '.join(spoken['cues'])}")
    print(f"error cues {' | '.join(erred['cues'])}")
    print("gui self-test ok")


if __name__ == "__main__":
    if "--self-test" in sys.argv:
        self_test()
    else:
        serve(PORT)
