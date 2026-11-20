[English](README.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md) · [한국어](README.ko.md)

# Potetobot

Potetobot은 책상 위에 두는 작은 기기다. 둥근 화면에 얼굴이 있고, 읽을 말이 생기면 말풍선이 나온다. 보드는 Waveshare ESP32-S3-Touch-AMOLED-2.16이다. 화면에는 poteto와 sunny, 두 얼굴이 있다. Windows의 작은 피어 프로그램이 USB 시리얼 한 줄로 표정과 답변 글을 보드에 보낸다.

펌웨어는 목소리로 대답하지 않는다. `firmware/main`에는 오디오 출력이 없다. 보드에는 ES8311 스피커와 마이크가 있지만 이 빌드는 둘 다 쓰지 않는다. 언어 설정 화면도 없고, 워키토키도 없다. 피어가 보낸 글이 화면에 그려진다.

얼굴 룩은 그대로다. 포테토는 고구마를 계속 들고, 써니는 신문과 눈 위의 선글라스를 그대로 둔다.

## 보드

Waveshare ESP32-S3-Touch-AMOLED-2.16(SKU 33969)은 ESP32-S3R8에 8MB PSRAM과 16MB 플래시를 달고 있다. 패널은 480×480 CO5300 AMOLED이고 QSPI로 붙는다. 터치는 CST9217(또는 CST9220)이고 I2C다. SCL은 GPIO14, SDA는 GPIO15, 인터럽트는 GPIO11, 리셋은 GPIO40이다. 전원은 AXP2101이다. 사용자 키는 GPIO18이다.

이 컴퓨터에서 USB Serial/JTAG는 COM5로 잡힌다. 피어는 115200보로 말한다. 포트를 열 때 DTR과 RTS를 끄므로, 피어를 켠다고 칩이 다운로드 모드로 리셋되지 않는다.

factory 앱 파티션은 8388608바이트이고, `0x10000`에서 `0x810000`까지다. 앱 이미지는 이 안에 들어가야 한다. 말풍선 글꼴은 들어가지 않는다. 글꼴은 이름이 `font`인 별도 데이터 파티션이다.

## 얼굴

포테토는 검은 머리, 볼터치, 둥근 금테 안경, 고구마다. 써니는 짧은 검은 머리, 눈 위에 얹힌 갈색 선글라스, 해가 그려진 신문이다. 그림의 기준점은 480×480 캔버스의 x 255, y 271이다. 이 프로젝트는 그 그림을 다시 그리지 않고, 프레임 속도도 바꾸지 않는다.

패킹된 표정과 루프가 쓰는 초당 프레임 수는 다음과 같다.

| 표정 | 초당 프레임 |
| --- | --- |
| idle | 6 |
| listening | 15 |
| thinking | 12 |
| speaking | 6 |
| tilt | 6 |
| breath | 6 |
| emphasis | 6 |
| smile | 6 |

포테토의 listening과 thinking에는 패킹된 프레임이 없다. 보드는 그 두 선택을 거절하고 ACK를 보내지 않으며, 이미 나와 있는 얼굴을 둔다. 써니에게는 listening 프레임과 thinking 프레임이 있다. 둘 다 speaking은 초당 6프레임의 짧은 루프다. 재생 태스크는 약 66밀리초마다 다시 그린다.

## SEL

선택 줄은 캐릭터와 표정을 적는다.

```
SEL v1 poteto idle
SEL v1 sunny speaking
```

표정 뒤에 밀리초 시간을 붙여도 된다. 보드는 한 줄로 답한다.

```
ACK poteto idle
```

피어는 자기 표준 출력에 `poteto idle`을 찍고, 원래 줄을 시리얼로 넘긴다. `grok-bot`과 `orca`가 둘 다 이렇게 한다. 읽는 루프는 하나다.

## SAY와 말풍선

답변 글은 같은 연결의 다음 줄이다.

```
SAY v1 thinking still working
SAY v1 answer hello potato
```

`thinking`은 봇이 아직 생각하는 동안의 글이다. `answer`는 답변이 도착했을 때의 글이다. 줄의 나머지는 UTF-8 텍스트이고, 480바이트까지 남긴다. 보드는 이렇게 답한다.

```
ACK say thinking
ACK say answer
```

`thinking`도 `answer`도 아닌 단계는 거절된다. `SAY` 줄은 얼굴 선택이 아니다.

두 단계 모두 화면에 말풍선을 연다. 사각형은 x = 12, y = 356, 너비 456, 높이 112다. 눈 아래, 선글라스 아래에 있다. 고구마의 윗부분과 신문의 해는 말풍선 위에서 원래 밝기를 유지한다. 말풍선 아래 픽셀은 밝기의 약 4분의 1로 어둡게 한다. 지우지 않는다. 말풍선 꼬리는 없다. 반지름 14의 흰 둥근 테두리가 어두운 영역을 감싼다. 생각 중 글은 부드러운 흰색, 답변 글은 흰색이다.

말풍선에는 네 줄 정도가 들어간다. 더 긴 글은 공백에서 줄바꿈한다. 보드는 최근 메시지 여덟 개를 남긴다. 새 메시지는 가장 최근 줄로 붙는다. 이전 글을 읽으려면 말풍선 안에 손가락을 대고 드래그한다. 손가락을 위로 움직이면 아래쪽 글이 나온다. 스크롤은 가장 오래된 줄과 가장 최근 줄에서 멈춘다. 말풍선 밖에서 시작한 드래그는 스크롤하지 않는다.

말풍선은 디코드된 얼굴 위에 그린 다음 패널로 보낸다. 얼굴, 고구마, 신문, 선글라스는 이전과 같은 그림이다.

## 글꼴

factory 이미지에 남은 공간은 약 1만 5천 바이트라 여러 언어 글꼴이 들어가지 않는다. 글꼴은 GNU Unifont 16.0.04이고, factory 이미지 밖에 둔다.

`tools/gen_bubble_font.py`가 `firmware/font/bubble_font.bin`을 만든다. 이 트리의 파일은 1325432바이트다. 파티션은 `firmware/partitions.csv`에 있다.

```
font, data, 0x40, 0x810000, 0x200000
```

`0x810000`에서 시작하는 2097152바이트이고, factory 앱 바로 다음이다. 펌웨어는 부팅할 때 이 파티션을 매핑한다. 매직이 없으면 앱 안의 작은 Unifont 표로 영문만 그리고, 다른 문자는 그리지 않는다. 출처는 `firmware/font/LICENSE.txt`에 있다. 공개할 파일에 Windows 글꼴을 래스터라이즈해 넣지 않는다. `tools/gen_font.py`는 이 보드가 읽는 글꼴이 아니다.

## 드라이버

`host/peer_main.c`가 피어다. 기본 드라이버는 `grok-bot`이다. `%APPDATA%\Grok Bot\desktop-status.json`을 읽고, `signedIn`이 참이고 적힌 프로세스 번호가 살아 있을 때만 진행한다.

`orca`는 실행 중인 `Orca.exe`를 찾는다. Grok Bot 상태 파일을 읽지 않고, Grok 로그인을 요구하지 않는다.

```
potetobot-peer.exe
potetobot-peer.exe --driver grok-bot
potetobot-peer.exe --driver orca
```

`POTETOBOT_DRIVER`도 같은 이름을 고른다. 둘 다 있으면 `--driver`가 이긴다. `POTETOBOT_PORT`는 보드 포트다. 예는 `COM5`다. 두 드라이버 모두 표준 입력을 읽는다. `SEL` 줄은 얼굴을 바꾸고, `SAY` 줄은 적힌 그대로 넘어간다. 그래서 어느 드라이버로도 생각 중 글과 답변 글을 화면에 올릴 수 있다.

저장소 루트에서 Visual Studio x64 도구로 피어를 만든다.

```
cl /nologo /O2 /I firmware\main /Fe:host\build\potetobot-peer.exe host\peer_main.c firmware\main\schedule.c firmware\main\frame_table.c firmware\main\say.c
```

`host/build/test_schedule.exe`는 표정 루프와 `SAY` 파서를 검사한다. 인자로 저장소 루트를 준다. `tests/test_peer.py`는 로그인된 Grok Bot이 필요하다. `tests/test_peer_orca.py`는 `Orca.exe`가 필요하다.

## 펌웨어 빌드

ESP-IDF 5.5를 쓴다. PowerShell에서:

```
. C:\Esp\esp-idf\export.ps1
cd firmware
idf.py build
```

얼굴 팩은 이미 `firmware/main/frames.bin`이다. 보통 빌드는 그 파일을 임베드하고, 캐릭터를 다시 그리지 않는다. 앱 이미지는 `firmware/build/potetobot.bin`이다. 8388608바이트보다 크면 플래시하지 않는다.

## COM5 플래시

`firmware\build`에서 ESP-IDF의 Python으로:

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

부트로더, 파티션 테이블, 앱은 늘 쓰던 세 이미지다. `bubble_font.bin`은 `0x810000`에 쓰며, 그 자리는 `font` 파티션이고 factory 앱의 일부가 아니다. 하드 리셋 뒤 로그에 앱이 `app_main`에서 돌아왔다고 나올 때까지 기다린 다음 `SEL`이나 `SAY`를 보낸다.
