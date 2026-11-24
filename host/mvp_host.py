"""One Windows entry for KEY3 utterances.

Sim and serial share one session: read AUD bytes, transcribe, ask once,
write CUE for that utterance id when the request id arrives, then CUE
speak with that answer. A bad utterance cues error and the next good
utterance still runs. While Whisper or the request blocks, a reader on
the same handle keeps draining RX into at most 16 utterances, each at
most 6.0 seconds. Past that cap the utterance is busy: its PCM is not
kept, an error cue is written for its id, and the next utterance still
runs. Serial opens POTETOBOT_PORT only when --inject is absent, and that
path does not launch the face simulator.
"""

import os
import queue
import subprocess
import sys
import tempfile
import threading
import time
import wave
from pathlib import Path

from aud_frame import CHUNK, MAX_BYTES, AudError, assemble

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "host" / "build"
AUD = BUILD / "aud_format.exe"
ASK = BUILD / "ask_main.exe"
SIM = BUILD / "sim_companion.exe"
MATCH = BUILD / "test_channel.exe"

AUD_BUILD = r"cl /nologo /O2 /std:c11 /I firmware\main /Fe:host\build\aud_format.exe host\aud_format.c firmware\main\companion.c"
ASK_BUILD = r"cl /nologo /O2 /std:c11 /I firmware\main /I host /Fe:host\build\ask_main.exe host\ask_main.c host\channel.c host\destination.c"
SIM_BUILD = r"cl /nologo /O2 /std:c11 /I firmware\main /I host /Fe:host\build\sim_companion.exe host\sim_companion.c host\channel.c host\destination.c firmware\main\companion.c firmware\main\schedule.c firmware\main\frame_table.c"
MATCH_BUILD = r"cl /nologo /O2 /std:c11 /I firmware\main /I host /Fe:host\build\test_channel.exe host\test_channel.c host\channel.c host\destination.c"

MAX_LINE = 2048
MAX_UTT_LINES = 400
UTT_WAIT_S = 8.0
MAX_QUEUED_UTTERANCES = 16
face_runs = 0


def fail(message):
    raise SystemExit(message)


class SerialReadError(Exception):
    pass


class SerialWriteError(Exception):
    pass


class LineTooLong(Exception):
    pass


class ReadTimeout(Exception):
    pass


def compile_prefix():
    vswhere = Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.exists():
        return ""
    found = subprocess.run(
        [str(vswhere), "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"],
        capture_output=True,
        text=True,
        check=False,
    )
    root = found.stdout.strip()
    if root == "":
        return ""
    bat = Path(root) / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
    if not bat.exists():
        return ""
    return f'call "{bat}" && '


def run_compile(command):
    built = subprocess.run(compile_prefix() + command, cwd=ROOT, capture_output=True, check=False, shell=True)
    if built.returncode != 0:
        sys.stderr.write(built.stdout.decode("utf-8", "replace"))
        sys.stderr.write(built.stderr.decode("utf-8", "replace"))
        fail(f"build exit {built.returncode}")


def clean_env(extra):
    env = os.environ.copy()
    for key in list(env):
        if key.startswith("ORCA_") or key == "POTETOBOT_PORT":
            env.pop(key, None)
    env.update(extra)
    return env


def load_wav(path):
    with wave.open(str(path), "rb") as handle:
        if handle.getnchannels() != 1 or handle.getsampwidth() != 2 or handle.getframerate() != 16000:
            fail("wav must be 16000 Hz mono s16")
        pcm = handle.readframes(handle.getnframes())
    if len(pcm) == 0 or (len(pcm) % 2) != 0 or len(pcm) > MAX_BYTES:
        fail(f"pcm length {len(pcm)} outside 2..{MAX_BYTES}")
    return pcm


def format_pcm(pcm, allow_drop=False):
    BUILD.mkdir(parents=True, exist_ok=True)
    raw_path = BUILD / "utt.pcm"
    raw_path.write_bytes(pcm)
    proc = subprocess.run([str(AUD), str(raw_path)], cwd=ROOT, capture_output=True, check=False)
    if proc.returncode != 0 and not (allow_drop and proc.returncode == 3):
        sys.stderr.write(proc.stderr.decode("utf-8", "replace"))
        fail(f"aud_format exit {proc.returncode}")
    return proc.stdout.decode("ascii").splitlines()


def excerpt(text):
    data = text.split("\n", 1)[0].encode("utf-8")
    out = bytearray()
    i = 0
    while i < len(data):
        lead = data[i]
        if lead < 0x80:
            span = 1
        elif lead & 0xE0 == 0xC0:
            span = 2
        elif lead & 0xF0 == 0xE0:
            span = 3
        elif lead & 0xF8 == 0xF0:
            span = 4
        else:
            break
        if i + span > len(data) or len(out) + span >= 96:
            break
        out += data[i : i + span]
        i += span
    return out.decode("utf-8")


class Bind:
    def __init__(self):
        self.inflight = None

    def start(self, utt):
        if self.inflight is not None:
            return False
        self.inflight = utt
        return True

    def finish(self, utt):
        if self.inflight != utt:
            return False
        self.inflight = None
        return True


STT_LANGUAGES = ("ko", "en")


def whisper_kwargs(language):
    """Arguments for Whisper transcribe. Korean stays the default."""
    chosen = language if language in STT_LANGUAGES else "ko"
    return {"language": chosen, "fp16": False, "temperature": 0}


def transcribe(pcm, language="ko", model=None):
    import numpy

    samples = numpy.frombuffer(pcm, dtype="<i2").astype("float32") / 32768.0
    kwargs = whisper_kwargs(language)
    if model is None:
        import whisper

        model = whisper.load_model("base", device="cpu")
    result = model.transcribe(samples, **kwargs)
    return str(result.get("text", "")).strip()


class InjectLink:
    """Board bytes in, host cues out. Empty read is end of stream."""

    eof_on_empty = True

    def __init__(self, data, read_size=4096):
        self.data = data
        self.read_size = read_size
        self.offset = 0
        self.sent = bytearray()
        self.states = []

    def read(self, _n):
        if self.offset >= len(self.data):
            return b""
        take = min(self.read_size, len(self.data) - self.offset)
        chunk = self.data[self.offset : self.offset + take]
        self.offset += take
        return chunk

    def write(self, data):
        self.sent += data

    def close(self):
        return None


class LineReader:
    def __init__(self, link):
        self.link = link
        self.buf = b""

    def unread(self, line):
        self.buf = line.encode("utf-8") + b"\n" + self.buf

    def readline(self, deadline=None, wait_s=UTT_WAIT_S):
        partial_at = None
        while b"\n" not in self.buf:
            now = time.monotonic()
            if self.buf:
                if partial_at is None:
                    partial_at = now
                elif now - partial_at >= wait_s:
                    self.buf = b""
                    raise ReadTimeout()
            if deadline is not None and now >= deadline:
                raise ReadTimeout()
            if len(self.buf) > MAX_LINE:
                self.buf = b""
                raise LineTooLong()
            chunk = self.link.read(self.link.read_size)
            if chunk:
                self.buf += chunk
                if len(self.buf) > MAX_LINE and b"\n" not in self.buf[: MAX_LINE + 1]:
                    nl = self.buf.find(b"\n")
                    if nl < 0:
                        self.buf = b""
                    else:
                        self.buf = self.buf[nl + 1 :]
                    raise LineTooLong()
                continue
            if self.link.eof_on_empty:
                break
            if deadline is None and partial_at is None:
                continue
            time.sleep(0.01)
        if b"\n" in self.buf:
            raw, self.buf = self.buf.split(b"\n", 1)
            if len(raw) > MAX_LINE:
                raise LineTooLong()
            return raw.decode("utf-8", "replace").strip("\r")
        if self.buf and self.link.eof_on_empty:
            raw = self.buf
            self.buf = b""
            if len(raw) > MAX_LINE:
                raise LineTooLong()
            return raw.decode("utf-8", "replace").strip("\r")
        return None


def aud_stream(pcm):
    lines = format_pcm(pcm)
    return ("\n".join(lines) + "\n").encode("ascii")


def aud_utt(line):
    parts = line.split(" ")
    if len(parts) < 3 or parts[0] != "AUD" or parts[1] != "v1" or not parts[2].isdigit():
        return None
    return int(parts[2])


def aud_mark(line):
    parts = line.split(" ")
    if len(parts) < 4:
        return ""
    return parts[3]


def drain_same(reader, utt, started, wait_s, line):
    if aud_mark(line) in ("end", "drop"):
        return
    while True:
        try:
            nxt = reader.readline(started + wait_s, wait_s)
        except (LineTooLong, ReadTimeout, SerialReadError):
            return
        if nxt is None:
            return
        if not nxt.startswith("AUD v1 "):
            continue
        if aud_utt(nxt) != utt:
            reader.unread(nxt)
            return
        if aud_mark(nxt) in ("end", "drop"):
            return


def next_pcm(reader, wait_s=UTT_WAIT_S):
    lines = []
    utt = None
    started = None
    started_utt = False
    while True:
        limit = None if started is None else started + wait_s
        try:
            line = reader.readline(limit, wait_s)
        except LineTooLong:
            return ("error", utt, "length")
        except ReadTimeout:
            return ("error", utt, "timeout")
        except SerialReadError:
            if lines or started_utt:
                return ("error", utt, "read")
            raise
        if line is None:
            if lines or started_utt:
                return ("error", utt, "truncated")
            return None
        if not line.startswith("AUD v1 "):
            continue
        new_utt = aud_utt(line)
        if started_utt and new_utt != utt:
            reader.unread(line)
            return ("error", utt, "end")
        if not started_utt:
            started = time.monotonic()
            utt = new_utt
            started_utt = True
        if len(lines) >= MAX_UTT_LINES:
            drain_same(reader, utt, started, wait_s, line)
            return ("error", utt, "overflow")
        lines.append(line)
        if aud_mark(line) in ("end", "drop"):
            try:
                return ("pcm", utt, assemble(lines))
            except AudError as exc:
                return ("error", utt, str(exc))


def show_face(script):
    global face_runs
    face_runs += 1
    proc = subprocess.run([str(SIM)], cwd=ROOT, input=script.encode("utf-8"), capture_output=True, check=False)
    out = proc.stdout.decode("utf-8", "replace")
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr.decode("utf-8", "replace"))
        fail(f"sim exit {proc.returncode}")
    print(out, end="")
    return out.splitlines()


def face_prepare(utt):
    cue = f"CUE v1 {utt} prepare"
    lines = show_face(f"cue {cue}\nat 332\nend\n")
    if "frame 2 4 poteto/tilt/f02.png" not in lines:
        fail("waiting frame missing")
    return lines


def face_speak(utt, bubble):
    cue = f"CUE v1 {utt} speak {bubble}"
    lines = show_face(f"cue {cue}\nat 0\nat 332\nend\n")
    if f"bubble {bubble}" not in lines:
        fail("speak bubble missing")
    if "frame 0 4 poteto/speaking/f00.png" not in lines or "frame 2 4 poteto/speaking/f02.png" not in lines:
        fail("speaking frames missing")
    return lines


def face_error(utt):
    cue = "CUE v1 error" if utt is None else f"CUE v1 {utt} error"
    lines = show_face(f"cue {cue}\nend\n")
    if "expression idle" not in lines or "bubble error" not in lines:
        fail("error face missing")
    return lines


def cue_bytes(utt, verb, text=None):
    head = f"CUE v1 {verb}" if utt is None else f"CUE v1 {utt} {verb}"
    if text is not None:
        head = f"{head} {text}"
    return (head + "\n").encode("utf-8")


def apply_cue(link, utt, verb, text=None, observe=False):
    raw = cue_bytes(utt, verb, text)
    try:
        link.write(raw)
    except SerialWriteError:
        link.states.append(("write-failed", utt, verb))
        print(f"state write-failed {utt if utt is not None else '-'} {verb}")
        return False
    link.states.append((verb, utt, text or ""))
    shown = text or ""
    print(f"state {verb} {utt if utt is not None else '-'} {shown}".rstrip())
    if observe:
        if verb == "prepare":
            face_prepare(utt)
        elif verb == "speak":
            face_speak(utt, text)
        elif verb == "error":
            face_error(utt)
    return True


def ask_stream(cmd, question, destination, on_receipt, timeout_s, ask_env=None):
    """One request, one temp directory. Concurrent sessions do not share the files."""
    folder = tempfile.TemporaryDirectory(prefix="potetobot-ask-")
    body = Path(folder.name) / "body.txt"
    answer = Path(folder.name) / "answer.txt"
    body.write_bytes(question.encode("utf-8"))
    extra = {"POTETOBOT_DESTINATION_FILE": str(destination)}
    if ask_env:
        extra.update({key: value for key, value in ask_env.items() if value is not None})
    env = clean_env(extra)
    env.pop("POTETOBOT_PORT", None)
    proc = subprocess.Popen(
        list(cmd) + ["--body-file", str(body), "--answer-file", str(answer)],
        cwd=ROOT,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        env=env,
    )
    lines = []

    def pump():
        assert proc.stderr is not None
        while True:
            raw = proc.stderr.readline()
            if raw == b"":
                break
            lines.append(raw)

    thread = threading.Thread(target=pump, daemon=True)
    thread.start()
    deadline = time.monotonic() + timeout_s
    saw = None
    printed = 0

    def flush_new():
        nonlocal saw, printed
        while printed < len(lines):
            text = lines[printed].decode("utf-8", "replace")
            printed += 1
            sys.stderr.write(text)
            if saw is None and text.startswith("request "):
                saw = text.split(None, 1)[1].strip()
                on_receipt(saw)

    try:
        while thread.is_alive():
            thread.join(0.05)
            flush_new()
            if time.monotonic() >= deadline:
                proc.kill()
                thread.join(1)
                proc.wait()
                return ("timeout", saw, None)
        thread.join(1)
        flush_new()
        rc = proc.wait()
        if rc != 0 or saw is None:
            return ("ask", saw, None)
        if not answer.exists():
            return ("empty", saw, None)
        text = answer.read_bytes().decode("utf-8")
        if text.strip() == "":
            return ("empty", saw, None)
        return ("ok", saw, text)
    finally:
        folder.cleanup()


class RxPump:
    """Read the link on one thread so a blocking transcribe or request cannot stall RX.

    The queue holds complete events only. Each PCM event is already capped
    at 6.0 seconds by assemble. A full queue drops the PCM and remembers the
    utterance id as busy.
    """

    def __init__(self, link, wait_s, max_queued, on_reject):
        self.reader = LineReader(link)
        self.wait_s = wait_s
        self.max_queued = max_queued
        self.on_reject = on_reject
        self.events = queue.Queue(maxsize=max_queued)
        self.busy = []
        self.lock = threading.Lock()
        self.read_error = None
        self.thread = threading.Thread(target=self._run, name="poteto-rx", daemon=True)

    def start(self):
        self.thread.start()

    def _run(self):
        try:
            while True:
                event = next_pcm(self.reader, self.wait_s)
                if event is None:
                    return
                self._keep(event)
        except SerialReadError as exc:
            self.read_error = exc
        except Exception as exc:
            self.read_error = exc

    def _keep(self, event):
        _kind, utt, payload = event
        try:
            self.events.put_nowait(event)
            return
        except queue.Full:
            pass
        del payload
        with self.lock:
            if len(self.busy) < self.max_queued:
                self.busy.append(utt)
        if self.on_reject is not None:
            self.on_reject(utt)

    def take(self):
        while True:
            try:
                return self.events.get_nowait()
            except queue.Empty:
                pass
            with self.lock:
                if self.busy and self.events.empty():
                    return ("error", self.busy.pop(0), "busy")
            if not self.thread.is_alive():
                with self.lock:
                    if self.busy:
                        return ("error", self.busy.pop(0), "busy")
                return None
            time.sleep(0.01)


def run_session(link, destination, ask_cmd, transcribe_fn, observe=False, ask_timeout=120, wait_s=UTT_WAIT_S, max_queued=MAX_QUEUED_UTTERANCES, on_reject=None, should_stop=None, ask_env=None):
    pump = RxPump(link, wait_s, max_queued, on_reject)
    pump.start()
    ok = 0
    errors = 0
    stopped = False
    while True:
        if should_stop is not None and should_stop():
            stopped = True
            break
        event = pump.take()
        if event is None:
            if should_stop is not None and should_stop():
                stopped = True
            elif pump.read_error is not None:
                print(f"link read {pump.read_error}")
            break
        if should_stop is not None and should_stop():
            stopped = True
            break
        kind, utt, payload = event
        if kind != "pcm":
            errors += 1
            print(f"audio {payload} {utt if utt is not None else '-'}")
            apply_cue(link, utt, "error", observe=observe)
            continue
        pcm = payload
        print(f"pcm_bytes {len(pcm)}")
        try:
            question = transcribe_fn(pcm)
        except Exception as exc:
            errors += 1
            print(f"audio transcript {exc}")
            apply_cue(link, utt, "error", observe=observe)
            continue
        print(f"transcript {question}")
        if question == "":
            errors += 1
            print(f"audio empty {utt}")
            apply_cue(link, utt, "error", observe=observe)
            continue
        if should_stop is not None and should_stop():
            stopped = True
            break
        order = []

        def on_receipt(request_id, _utt=utt, _order=order):
            if should_stop is not None and should_stop():
                return
            if apply_cue(link, _utt, "prepare", observe=observe):
                _order.append(("wait", request_id))
                print(f"wait {request_id}")

        status, _request, answer = ask_stream(ask_cmd, question, destination, on_receipt, ask_timeout, ask_env)
        if should_stop is not None and should_stop():
            stopped = True
            break
        if status != "ok":
            errors += 1
            print(f"audio {status} {utt}")
            apply_cue(link, utt, "error", observe=observe)
            continue
        bubble = excerpt(answer)
        if bubble == "":
            errors += 1
            print(f"audio bubble {utt}")
            apply_cue(link, utt, "error", observe=observe)
            continue
        if not apply_cue(link, utt, "speak", bubble, observe=observe):
            errors += 1
            apply_cue(link, utt, "error", observe=observe)
            continue
        order.append(("speak", bubble))
        print(f"answer {answer}")
        print(f"bubble {bubble}")
        if observe and order != [("wait", order[0][1]), ("speak", bubble)]:
            fail("cue order")
        if cue_bytes(utt, "speak", bubble) not in bytes(link.sent):
            fail("board cue mismatch")
        ok += 1
    pump.thread.join(2)
    try:
        link.session_read_error = None if stopped else pump.read_error
    except AttributeError:
        pass
    if stopped:
        return ok, errors
    if pump.read_error is not None and ok == 0 and errors == 0:
        fail(f"link read {pump.read_error}")
    if ok == 0 and errors == 0:
        fail("no utterance")
    return ok, errors


class SerialLink:
    """One Win32 COM handle. A timeout read is empty and is not end of stream."""

    eof_on_empty = False
    read_size = 512

    def __init__(self, kernel, handle):
        self.kernel = kernel
        self.handle = handle
        self.sent = bytearray()
        self.states = []

    def read(self, n):
        import ctypes
        from ctypes import wintypes

        buf = ctypes.create_string_buffer(n)
        got = wintypes.DWORD(0)
        ok = self.kernel.ReadFile(self.handle, buf, n, ctypes.byref(got), None)
        if not ok:
            raise SerialReadError("read failed")
        if got.value == 0:
            return b""
        if got.value > n:
            raise SerialReadError("read length")
        return buf.raw[: got.value]

    def write(self, data):
        import ctypes
        from ctypes import wintypes

        view = bytes(data)
        offset = 0
        while offset < len(view):
            got = wintypes.DWORD(0)
            ok = self.kernel.WriteFile(self.handle, view[offset:], len(view) - offset, ctypes.byref(got), None)
            if not ok or got.value == 0 or got.value > len(view) - offset:
                raise SerialWriteError("write failed")
            offset += got.value
        self.sent += view

    def close(self):
        if self.handle is None:
            return
        self.kernel.CloseHandle(self.handle)
        self.handle = None


def open_serial(name, kernel=None):
    if name is None or name == "":
        fail("POTETOBOT_PORT is empty")
    import ctypes
    from ctypes import wintypes

    class DCB(ctypes.Structure):
        _fields_ = [
            ("DCBlength", wintypes.DWORD),
            ("BaudRate", wintypes.DWORD),
            ("flags", wintypes.DWORD),
            ("wReserved", wintypes.WORD),
            ("XonLim", wintypes.WORD),
            ("XoffLim", wintypes.WORD),
            ("ByteSize", wintypes.BYTE),
            ("Parity", wintypes.BYTE),
            ("StopBits", wintypes.BYTE),
            ("XonChar", ctypes.c_char),
            ("XoffChar", ctypes.c_char),
            ("ErrorChar", ctypes.c_char),
            ("EofChar", ctypes.c_char),
            ("EvtChar", ctypes.c_char),
            ("wReserved1", wintypes.WORD),
        ]

    class COMMTIMEOUTS(ctypes.Structure):
        _fields_ = [
            ("ReadIntervalTimeout", wintypes.DWORD),
            ("ReadTotalTimeoutMultiplier", wintypes.DWORD),
            ("ReadTotalTimeoutConstant", wintypes.DWORD),
            ("WriteTotalTimeoutMultiplier", wintypes.DWORD),
            ("WriteTotalTimeoutConstant", wintypes.DWORD),
        ]

    if kernel is None:
        kernel = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel.CreateFileA.argtypes = [
            ctypes.c_char_p,
            wintypes.DWORD,
            wintypes.DWORD,
            ctypes.c_void_p,
            wintypes.DWORD,
            wintypes.DWORD,
            wintypes.HANDLE,
        ]
        kernel.CreateFileA.restype = wintypes.HANDLE
        kernel.GetCommState.argtypes = [wintypes.HANDLE, ctypes.POINTER(DCB)]
        kernel.GetCommState.restype = wintypes.BOOL
        kernel.SetCommState.argtypes = [wintypes.HANDLE, ctypes.POINTER(DCB)]
        kernel.SetCommState.restype = wintypes.BOOL
        kernel.SetCommTimeouts.argtypes = [wintypes.HANDLE, ctypes.POINTER(COMMTIMEOUTS)]
        kernel.SetCommTimeouts.restype = wintypes.BOOL
        kernel.ReadFile.argtypes = [
            wintypes.HANDLE,
            ctypes.c_void_p,
            wintypes.DWORD,
            ctypes.POINTER(wintypes.DWORD),
            ctypes.c_void_p,
        ]
        kernel.ReadFile.restype = wintypes.BOOL
        kernel.WriteFile.argtypes = [
            wintypes.HANDLE,
            ctypes.c_void_p,
            wintypes.DWORD,
            ctypes.POINTER(wintypes.DWORD),
            ctypes.c_void_p,
        ]
        kernel.WriteFile.restype = wintypes.BOOL
        kernel.CloseHandle.argtypes = [wintypes.HANDLE]
        kernel.CloseHandle.restype = wintypes.BOOL
    path = ("\\\\.\\" + name).encode("ascii")
    handle = kernel.CreateFileA(path, 0x80000000 | 0x40000000, 0, None, 3, 0, None)
    invalid = wintypes.HANDLE(-1).value
    if handle == invalid or handle is None:
        fail(f"serial open failed {name}")

    def close_and_fail(message):
        kernel.CloseHandle(handle)
        fail(message)

    dcb = DCB()
    dcb.DCBlength = ctypes.sizeof(DCB)
    if not kernel.GetCommState(handle, ctypes.byref(dcb)):
        close_and_fail(f"serial state failed {name}")
    dcb.BaudRate = 115200
    dcb.ByteSize = 8
    dcb.Parity = 0
    dcb.StopBits = 0
    dcb.flags = 1
    if not kernel.SetCommState(handle, ctypes.byref(dcb)):
        close_and_fail(f"serial state failed {name}")
    timeouts = COMMTIMEOUTS(50, 0, 400, 0, 400)
    if not kernel.SetCommTimeouts(handle, ctypes.byref(timeouts)):
        close_and_fail(f"serial timeout failed {name}")
    return SerialLink(kernel, handle)


class KernelMock:
    """Injected Win32 stand-in. Tests pass this instead of kernel32."""

    def __init__(self):
        self.pending = b""
        self.write_plan = []
        self.written = []
        self.closed = []
        self.read_fails_after = True
        self.fail_read = False
        self.get_state = True
        self.set_state = True
        self.set_timeouts = True
        self.handle = 42
        self.created = 0
        self.read_calls = 0
        self.lock = threading.Lock()

    def CreateFileA(self, *_args):
        self.created += 1
        return self.handle

    def CloseHandle(self, handle):
        self.closed.append(handle)
        return 1

    def GetCommState(self, _handle, _dcb):
        return 1 if self.get_state else 0

    def SetCommState(self, _handle, _dcb):
        return 1 if self.set_state else 0

    def SetCommTimeouts(self, _handle, _timeouts):
        return 1 if self.set_timeouts else 0

    def ReadFile(self, _handle, buf, n, got, _overlapped):
        with self.lock:
            self.read_calls += 1
            if self.fail_read:
                got._obj.value = 0
                return 0
            if not self.pending:
                got._obj.value = 0
                return 0 if self.read_fails_after else 1
            take = self.pending[:n]
            self.pending = self.pending[n:]
            buf[: len(take)] = take
            got._obj.value = len(take)
            return 1

    def WriteFile(self, _handle, data, n, got, _overlapped):
        with self.lock:
            raw = bytes(data[:n])
            if not self.write_plan:
                got._obj.value = n
                self.written.append(raw)
                return 1
            step = self.write_plan.pop(0)
            if step[0] == "fail":
                got._obj.value = 0
                return 0
            if step[0] == "zero":
                got._obj.value = 0
                return 1
            take_n = n if step[0] == "full" else min(step[1], n)
            if take_n <= 0:
                got._obj.value = 0
                return 0
            got._obj.value = take_n
            self.written.append(raw[:take_n])
            return 1


class BoundedKernel:
    """Finite RX buffer in front of ReadFile. Bytes that do not fit are dropped.

    This is not InjectLink: InjectLink keeps the whole stream in memory.
    offer() is the virtual wire. ReadFile is the same call SerialLink uses.
    """

    def __init__(self, cap):
        self.cap = cap
        self.buf = bytearray()
        self.dropped = 0
        self.closed = False
        self.handle = 42
        self.cv = threading.Condition()

    def offer(self, data):
        with self.cv:
            room = self.cap - len(self.buf)
            keep = data[: room if room > 0 else 0]
            if keep:
                self.buf += keep
            self.dropped += len(data) - len(keep)
            self.cv.notify()

    def finish(self):
        with self.cv:
            self.closed = True
            self.cv.notify_all()

    def ReadFile(self, _handle, buf, n, got, _overlapped):
        with self.cv:
            while not self.buf and not self.closed:
                self.cv.wait(0.02)
            if self.buf:
                take = bytes(self.buf[:n])
                del self.buf[: len(take)]
                buf[: len(take)] = take
                got._obj.value = len(take)
                return 1
            got._obj.value = 0
            return 0

    def WriteFile(self, _handle, _data, n, got, _overlapped):
        got._obj.value = n
        return 1

    def CloseHandle(self, _handle):
        self.finish()
        return 1


def expect_audio_error(lines, reason):
    try:
        assemble(lines)
    except AudError as exc:
        if str(exc) != reason:
            fail(f"audio error {exc} wanted {reason}")
        return
    fail(f"audio accepted {reason}")


def retag(pcm, utt):
    text = aud_stream(pcm).decode("ascii")
    out = []
    for line in text.splitlines():
        parts = line.split(" ")
        if len(parts) < 3 or parts[0] != "AUD" or parts[1] != "v1":
            fail(f"retag {line}")
        parts[2] = str(utt)
        out.append(" ".join(parts))
    return ("\n".join(out) + "\n").encode("ascii")


def expressions(lines):
    return [line.split(" ", 1)[1] for line in lines if line.startswith("expression ")]


def self_test():
    run_compile(AUD_BUILD)
    run_compile(MATCH_BUILD)
    run_compile(ASK_BUILD)
    matched = subprocess.run([str(MATCH)], cwd=ROOT, capture_output=True, check=False)
    sys.stdout.write(matched.stdout.decode("utf-8", "replace"))
    if matched.returncode != 0:
        sys.stderr.write(matched.stderr.decode("utf-8", "replace"))
        fail("channel match failed")
    pcm = bytes([i % 256 for i in range(CHUNK * 2 + 10)])
    lines = format_pcm(pcm)
    if assemble(lines) != pcm:
        fail("reassembly mismatch")
    mixed = []
    for line in lines:
        mixed.append("ACK poteto listening")
        mixed.append("I (10) key3: es7210 open")
        mixed.append(line)
    if assemble(mixed) != pcm:
        fail("log line changed the pcm")
    expect_audio_error(lines[:-1], "truncated")
    bad = list(lines)
    bad[1] = bad[0]
    expect_audio_error(bad, "sequence")
    short = lines[0][:-2]
    expect_audio_error([short], "length")
    expect_audio_error(["AUD v1 nope"], "malformed")
    expect_audio_error(["AUD v1 1 drop overflow"], "overflow")
    huge = b"\x00" * (MAX_BYTES + 2)
    expect_audio_error(format_pcm(huge, allow_drop=True), "overflow")
    gate = Bind()
    if not gate.start(1) or gate.start(2) or not gate.finish(1) or gate.finish(1):
        fail("bind")
    print("bind ok")
    empty = BUILD / "empty-dest.json"
    grok = BUILD / "grok-dest.json"
    session_dest = BUILD / "session-dest.json"
    empty.write_text('{"provider":"","botId":""}\n', encoding="utf-8")
    grok.write_text('{"provider":"grok_bot","botId":"Poteto"}\n', encoding="utf-8")
    session_dest.write_text('{"provider":"orca_bots","botId":"Poteto"}\n', encoding="utf-8")
    body = BUILD / "ask-body.txt"
    answer = BUILD / "ask-answer.txt"
    body.write_text("ping\n", encoding="utf-8")
    for dest, needle in ((empty, "not_sent"), (grok, "blocked")):
        if answer.exists():
            answer.unlink()
        proc = subprocess.run(
            [str(ASK), "--body-file", str(body), "--answer-file", str(answer)],
            cwd=ROOT,
            capture_output=True,
            check=False,
            env=clean_env({"POTETOBOT_DESTINATION_FILE": str(dest)}),
        )
        err = proc.stderr.decode("utf-8", "replace")
        if proc.returncode == 0 or needle not in err or "request msg_" in err:
            fail(f"destination {dest.name} sent or hid {needle}")
    try:
        open_serial("")
    except SystemExit as exc:
        if str(exc) != "POTETOBOT_PORT is empty":
            raise
    else:
        fail("empty port opened")
    drop_reader = LineReader(InjectLink(b"AUD v1 1 drop queue\nACK poteto idle\n"))
    drop_event = next_pcm(drop_reader)
    if drop_event != ("error", 1, "queue") or next_pcm(drop_reader) is not None:
        fail(f"drop recovery {drop_event}")
    print("drop ok")
    stall = InjectLink(b"AUD v1 4 0 2 0102", read_size=64)
    stall.eof_on_empty = False
    started = time.monotonic()
    stalled = next_pcm(LineReader(stall), 0.2)
    if time.monotonic() - started > 2 or stalled != ("error", None, "timeout"):
        fail(f"partial wait {stalled}")
    print("wait cap ok")
    long_reader = LineReader(InjectLink(b"X" * 3000 + b"\nAUD v1 8 drop queue\n", read_size=64))
    if next_pcm(long_reader) != ("error", None, "length"):
        fail("long line")
    if next_pcm(long_reader) != ("error", 8, "queue"):
        fail("long line ate the next utterance")
    print("length cap ok")
    overflow = "".join(f"AUD v1 9 {i} 2 ab\n" for i in range(MAX_UTT_LINES + 1))
    overflow += "AUD v1 10 drop queue\n"
    overflow_reader = LineReader(InjectLink(overflow.encode("ascii"), read_size=64))
    if next_pcm(overflow_reader) != ("error", 9, "overflow"):
        fail("overflow cap")
    if next_pcm(overflow_reader) != ("error", 10, "queue"):
        fail("overflow ate the next utterance")
    print("buffer cap ok")
    import ctypes
    from ctypes import wintypes

    invalid = wintypes.HANDLE(-1).value
    refused = KernelMock()
    refused.handle = invalid
    try:
        open_serial("COM5", kernel=refused)
    except SystemExit as exc:
        if "serial open failed" not in str(exc):
            raise
    else:
        fail("invalid handle opened")
    if refused.closed:
        fail("closed an invalid handle")
    state_fail = KernelMock()
    state_fail.get_state = False
    try:
        open_serial("COM5", kernel=state_fail)
    except SystemExit as exc:
        if "serial state failed" not in str(exc):
            raise
    else:
        fail("GetCommState failure stayed open")
    if state_fail.closed != [42]:
        fail(f"state handle {state_fail.closed}")
    timeout_fail = KernelMock()
    timeout_fail.set_timeouts = False
    try:
        open_serial("COM5", kernel=timeout_fail)
    except SystemExit as exc:
        if "serial timeout failed" not in str(exc):
            raise
    else:
        fail("timeout failure stayed open")
    if timeout_fail.closed != [42]:
        fail(f"timeout handle {timeout_fail.closed}")
    zero = KernelMock()
    zero.write_plan = [("zero",)]
    zero_link = SerialLink(zero, 42)
    try:
        zero_link.write(b"CUE v1 1 speak old\n")
        fail("zero write counted as sent")
    except SerialWriteError:
        pass
    if bytes(zero_link.sent):
        fail("sent recorded a zero write")
    failed = KernelMock()
    failed.write_plan = [("fail",)]
    failed_link = SerialLink(failed, 42)
    try:
        failed_link.write(b"CUE v1 1 speak old\n")
        fail("failed write counted as sent")
    except SerialWriteError:
        pass
    if bytes(failed_link.sent):
        fail("sent recorded a failed write")
    partial = KernelMock()
    partial_data = b"CUE v1 1 prepare\n"
    partial.write_plan = [("short", 4), ("full",)]
    partial_link = SerialLink(partial, 42)
    partial_link.write(partial_data)
    if bytes(partial_link.sent) != partial_data or partial.written != [partial_data[:4], partial_data[4:]]:
        fail(f"partial write {partial.written} {partial_link.sent}")
    read_fail = KernelMock()
    read_fail.fail_read = True
    try:
        SerialLink(read_fail, 42).read(8)
        fail("read failure looked like a timeout")
    except SerialReadError:
        pass
    try:
        LineReader(SerialLink(read_fail, 42)).readline()
        fail("readline spun on read failure")
    except SerialReadError:
        pass
    if read_fail.read_calls != 2:
        fail(f"read calls {read_fail.read_calls}")
    quiet = KernelMock()
    quiet.read_fails_after = False
    if SerialLink(quiet, 42).read(8) != b"":
        fail("timeout read was not empty")
    print("win32 mock ok")
    run_compile(SIM_BUILD)
    stale = show_face(
        "btn 0 0102\n"
        "btn 1 -\n"
        "btn 0 0506\n"
        "cue CUE v1 speak old\n"
        "cue CUE v1 1 speak old\n"
        "btn 1 -\n"
        "cue CUE v1 2 prepare\n"
        "at 332\n"
        "cue CUE v1 2 speak next\n"
        "end\n"
    )
    if "bubble old" in stale or expressions(stale) != [
        "listening",
        "thinking",
        "listening",
        "listening",
        "listening",
        "thinking",
        "tilt",
        "tilt",
        "speaking",
    ]:
        fail(f"stale cue {expressions(stale)}")
    if "frame 2 4 poteto/tilt/f02.png" not in stale or "bubble next" not in stale:
        fail("stale cue did not recover")
    kept = show_face(
        "btn 0 0102\nbtn 1 -\ncue CUE v1 prepare\nat 332\ncue CUE v1 speak kept\nend\n"
    )
    if expressions(kept) != ["listening", "thinking", "tilt", "tilt", "speaking"] or "bubble kept" not in kept:
        fail("single press untagged cue")
    print("cue bind ok")
    stub = BUILD / "stub_ask.py"
    stub.write_text(
        "import os, subprocess, sys, time\n"
        f"ask = {str(ASK)!r}\n"
        f"empty = {str(empty)!r}\n"
        "args = sys.argv\n"
        "body = args[args.index('--body-file') + 1]\n"
        "answer = args[args.index('--answer-file') + 1]\n"
        "text = open(body, 'rb').read().decode('utf-8').strip()\n"
        "if text == '\\ube48':\n"
        "    env = os.environ.copy()\n"
        "    env['POTETOBOT_DESTINATION_FILE'] = empty\n"
        "    proc = subprocess.run([ask, '--body-file', body, '--answer-file', answer], stderr=subprocess.PIPE, env=env)\n"
        "    sys.stderr.buffer.write(proc.stderr)\n"
        "    raise SystemExit(proc.returncode if proc.returncode != 0 else 2)\n"
        "if text == '\\ub290\\ub9bc':\n"
        "    time.sleep(30)\n"
        "    raise SystemExit(3)\n"
        "sys.stderr.buffer.write(b'request msg_local\\n')\n"
        "sys.stderr.buffer.flush()\n"
        "time.sleep(0.05)\n"
        "open(answer, 'wb').write(text.encode('utf-8'))\n",
        encoding="utf-8",
    )

    def named(blob):
        if blob == bytes([1, 2, 3, 4]):
            return "하나"
        if blob == bytes([5, 6] * 40):
            return "둘"
        if blob == bytes([7, 8]):
            return "빈"
        if blob == bytes([9, 10]):
            return "느림"
        if blob == bytes([11, 12]):
            return "다시"
        return ""

    session_blob = b"X" * 3000 + b"\n"
    session_blob += b"AUD v1 1 drop queue\nACK poteto idle\n"
    session_blob += b"AUD v1 nope\n"
    session_blob += b"AUD v1 2 0 2 aabb\n"
    session_blob += retag(bytes([1, 2, 3, 4]), 3)
    session_blob += retag(bytes([5, 6] * 40), 4)
    session_blob += retag(bytes([7, 8]), 5)
    session_blob += retag(bytes([9, 10]), 6)
    session_blob += retag(bytes([11, 12]), 7)
    link = InjectLink(session_blob, read_size=64)
    began = time.monotonic()
    ok, errors = run_session(
        link,
        session_dest,
        [sys.executable, str(stub)],
        named,
        observe=True,
        ask_timeout=2,
    )
    elapsed = time.monotonic() - began
    expect = (
        "CUE v1 error\n"
        "CUE v1 1 error\n"
        "CUE v1 error\n"
        "CUE v1 2 error\n"
        "CUE v1 3 prepare\n"
        "CUE v1 3 speak 하나\n"
        "CUE v1 4 prepare\n"
        "CUE v1 4 speak 둘\n"
        "CUE v1 5 error\n"
        "CUE v1 6 error\n"
        "CUE v1 7 prepare\n"
        "CUE v1 7 speak 다시\n"
    )
    sent = bytes(link.sent).decode("utf-8")
    if ok != 3 or errors != 6 or sent != expect or elapsed >= 15:
        fail(f"session {ok} {errors} {elapsed:.2f} {sent}")
    print("session ok")
    faces = face_runs
    kernel = KernelMock()
    kernel.pending = retag(bytes([1, 2, 3, 4]), 3) + retag(bytes([5, 6] * 40), 4)
    kernel.write_plan = [("short", 4), ("full",), ("fail",), ("full",), ("full",), ("full",)]
    serial = SerialLink(kernel, 42)
    ok, errors = run_session(
        serial,
        session_dest,
        [sys.executable, str(stub)],
        named,
        observe=False,
        ask_timeout=2,
    )
    serial_sent = bytes(serial.sent).decode("utf-8")
    serial_expect = "CUE v1 3 prepare\nCUE v1 3 error\nCUE v1 4 prepare\nCUE v1 4 speak 둘\n"
    if face_runs != faces:
        fail("serial session launched the face exe")
    if ok != 1 or errors != 1 or serial_sent != serial_expect or kernel.write_plan:
        fail(f"serial session {ok} {errors} {serial_sent} plan {kernel.write_plan}")
    if kernel.written[0] != b"CUE " or b"speak \xed\x95\x98\xeb\x82\x98" in bytes(serial.sent):
        fail(f"partial serial {kernel.written}")
    if kernel.read_calls < 2:
        fail("serial read did not reach the failure")
    serial.close()
    if serial.handle is not None or kernel.closed != [42]:
        fail("serial close")
    print("serial mock ok")

    second = bytes([9]) * 16000
    second_frames = retag(second, 2)
    cold = BoundedKernel(8192)
    cold.offer(second_frames)
    if len(second_frames) <= 8192 or cold.dropped != len(second_frames) - 8192:
        fail(f"finite fixture {len(second_frames)} {cold.dropped}")
    print(f"finite fixture {len(second_frames)} undrained {cold.dropped}")
    finite = BoundedKernel(8192)
    first = bytes([1, 2, 3, 4])
    finite.offer(retag(first, 1))
    if finite.dropped:
        fail("finite preload dropped")
    entered = threading.Event()
    release = threading.Event()
    box = {}

    def transcribe_finite(pcm):
        if pcm == first:
            entered.set()
            if not release.wait(5):
                fail(box.get("err", "finite stream"))
            if "err" in box:
                fail(box["err"])
            return "하나"
        if pcm == second:
            return "둘"
        return ""

    def produce_finite():
        try:
            if not entered.wait(5):
                fail("finite not in transcribe")
            for offset in range(0, len(second_frames), 512):
                finite.offer(second_frames[offset : offset + 512])
                time.sleep(0.002)
            if finite.dropped != 0:
                fail(f"finite drained drop {finite.dropped}")
            finite.finish()
        except SystemExit as exc:
            box["err"] = str(exc)
        finally:
            release.set()

    threading.Thread(target=produce_finite, name="finite-tx", daemon=True).start()
    finite_link = SerialLink(finite, 42)
    finite_ok, finite_errors = run_session(
        finite_link,
        session_dest,
        [sys.executable, str(stub)],
        transcribe_finite,
        observe=False,
        ask_timeout=2,
    )
    finite_sent = bytes(finite_link.sent).decode("utf-8")
    if finite_ok != 2 or finite_errors != 0 or finite.dropped != 0:
        fail(f"finite rx {finite_ok} {finite_errors} drop {finite.dropped} {finite_sent}")
    if "CUE v1 1 speak 하나\n" not in finite_sent or "CUE v1 2 speak 둘\n" not in finite_sent:
        fail(f"finite cues {finite_sent}")
    print("finite rx ok")

    pieces = {1: bytes([1, 1]), 2: bytes([2, 2]), 3: bytes([3, 3]), 4: bytes([4, 4])}
    busy_answers = {1: "첫번째", 2: "둘째", 4: "회복"}
    busy_port = BoundedKernel(65536)
    busy_port.offer(retag(pieces[1], 1))
    busy_entered = threading.Event()
    busy_rejected = threading.Event()
    busy_release = threading.Event()
    busy_box = {"err": ""}

    def reject_busy(utt):
        if utt == 3:
            busy_rejected.set()

    def transcribe_busy(pcm):
        if pcm == pieces[1]:
            busy_entered.set()
            if not busy_release.wait(5):
                fail(busy_box["err"] or "busy stream")
            if busy_box["err"]:
                fail(busy_box["err"])
            return busy_answers[1]
        for utt, blob in pieces.items():
            if pcm == blob and utt in busy_answers:
                return busy_answers[utt]
        return ""

    busy_link = SerialLink(busy_port, 42)

    def produce_busy():
        try:
            if not busy_entered.wait(5):
                fail("busy not in transcribe")
            busy_port.offer(retag(pieces[2], 2))
            busy_port.offer(retag(pieces[3], 3))
            if not busy_rejected.wait(5):
                fail("busy was queued")
            busy_release.set()
            deadline = time.monotonic() + 5
            while b"CUE v1 3 error\n" not in bytes(busy_link.sent):
                if time.monotonic() > deadline:
                    fail("busy cue missing")
                time.sleep(0.01)
            busy_port.offer(retag(pieces[4], 4))
            busy_port.finish()
        except SystemExit as exc:
            busy_box["err"] = str(exc)
            busy_port.finish()
            busy_release.set()

    threading.Thread(target=produce_busy, name="busy-tx", daemon=True).start()
    busy_ok, busy_errors = run_session(
        busy_link,
        session_dest,
        [sys.executable, str(stub)],
        transcribe_busy,
        observe=False,
        ask_timeout=2,
        max_queued=1,
        on_reject=reject_busy,
    )
    busy_sent = bytes(busy_link.sent).decode("utf-8")
    if busy_ok != 3 or busy_errors != 1:
        fail(f"busy cap {busy_ok} {busy_errors} {busy_sent}")
    for needle in ("CUE v1 2 speak 둘째\n", "CUE v1 3 error\n", "CUE v1 4 speak 회복\n"):
        if needle not in busy_sent:
            fail(f"busy cues {busy_sent}")
    print("busy cap ok")
    print("self-test ok")


def run_injected(wav_path, destination):
    run_compile(AUD_BUILD)
    run_compile(ASK_BUILD)
    run_compile(SIM_BUILD)
    pcm = load_wav(wav_path)
    link = InjectLink(aud_stream(pcm))
    run_session(link, destination, [str(ASK)], transcribe, observe=True)
    print("inject ok")


def transcribe_for(language):
    """None keeps transcribe()'s Korean default. An explicit value is pinned."""
    if language is None:
        return transcribe
    if language not in STT_LANGUAGES:
        fail("stt language")

    def bound(pcm, _language=language):
        return transcribe(pcm, language=_language)

    return bound


def flag_value(argv, name):
    if name not in argv:
        return None
    index = argv.index(name)
    if index + 1 >= len(argv):
        fail(f"{name} missing")
    return argv[index + 1]


def main(argv):
    if "--self-test" in argv:
        self_test()
        return 0
    if "--transport" not in argv or "--destination" not in argv:
        fail("use --self-test or --transport sim|serial --destination <file>")
    transport = argv[argv.index("--transport") + 1]
    destination = Path(argv[argv.index("--destination") + 1])
    if not destination.is_file():
        fail("destination missing")
    transcribe_fn = transcribe_for(flag_value(argv, "--stt-language"))
    if transport == "serial" and "--inject" not in argv:
        run_compile(AUD_BUILD)
        run_compile(ASK_BUILD)
        link = open_serial(os.environ.get("POTETOBOT_PORT", ""))
        try:
            run_session(link, destination, [str(ASK)], transcribe_fn, observe=False)
        finally:
            link.close()
        if getattr(link, "session_read_error", None):
            return 1
        return 0
    if transport not in ("sim", "serial") or "--wav" not in argv:
        fail("unknown transport")
    wav_path = Path(argv[argv.index("--wav") + 1])
    if not wav_path.is_file():
        fail("wav missing")
    run_compile(AUD_BUILD)
    run_compile(ASK_BUILD)
    run_compile(SIM_BUILD)
    pcm = load_wav(wav_path)
    link = InjectLink(aud_stream(pcm))
    run_session(link, destination, [str(ASK)], transcribe_fn, observe=True)
    print("inject ok")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
