"""Build the speech-bubble glyph atlas outside the factory app image.

The atlas is a subset of GNU Unifont. It is flashed into the data partition
named font, which starts after the 8388608-byte factory app. It is not linked
into potetobot.bin.
"""

import gzip
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PARTITIONS = ROOT / "firmware" / "partitions.csv"
OUT_BIN = ROOT / "firmware" / "font" / "bubble_font.bin"
OUT_ASCII = ROOT / "firmware" / "main" / "ascii8x16.inc"
HEX_GZ = Path(r"C:\Users\windo\AppData\Local\Temp\poteto-fonts\unifont.hex.gz")

RANGES = (
    (0x20, 0x7E),
    (0xA0, 0x024F),
    (0x0600, 0x06FF),
    (0x0900, 0x097F),
    (0x0980, 0x09FF),
    (0x2000, 0x206F),
    (0x3000, 0x303F),
    (0x4E00, 0x9FFF),
    (0xAC00, 0xD7A3),
    (0xFF00, 0xFF60),
)


def wanted(codepoint):
    for start, end in RANGES:
        if start <= codepoint <= end:
            return True
    return False


def font_partition_bytes():
    found = None
    for raw in PARTITIONS.read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        parts = [part.strip() for part in line.split(",")]
        if len(parts) >= 5 and parts[0] == "font":
            found = int(parts[4], 0)
    if found is None:
        raise SystemExit("partitions.csv has no font data partition")
    return found


def load_glyphs(path):
    glyphs = {}
    opener = gzip.open if path.suffix == ".gz" else open
    with opener(path, "rt", encoding="ascii", errors="replace") as handle:
        for line in handle:
            line = line.strip()
            if ":" not in line:
                continue
            key, blob = line.split(":", 1)
            try:
                codepoint = int(key, 16)
            except ValueError:
                continue
            if not wanted(codepoint):
                continue
            blob = "".join(blob.split())
            if len(blob) == 32:
                width = 8
            elif len(blob) == 64:
                width = 16
            else:
                continue
            try:
                bits = bytes.fromhex(blob)
            except ValueError:
                continue
            if codepoint != 0x20 and bits == b"\x00" * len(bits):
                continue
            glyphs[codepoint] = (width, bits)
    return glyphs


def build(glyphs):
    ordered = sorted(glyphs)
    header = bytearray(16)
    header[0:4] = b"PBF1"
    header[4:6] = struct.pack("<H", 16)
    header[6:8] = struct.pack("<H", 8)
    header[8:12] = struct.pack("<I", len(ordered))
    index = bytearray()
    bitmaps = bytearray()
    bitmap_base = 16 + len(ordered) * 8
    for codepoint in ordered:
        width, bits = glyphs[codepoint]
        offset = bitmap_base + len(bitmaps)
        if offset > 0xFFFFFF:
            raise SystemExit(f"glyph offset for U+{codepoint:04X} does not fit in 24 bits")
        info = offset | (width << 24)
        index += struct.pack("<II", codepoint, info)
        bitmaps += bits
    header[12:16] = struct.pack("<I", bitmap_base)
    return bytes(header) + bytes(index) + bytes(bitmaps)


def write_ascii(glyphs):
    rows = []
    for codepoint in range(0x20, 0x7F):
        width, bits = glyphs.get(codepoint, (8, b"\x00" * 16))
        if width == 16:
            bits = bytes(bits[i] for i in range(0, 32, 2))
        if len(bits) != 16:
            bits = (bits + b"\x00" * 16)[:16]
        cells = ", ".join(f"0x{byte:02X}" for byte in bits)
        rows.append(f"    {{{cells}}}")
    text = "\n".join(
        [
            "/* GNU Unifont basic Latin, U+0020 through U+007E, 8x16, MSB on the left. */",
            "static const uint8_t k_ascii8x16[95][16] = {",
            ",\n".join(rows),
            "};",
            "",
        ]
    )
    OUT_ASCII.write_text(text, encoding="ascii")


def main():
    if not HEX_GZ.is_file():
        raise SystemExit(f"missing {HEX_GZ}")
    budget = font_partition_bytes()
    glyphs = load_glyphs(HEX_GZ)
    if 0xAC00 not in glyphs or 0x0041 not in glyphs:
        raise SystemExit("unifont subset is missing Hangul or Latin")
    blob = build(glyphs)
    if len(blob) > budget:
        raise SystemExit(f"bubble font is {len(blob)} bytes, font partition is {budget}")
    OUT_BIN.parent.mkdir(parents=True, exist_ok=True)
    OUT_BIN.write_bytes(blob)
    write_ascii(glyphs)
    print(f"glyphs {len(glyphs)}")
    print(f"bubble_font.bin {len(blob)} bytes, partition {budget}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
