"""Provider boundaries. Orca is a local list. Grok Bot is not the model API."""

import json
import os
import shutil
import subprocess
from pathlib import Path
from urllib.parse import quote

GROK_DOCS = {
    "bots": "https://docs.x.ai/grok-bot/bots",
    "deepLinks": "https://docs.x.ai/grok-bot/deep-links",
    "routines": "https://docs.x.ai/grok-bot/skills-routines-and-automations",
}

GROK_BOUNDARY = {
    "provider": "grok_bot",
    "bots": [],
    "reachable": False,
    "listApi": False,
    "roundTrip": False,
    "reason": "no_official_list_or_round_trip",
    "docs": GROK_DOCS,
    "deepLinkSends": False,
    "webhookHttp200IsAnswer": False,
    "modelApiSubstitute": False,
    "userCanPasteBotId": True,
    "note": (
        "Grok Bot docs describe creating a bot, a primary bot, and Share → Copy link in the app. "
        "They do not document an account bot list or a programmatic send-and-answer path. "
        "grokbot://app/v1/prompt opens the signed-in app and fills the box only when it is empty. "
        "It does not send. A webhook HTTP 200 means the call was accepted and a run started. "
        "It is not the bot's answer. The xAI model API is a different product and is not used here."
    ),
}


def orca_executable():
    found = shutil.which("orca")
    if found:
        return found
    local = os.environ.get("LOCALAPPDATA", "")
    candidate = Path(local) / "Programs" / "orca" / "resources" / "bin" / "orca.exe"
    if candidate.is_file():
        return str(candidate)
    return "orca"


def parse_orca_list(raw):
    data = json.loads(raw)
    if not isinstance(data, dict):
        raise ValueError("orca list shape")
    result = data.get("result")
    if isinstance(result, dict) and isinstance(result.get("bots"), list):
        bots = result["bots"]
        unreachable = result.get("unreachable") or []
    elif isinstance(data.get("bots"), list):
        bots = data["bots"]
        unreachable = data.get("unreachable") or []
    else:
        raise ValueError("orca list shape")
    out = []
    for bot in bots:
        if not isinstance(bot, dict):
            continue
        name = str(bot.get("name") or "").strip()
        if not name or any(ord(ch) < 32 or ch in '\\/"' for ch in name):
            continue
        out.append(
            {
                "id": str(bot.get("id") or ""),
                "name": name,
                "agent": str(bot.get("agent") or ""),
                "specialty": str(bot.get("specialty") or ""),
                "paused": bool(bot.get("paused")),
            }
        )
    if not isinstance(unreachable, list):
        unreachable = []
    return out, unreachable


def list_orca_bots(query=""):
    proc = subprocess.run(
        [orca_executable(), "bot", "list", "--json"],
        capture_output=True,
        check=False,
        timeout=40,
    )
    if proc.returncode != 0:
        err = proc.stderr.decode("utf-8", "replace").strip() or proc.stdout.decode("utf-8", "replace").strip()
        return {
            "provider": "orca_bots",
            "ok": False,
            "bots": [],
            "unreachable": [],
            "error": err[:500] or f"orca bot list exit {proc.returncode}",
        }
    try:
        bots, unreachable = parse_orca_list(proc.stdout.decode("utf-8"))
    except (ValueError, json.JSONDecodeError) as exc:
        return {
            "provider": "orca_bots",
            "ok": False,
            "bots": [],
            "unreachable": [],
            "error": f"orca list shape {exc}",
        }
    needle = query.strip().casefold()
    if needle:
        bots = [
            bot
            for bot in bots
            if needle in bot["name"].casefold()
            or needle in bot["id"].casefold()
            or needle in bot["specialty"].casefold()
        ]
    return {
        "provider": "orca_bots",
        "ok": True,
        "bots": bots,
        "unreachable": unreachable,
        "error": "",
    }


def grok_status():
    return dict(GROK_BOUNDARY)


def grok_prompt_link(bot_id, text):
    """App link only. Opening it does not send the message."""
    body = str(text or "")[:1000]
    ident = str(bot_id or "").strip()
    if ident and any(not (ch.isalnum() or ch in "-_") for ch in ident):
        raise ValueError("grok id characters")
    quoted = quote(body, safe="")
    if ident:
        return f"grokbot://app/v1/prompt?id={quote(ident, safe='')}&text={quoted}"
    return f"grokbot://app/v1/prompt?text={quoted}"
