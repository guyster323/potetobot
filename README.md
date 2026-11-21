# Potetobot

**Grokbot with me anywhere.**

A round 480 by 480 panel shows one face, poteto or sunny, and draws answer text in a speech bubble on that same frame.

**English** · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md)

[Quick start](#quick-start) · [The serial line](#one-serial-line-selects-the-face) · [Packed frames](#packed-frames-are-the-face)

## The panel shows two faces and a bubble.

Poteto has long black hair, bangs, round gold glasses, blush, black clothes, and a steaming sweet potato beside the face. Sunny has short black hair, brown sunglasses over the eyes, blush, a dark zip, and a newspaper with a sun drawing. The background is black. The face fills the 480 canvas.

| poteto idle, frame f00 | sunny idle, frame f00 |
| --- | --- |
| ![poteto idle, frame f00](assets/frames/poteto/idle/f00.png) | ![sunny idle, frame f00](assets/frames/sunny/idle/f00.png) |
| poteto smile, frame f00 | sunny smile, frame f00 |
| ![poteto smile, frame f00](assets/frames/poteto/smile/f00.png) | ![sunny smile, frame f00](assets/frames/sunny/smile/f00.png) |

These pictures are packed frames. This repository has no demo video.

## Quick start

1. **Build the firmware with ESP-IDF 5.5.** From PowerShell, on the bench:

```
. C:\Esp\esp-idf\export.ps1
cd firmware
idf.py build
```

The face pack is already `firmware/main/frames.bin`. A normal build embeds that file. It does not repaint the characters. The app image is `firmware/build/potetobot.bin`.

2. **Stop if that image is larger than 8388608 bytes.** That is the factory partition. The bench image built on 2026-11-21 is 8377584 bytes, so 11024 bytes remain.

3. **Flash the bootloader, the partition table, the app, and the font.** From `firmware\build`, with the ESP-IDF Python. The bench board enumerates as COM5. Use the port you actually have.

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **Wait until the log says the app has returned from `app_main`.** Then send `SEL` or `TXT`.

5. **Start the peer and send one selection, then one line of text.**

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

The peer reads stdin. Build it first when `host\build\potetobot-peer.exe` is missing. The peer section has the `cl` line.

## The board is a Waveshare ESP32-S3-Touch-AMOLED-2.16.

| Item | Value |
| --- | --- |
| Target | ESP32-S3 at 240 MHz (`sdkconfig.defaults`) |
| Flash | 16 MB. The defaults file sets QIO. The bench write used `--flash_mode dio` and `--flash_freq 80m` |
| PSRAM | Octal, 80 MHz |
| Console | USB Serial/JTAG, 115200 8N1 |
| Panel | 480×480 |
| Factory app | Offset `0x10000`, size `0x800000` (8388608 bytes) |
| Font partition | Name `font`, subtype `0x40`, offset `0x810000`, size `0x200000` (2097152 bytes) |
| Touch | Address `0x5A`. SCL GPIO14, SDA GPIO15, interrupt GPIO11, reset GPIO40 |

## Packed frames are the face.

`play_task` draws again every 66 ms. The frame index advances on the expression's own step, `1000 / fps`.

| Character | Expression | Frames | fps | Step |
| --- | --- | --- | --- | --- |
| poteto | idle | 3 | 6 | 166 ms |
| poteto | speaking | 4 | 6 | 166 ms |
| poteto | tilt | 4 | 6 | 166 ms |
| poteto | breath | 1 | 6 | 166 ms |
| poteto | emphasis | 3 | 6 | 166 ms |
| poteto | smile | 5 | 6 | 166 ms |
| sunny | idle | 2 | 6 | 166 ms |
| sunny | listening | 1 | 15 | 66 ms |
| sunny | thinking | 1 | 12 | 83 ms |
| sunny | speaking | 4 | 6 | 166 ms |
| sunny | smile | 1 | 6 | 166 ms |

`1000/6` is 166, `1000/15` is 66, and `1000/12` is 83, all integer division.

These names are in the scheduler and are packed with zero frames. The board rejects them. It sends no `ACK`, and the face that is already showing stays.

| Character | Names with zero frames |
| --- | --- |
| poteto | listening, thinking |
| sunny | tilt, breath, emphasis |
| both | happy, surprised, sleepy, dizzy, wink |

## One serial line selects the face.

```
SEL v1 poteto idle
SEL v1 sunny smile
```

A packed expression answers in this shape:

```
ACK poteto idle
```

The parser accepts a trailing millisecond field, as in `SEL v1 poteto speaking 500`. The board then starts the loop at the first frame. That field does not seek.

`SEL v1 poteto listening` and `SEL v1 sunny tilt` match the grammar and have no frames, so they produce no `ACK`.

## Answer text rides that same line.

```
TXT v1 thinking
TXT v1 The potato is hot.
SAY v1 thinking The potato is hot.
SAY v1 answer The potato is hot.
```

| Line | Phase | Text stored in the bubble |
| --- | --- | --- |
| `TXT v1 thinking` | thinking | `...` |
| `TXT v1` plus any other text | answer | that text |
| `SAY v1 thinking` plus text | thinking | that text |
| `SAY v1 answer` plus text | answer | that text |

The board answers `ACK say thinking` or `ACK say answer`. `POTETO_SAY_TEXT_MAX` is 480 bytes, including the terminating NUL, so the stored text is at most 479 bytes.

The bubble rectangle is x=12, y=356, width=456, height=112, with a corner radius of 14. Pixels under it are shifted right twice, which takes each RGB565 channel to about a quarter, and they are not erased. The rim and answer glyphs are `0xFFFF`. Thinking glyphs are `0xCE79`. The board keeps the last 8 messages. The text window is 80 pixels tall and each line is 18 pixels, so four lines fit. A vertical drag scrolls the stored lines.

When the peer accepts a `SEL` whose expression is `thinking`, it also writes `TXT v1 thinking`.

## The Windows peer speaks grok-bot or orca.

| Driver | When it agrees to run |
| --- | --- |
| `grok-bot` | `%APPDATA%\Grok Bot\desktop-status.json` contains `"signedIn":true`, and the pid in that file is still running. This is the default. |
| `orca` | A process image named `Orca.exe` is running. |

```
host\build\potetobot-peer.exe --driver grok-bot
host\build\potetobot-peer.exe --driver orca
```

`--driver` wins over `POTETOBOT_DRIVER`. The port name comes from `POTETOBOT_PORT`. The peer opens it at 115200 8N1 with DTR off and RTS off, so opening the port does not reset the chip into download mode. It forwards `SEL` lines and lines that start with `TXT v1`. On stderr, grok-bot prints `driver=grok-bot pid=<pid> signedIn=true`. orca prints `driver=orca`.

Build the peer from the repository root with the Visual Studio x64 compiler:

```
cl /nologo /O2 /I firmware\main /Fe:host\build\potetobot-peer.exe host\peer_main.c firmware\main\schedule.c firmware\main\frame_table.c firmware\main\say.c
```

`host/test_schedule.c` checks the frame loop. Run `host\build\test_schedule.exe` with the repository root, or with no argument to use `.`. Its speaking checks use 6 fps: poteto at `(1000/6)*2` expects `poteto/speaking/f02.png`, and sunny at `(1000/6)*3` expects `sunny/speaking/f03.png`. `tests/test_peer.py` needs a signed-in Grok Bot. `tests/test_peer_orca.py` needs `Orca.exe`. `tests/test_frame_budget.py` checks the packed blob against the factory limit.

## The glyph atlas stays out of the factory image.

`firmware/font/bubble_font.bin` is 1325432 bytes. It is a subset of GNU Unifont 16.0.04, and `firmware/main/ascii8x16.inc` is the ASCII fallback named beside it in `firmware/font/LICENSE.txt`. The flash line writes the atlas at `0x810000`. That is the `font` partition, not part of the 8388608-byte factory app.

## This build does not play audio.

`firmware/main` has no audio output. The serial protocol has no language setting and no walkie-talkie. Words from the peer are drawn in the bubble.

## The tree is split this way.

| Path | What it holds |
| --- | --- |
| `firmware/` | ESP-IDF app, `partitions.csv`, and the font image |
| `firmware/main/frames.bin` | Packed RGB565 RLE frames the build embeds |
| `host/peer_main.c` | Windows peer |
| `assets/frames/` | Source frames, 480×480 |
| `tests/` | Peer and frame-budget checks |
