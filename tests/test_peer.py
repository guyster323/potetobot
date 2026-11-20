"""Drive the shipped peer entry twice and require the same loop identities."""

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PEER = ROOT / "host" / "build" / "potetobot-peer.exe"
SEQUENCE = (
    "SEL v1 poteto idle\n"
    "SEL v1 poteto speaking\n"
    "SEL v1 sunny listening\n"
    "SEL v1 sunny thinking\n"
    "SAY v1 thinking still working\n"
    "SAY v1 answer hello potato\n"
)
EXPECTED = (
    "poteto idle\n"
    "poteto speaking\n"
    "sunny listening\n"
    "sunny thinking\n"
    "say thinking\n"
    "say answer\n"
)


def run_once():
    env = os.environ.copy()
    env.pop("POTETOBOT_DRIVER", None)
    completed = subprocess.run(
        [str(PEER)],
        input=SEQUENCE,
        text=True,
        capture_output=True,
        check=False,
        cwd=ROOT,
        env=env,
    )
    if completed.returncode != 0:
        sys.stderr.write(completed.stderr)
        raise SystemExit(f"peer exit {completed.returncode}")
    return completed.stdout, completed.stderr


def main():
    if not PEER.exists():
        raise SystemExit(f"missing peer binary {PEER}")
    first_out, first_err = run_once()
    second_out, second_err = run_once()
    if first_out != second_out:
        raise SystemExit("peer transcripts differ")
    if first_out != EXPECTED:
        raise SystemExit(f"unexpected identities:\n{first_out}")
    if "driver=grok-bot" not in first_err or "signedIn=true" not in first_err:
        raise SystemExit("peer did not observe the signed-in Grok Bot app")
    if "driver=grok-bot" not in second_err:
        raise SystemExit("second run did not observe Grok Bot")
    sys.stdout.write(first_out)
    return 0


if __name__ == "__main__":
    os.environ.setdefault("PYTHONIOENCODING", "utf-8")
    raise SystemExit(main())
