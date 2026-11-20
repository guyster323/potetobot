"""Not the shipped bubble font. tools/gen_bubble_font.py writes that atlas.

This script rasterizes Windows fonts into firmware/font.bin. Do not commit
that file and do not flash it. The board reads GNU Unifont from the font
partition instead.
"""

import struct
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "firmware" / "font.bin"
GLYPH = 16
ASCII_FIRST = 32
ASCII_COUNT = 95
HANGUL_FIRST = 0xAC00
HANGUL_COUNT = 11172
FONT_CANDIDATES = (
    Path(r"C:\Windows\Fonts\malgun.ttf"),
    Path(r"C:\Windows\Fonts\malgunbd.ttf"),
    Path(r"C:\Windows\Fonts\segoeui.ttf"),
)


def load_font():
    for path in FONT_CANDIDATES:
        if path.is_file():
            return ImageFont.truetype(str(path), 14)
    raise SystemExit("no TrueType font found for the bubble atlas")


def pack_glyph(font, text):
    image = Image.new("L", (GLYPH, GLYPH), 0)
    draw = ImageDraw.Draw(image)
    draw.text((GLYPH / 2, GLYPH / 2), text, font=font, fill=255, anchor="mm")
    packed = bytearray()
    for y in range(GLYPH):
        bits = 0
        for x in range(GLYPH):
            if image.getpixel((x, y)) > 80:
                bits |= 0x8000 >> x
        packed.append((bits >> 8) & 0xFF)
        packed.append(bits & 0xFF)
    return packed


def main():
    font = load_font()
    glyphs = bytearray()
    for code in range(ASCII_FIRST, ASCII_FIRST + ASCII_COUNT):
        glyphs += pack_glyph(font, chr(code))
    for index in range(HANGUL_COUNT):
        glyphs += pack_glyph(font, chr(HANGUL_FIRST + index))
        if index % 2000 == 0:
            print(f"glyph {index}", flush=True)
    stride = GLYPH * GLYPH // 8
    header = struct.pack("<4sHHHH", b"PBF1", GLYPH, GLYPH, ASCII_COUNT, stride)
    header += struct.pack("<I", HANGUL_COUNT)
    if len(header) != 16:
        raise SystemExit(f"header is {len(header)} bytes")
    OUT.write_bytes(header + glyphs)
    print(f"wrote {OUT} ({OUT.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
