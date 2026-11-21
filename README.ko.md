# Potetobot

**Grokbot with me anywhere.**

지름 480인 둥근 패널이 poteto 또는 sunny의 얼굴 하나를 보여주고, 같은 화면 위에 답 글을 말풍선으로 그린다.

[English](README.md) · **한국어** · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md)

[빠른 시작](#빠른-시작) · [시리얼 한 줄](#시리얼-한-줄이-얼굴을-고른다) · [팩된 프레임](#팩된-프레임이-얼굴이다)

## 패널은 두 얼굴과 말풍선을 보여 준다.

poteto는 긴 검은 머리, 앞머리, 둥근 금테 안경, 볼터치, 검은 옷, 그리고 얼굴 옆의 김이 나는 고구마를 유지한다. sunny는 짧은 검은 머리, 눈 위의 갈색 선글라스, 볼터치, 어두운 지퍼, 그리고 해가 그려진 신문을 유지한다. 배경은 검다. 얼굴이 480 캔버스를 채운다.

| poteto 대기, 프레임 f00 | sunny 대기, 프레임 f00 |
| --- | --- |
| ![poteto 대기, 프레임 f00](assets/frames/poteto/idle/f00.png) | ![sunny 대기, 프레임 f00](assets/frames/sunny/idle/f00.png) |
| poteto 미소, 프레임 f00 | sunny 미소, 프레임 f00 |
| ![poteto 미소, 프레임 f00](assets/frames/poteto/smile/f00.png) | ![sunny 미소, 프레임 f00](assets/frames/sunny/smile/f00.png) |

이 그림은 팩된 프레임이다. 이 저장소에는 데모 영상이 없다.

## 빠른 시작

1. **ESP-IDF 5.5로 펌웨어를 빌드한다.** 벤치의 PowerShell에서:

```
. C:\Esp\esp-idf\export.ps1
cd firmware
idf.py build
```

얼굴 팩은 이미 `firmware/main/frames.bin`이다. 보통 빌드는 그 파일을 집어넣을 뿐이고, 얼굴을 다시 그리지 않는다. 앱 이미지는 `firmware/build/potetobot.bin`이다.

2. **그 이미지가 8388608바이트보다 크면 멈춘다.** 그 크기가 factory 파티션이다. 2026-11-21에 벤치에서 만든 이미지는 8377584바이트라, 11024바이트가 남는다.

3. **부트로더, 파티션 테이블, 앱, 폰트를 플래시한다.** `firmware\build`에서 ESP-IDF Python으로 한다. 벤치 보드는 COM5로 잡힌다. 실제로 잡힌 포트를 쓴다.

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **로그가 `app_main`에서 돌아왔다고 할 때까지 기다린다.** 그 다음에 `SEL`이나 `TXT`를 보낸다.

5. **피어를 켜고 선택 한 줄, 이어서 글 한 줄을 보낸다.**

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

피어는 stdin을 읽는다. `host\build\potetobot-peer.exe`가 없으면 먼저 빌드한다. `cl` 줄은 피어 절에 있다.

## 보드는 Waveshare ESP32-S3-Touch-AMOLED-2.16이다.

| 항목 | 값 |
| --- | --- |
| 타깃 | ESP32-S3, 240 MHz (`sdkconfig.defaults`) |
| 플래시 | 16 MB. 기본값 파일은 QIO다. 벤치 기록은 `--flash_mode dio`, `--flash_freq 80m` |
| PSRAM | 옥탈, 80 MHz |
| 콘솔 | USB Serial/JTAG, 115200 8N1 |
| 패널 | 480×480 |
| factory 앱 | 오프셋 `0x10000`, 크기 `0x800000` (8388608바이트) |
| font 파티션 | 이름 `font`, 서브타입 `0x40`, 오프셋 `0x810000`, 크기 `0x200000` (2097152바이트) |
| 터치 | 주소 `0x5A`. SCL GPIO14, SDA GPIO15, 인터럽트 GPIO11, 리셋 GPIO40 |

## 팩된 프레임이 얼굴이다.

`play_task`는 66 ms마다 다시 그린다. 프레임 번호는 표정 자신의 간격 `1000 / fps`로 나아간다.

| 캐릭터 | 표정 | 프레임 | fps | 간격 |
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

`1000/6`은 166, `1000/15`는 66, `1000/12`는 83이다. 모두 정수 나눗셈이다.

이 이름은 스케줄러에 있으나 프레임이 0장이다. 보드는 거절한다. `ACK`를 보내지 않고, 이미 나와 있는 얼굴을 바꾸지 않는다.

| 캐릭터 | 프레임이 0장인 이름 |
| --- | --- |
| poteto | listening, thinking |
| sunny | tilt, breath, emphasis |
| 둘 다 | happy, surprised, sleepy, dizzy, wink |

## 시리얼 한 줄이 얼굴을 고른다.

```
SEL v1 poteto idle
SEL v1 sunny smile
```

팩된 표정은 이런 모양으로 답한다.

```
ACK poteto idle
```

파서는 뒤의 밀리초 필드를 받는다. 예는 `SEL v1 poteto speaking 500`이다. 보드는 그 다음 루프를 첫 프레임에서 시작한다. 그 필드는 위치를 찾지 않는다.

`SEL v1 poteto listening`과 `SEL v1 sunny tilt`는 문법은 맞고 프레임이 없어서 `ACK`가 없다.

## 답 글은 그 같은 줄에 탄다.

```
TXT v1 thinking
TXT v1 The potato is hot.
SAY v1 thinking The potato is hot.
SAY v1 answer The potato is hot.
```

| 줄 | 단계 | 말풍선에 저장되는 글 |
| --- | --- | --- |
| `TXT v1 thinking` | thinking | `...` |
| `TXT v1` 뒤에 다른 글 | answer | 그 글 |
| `SAY v1 thinking` 뒤에 글 | thinking | 그 글 |
| `SAY v1 answer` 뒤에 글 | answer | 그 글 |

보드는 `ACK say thinking` 또는 `ACK say answer`로 답한다. `POTETO_SAY_TEXT_MAX`는 끝의 NUL을 포함해 480바이트라, 저장되는 글은 최대 479바이트다.

말풍선 사각형은 x=12, y=356, 너비 456, 높이 112이고, 모서리 반지름은 14다. 그 아래 픽셀은 오른쪽으로 두 번 밀려 RGB565 채널이 약 4분의 1이 되며, 지워지지 않는다. 테두리와 answer 글자는 `0xFFFF`다. thinking 글자는 `0xCE79`다. 보드는 최근 8개 메시지를 유지한다. 글 창은 높이 80픽셀이고 한 줄은 18픽셀이라 네 줄이 들어간다. 세로로 끌면 저장된 줄이 스크롤된다.

피어가 표정이 `thinking`인 `SEL`을 받아들이면 `TXT v1 thinking`도 쓴다.

## Windows 피어는 grok-bot 또는 orca로 말한다.

| 드라이버 | 실행을 허락하는 때 |
| --- | --- |
| `grok-bot` | `%APPDATA%\Grok Bot\desktop-status.json`에 `"signedIn":true`가 있고, 그 파일의 pid가 아직 살아 있다. 이것이 기본이다. |
| `orca` | 프로세스 이미지 이름이 `Orca.exe`인 프로세스가 돌고 있다. |

```
host\build\potetobot-peer.exe --driver grok-bot
host\build\potetobot-peer.exe --driver orca
```

`--driver`가 `POTETOBOT_DRIVER`보다 우선한다. 포트 이름은 `POTETOBOT_PORT`에서 온다. 피어는 115200 8N1로 열고 DTR과 RTS를 끈다. 포트를 열어도 칩이 다운로드 모드로 리셋되지 않는다. `SEL` 줄과 `TXT v1`로 시작하는 줄을 전달한다. stderr에 grok-bot은 `driver=grok-bot pid=<pid> signedIn=true`를 찍고, orca는 `driver=orca`를 찍는다.

저장소 루트에서 Visual Studio x64 컴파일러로 피어를 빌드한다.

```
cl /nologo /O2 /I firmware\main /Fe:host\build\potetobot-peer.exe host\peer_main.c firmware\main\schedule.c firmware\main\frame_table.c firmware\main\say.c
```

`host/test_schedule.c`가 프레임 루프를 검사한다. `host\build\test_schedule.exe`에 저장소 루트를 넘기거나, 인자를 생략하면 `.`를 쓴다. speaking 검사는 6 fps다. poteto는 `(1000/6)*2`에서 `poteto/speaking/f02.png`를 기대하고, sunny는 `(1000/6)*3`에서 `sunny/speaking/f03.png`를 기대한다. `tests/test_peer.py`는 로그인된 Grok Bot이 필요하다. `tests/test_peer_orca.py`는 `Orca.exe`가 필요하다. `tests/test_frame_budget.py`는 팩된 블롭을 factory 한계와 비교한다.

## 글자 아틀라스는 factory 이미지 밖에 있다.

`firmware/font/bubble_font.bin`은 1325432바이트다. GNU Unifont 16.0.04의 부분집합이고, `firmware/main/ascii8x16.inc`는 `firmware/font/LICENSE.txt`가 그 옆에 적은 ASCII 폴백이다. 플래시 줄은 아틀라스를 `0x810000`에 쓴다. 그곳이 `font` 파티션이며, 8388608바이트 factory 앱의 일부가 아니다.

## 이 빌드는 소리를 내지 않는다.

`firmware/main`에는 오디오 출력이 없다. 시리얼 프로토콜에는 언어 설정도 무전기도 없다. 피어에서 온 말은 말풍선에 그려진다.

## 트리는 이렇게 나뉜다.

| 경로 | 들어 있는 것 |
| --- | --- |
| `firmware/` | ESP-IDF 앱, `partitions.csv`, 폰트 이미지 |
| `firmware/main/frames.bin` | 빌드가 집어넣는 RGB565 RLE 프레임 |
| `host/peer_main.c` | Windows 피어 |
| `assets/frames/` | 원본 프레임, 480×480 |
| `tests/` | 피어와 프레임 예산 검사 |
