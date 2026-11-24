"""Host GUI settings. This file is not the destination file."""

import json
import os
from pathlib import Path

UI_LANGUAGES = ("ko", "en")
STT_LANGUAGES = ("ko", "en")
PROVIDERS = ("orca_bots", "grok_bot")

DEFAULTS = {
    "uiLanguage": "ko",
    "sttLanguage": "ko",
    "provider": "orca_bots",
    "botId": "",
    "grokBotId": "",
    "characterId": "poteto",
}


class SettingsError(ValueError):
    pass


def settings_path():
    override = os.environ.get("POTETOBOT_GUI_FILE", "").strip()
    if override:
        return Path(override)
    appdata = os.environ.get("APPDATA", "").strip()
    if appdata:
        return Path(appdata) / "potetobot" / "gui.json"
    return Path.home() / ".potetobot" / "gui.json"


def destination_path():
    appdata = os.environ.get("APPDATA", "").strip()
    if appdata:
        return Path(appdata) / "potetobot" / "destination.json"
    return Path.home() / ".potetobot" / "destination.json"


def sender_path():
    appdata = os.environ.get("APPDATA", "").strip()
    if appdata:
        return Path(appdata) / "potetobot" / "host-terminal.json"
    return Path.home() / ".potetobot" / "host-terminal.json"


def _clean_name(value, limit):
    text = str(value or "").strip()
    if len(text) > limit:
        raise ValueError("name length")
    if any(ord(ch) < 32 or ch in '\\/"' for ch in text):
        raise ValueError("name characters")
    return text


def _clean_grok_id(value):
    text = str(value or "").strip()
    if len(text) > 80:
        raise ValueError("grok id length")
    if text and any(not (ch.isalnum() or ch in "-_") for ch in text):
        raise ValueError("grok id characters")
    return text


def _character_ids():
    import gui_characters

    return {item["id"] for item in gui_characters.catalog()["characters"]}


def normalize(raw):
    if not isinstance(raw, dict):
        raw = {}
    out = dict(DEFAULTS)
    ui = raw.get("uiLanguage", out["uiLanguage"])
    stt = raw.get("sttLanguage", out["sttLanguage"])
    provider = raw.get("provider", out["provider"])
    character = raw.get("characterId", out["characterId"])
    if ui not in UI_LANGUAGES:
        raise ValueError("ui language")
    if stt not in STT_LANGUAGES:
        raise ValueError("stt language")
    if provider not in PROVIDERS:
        raise ValueError("provider")
    if character not in _character_ids():
        raise ValueError("character")
    out["uiLanguage"] = ui
    out["sttLanguage"] = stt
    out["provider"] = provider
    out["botId"] = _clean_name(raw.get("botId", ""), 64)
    out["grokBotId"] = _clean_grok_id(raw.get("grokBotId", ""))
    out["characterId"] = character
    return out


def read_stored():
    """None when the file is absent. A present file that cannot be read raises."""
    path = settings_path()
    if not path.exists():
        return None
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise SettingsError(f"settings unreadable: {exc}") from exc
    if not isinstance(data, dict):
        raise SettingsError("settings shape")
    return data


def load():
    stored = read_stored()
    if stored is None:
        return dict(DEFAULTS)
    try:
        return normalize(stored)
    except ValueError as exc:
        raise SettingsError(str(exc)) from exc


def save(raw):
    """Replace only known fields that this call sets. Keep every other field.

    The write is a temp file in the same directory, then replace. A corrupt
    existing file is left untouched.
    """
    path = settings_path()
    if path.name.lower() == "destination.json":
        raise SettingsError("refusing destination.json")
    stored = read_stored()
    base = {} if stored is None else stored
    merged = dict(DEFAULTS)
    for key in DEFAULTS:
        if key in base:
            merged[key] = base[key]
    if isinstance(raw, dict):
        for key in DEFAULTS:
            if key in raw:
                merged[key] = raw[key]
    try:
        data = normalize(merged)
    except ValueError as exc:
        raise SettingsError(str(exc)) from exc
    payload = {key: value for key, value in base.items() if key not in DEFAULTS}
    payload.update(data)
    text = json.dumps(payload, ensure_ascii=False, indent=2) + "\n"
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + ".tmp")
    temporary.write_text(text, encoding="utf-8")
    os.replace(temporary, path)
    return data
