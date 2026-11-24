"""One product serial session for the loopback page.

The selected port is an argument to open_serial inside a child process.
It is not written to POTETOBOT_PORT, destination.json, or the ask child.
The destination is a session file. The speech language is an explicit
argument; omitting it on the CLI still uses transcribe()'s Korean default.
Start does not send. Stop does not write a late speak cue.
"""

import json
import os
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

import mvp_host

ROOT = mvp_host.ROOT


def _write_json(path, payload):
    path = Path(path)
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(payload, ensure_ascii=False), encoding="utf-8")
    for attempt in range(30):
        try:
            os.replace(temporary, path)
            return
        except PermissionError:
            if attempt == 29:
                raise
            time.sleep(0.02)


def _read_json(path):
    path = Path(path)
    if not path.is_file():
        return None
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return None


def clean_port(value):
    text = str(value or "").strip()
    if not text or len(text) > 64:
        raise ValueError("port")
    if any(ord(ch) < 32 or ch in '"\\' for ch in text):
        raise ValueError("port")
    return text


def _blank_status():
    return {
        "state": "stopped",
        "port": "",
        "provider": "",
        "botId": "",
        "sttLanguage": "",
        "handle": None,
        "error": "",
        "cues": [],
        "opener": "",
        "rxCap": 0,
        "dropped": 0,
        "autoSend": False,
        "pid": 0,
    }


class Supervisor:
    def __init__(self):
        self.lock = threading.RLock()
        self.proc = None
        self.folder = None
        self.status_path = None
        self.stop_path = None
        self.rx_path = None
        self.dest_path = None
        self.use_virtual = False
        self.probe = False
        self.transcripts = None
        self.ask_cmd = None
        self.ask_env = None
        self.virtual_cap = 65536
        self.ask_timeout = 120

    def status(self):
        with self.lock:
            current = _read_json(self.status_path) if self.status_path else None
        if not isinstance(current, dict):
            return _blank_status()
        base = _blank_status()
        base.update(current)
        base["autoSend"] = False
        return base

    def offer(self, data):
        with self.lock:
            path = self.rx_path
            virtual = self.use_virtual
        if not virtual or path is None:
            raise RuntimeError("virtual port is not the opener")
        with open(path, "ab") as handle:
            handle.write(data)

    def start(self, pinned):
        with self.lock:
            if self.proc is not None and self.proc.poll() is None:
                current = self.status()
                current["error"] = "session already running"
                current["accepted"] = False
                return current
            folder = Path(tempfile.mkdtemp(prefix="potetobot-session-"))
            dest = folder / "session-dest.json"
            bot_id = pinned["botId"] if pinned["provider"] == "orca_bots" else pinned.get("grokBotId") or ""
            dest.write_text(
                json.dumps({"provider": pinned["provider"], "botId": bot_id}, ensure_ascii=False) + "\n",
                encoding="utf-8",
            )
            status_path = folder / "status.json"
            stop_path = folder / "stop"
            rx_path = folder / "rx.bin"
            rx_path.write_bytes(b"")
            spec = {
                "port": pinned["port"],
                "provider": pinned["provider"],
                "botId": bot_id,
                "sttLanguage": pinned["sttLanguage"],
                "destination": str(dest),
                "status": str(status_path),
                "stop": str(stop_path),
                "rx": str(rx_path),
                "virtual": bool(self.use_virtual),
                "rxCap": int(self.virtual_cap),
                "probe": bool(self.probe),
                "transcripts": list(self.transcripts or []),
                "ask": list(self.ask_cmd or [str(mvp_host.ASK)]),
                "askEnv": dict(self.ask_env or {}),
                "askTimeout": int(self.ask_timeout),
                "whisper": str(folder / "whisper.jsonl"),
            }
            spec_path = folder / "spec.json"
            _write_json(spec_path, spec)
            env = os.environ.copy()
            env.pop("POTETOBOT_PORT", None)
            log_path = folder / "child.log"
            log_handle = open(log_path, "ab")
            try:
                proc = subprocess.Popen(
                    [sys.executable, str(Path(__file__).resolve()), "--child", str(spec_path)],
                    cwd=str(ROOT),
                    env=env,
                    stdout=log_handle,
                    stderr=log_handle,
                )
            finally:
                log_handle.close()
            self.proc = proc
            self.child_log = log_path
            self.folder = folder
            self.status_path = status_path
            self.stop_path = stop_path
            self.rx_path = rx_path
            self.dest_path = dest
        deadline = time.monotonic() + 8
        while time.monotonic() < deadline:
            current = self.status()
            if current["state"] in ("connected", "failed"):
                if current["state"] == "failed":
                    try:
                        proc.wait(timeout=3)
                    except subprocess.TimeoutExpired:
                        pass
                current["accepted"] = True
                return current
            if proc.poll() is not None:
                break
            time.sleep(0.02)
        current = self.status()
        if current["state"] == "stopped" and not current["error"]:
            current["state"] = "failed"
            current["error"] = "session did not report status"
        current["accepted"] = True
        return current

    def stop(self):
        with self.lock:
            proc = self.proc
            stop_path = self.stop_path
        if proc is None or proc.poll() is not None:
            current = self.status()
            if current["state"] == "connected":
                current["state"] = "stopped"
            return current
        Path(stop_path).write_text("stop\n", encoding="utf-8")
        try:
            proc.wait(timeout=8)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait(timeout=3)
            current = self.status()
            current["state"] = "failed"
            current["error"] = current["error"] or "stop timed out"
            return current
        return self.status()


def _publish(path, box, lock):
    with lock:
        payload = dict(box)
        payload["cues"] = list(box["cues"])
    _write_json(path, payload)


def _child(spec):
    os.environ.pop("POTETOBOT_PORT", None)
    status_path = Path(spec["status"])
    stop_path = Path(spec["stop"])
    box = _blank_status()
    box.update(
        {
            "port": spec["port"],
            "provider": spec["provider"],
            "botId": spec["botId"],
            "sttLanguage": spec["sttLanguage"],
            "opener": "virtual" if spec.get("virtual") else "win32",
            "rxCap": int(spec.get("rxCap") or 0),
            "pid": os.getpid(),
        }
    )
    lock = threading.Lock()
    done = threading.Event()
    link = None

    def stopped():
        return stop_path.is_file()

    try:
        if spec.get("virtual"):
            kernel = mvp_host.BoundedKernel(int(spec.get("rxCap") or 65536))
            link = mvp_host.SerialLink(kernel, 7)
            link.kernel_ref = kernel
        else:
            link = mvp_host.open_serial(spec["port"])
        box["state"] = "connected"
        box["handle"] = int(link.handle) if link.handle is not None else None
        _publish(status_path, box, lock)

        def watch():
            while not done.is_set():
                if stopped():
                    try:
                        link.close()
                    except Exception:
                        pass
                    return
                time.sleep(0.02)

        def feed():
            if not spec.get("virtual"):
                return
            rx = Path(spec["rx"])
            offset = 0
            while not done.is_set():
                if rx.with_name("rx.fail").is_file():
                    kernel.finish()
                    return
                try:
                    data = rx.read_bytes()
                except OSError:
                    data = b""
                if len(data) > offset:
                    kernel.offer(data[offset:])
                    offset = len(data)
                time.sleep(0.02)

        def mirror():
            while not done.wait(0.05):
                with lock:
                    if box["state"] in ("failed", "stopped"):
                        return
                    box["cues"] = bytes(link.sent).decode("utf-8", "replace").splitlines()
                    if spec.get("virtual"):
                        box["dropped"] = kernel.dropped
                try:
                    _publish(status_path, box, lock)
                except OSError:
                    continue

        threading.Thread(target=watch, name="poteto-session-stop", daemon=True).start()
        threading.Thread(target=feed, name="poteto-session-rx", daemon=True).start()
        threading.Thread(target=mirror, name="poteto-session-status", daemon=True).start()
        transcripts = list(spec.get("transcripts") or [])
        whisper_path = Path(spec["whisper"])

        def transcribe_fn(pcm, language=spec["sttLanguage"]):
            if spec.get("probe"):
                import gui_virtual

                model = gui_virtual.ProbeModel()
                if transcripts:
                    model.transcribe = _scripted(model, transcripts)
                text = mvp_host.transcribe(pcm, language=language, model=model)
                record = {
                    "language": None if model.kwargs is None else model.kwargs.get("language"),
                    "fp16": None if model.kwargs is None else model.kwargs.get("fp16"),
                    "temperature": None if model.kwargs is None else model.kwargs.get("temperature"),
                    "samples": model.samples,
                    "text": text,
                }
                with whisper_path.open("a", encoding="utf-8") as handle:
                    handle.write(json.dumps(record) + "\n")
                return text
            return mvp_host.transcribe(pcm, language=language)

        try:
            mvp_host.run_session(
                link,
                Path(spec["destination"]),
                list(spec["ask"]),
                transcribe_fn,
                observe=False,
                ask_timeout=int(spec.get("askTimeout") or 120),
                should_stop=stopped,
                ask_env=spec.get("askEnv") or None,
            )
        except SystemExit as exc:
            with lock:
                if not stopped():
                    box["state"] = "failed"
                    box["error"] = str(exc)
            return
        with lock:
            if stopped():
                box["state"] = "stopped"
                box["error"] = ""
            elif getattr(link, "session_read_error", None):
                box["state"] = "failed"
                box["error"] = str(link.session_read_error)
            else:
                box["state"] = "stopped"
    except SystemExit as exc:
        with lock:
            box["state"] = "failed"
            box["error"] = str(exc)
            box["handle"] = None
    except Exception as exc:
        with lock:
            box["state"] = "failed"
            box["error"] = f"{type(exc).__name__}: {exc}"
            box["handle"] = None
    finally:
        done.set()
        time.sleep(0.12)
        if link is not None:
            try:
                with lock:
                    box["cues"] = bytes(link.sent).decode("utf-8", "replace").splitlines()
                    if spec.get("virtual") and hasattr(link, "kernel_ref"):
                        box["dropped"] = link.kernel_ref.dropped
                link.close()
            except Exception as exc:
                with lock:
                    if box["state"] == "connected":
                        box["state"] = "failed"
                        box["error"] = str(exc)
        _publish(status_path, box, lock)


def _scripted(model, transcripts):
    def transcribe(samples, **kwargs):
        model.kwargs = dict(kwargs)
        model.samples = len(samples)
        if not transcripts:
            return {"text": ""}
        return {"text": transcripts.pop(0)}

    return transcribe


def child_main(spec_path):
    spec = json.loads(Path(spec_path).read_text(encoding="utf-8"))
    try:
        _child(spec)
    except Exception as exc:
        path = Path(spec.get("status", ""))
        if path:
            current = _blank_status()
            current["state"] = "failed"
            current["error"] = f"{type(exc).__name__}: {exc}"
            try:
                _write_json(path, current)
            except OSError:
                pass
        raise


def write_orca_band(directory):
    """Stand-in for the orca binary. ask_main/channel.c still parses the reply."""
    script = Path(directory) / "band.py"
    script.write_text(
        "import json, os, sys, time\n"
        "from pathlib import Path\n"
        "log = Path(os.environ['POTETOBOT_BAND_LOG'])\n"
        "args = sys.argv[1:]\n"
        "kind = 'other'\n"
        "if 'send' in args:\n"
        "    kind = 'send'\n"
        "elif 'inbox' in args:\n"
        "    kind = 'inbox'\n"
        "elif 'chats' in args:\n"
        "    kind = 'chats'\n"
        "elif 'create' in args:\n"
        "    kind = 'create'\n"
        "record = {'kind': kind}\n"
        "if kind == 'send':\n"
        "    record['to'] = args[args.index('--to') + 1]\n"
        "    body_path = args[args.index('--body-file') + 1]\n"
        "    record['bodyPath'] = body_path\n"
        "    record['body'] = Path(body_path).read_text(encoding='utf-8')\n"
        "    state = log.with_suffix('.n')\n"
        "    n = int(state.read_text(encoding='utf-8')) + 1 if state.is_file() else 1\n"
        "    state.write_text(str(n), encoding='utf-8')\n"
        "    record['id'] = 'msg_band%d' % n\n"
        "    log.parent.mkdir(parents=True, exist_ok=True)\n"
        "    with log.open('a', encoding='utf-8') as handle:\n"
        "        handle.write(json.dumps(record) + '\\n')\n"
        "    if record['body'].strip() == 'utt-slow':\n"
        "        time.sleep(3)\n"
        "    sys.stdout.write(json.dumps({'messageId': record['id']}) + '\\n')\n"
        "    raise SystemExit(0)\n"
        "log.parent.mkdir(parents=True, exist_ok=True)\n"
        "with log.open('a', encoding='utf-8') as handle:\n"
        "    handle.write(json.dumps(record) + '\\n')\n"
        "if kind == 'inbox':\n"
        "    last = None\n"
        "    if log.is_file():\n"
        "        for line in log.read_text(encoding='utf-8').splitlines():\n"
        "            row = json.loads(line)\n"
        "            if row.get('kind') == 'send':\n"
        "                last = row\n"
        "    ident = last['id'] if last else 'msg_band0'\n"
        "    message = {\n"
        "        'thread_id': ident,\n"
        "        'body': 'band-answer',\n"
        "        'from_handle': 'orca_session_id:11111111-2222-3333-4444-555555555555',\n"
        "        'payload': {'requestId': ident},\n"
        "    }\n"
        "    sys.stdout.write(json.dumps({'messages': [message]}) + '\\n')\n"
        "    raise SystemExit(0)\n"
        "if kind == 'chats':\n"
        "    sys.stdout.write(json.dumps({'result': {'chats': [{'id': '11111111-2222-3333-4444-555555555555'}]}}) + '\\n')\n"
        "    raise SystemExit(0)\n"
        "if kind == 'create':\n"
        "    sys.stdout.write(json.dumps({'handle': 'term_bandtest'}) + '\\n')\n"
        "    raise SystemExit(0)\n"
        "sys.stdout.write('{}\\n')\n",
        encoding="utf-8",
    )
    return script


def build_band_launcher(directory, script):
    c_path = Path(directory) / "band_launch.c"
    exe = Path(directory) / "band_launch.exe"
    py = str(Path(sys.executable)).replace("\\", "\\\\")
    sc = str(Path(script)).replace("\\", "\\\\")
    c_path.write_text(
        "#include <windows.h>\n"
        "#include <stdio.h>\n"
        "int main(void) {\n"
        "    char *full = GetCommandLineA();\n"
        "    char *rest = full;\n"
        "    char line[8192];\n"
        "    STARTUPINFOA si;\n"
        "    PROCESS_INFORMATION pi;\n"
        "    DWORD code = 1;\n"
        "    if (*rest == '\"') {\n"
        "        rest++;\n"
        "        while (*rest && *rest != '\"') rest++;\n"
        "        if (*rest == '\"') rest++;\n"
        "    } else {\n"
        "        while (*rest && *rest != ' ') rest++;\n"
        "    }\n"
        "    while (*rest == ' ') rest++;\n"
        f'    snprintf(line, sizeof(line), "\\"%s\\" \\"%s\\" %s", "{py}", "{sc}", rest);\n'
        "    memset(&si, 0, sizeof(si));\n"
        "    si.cb = sizeof(si);\n"
        "    if (!CreateProcessA(NULL, line, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) return 1;\n"
        "    WaitForSingleObject(pi.hProcess, 20000);\n"
        "    GetExitCodeProcess(pi.hProcess, &code);\n"
        "    CloseHandle(pi.hThread);\n"
        "    CloseHandle(pi.hProcess);\n"
        "    return (int)code;\n"
        "}\n",
        encoding="utf-8",
    )
    mvp_host.run_compile(f'cl /nologo /O2 /Fe:"{exe}" "{c_path}"')
    if not exe.is_file():
        raise RuntimeError("band launcher missing")
    return exe


def band_sends(log_path):
    path = Path(log_path)
    if not path.is_file():
        return []
    rows = []
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        row = json.loads(line)
        if row.get("kind") == "send":
            rows.append(row)
    return rows


def _wait_status(supervisor, pred, timeout):
    deadline = time.monotonic() + timeout
    last = supervisor.status()
    while time.monotonic() < deadline:
        last = supervisor.status()
        if pred(last):
            return last
        time.sleep(0.05)
    return last


def prove_session(server, request, check):
    """Same start/status/stop HTTP path. Serial is virtual. Orca is the band."""
    import shutil
    import tempfile as _temp

    outer_check = check
    supervisor = server.supervisor

    def check(cond, message):
        if cond:
            outer_check(True, message)
            return
        tail = ""
        log = getattr(supervisor, "child_log", None)
        if log and Path(log).is_file():
            tail = Path(log).read_text(encoding="utf-8", errors="replace")[-2000:]
        outer_check(False, f"{message}\n{tail}")
    folder = Path(_temp.mkdtemp(prefix="potetobot-band-"))
    script = write_orca_band(folder)
    launcher = build_band_launcher(folder, script)
    if not mvp_host.ASK.is_file():
        mvp_host.run_compile(mvp_host.ASK_BUILD)
    else:
        mvp_host.run_compile(mvp_host.ASK_BUILD)
    if not mvp_host.AUD.is_file():
        mvp_host.run_compile(mvp_host.AUD_BUILD)
    sender = folder / "sender.json"
    sender.write_text('{"handle":"term_bandtest"}\n', encoding="utf-8")
    log_path = folder / "band.jsonl"

    def arm(log, transcripts, virtual, timeout):
        supervisor.use_virtual = virtual
        supervisor.probe = True
        supervisor.transcripts = list(transcripts)
        supervisor.ask_cmd = [str(mvp_host.ASK)]
        supervisor.ask_timeout = timeout
        supervisor.ask_env = {
            "ORCA_CLI_COMMAND": str(launcher),
            "POTETOBOT_SENDER_FILE": str(sender),
            "POTETOBOT_BAND_LOG": str(log),
        }
        if Path(log).exists():
            Path(log).unlink()

    def post(path, payload, origin=None):
        return request("POST", path, payload, origin=origin)

    status, _ctype, raw = post("/api/session/start", {
        "port": "COM99",
        "provider": "orca_bots",
        "botId": "Poteto",
        "grokBotId": "",
        "sttLanguage": "ko",
    }, origin="http://evil.example")
    rejected = json.loads(raw.decode("utf-8"))
    check(status == 403 and rejected.get("error") == "origin", f"origin {status} {rejected}")
    check(supervisor.proc is None, "origin spawned a session")

    arm(log_path, [], False, 5)
    status, _ctype, raw = post("/api/session/start", {
        "port": "COM99",
        "provider": "orca_bots",
        "botId": "Poteto",
        "grokBotId": "",
        "sttLanguage": "en",
    })
    opened = json.loads(raw.decode("utf-8"))
    check(opened.get("state") == "failed" and "serial open failed" in opened.get("error", ""), f"missing port {opened}")
    check(opened.get("handle") is None and opened.get("autoSend") is False, f"missing handle {opened}")
    check(band_sends(log_path) == [], "missing port asked")

    arm(log_path, ["hello-band", "hello-two"], True, 8)
    status, _ctype, raw = post("/api/session/start", {
        "port": "VIRTUAL",
        "provider": "orca_bots",
        "botId": "Poteto",
        "grokBotId": "",
        "sttLanguage": "en",
    })
    started = json.loads(raw.decode("utf-8"))
    check(status == 200 and started.get("state") == "connected" and started.get("handle") == 7, f"start {status} {started}")
    check(started.get("opener") == "virtual" and started.get("autoSend") is False, f"opener {started}")
    check("POTETOBOT_PORT" not in (supervisor.proc.args if False else os.environ) or os.environ.get("POTETOBOT_PORT", "") == "", "parent port env")
    dest_bytes = supervisor.dest_path.read_bytes()
    check(b'"botId": "Poteto"' in dest_bytes or b'"botId":"Poteto"' in dest_bytes, dest_bytes.decode("utf-8"))
    check(band_sends(log_path) == [], "start sent before audio")
    status, _ctype, raw = post("/api/session/start", {
        "port": "VIRTUAL",
        "provider": "orca_bots",
        "botId": "Poteto",
        "grokBotId": "",
        "sttLanguage": "en",
    })
    duplicate = json.loads(raw.decode("utf-8"))
    check(status == 409 and duplicate.get("error") == "session already running", f"duplicate {status} {duplicate}")
    audio = mvp_host.aud_stream(bytes([1, 2, 3, 4]))
    supervisor.offer(audio)
    spoken = _wait_status(supervisor, lambda row: any("speak band-answer" in line for line in row.get("cues") or []), 8)
    sends = band_sends(log_path)
    check(len(sends) == 1 and sends[0].get("to") == "bot:Poteto" and sends[0].get("body", "").strip() == "hello-band", f"first ask {sends}")
    check("potetobot-ask-body.txt" not in sends[0].get("bodyPath", ""), sends[0].get("bodyPath"))
    cues = spoken.get("cues") or []
    prepare = [line for line in cues if line.endswith("prepare")]
    speak = [line for line in cues if "speak band-answer" in line]
    check(len(prepare) == 1 and len(speak) == 1 and prepare[0].split(" ")[2] == speak[0].split(" ")[2], f"cue pair {cues}")
    check(supervisor.dest_path.read_bytes() == dest_bytes, "session dest changed")
    supervisor.offer(audio)
    spoken = _wait_status(supervisor, lambda row: sum("speak band-answer" in line for line in row.get("cues") or []) >= 2, 8)
    sends = band_sends(log_path)
    check(len(sends) == 2 and sends[1].get("body", "").strip() == "hello-two", f"second ask {sends}")
    check(sends[0].get("bodyPath") != sends[1].get("bodyPath"), "shared ask body")
    whisper = Path(supervisor.folder) / "whisper.jsonl"
    rows = [json.loads(line) for line in whisper.read_text(encoding="utf-8").splitlines() if line.strip()]
    check(len(rows) == 2 and all(row.get("language") == "en" and row.get("fp16") is False and row.get("temperature") == 0 for row in rows), f"whisper {rows}")
    status, _ctype, raw = post("/api/session/stop", {})
    stopped = json.loads(raw.decode("utf-8"))
    check(stopped.get("state") == "stopped", f"stop {stopped}")
    before = len(band_sends(log_path))
    supervisor.offer(audio)
    time.sleep(0.4)
    check(len(band_sends(log_path)) == before, "stop sent again")
    check(sum("speak" in line for line in supervisor.status().get("cues") or []) == 2, "late speak")

    empty_log = folder / "empty.jsonl"
    arm(empty_log, ["hello-band"], True, 8)
    status, _ctype, raw = post("/api/session/start", {
        "port": "VIRTUAL",
        "provider": "orca_bots",
        "botId": "",
        "grokBotId": "",
        "sttLanguage": "ko",
    })
    empty = json.loads(raw.decode("utf-8"))
    check(empty.get("state") == "connected", f"empty start {empty}")
    supervisor.offer(audio)
    empty_done = _wait_status(supervisor, lambda row: any(line.endswith("error") for line in row.get("cues") or []), 8)
    check(band_sends(empty_log) == [], f"empty asked {band_sends(empty_log)}")
    check(any(line.endswith("error") for line in empty_done.get("cues") or []), f"empty cue {empty_done}")
    check(not any("speak" in line for line in empty_done.get("cues") or []), "empty spoke")
    post("/api/session/stop", {})

    grok_log = folder / "grok.jsonl"
    arm(grok_log, ["hello-band"], True, 8)
    status, _ctype, raw = post("/api/session/start", {
        "port": "VIRTUAL",
        "provider": "grok_bot",
        "botId": "Poteto",
        "grokBotId": "fromshare",
        "sttLanguage": "ko",
    })
    grok = json.loads(raw.decode("utf-8"))
    check(grok.get("state") == "connected", f"grok start {grok}")
    supervisor.offer(audio)
    grok_done = _wait_status(supervisor, lambda row: any(line.endswith("error") for line in row.get("cues") or []), 8)
    check(band_sends(grok_log) == [], f"grok asked {band_sends(grok_log)}")
    check(any("error" in line for line in grok_done.get("cues") or []), f"grok cue {grok_done}")
    post("/api/session/stop", {})

    slow_log = folder / "slow.jsonl"
    arm(slow_log, ["utt-slow"], True, 2)
    status, _ctype, raw = post("/api/session/start", {
        "port": "VIRTUAL",
        "provider": "orca_bots",
        "botId": "Poteto",
        "grokBotId": "",
        "sttLanguage": "ko",
    })
    slow = json.loads(raw.decode("utf-8"))
    check(slow.get("state") == "connected", f"slow start {slow}")
    supervisor.offer(audio)
    sent = _wait_status(supervisor, lambda _row: len(band_sends(slow_log)) >= 1, 6)
    check(len(band_sends(slow_log)) == 1, f"slow send {sent}")
    status, _ctype, raw = post("/api/session/stop", {})
    halted = json.loads(raw.decode("utf-8"))
    check(halted.get("state") == "stopped", f"slow stop {halted}")
    check(not any("speak" in line for line in halted.get("cues") or []), f"slow spoke {halted.get('cues')}")
    time.sleep(0.3)
    check(len(band_sends(slow_log)) == 1, "slow sent after stop")

    next_log = folder / "next.jsonl"
    arm(next_log, ["utt-next"], True, 8)
    status, _ctype, raw = post("/api/session/start", {
        "port": "VIRTUAL",
        "provider": "orca_bots",
        "botId": "Poteto",
        "grokBotId": "",
        "sttLanguage": "ko",
    })
    again = json.loads(raw.decode("utf-8"))
    check(again.get("state") == "connected", f"recover start {again}")
    supervisor.offer(audio)
    recovered = _wait_status(supervisor, lambda row: any("speak band-answer" in line for line in row.get("cues") or []), 8)
    rows = [json.loads(line) for line in (Path(supervisor.folder) / "whisper.jsonl").read_text(encoding="utf-8").splitlines() if line.strip()]
    check(len(band_sends(next_log)) == 1 and band_sends(next_log)[0].get("body", "").strip() == "utt-next", band_sends(next_log))
    check(rows and rows[-1].get("language") == "ko", f"recover language {rows}")
    check(any("speak band-answer" in line for line in recovered.get("cues") or []), f"recover cue {recovered}")
    post("/api/session/stop", {})

    fail_log = folder / "fail.jsonl"
    arm(fail_log, ["hello-band"], True, 5)
    status, _ctype, raw = post("/api/session/start", {
        "port": "VIRTUAL",
        "provider": "orca_bots",
        "botId": "Poteto",
        "grokBotId": "",
        "sttLanguage": "ko",
    })
    live = json.loads(raw.decode("utf-8"))
    check(live.get("state") == "connected", f"fail setup {live}")
    (supervisor.folder / "rx.fail").write_text("fail\n", encoding="utf-8")
    failed = _wait_status(supervisor, lambda row: row.get("state") == "failed", 8)
    check(failed.get("state") == "failed" and failed.get("error"), f"read fail hidden {failed}")
    check(band_sends(fail_log) == [], "read fail asked")
    try:
        supervisor.stop()
    finally:
        shutil.rmtree(folder, ignore_errors=True)


if __name__ == "__main__":
    if len(sys.argv) >= 3 and sys.argv[1] == "--child":
        try:
            child_main(sys.argv[2])
        except Exception:
            raise SystemExit(1)
        raise SystemExit(0)
    raise SystemExit("gui session child")
