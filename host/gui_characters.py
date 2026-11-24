"""Character catalog from the firmware table and the asset folders.

Board support is the count in firmware/main/frame_table.c. A PNG on disk
is a preview only. A folder that is not in the table is a preview profile.
This module does not write frames into the firmware and does not send SEL.
"""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TABLE = ROOT / "firmware" / "main" / "frame_table.c"
ASSETS = ROOT / "assets" / "frames"
VOICE_FACE = "poteto"
ID_RE = re.compile(r"^[a-z][a-z0-9_]{0,31}$")
LOOP_RE = re.compile(r'\{\s*"([a-z][a-z0-9_]*)"\s*,\s*"([a-z]+)"\s*,\s*(\d+)\s*,')
FRAME_RE = re.compile(r"^f\d+\.png$")

NAMES = {
    "poteto": {"ko": "포테토", "en": "Poteto"},
    "sunny": {"ko": "써니", "en": "Sunny"},
    "mint": {"ko": "민트", "en": "Mint"},
}


def valid_id(value):
    return bool(ID_RE.fullmatch(str(value or "")))


def board_loops(text=None):
    body = TABLE.read_text(encoding="utf-8") if text is None else text
    loops = []
    for match in LOOP_RE.finditer(body):
        loops.append(
            {
                "character": match.group(1),
                "state": match.group(2),
                "boardFrames": int(match.group(3)),
            }
        )
    if not loops:
        raise ValueError("frame table empty")
    return loops


def asset_frames(character, state):
    folder = ASSETS / character / state
    if not folder.is_dir():
        return []
    names = [path.name for path in folder.iterdir() if path.is_file() and FRAME_RE.match(path.name)]
    names.sort()
    return [f"{character}/{state}/{name}" for name in names]


def _state_row(character, state, board_frames):
    frames = asset_frames(character, state)
    return {
        "id": state,
        "boardFrames": board_frames,
        "boardCompatible": board_frames > 0,
        "assetFrames": frames,
    }


def _asset_states(character, known):
    folder = ASSETS / character
    rows = []
    if not folder.is_dir():
        return rows
    for state_dir in sorted(path for path in folder.iterdir() if path.is_dir()):
        if not state_dir.name.isalpha() or state_dir.name in known:
            continue
        frames = asset_frames(character, state_dir.name)
        if not frames:
            continue
        rows.append(_state_row(character, state_dir.name, 0))
    return rows


def _note(character, on_board):
    if character == VOICE_FACE:
        return {
            "ko": "음성 큐는 이 얼굴을 쓴다. listening, thinking, prepare의 tilt, speaking, error의 idle. 카드 선택은 미리보기이고 SEL을 보내지 않는다.",
            "en": "Voice cues use this face: listening, thinking, tilt on prepare, speaking, idle on error. The card is a preview and does not send SEL.",
        }
    if on_board:
        return {
            "ko": "보드는 SEL로만 이 얼굴을 바꾼다. 이 화면은 SEL을 보내지 않고, 카드는 미리보기다.",
            "en": "The board changes this face only by SEL. This page does not send SEL. The card is a preview.",
        }
    return {
        "ko": "이 프로필은 화면 미리보기만이다. 보드 표에 없고, 음성 큐와 SEL을 쓰지 않는다.",
        "en": "This profile is an on-screen preview. It is not in the board table and does not use voice cues or SEL.",
    }


def _preview(states):
    for state in states:
        if state["id"] == "idle" and state["assetFrames"]:
            return state["assetFrames"][0]
    for state in states:
        if state["assetFrames"]:
            return state["assetFrames"][0]
    return ""


def catalog():
    by_character = {}
    order = []
    for loop in board_loops():
        character = loop["character"]
        if not valid_id(character):
            continue
        if character not in by_character:
            by_character[character] = []
            order.append(character)
        known = {row["id"] for row in by_character[character]}
        if loop["state"] in known:
            continue
        by_character[character].append(_state_row(character, loop["state"], loop["boardFrames"]))
    for character in list(order):
        known = {row["id"] for row in by_character[character]}
        by_character[character].extend(_asset_states(character, known))
    if ASSETS.is_dir():
        for folder in sorted(path for path in ASSETS.iterdir() if path.is_dir()):
            if not valid_id(folder.name) or folder.name in by_character:
                continue
            states = _asset_states(folder.name, set())
            if not states:
                continue
            by_character[folder.name] = states
            order.append(folder.name)
    characters = []
    for character in order:
        states = by_character[character]
        on_board = any(state["boardFrames"] > 0 for state in states)
        label = NAMES.get(character) or {"ko": character, "en": character}
        characters.append(
            {
                "id": character,
                "name": dict(label),
                "preview": _preview(states),
                "states": states,
                "onBoard": on_board,
                "voiceCuesUseThisFace": character == VOICE_FACE,
                "selectionSendsSel": False,
                "note": _note(character, on_board),
            }
        )
    return {"characters": characters, "firmwareImageUnchanged": True}
