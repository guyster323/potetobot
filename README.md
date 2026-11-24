# Potetobot

**Grokbot with me anywhere.**

A round 480 by 480 panel shows one face, poteto or sunny, and draws answer text in a speech bubble on that same frame.

**English** · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md)

[Quick start](#quick-start) · [The host](#the-host-asks-one-orca-bot) · [The serial line](#one-serial-line-selects-the-face) · [Packed frames](#packed-frames-are-the-face)

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

2. **Stop if that image is larger than 8388608 bytes.** That is the factory partition. The clean rebuild on 2026-11-24 from `1a20bc0` plus the KEY3 and frame sources is 7519200 bytes, so 869408 bytes remain. `check_sizes` reported `0x72bbe0` against `0x800000`, with `0xd4420` bytes free. SHA256 is `ea8bb1687f014dc02fbfc2454eb640363f7c195d6483e614d195d9c8213fa6ff`. `esptool image_info` checksum is `61` and the validation hash is `1b61a0a6f1d26abc2ec27ba97d4bfd57781db3604078c6fec8bb1152eef3fb26`. The app version string is `1a20bc0-dirty`. An earlier same-size image had SHA256 `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`. This rebuild was not flashed.

3. **Flash is the owner's step.** This finish did not open a port and did not flash. From `firmware\build`, with the ESP-IDF Python, the bootloader is 22288 bytes at `0x0`, the partition table is 3072 bytes at `0x8000`, the app is at `0x10000`, and the font is 1325432 bytes at `0x810000`. Use the port you actually have.

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **Start the host on the PC before the board test.** The sim transport does not set `POTETOBOT_PORT`. The default `%APPDATA%\potetobot\destination.json` stays absent. Pass one destination file. `C:\Python313\python.exe` has whisper, numpy, and torch. The command below was run once on 2026-11-24. Running it again sends another question.

```
C:\Python313\python.exe host\mvp_host.py --transport sim --wav tests\fixtures\heami-capital.wav --destination host\build\orca-dest.json
```

5. **After the flash, the voice face is poteto.** Hold KEY3 for listening, two frames at 15 fps. Release for thinking, two frames at 12 fps. While the request waits, `CUE v1 <utt> prepare` selects tilt, four frames at 6 fps. When the answer arrives, `CUE v1 <utt> speak` selects speaking, four frames at 6 fps, and draws the short bubble. `CUE v1 <utt> idle` and `CUE v1 <utt> error` select idle. Scroll the bubble with touch. `host\mvp_host.py --transport serial` keeps reading `POTETOBOT_PORT` through the Win32 port API. `--inject` feeds board bytes and does not open a handle. pyserial is not used. This finish did not flash. A cue for a different KEY3 press does not change the face. A drop, a damaged frame, a missing end, an empty destination, a bot failure, or a timeout shows error for that utterance, and the next good utterance still runs. Serial without `--inject` does not launch the face simulator. A Win32 write that returns 0 or writes nothing is not recorded as a sent cue, and a failed read is not a timeout. This pass did not open a COM port.

6. **SEL, TXT, and SAY still go through the peer.** The peer is the manual line. It is not the voice path.

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

The peer reads stdin. Build it first when `host\build\potetobot-peer.exe` is missing. The peer section has the `cl` line.

## The host asks one Orca bot.

`host\mvp_host.py` is the Windows entry. The command in step 4 is the start that ran. `--transport sim` and `--transport serial --inject` share that session: AUD bytes in, `CUE v1 <utt> prepare` when the request id is printed, then `CUE v1 <utt> speak` with that answer. `--transport serial` without `--inject` opens one COM handle from `POTETOBOT_PORT` and keeps reading. pyserial is not imported. The peer opens its own handle for SEL, TXT, and SAY. Sim mode chunks the WAV with `host\aud_format.exe`, which calls the same `AUD v1` formatter as the firmware. The host keeps a frame only when the utterance id, sequence, byte length, and end line agree. A gap, a duplicate, a short hex line, a mixed log, a drop, or a missing end is not transcribed. One utterance is bound at a time. A second send does not start for a late answer. A bad utterance writes `CUE v1 <utt> error` for that id and the session continues. One line stops at 2048 bytes. A started utterance waits at most 8 seconds and at most 400 AUD lines. A lost end is not joined to the next utterance. While Whisper or the request runs, the host keeps reading that same handle. It stores at most 16 utterances, each PCM at most 192000 bytes (6.0 s). Past that cap it prints `audio busy` and writes `CUE v1 <utt> error`, and the next utterance still runs.

| Limit | Value |
| --- | --- |
| PCM | 16 kHz, mono, s16, 32000 bytes each second |
| One frame | 640 bytes, then one `AUD v1 <utt> <seq> <len> <hex>` line |
| One utterance | 192000 bytes, which is 6.0 seconds |
| Question | at most 2000 UTF-8 bytes, in `--body-file` |
| Bubble | the answer excerpt under 96 UTF-8 bytes |
| Line | at most 2048 bytes |
| Started utterance | at most 8 seconds and 400 AUD lines |
| Waiting utterances | at most 16, each PCM at most 192000 bytes (6.0 s) |

The sim run on 2026-11-24 read `tests\fixtures\heami-capital.wav` (Microsoft Heami, 16 kHz, mono, s16). Reassembly was 151680 bytes in 238 lines. Whisper base on CPU transcribed `안녕하세요. 대한민국의 수도는 어디인가요?`. The one request was `msg_5f8b8f406d35`. The inbox row `msg_789251e073d5` came from `orca_session_id:6176a306-e513-4dff-b903-5bed98beb33d` and its body was `서울입니다.`. The bubble was that same excerpt. Empty destination and `grok_bot` printed no `request`.

The later `--transport serial --inject` run used that same WAV and did not open a COM handle. PCM was 151680 bytes. The request was `msg_bf26672f0e57`. After that id, the waiting frame was `poteto/tilt/f02.png`. The answer and the speak bubble were `서울입니다.`. `from` was `orca_session_id:1398eeac-555a-4337-bc0c-86e4e5d49458`.

The sender handle stays in `%APPDATA%\potetobot\host-terminal.json`. The destination file and the firmware do not store a chat id. `pyserial` lives in the ESP-IDF virtualenv and the sim run did not import it.

The device command below is what the code accepts. This pass did not run it and did not open a COM port. Start it from `C:\Dev\ESP32Bot`.

```
$env:POTETOBOT_PORT = "COM5"
C:\Python313\python.exe host\mvp_host.py --transport serial --destination host\build\orca-dest.json
```

`--inject` and `--transport sim` share the observed session and do not open a handle. The default `%APPDATA%\potetobot\destination.json` stays absent. `host\build\orca-dest.json` contains `{"provider":"orca_bots","botId":"Poteto"}`. The sender file `%APPDATA%\potetobot\host-terminal.json` is already present and holds a `term_` handle. If that file is missing, the host runs `orca terminal create --title potetobot-host --json` and writes `{"handle":"term_..."}`. An earlier measured create used `orca terminal create --worktree path:C:\Dev\ESP32Bot --title potetobot-host --json` and did not pass `--focus`. Do not put a chat id in the destination file or the firmware.

On 2026-11-24, `C:\Python313\python.exe host\mvp_host.py --self-test` exited 0 from `C:\Dev\ESP32Bot` with `POTETOBOT_PORT` unset. The last line was `self-test ok`. It opened no COM port and sent no new Orca question. One session answered `하나` and `둘`. An empty destination printed `channel status not_sent` and no `request` line. A timeout printed `audio timeout 6`, and the next utterance answered `다시`. A drop, a damaged frame, and a missing end showed idle with bubble `error`, and the next utterance still ran. After `btn 0 0102`, `btn 1 -`, and `btn 0 0506`, the untagged `CUE v1 speak old` and `CUE v1 1 speak old` left the face on listening with no bubble `old`. `CUE v1 2 prepare` showed tilt `poteto/tilt/f02.png`, and `CUE v1 2 speak next` showed speaking and bubble `next`. One press still accepted an untagged `CUE v1 prepare` and `CUE v1 speak kept`. A Win32 `WriteFile` that returned 0, and one that failed, were not stored in `sent`. A 4-byte partial write of `CUE v1 1 prepare` was finished by the next write. On the mock serial session the first chunk was `CUE `, `speak 하나` was not stored, and the stored cues were `CUE v1 3 prepare`, `CUE v1 3 error`, `CUE v1 4 prepare`, and `CUE v1 4 speak 둘`. After those bytes, `ReadFile` failed and the session printed `link read read failed` and stopped. `readline` on a failed read called `ReadFile` twice and did not spin. The face executable did not run on that mock path. This run is separate from `msg_bf26672f0e57` and from `msg_5f8b8f406d35`.

The same self-test then used a finite virtual receive buffer, not a preloaded InjectLink. A second PCM of 16000 bytes (0.5 s) became 32437 frame bytes. An undrained 8192-byte buffer dropped 24245 of those bytes. With the host reading during the blocked transcription, the dropped count was 0 and both answers were `하나` and `둘`. With one waiting slot, the extra utterance printed `audio busy` and its error cue, and the next answer was `회복`. Firmware was not rebuilt. The image remains 7519200 bytes. The device command above is unchanged. No COM port was opened.

`host\gui_server.py` is the loopback page. On 2026-11-24 it listened on `127.0.0.1:8765` only. It did not open a COM port, flash, or write `%APPDATA%\potetobot\destination.json`. The screen language and the speech-recognition language are stored apart in `%APPDATA%\potetobot\gui.json`. Korean is the default for both. English speech recognition is passed to Whisper as `language=en`, with `fp16=false` and `temperature=0`. Orca names come from `orca bot list --json`. One saved name is the destination `botId` only when a real ask starts. This pass sent no new Orca question. Grok Bot has no documented account list or answer path. A pasted Share id built `grokbot://app/v1/prompt?id=fromshare&text=hello`. That link does not send. A webhook HTTP 200 is acceptance, not an answer. The xAI model API is not used. Character cards read `firmware\main\frame_table.c` and `assets\frames`. Poteto on the board is idle 3, listening 2, thinking 2, speaking 4, tilt 4, breath 1. Sunny on the board is idle 2, listening 1, thinking 1, speaking 4, smile 1. Poteto smile and emphasis stay at 0 board frames, with 5 and 3 preview PNGs. This pass added no images and left the image at 7519200 bytes. Windows listed `COM5` from the registry and did not open it. The Mac and Linux adapters returned `not_measured` on this PC. `host\gui_server.py --self-test` exited 0. In the browser, a virtual good utterance wrote `CUE v1 1 prepare` and `CUE v1 1 speak virtual` with `whisper language=en`. A bad line wrote `CUE v1 error`. Both used BoundedKernel.

```
C:\Python313\python.exe host\gui_server.py
```

## Start pins one session.

On 2026-11-24 the same page gained Start and Stop. Start does not write `gui.json` or `destination.json`. The port, provider, selected bot, and speech language are arguments of that session process. The port is not copied into `POTETOBOT_PORT`. Omitting `--stt-language` on the CLI still uses Korean. A POST needs `Content-Type: application/json`, `Host: 127.0.0.1:<port>`, `Origin: http://127.0.0.1:<port>`, and the page token. A wrong origin returns 403 and writes nothing. A second Start while connected returns 409.

The self-test keeps that start path. It replaces only the serial opener with a finite virtual port and only the Orca CLI with a local band. One utterance sent one question to `bot:Poteto`. `prepare` and `speak` shared that utterance id, and the answer text was `band-answer`. An empty bot id and `grok_bot` sent zero questions. Stop added no later speak. `COM99` through the real opener showed `serial open failed COM99` and no handle. COM5 was not opened.

In the Windows browser the port field was changed from `COM5` to `COM99` before Start. The page showed `Failed · serial open failed COM99` and an empty handle. Search `zzz-no-such` showed `No search matches.` The full list stayed `It'sMe`, `서치린`, `Poteto`, `정리린`, `버그린`, `리뷰린`. Clearing the box showed all six again. Refresh while the box said `Poteto` did not drop the other names. Mint is a preview card only: 0 board frames, voice cues off, SEL off, one idle PNG. The Grok control built a real link `grokbot://app/v1/prompt?id=fromshare&text=hello` with target `_blank`. Copy changed the button to `Copied`. Clicking that link did not start a session. With speech language Korean, the good virtual utterance wrote `CUE v1 1 prepare` and `CUE v1 1 speak 가상`, `openedPort=false`. The error utterance wrote `CUE v1 1 error` and no speak line.

This PC has no `os.openpty`. The parameter check used a socket pair and the bytes `sock-byte`. It did not measure a Mac or Linux device. Windows open, read, write, and close went through `open_serial` with a kernel stand-in: 115200, 8 data bits, parity 0, stop bits 0, flags 1. No device handle was created.

`C:\Python313\python.exe host\gui_server.py --self-test` exited 0. `C:\Python313\python.exe host\mvp_host.py --self-test` exited 0. Both ran from `C:\Dev\ESP32Bot` with `POTETOBOT_PORT` unset. `%APPDATA%\potetobot\gui.json`, `destination.json`, and `host-terminal.json` were unchanged. Firmware was not rebuilt. `firmware\build\potetobot.bin` is 7519200 bytes. SHA256 is `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`. The factory cap remains 8388608 bytes.

From a clean checkout, create the selection file under `host\build`. That file is not `%APPDATA%\potetobot\destination.json`.

```
C:\Python313\python.exe -c "import json, pathlib; p=pathlib.Path('host/build'); p.mkdir(parents=True, exist_ok=True); p.joinpath('orca-dest.json').write_text(json.dumps({'provider':'orca_bots','botId':'Poteto'})+'\n', encoding='utf-8')"
```

## The frame pack matches the table.

On 2026-11-24 a clean checkout of `1a20bc0` failed `host\build\test_schedule.exe` with `schedule: poteto listening was accepted`. The frame table already counted poteto listening and thinking at 2, while `frames.bin` was still the older 8089636-byte blob and the schedule test still rejected those loops.

`tools\gen_frames.py` rewrote that blob to 7199282 bytes. SHA256 is `19401f0a6ce56bfd05e09209868b9379d05db4f323b84a852ead53734e9ab1bb`. `firmware\main\frame_table.c` did not change. The budget check printed `budget ok 8068962 blob 7199282`. Poteto smile and emphasis stay at 0 packed frames. The rebuilt schedule test printed `schedule ok`. `tests\test_key3.py` printed `key3 ok`. `tests\test_bubble_font.py` printed `pbf1 ok` for the existing 1325432-byte atlas.

`idf.py build` in `firmware` exited 0. `firmware\build\potetobot.bin` is 7519200 bytes. SHA256 is `ea8bb1687f014dc02fbfc2454eb640363f7c195d6483e614d195d9c8213fa6ff`. `check_sizes` reported `0x72bbe0` against `0x800000`, with `0xd4420` bytes free. `esptool image_info` checksum is `61` and the validation hash is `1b61a0a6f1d26abc2ec27ba97d4bfd57781db3604078c6fec8bb1152eef3fb26`. The app version string is `1a20bc0-dirty`. The image contains `AUD v1`, `i2s rx 16000`, and `key3 gpio18 armed`. The bootloader is 22288 bytes and the partition table is 3072 bytes. The factory cap remains 8388608 bytes. This clean rebuild was not flashed. On 2026-11-24 COM5 received the earlier 7519200-byte image, SHA256 `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`. esptool verified all four images, and the rebooted board returned `ACK poteto idle`.

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
| poteto | listening | 2 | 15 | 66 ms |
| poteto | thinking | 2 | 12 | 83 ms |
| poteto | speaking | 4 | 6 | 166 ms |
| poteto | tilt | 4 | 6 | 166 ms |
| poteto | breath | 1 | 6 | 166 ms |
| sunny | idle | 2 | 6 | 166 ms |
| sunny | listening | 1 | 15 | 66 ms |
| sunny | thinking | 1 | 12 | 83 ms |
| sunny | speaking | 4 | 6 | 166 ms |
| sunny | smile | 1 | 6 | 166 ms |

`1000/6` is 166, `1000/15` is 66, and `1000/12` is 83, all integer division.

Voice selects poteto only: listening while KEY3 is held, thinking on release, tilt on `CUE v1 <utt> prepare`, speaking on `CUE v1 <utt> speak`, and idle on `CUE v1 <utt> idle` or `CUE v1 <utt> error`. Sunny is selected only by `SEL`. A cue for another KEY3 press does not change the face.

These names are in the scheduler and are packed with zero frames. The board rejects them. It sends no `ACK`, and the face that is already showing stays.

| Character | Names with zero frames |
| --- | --- |
| poteto | emphasis, smile |
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

`SEL v1 poteto emphasis` and `SEL v1 sunny tilt` match the grammar and have no frames, so they produce no `ACK`. `SEL v1 poteto listening` has two frames.

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

The speaker stays off. KEY3 capture is I2S receive only: 16 kHz, mono, signed 16-bit, in `firmware/main/key3.c`. A capture task reads that stream apart from the 66 ms draw. Full chunks leave as `AUD v1` lines. The serial protocol has no language setting and no walkie-talkie. Words from the peer, and the short answer excerpt, are drawn in the bubble.

## The tree is split this way.

| Path | What it holds |
| --- | --- |
| `firmware/` | ESP-IDF app, `partitions.csv`, and the font image |
| `firmware/main/frames.bin` | Packed RGB565 RLE frames the build embeds |
| `host/peer_main.c` | Windows peer |
| `host/mvp_host.py` | Windows voice entry |
| `host/gui_server.py` | Windows loopback page |
| `assets/frames/` | Source frames, 480×480 |
| `tests/` | Peer and frame-budget checks |
