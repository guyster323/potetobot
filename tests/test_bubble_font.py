"""Open the real bubble_font.bin. The check uses that file's bytes.

Build from the repository root:

cl /nologo /O2 /I firmware\\main /Fe:host\\build\\test_pbf1.exe host\\test_pbf1.c firmware\\main\\pbf1.c
"""

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / "host" / "build" / "test_pbf1.exe"
FONT = ROOT / "firmware" / "font" / "bubble_font.bin"


def main():
    if not EXE.exists():
        raise SystemExit(f"missing {EXE}")
    if not FONT.is_file():
        raise SystemExit(f"missing {FONT}")
    completed = subprocess.run(
        [str(EXE), str(FONT)],
        text=True,
        capture_output=True,
        check=False,
        cwd=ROOT,
    )
    if completed.returncode != 0:
        sys.stderr.write(completed.stderr)
        raise SystemExit(f"pbf1 exit {completed.returncode}")
    if "pbf1 ok" not in completed.stdout or "oor=rejected" not in completed.stdout:
        raise SystemExit(f"unexpected pbf1 output:\n{completed.stdout}")
    if "U+0041" not in completed.stdout or "U+ACE0" not in completed.stdout:
        raise SystemExit(f"glyph check missing:\n{completed.stdout}")
    sys.stdout.write(completed.stdout)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
