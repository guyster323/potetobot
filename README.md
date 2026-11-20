[English](README.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md) · [한국어](README.ko.md)

# Potetobot

Potetobot is a desk gadget that shows a face and, when there is something to read, a speech bubble. It runs on a Waveshare ESP32-S3-Touch-AMOLED-2.16. Two characters live on the round panel: poteto and sunny. A small Windows program, the peer, sends expression lines and answer text to the board over one USB serial connection.

The firmware does not answer with a voice. `firmware/main` has no audio output. The board carries an ES8311 speaker and a microphone, and this build leaves both of them unused. There is no language menu and no walkie-talkie. Words from the peer are drawn on the screen.

The face artwork stays as it is. Poteto keeps the sweet potato. Sunny keeps the newspaper and the sunglasses above the eyes.

## The board

The Waveshare ESP32-S3-Touch-AMOLED-2.16 (SKU 33969) is an ESP32-S3R8 module with 8 MB of PSRAM and 16 MB of flash. The panel is a 480 by 480 CO5300 AMOLED on a quad SPI bus. Touch is a CST9217 (or CST9220) on I2C: SCL is GPIO14, SDA is GPIO15, interrupt is GPIO11, and reset is GPIO40. Power is an AXP2101. The user key is GPIO18.

On this machine the USB Serial/JTAG port enumerates as COM5. The peer talks to it at 115200 baud. It opens the port with DTR disabled and RTS disabled, so opening the peer does not reset the chip into download mode.

The factory application partition is 8388608 bytes, from `0x10000` through `0x810000`. The app image has to fit in that partition. The speech-bubble glyph atlas does not. It is a separate data partition named `font`.

## Faces

Poteto has black hair, a blush, round gold glasses, and a sweet potato. Sunny has short black hair, brown sunglasses sitting above the eyes, and a newspaper with a sun on it. The art is anchored at x 255, y 271 on the 480 by 480 canvas. This project does not redraw those pictures and does not change how fast the frames play.

Packed expressions, and the frame rate the loop uses:

| Expression | Frames per second |
| --- | --- |
| idle | 6 |
| listening | 15 |
| thinking | 12 |
| speaking | 6 |
| tilt | 6 |
| breath | 6 |
| emphasis | 6 |
| smile | 6 |

Poteto listening and poteto thinking have no packed frames. The board rejects those two selections, does not acknowledge them, and leaves the face that is already on screen. Sunny does have a listening frame and a thinking frame. Speaking for both characters is a short loop at 6 frames per second. The player redraws about every 66 ms.

## SEL

A selection line names the character and the expression:

```
SEL v1 poteto idle
SEL v1 sunny speaking
```

An optional time in milliseconds may follow the expression. The board answers with one line:

```
ACK poteto idle
```

The peer prints `poteto idle` on its own stdout and forwards the original line on the serial port. `grok-bot` and `orca` both do this. They share one read loop.

## SAY and the speech bubble

Answer text uses a second line on that same connection:

```
SAY v1 thinking still working
SAY v1 answer hello potato
```

`thinking` is the line for while the bot is still working. `answer` is the line for when the reply has arrived. The rest of the line is UTF-8 text, kept up to 480 bytes. The board answers:

```
ACK say thinking
ACK say answer
```

Anything that is not `thinking` or `answer` is rejected. A `SAY` line is not a face selection.

Both phases open a speech bubble on the panel. The rectangle is x = 12, y = 356, width = 456, height = 112. That sits below the eyes and below the sunglasses. The upper part of the sweet potato, and the sun on the newspaper, stay fully bright above the bubble. Pixels under the bubble are dimmed to about a quarter of their brightness. They are not erased, and the bubble has no tail. A white rounded border, radius 14, frames the dimmed area. Thinking text is drawn in a softer white. Answer text is drawn in white.

About four lines fit in the bubble. Longer text wraps on spaces. The board keeps the last eight messages. A new message snaps to the newest lines. To read older lines, put a finger down inside the bubble and drag. Moving the finger up reveals the text below. The scroll stops at the oldest line and at the newest line. A drag that starts outside the bubble does not scroll it.

The bubble is painted onto the decoded face, then the frame is sent to the panel. The face, the potato, the newspaper, and the sunglasses are the same pictures as before.

## The glyph atlas

The factory image has only about fifteen kilobytes of spare room, which is not enough for a multilingual font. The atlas is GNU Unifont 16.0.04, stored outside the factory image.

`tools/gen_bubble_font.py` writes `firmware/font/bubble_font.bin`. The file in this tree is 1325432 bytes. The partition that holds it is declared in `firmware/partitions.csv`:

```
font, data, 0x40, 0x810000, 0x200000
```

That is 2097152 bytes starting at `0x810000`, after the factory app. The firmware maps that partition at boot. If the magic is missing, English letters still draw from a small Unifont table linked into the app, and other scripts do not. The attribution is in `firmware/font/LICENSE.txt`. Do not rasterize a Windows font into a file you publish. `tools/gen_font.py` is not the font this board reads.

## Drivers

`host/peer_main.c` is the peer. The default driver is `grok-bot`. It reads `%APPDATA%\Grok Bot\desktop-status.json` and continues only when `signedIn` is true and the recorded process id is still alive.

`orca` looks for a running `Orca.exe`. It does not read the Grok Bot status file and it does not ask for a Grok login.

```
potetobot-peer.exe
potetobot-peer.exe --driver grok-bot
potetobot-peer.exe --driver orca
```

`POTETOBOT_DRIVER` selects the same names. `--driver` wins when both are set. `POTETOBOT_PORT` selects the board, for example `COM5`. Both drivers read stdin. A `SEL` line changes the face. A `SAY` line is forwarded as written, so either driver can put thinking text or answer text on the panel.

Build the peer with the Visual Studio x64 tools, from the repository root:

```
cl /nologo /O2 /I firmware\main /Fe:host\build\potetobot-peer.exe host\peer_main.c firmware\main\schedule.c firmware\main\frame_table.c firmware\main\say.c
```

`host/build/test_schedule.exe` checks the frame loop and the `SAY` parser. Give it the repository root as its argument. `tests/test_peer.py` needs a signed-in Grok Bot. `tests/test_peer_orca.py` needs `Orca.exe`.

## Build the firmware

Use ESP-IDF 5.5. In PowerShell:

```
. C:\Esp\esp-idf\export.ps1
cd firmware
idf.py build
```

The face pack is already `firmware/main/frames.bin`. A normal build embeds that file. It does not repaint the characters. The app image is `firmware/build/potetobot.bin`. Do not flash it if it is larger than 8388608 bytes.

## Flash on COM5

From `firmware\build`, with the ESP-IDF Python:

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

The bootloader, the partition table, and the app are the usual three images. `bubble_font.bin` is written at `0x810000`, which is the `font` partition and not part of the factory app. After the hard reset, wait until the log says the app has returned from `app_main` before sending `SEL` or `SAY`.
