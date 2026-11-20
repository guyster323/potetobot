"""The packed blob must fail when the linked app would pass 8388608 bytes."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import gen_frames


def main():
    budget = gen_frames.frame_blob_budget()
    factory = gen_frames.factory_app_bytes()
    if factory != 0x800000:
        raise SystemExit(f"factory app is {factory}, expected 0x800000")
    if budget != factory - gen_frames.FIRMWARE_RESERVE_BYTES:
        raise SystemExit("budget does not leave the firmware reserve")
    if gen_frames.PSRAM_FRAME_BYTES != 480 * 480 * 2:
        raise SystemExit("PSRAM frame is not one RGB565 canvas")
    if gen_frames.INTERNAL_DMA_BYTES != 480 * 24 * 2:
        raise SystemExit("DMA strip is not 24 rows")
    try:
        gen_frames.enforce_budget(budget + 1)
    except SystemExit as exc:
        if "budget" not in str(exc):
            raise
    else:
        raise SystemExit("over-budget blob was accepted")
    if gen_frames.enforce_budget(budget) != budget:
        raise SystemExit("budget-sized blob was rejected")
    blob_path = Path(__file__).resolve().parents[1] / "firmware" / "main" / "frames.bin"
    blob_size = blob_path.stat().st_size
    if blob_size > budget:
        raise SystemExit(f"current blob {blob_size} exceeds budget {budget}")
    if blob_size + gen_frames.FIRMWARE_RESERVE_BYTES > factory:
        raise SystemExit("current blob plus app overhead exceeds 8388608")
    print(f"budget ok {budget} blob {blob_size}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
