"""Pack worker PNGs into the shared frame table and the firmware RLE blob."""

import struct
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    import subprocess

    subprocess.check_call([sys.executable, "-m", "pip", "install", "pillow"])
    from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
FRAMES = ROOT / "assets" / "frames"
OUT_C = ROOT / "firmware" / "main" / "frame_table.c"
OUT_BIN = ROOT / "firmware" / "main" / "frames.bin"

CHARACTERS = ("poteto", "sunny")
EXPRESSIONS = (
    "idle",
    "listening",
    "thinking",
    "speaking",
    "tilt",
    "breath",
    "emphasis",
    "smile",
    "happy",
    "surprised",
    "sleepy",
    "dizzy",
    "wink",
)
# Emphasis and smile PNGs stay on disk. Their packed bytes were freed so
# poteto listening and thinking can each keep two frames under 8388608.
SKIP_LOOPS = frozenset({("poteto", "emphasis"), ("poteto", "smile")})
CANVAS = 480
BYTES_PER_PIXEL = 2
DMA_STRIP_ROWS = 24
# One full-frame decode buffer in PSRAM, and the existing 24-row internal DMA strip.
PSRAM_FRAME_BYTES = CANVAS * CANVAS * BYTES_PER_PIXEL
INTERNAL_DMA_BYTES = CANVAS * DMA_STRIP_ROWS * BYTES_PER_PIXEL
# Bytes of potetobot.bin that are not frames.bin. The factory partition is
# 8388608 bytes, so the largest blob that still links is factory minus this
# overhead. Measured 2026-11-24 from potetobot.bin 7518928 minus frames.bin 7199282.
FIRMWARE_RESERVE_BYTES = 319646
PARTITIONS = ROOT / "firmware" / "partitions.csv"


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def encode_rle(pixels):
    parts = []
    index = 0
    total = len(pixels)
    while index < total:
        black = pixels[index] == 0
        run = 1
        limit = min(total - index, 0x7FFF)
        while run < limit and (pixels[index + run] == 0) == black:
            if not black and run >= 0x7FFF:
                break
            run += 1
        if black:
            parts.append(struct.pack("<H", 0x8000 | run))
        else:
            parts.append(struct.pack("<H", run))
            for value in pixels[index : index + run]:
                parts.append(struct.pack("<H", value))
        index += run
    return b"".join(parts)


def load_frame(path):
    image = Image.open(path).convert("RGB")
    if image.size != (CANVAS, CANVAS):
        raise SystemExit(f"{path} is {image.size[0]}x{image.size[1]}, expected {CANVAS}x{CANVAS}")
    pixels = []
    for r, g, b in image.getdata():
        pixels.append(rgb565(r, g, b))
    return pixels


def factory_app_bytes():
    for raw in PARTITIONS.read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        parts = [part.strip() for part in line.split(",")]
        if len(parts) >= 5 and parts[0] == "factory" and parts[1] == "app":
            return int(parts[4], 0)
    raise SystemExit(f"{PARTITIONS} has no factory app partition")


def frame_blob_budget():
    reserve = FIRMWARE_RESERVE_BYTES
    factory = factory_app_bytes()
    if reserve <= 0 or reserve >= factory:
        raise SystemExit(f"firmware reserve {reserve} does not fit factory {factory}")
    return factory - reserve


def enforce_budget(blob_size):
    if CANVAS != 480 or BYTES_PER_PIXEL != 2:
        raise SystemExit("canvas stays 480 and pixels stay RGB565")
    if DMA_STRIP_ROWS != 24 or INTERNAL_DMA_BYTES != CANVAS * 24 * BYTES_PER_PIXEL:
        raise SystemExit("internal DMA strip stays 24 rows")
    if PSRAM_FRAME_BYTES != CANVAS * CANVAS * BYTES_PER_PIXEL:
        raise SystemExit("PSRAM decode buffer stays one 480x480 RGB565 frame")
    budget = frame_blob_budget()
    if blob_size > budget:
        raise SystemExit(f"frames.bin is {blob_size} bytes, budget is {budget} bytes")
    return budget


def frame_files(character, expression):
    if (character, expression) in SKIP_LOOPS:
        return []
    folder = FRAMES / character / expression
    if not folder.is_dir():
        return []
    files = sorted(folder.glob("f*.png"))
    return files


def c_string(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def main():
    loops = []
    for character in CHARACTERS:
        for expression in EXPRESSIONS:
            files = frame_files(character, expression)
            payloads = []
            rels = []
            for path in files:
                rel = f"{character}/{expression}/{path.name}"
                payload = encode_rle(load_frame(path))
                payloads.append(payload)
                rels.append(rel)
            loops.append((character, expression, rels, payloads))

    # Directory, then payloads. Offsets are from the start of the blob.
    # Header is 8 bytes: u32 magic, u16 loop count, u16 zero.
    # Each loop is 16s character, 16s expression, u16 frame count, u16 reserved,
    # then frame_count times (u32 offset, u32 size).
    dir_size = 8
    for _character, _expression, rels, _payloads in loops:
        dir_size += 16 + 16 + 4 + (8 * len(rels))
    payload_offset = dir_size
    directory = bytearray()
    directory += struct.pack("<IH", 0x31525450, len(loops))  # 'PTR1' little endian, loop count
    directory += struct.pack("<H", 0)
    cursor = payload_offset
    payload_blob = bytearray()
    for character, expression, rels, payloads in loops:
        name_c = character.encode("ascii")[:15]
        name_e = expression.encode("ascii")[:15]
        directory += name_c + b"\0" * (16 - len(name_c))
        directory += name_e + b"\0" * (16 - len(name_e))
        directory += struct.pack("<HH", len(rels), 0)
        for payload in payloads:
            directory += struct.pack("<II", cursor, len(payload))
            cursor += len(payload)
            payload_blob += payload
    if len(directory) != dir_size:
        raise SystemExit(f"directory size {len(directory)} != planned {dir_size}")
    for character, expression, _rels, _payloads in loops:
        if len(expression) > 15 or len(character) > 15:
            raise SystemExit(f"{character}/{expression} exceeds the 16-byte name field")
    blob = directory + payload_blob
    budget = enforce_budget(len(blob))
    OUT_BIN.parent.mkdir(parents=True, exist_ok=True)
    OUT_BIN.write_bytes(blob)

    lines = []
    lines.append("#include <string.h>")
    lines.append("")
    lines.append("typedef struct {")
    lines.append("    const char *character;")
    lines.append("    const char *expression;")
    lines.append("    int count;")
    lines.append("    const char *const *paths;")
    lines.append("} poteto_packed_loop_t;")
    lines.append("")
    for character, expression, rels, _payloads in loops:
        symbol = f"k_{character}_{expression}"
        if rels:
            joined = ", ".join(c_string(rel) for rel in rels)
            lines.append(f"static const char *const {symbol}[] = {{{joined}}};")
        else:
            lines.append(f"static const char *const {symbol}[] = {{0}};")
    lines.append("")
    lines.append("static const poteto_packed_loop_t k_loops[] = {")
    for character, expression, rels, _payloads in loops:
        symbol = f"k_{character}_{expression}"
        lines.append(
            f'    {{{c_string(character)}, {c_string(expression)}, {len(rels)}, {symbol}}},'
        )
    lines.append("};")
    lines.append("")
    lines.append("int poteto_frame_count(const char *character, const char *expression) {")
    lines.append("    size_t i;")
    lines.append("    for (i = 0; i < sizeof(k_loops) / sizeof(k_loops[0]); i++) {")
    lines.append("        if (strcmp(k_loops[i].character, character) == 0 &&")
    lines.append("            strcmp(k_loops[i].expression, expression) == 0) {")
    lines.append("            return k_loops[i].count;")
    lines.append("        }")
    lines.append("    }")
    lines.append("    return -1;")
    lines.append("}")
    lines.append("")
    lines.append("const char *poteto_frame_path(const char *character, const char *expression, int index) {")
    lines.append("    size_t i;")
    lines.append("    for (i = 0; i < sizeof(k_loops) / sizeof(k_loops[0]); i++) {")
    lines.append("        if (strcmp(k_loops[i].character, character) == 0 &&")
    lines.append("            strcmp(k_loops[i].expression, expression) == 0) {")
    lines.append("            if (index < 0 || index >= k_loops[i].count) {")
    lines.append("                return 0;")
    lines.append("            }")
    lines.append("            return k_loops[i].paths[index];")
    lines.append("        }")
    lines.append("    }")
    lines.append("    return 0;")
    lines.append("}")
    lines.append("")
    OUT_C.write_text("\n".join(lines), encoding="utf-8")
    packed = sum(len(rels) for _c, _e, rels, _p in loops)
    print(
        f"packed {packed} frames into {OUT_BIN} ({OUT_BIN.stat().st_size} bytes), "
        f"budget {budget} bytes"
    )


if __name__ == "__main__":
    main()
