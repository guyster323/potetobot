"""Virtual serial session. BoundedKernel is the wire. No COM handle is opened."""

import io
import sys
import tempfile
import threading
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path

import mvp_host

ROOT = mvp_host.ROOT
BUILD = mvp_host.BUILD
STUB = BUILD / "gui_stub_ask.py"

_LOCK = threading.Lock()


class ProbeModel:
    def __init__(self):
        self.kwargs = None
        self.samples = 0

    def transcribe(self, samples, **kwargs):
        self.kwargs = dict(kwargs)
        self.samples = len(samples)
        if kwargs.get("language") == "en":
            return {"text": "virtual"}
        return {"text": "가상"}


def ensure_formatter():
    if not mvp_host.AUD.is_file():
        mvp_host.run_compile(mvp_host.AUD_BUILD)


def ensure_stub():
    BUILD.mkdir(parents=True, exist_ok=True)
    STUB.write_text(
        "import sys\n"
        "args = sys.argv\n"
        "body = args[args.index('--body-file') + 1]\n"
        "answer = args[args.index('--answer-file') + 1]\n"
        "text = open(body, 'rb').read().decode('utf-8')\n"
        "sys.stderr.buffer.write(b'request msg_gui_stub\\n')\n"
        "sys.stderr.buffer.flush()\n"
        "open(answer, 'wb').write(text.encode('utf-8'))\n",
        encoding="utf-8",
    )


def probe_transcribe(language):
    """Call the product transcribe() with a stand-in model. Whisper weights stay unloaded."""
    model = ProbeModel()
    pcm = b"\x00\x10" * 8
    text = mvp_host.transcribe(pcm, language=language, model=model)
    return {
        "text": text,
        "language": None if model.kwargs is None else model.kwargs.get("language"),
        "fp16": None if model.kwargs is None else model.kwargs.get("fp16"),
        "temperature": None if model.kwargs is None else model.kwargs.get("temperature"),
        "samples": model.samples,
        "weightsLoaded": False,
    }


def run_virtual(mode, stt_language):
    if mode not in ("speak", "error"):
        raise ValueError("mode")
    language = stt_language if stt_language in mvp_host.STT_LANGUAGES else "ko"
    ensure_stub()
    folder = tempfile.TemporaryDirectory(prefix="potetobot-virtual-")
    dest = Path(folder.name) / "session-dest.json"
    dest.write_text('{"provider":"orca_bots","botId":"Poteto"}\n', encoding="utf-8")
    kernel = mvp_host.BoundedKernel(65536)
    link = mvp_host.SerialLink(kernel, kernel.handle)
    opened_port = {"called": False}

    def refuse_open(*_args, **_kwargs):
        opened_port["called"] = True
        raise RuntimeError("physical port refused")

    previous = mvp_host.open_serial
    mvp_host.open_serial = refuse_open
    model = ProbeModel()

    def transcribe_fn(pcm, _language=language, _model=model):
        return mvp_host.transcribe(pcm, language=_language, model=_model)

    def produce():
        try:
            if mode == "error":
                kernel.offer(b"AUD v1 1 drop overflow\n")
            else:
                ensure_formatter()
                kernel.offer(mvp_host.aud_stream(bytes([1, 2, 3, 4])))
        finally:
            kernel.finish()

    out = io.StringIO()
    err = io.StringIO()
    worker = threading.Thread(target=produce, name="poteto-virtual-tx", daemon=True)
    try:
        with _LOCK:
            worker.start()
            with redirect_stdout(out), redirect_stderr(err):
                try:
                    ok, errors = mvp_host.run_session(
                        link,
                        dest,
                        [sys.executable, str(STUB)],
                        transcribe_fn,
                        observe=False,
                        ask_timeout=5,
                    )
                except SystemExit as exc:
                    ok, errors = 0, 1
                    out.write(f"session exit {exc}\n")
    finally:
        mvp_host.open_serial = previous
        kernel.finish()
        worker.join(2)
        folder.cleanup()
    cues = bytes(link.sent).decode("utf-8", "replace").splitlines()
    return {
        "mode": mode,
        "ok": ok,
        "errors": errors,
        "cues": cues,
        "transcriptLanguage": None if model.kwargs is None else model.kwargs.get("language"),
        "fp16": None if model.kwargs is None else model.kwargs.get("fp16"),
        "samples": model.samples,
        "ask": "local_stub",
        "sentToProvider": False,
        "openedPort": opened_port["called"],
        "log": out.getvalue()[-4000:],
        "transport": "BoundedKernel",
    }
