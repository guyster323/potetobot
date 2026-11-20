"""Select the Orca driver and forward the same SEL lines without a board port."""

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


def run(args, env):
    completed = subprocess.run(
        [str(PEER), *args],
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
    if completed.stdout != EXPECTED:
        raise SystemExit(f"unexpected identities:\n{completed.stdout}")
    if "driver=orca" not in completed.stderr:
        raise SystemExit("peer did not select the Orca driver")
    if "signedIn" in completed.stderr:
        raise SystemExit("Orca driver asked for a Grok login")
    return completed.stdout


def main():
    if not PEER.exists():
        raise SystemExit(f"missing peer binary {PEER}")
    base = os.environ.copy()
    base.pop("POTETOBOT_PORT", None)
    base.pop("POTETOBOT_DRIVER", None)
    run(["--driver", "orca"], base)
    env_driver = base.copy()
    env_driver["POTETOBOT_DRIVER"] = "orca"
    sys.stdout.write(run([], env_driver))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
