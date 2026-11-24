r"""KEY3 slot, bus fault, and press edge.

Build from the repository root:

cl /nologo /O2 /std:c11 /I firmware\main /Fe:host\build\test_key3.exe host\test_key3.c firmware\main\bubble_slot.c firmware\main\key3_edge.c firmware\main\key3_bus.c
"""

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / "host" / "build" / "test_key3.exe"
BUILD = (
    r"cl /nologo /O2 /std:c11 /I firmware\main /Fe:host\build\test_key3.exe "
    r"host\test_key3.c firmware\main\bubble_slot.c firmware\main\key3_edge.c firmware\main\key3_bus.c"
)


def fail(message):
    raise SystemExit(message)


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


def main():
    if BUILD not in Path(__file__).read_text(encoding="utf-8"):
        fail("build command is missing beside this test")
    EXE.parent.mkdir(parents=True, exist_ok=True)
    built = subprocess.run(compile_prefix() + BUILD, cwd=ROOT, capture_output=True, check=False, shell=True)
    if built.returncode != 0:
        sys.stderr.write(built.stdout.decode("utf-8", "replace"))
        sys.stderr.write(built.stderr.decode("utf-8", "replace"))
        fail(f"key3 build exit {built.returncode}")
    ran = subprocess.run([str(EXE)], cwd=ROOT, capture_output=True, check=False)
    stdout = ran.stdout.decode("utf-8", "replace")
    stderr = ran.stderr.decode("utf-8", "replace")
    if ran.returncode != 0:
        sys.stderr.write(stderr)
        fail(f"key3 exit {ran.returncode}")
    if stdout.strip() != "key3 ok":
        fail(f"unexpected stdout:\n{stdout}")
    if stderr != "":
        fail(f"key3 wrote stderr:\n{stderr}")
    print("key3 ok")


if __name__ == "__main__":
    main()
