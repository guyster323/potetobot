# Potetobot

**Grokbot with me anywhere.**

지름 480인 둥근 패널이 poteto 또는 sunny의 얼굴 하나를 보여주고, 같은 화면 위에 답 글을 말풍선으로 그린다.

[English](README.md) · **한국어** · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md)

[빠른 시작](#빠른-시작) · [호스트](#호스트는-orca-봇-하나에-한-번-묻는다) · [시리얼 한 줄](#시리얼-한-줄이-얼굴을-고른다) · [팩된 프레임](#팩된-프레임이-얼굴이다)

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

2. **그 이미지가 8388608바이트보다 크면 멈춘다.** 그 크기가 factory 파티션이다. 2026-11-24에 `1a20bc0`과 KEY3·프레임 소스로 다시 만든 이미지는 7519200바이트라, 869408바이트가 남는다. `check_sizes`는 `0x72bbe0`과 파티션 `0x800000`에서 `0xd4420`바이트가 남았다고 했다. SHA256은 `ea8bb1687f014dc02fbfc2454eb640363f7c195d6483e614d195d9c8213fa6ff`다. `esptool image_info` 체크섬은 `61`이고 검증 해시는 `1b61a0a6f1d26abc2ec27ba97d4bfd57781db3604078c6fec8bb1152eef3fb26`다. 앱 버전 문자열은 `1a20bc0-dirty`다. 같은 크기의 앞 이미지 SHA256은 `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`였다. 이 빌드는 플래시하지 않았다.

3. **플래시는 오너 단계다.** 이번 마무리는 포트를 열지 않았고 플래시하지 않았다. `firmware\build`에서 ESP-IDF Python으로 한다. 부트로더는 22288바이트로 `0x0`, 파티션 테이블은 3072바이트로 `0x8000`, 앱은 `0x10000`, 폰트는 1325432바이트로 `0x810000`이다. 실제로 잡힌 포트를 쓴다.

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **보드 시험 전에 PC에서 호스트를 시작한다.** 시뮬레이션은 `POTETOBOT_PORT`를 넣지 않는다. 기본 `%APPDATA%\potetobot\destination.json`은 비운 채로 둔다. 목적지 파일을 하나 넘긴다. `C:\Python313\python.exe`에는 whisper, numpy, torch가 있다. 아래 명령은 2026-11-24에 한 번 실행했다. 다시 실행하면 질문을 한 번 더 보낸다.

```
C:\Python313\python.exe host\mvp_host.py --transport sim --wav tests\fixtures\heami-capital.wav --destination host\build\orca-dest.json
```

5. **플래시 뒤 음성 얼굴은 poteto다.** KEY3를 누르는 동안 listening 2장, 15 fps다. 떼면 thinking 2장, 12 fps다. 요청을 기다리는 동안 `CUE v1 <utt> prepare`가 tilt 4장, 6 fps를 고른다. 답이 오면 `CUE v1 <utt> speak`가 speaking 4장, 6 fps와 짧은 말풍선을 그린다. `CUE v1 <utt> idle`과 `CUE v1 <utt> error`는 idle이다. 터치는 말풍선을 민다. `host\mvp_host.py --transport serial`은 `POTETOBOT_PORT`를 Win32 포트 API로 계속 읽는다. `--inject`는 보드 바이트를 넣고 핸들을 열지 않는다. pyserial은 쓰지 않는다. 이번 마무리는 플래시하지 않았다. 다른 KEY3 누름의 큐는 얼굴을 바꾸지 않는다. drop, 손상된 프레임, 없는 끝, 빈 목적지, 봇 실패, 타임아웃은 그 발화에 error를 보여주고 다음 정상 발화는 그대로 돈다. `--inject`가 없는 시리얼은 얼굴 시뮬레이터를 실행하지 않는다. 0을 반환하거나 바이트를 쓰지 않은 Win32 쓰기는 보낸 큐로 기록하지 않고, 실패한 읽기는 타임아웃이 아니다. 이번 수정은 COM 포트를 열지 않았다.

6. **SEL, TXT, SAY는 피어로 남긴다.** 피어는 수동 줄이다. 음성 경로가 아니다.

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

피어는 stdin을 읽는다. `host\build\potetobot-peer.exe`가 없으면 먼저 빌드한다. `cl` 줄은 피어 절에 있다.

## 호스트는 Orca 봇 하나에 한 번 묻는다.

`host\mvp_host.py`가 Windows 진입점이다. 4단계 명령이 실행된 시작점이다. `--transport sim`과 `--transport serial --inject`는 같은 세션이다. AUD 바이트를 읽고, 요청 아이디가 찍히면 `CUE v1 <utt> prepare`, 그 답이 오면 `CUE v1 <utt> speak`를 쓴다. `--inject`가 없는 `--transport serial`은 `POTETOBOT_PORT`에서 COM 핸들 하나를 열고 계속 읽는다. pyserial은 가져오지 않는다. 피어는 SEL, TXT, SAY용 핸들을 따로 연다. 시뮬레이션은 WAV를 `host\aud_format.exe`로 자르고, 그 프로그램은 펌웨어와 같은 `AUD v1` 포맷을 부른다. 호스트는 발화 번호, 순서, 바이트 길이, 끝 줄이 맞을 때만 PCM을 남긴다. 빠진 순서, 중복, 짧은 hex, 섞인 로그, drop, 없는 끝은 전사하지 않는다. 한 번에 발화 하나를 묶는다. 늦은 답 때문에 두 번째 질문을 보내지 않는다. 나쁜 발화는 그 번호의 `CUE v1 <utt> error`를 쓰고 세션은 계속된다. 한 줄은 2048바이트에서 멈춘다. 시작된 발화는 최대 8초, AUD 줄은 최대 400개다. 잃어버린 끝은 다음 발화에 붙지 않는다. Whisper나 요청이 도는 동안 호스트는 그 핸들을 계속 읽는다. 대기 발화는 최대 16개, PCM은 각 192000바이트(6.0초)까지다. 한도를 넘으면 `audio busy`를 찍고 `CUE v1 <utt> error`를 쓰며, 다음 발화는 그대로 돈다.

| 한계 | 값 |
| --- | --- |
| PCM | 16 kHz, mono, s16, 초당 32000바이트 |
| 프레임 하나 | 640바이트, 줄은 `AUD v1 <utt> <seq> <len> <hex>` |
| 발화 하나 | 192000바이트, 6.0초 |
| 질문 | UTF-8 `--body-file`로 최대 2000바이트 |
| 말풍선 | 답 발췌, UTF-8 96바이트 미만 |
| 줄 | 최대 2048바이트 |
| 시작된 발화 | 최대 8초, AUD 줄 400개 |
| 대기 발화 | 최대 16개, PCM은 각 192000바이트(6.0초) |

2026-11-24 시뮬레이션은 `tests\fixtures\heami-capital.wav`(Microsoft Heami, 16 kHz, mono, s16)를 읽었다. 재조립은 151680바이트, 238줄이었다. CPU의 Whisper base 전사는 `안녕하세요. 대한민국의 수도는 어디인가요?`였다. 요청은 `msg_5f8b8f406d35` 한 번이었다. 수신함 `msg_789251e073d5`의 `from_handle`은 `orca_session_id:6176a306-e513-4dff-b903-5bed98beb33d`이고 본문은 `서울입니다.`였다. 말풍선도 그 발췌였다. 빈 목적지와 `grok_bot`은 `request`를 찍지 않았다.

이후 `--transport serial --inject`는 같은 WAV를 썼고 COM 핸들은 열지 않았다. PCM은 151680바이트였다. 요청은 `msg_bf26672f0e57`였다. 그 아이디 뒤에 대기 프레임은 `poteto/tilt/f02.png`였다. 답과 speak 말풍선은 `서울입니다.`였다. `from`은 `orca_session_id:1398eeac-555a-4337-bc0c-86e4e5d49458`였다.

발신 핸들은 `%APPDATA%\potetobot\host-terminal.json`에만 있다. 목적지 파일과 펌웨어에는 채팅 아이디를 넣지 않는다. `pyserial`은 ESP-IDF 가상환경에 있고, 이번 시뮬레이션은 가져오지 않았다.

아래 실기 명령은 코드가 받는 시작이다. 이번 수정은 이 명령을 실행하지 않았고 COM 포트를 열지 않았다. `C:\Dev\ESP32Bot`에서 시작한다.

```
$env:POTETOBOT_PORT = "COM5"
C:\Python313\python.exe host\mvp_host.py --transport serial --destination host\build\orca-dest.json
```

`--inject`와 `--transport sim`은 관찰하는 세션을 같이 쓰고 핸들을 열지 않는다. 기본 `%APPDATA%\potetobot\destination.json`은 없는 채로 둔다. `host\build\orca-dest.json`은 `{"provider":"orca_bots","botId":"Poteto"}`다. 발신 파일 `%APPDATA%\potetobot\host-terminal.json`은 이미 있고 `term_` 핸들을 가진다. 그 파일이 없으면 호스트는 `orca terminal create --title potetobot-host --json`을 실행하고 `{"handle":"term_..."}`를 쓴다. 앞서 잰 생성은 `orca terminal create --worktree path:C:\Dev\ESP32Bot --title potetobot-host --json`이었고 `--focus`는 넘기지 않았다. 목적지 파일과 펌웨어에 채팅 아이디를 넣지 않는다.

2026-11-24에 `C:\Dev\ESP32Bot`에서 `POTETOBOT_PORT` 없이 `C:\Python313\python.exe host\mvp_host.py --self-test`가 종료 0이었다. 마지막 줄은 `self-test ok`였다. COM 포트를 열지 않았고 새 Orca 질문을 보내지 않았다. 한 세션의 답은 `하나`와 `둘`이었다. 빈 목적지는 `channel status not_sent`만 찍고 `request` 줄은 없었다. 타임아웃은 `audio timeout 6`을 찍고 다음 발화의 답은 `다시`였다. drop, 손상된 프레임, 없는 끝은 idle과 말풍선 `error`를 보여 주고 다음 발화는 그대로 돌았다. `btn 0 0102`, `btn 1 -`, `btn 0 0506` 뒤에 태그 없는 `CUE v1 speak old`와 `CUE v1 1 speak old`는 listening을 유지하고 말풍선 `old`가 없었다. `CUE v1 2 prepare`는 tilt `poteto/tilt/f02.png`를 보였고, `CUE v1 2 speak next`는 speaking과 말풍선 `next`를 보였다. 한 번 누름은 태그 없는 `CUE v1 prepare`와 `CUE v1 speak kept`를 그대로 받았다. 0을 반환한 Win32 `WriteFile`과 실패한 쓰기는 `sent`에 남지 않았다. `CUE v1 1 prepare`의 4바이트 부분 쓰기는 다음 쓰기가 마저 넣었다. 모의 시리얼 세션의 첫 조각은 `CUE `였고 `speak 하나`는 저장되지 않았다. 저장된 큐는 `CUE v1 3 prepare`, `CUE v1 3 error`, `CUE v1 4 prepare`, `CUE v1 4 speak 둘`이었다. 그 바이트 뒤에 `ReadFile`이 실패해서 세션은 `link read read failed`를 찍고 멈췄다. 실패한 읽기에서 `readline`은 `ReadFile`을 두 번 부르고 돌지 않았다. 이 모의 경로에서는 얼굴 실행 파일을 실행하지 않았다. 이 실행은 `msg_bf26672f0e57` 및 `msg_5f8b8f406d35`와 따로다.

같은 셀프 테스트는 이어서 유한 가상 수신 버퍼를 썼고, 입력을 미리 전부 넣는 InjectLink는 쓰지 않았다. 둘째 PCM 16000바이트(0.5초)의 프레임은 32437바이트였다. 읽지 않은 8192바이트 버퍼는 그중 24245바이트를 버렸다. 전사가 막힌 동안 호스트가 읽자 버린 바이트는 0이었고 두 답은 `하나`와 `둘`이었다. 대기 슬롯을 하나로 두면 넘는 발화는 `audio busy`와 error 큐를 남기고 다음 답은 `회복`이었다. 펌웨어는 다시 빌드하지 않았다. 이미지는 7519200바이트 그대로다. 위 실기 명령은 바꾸지 않았다. COM 포트는 열지 않았다.

`host\gui_server.py`는 루프백 화면이다. 2026-11-24에 `127.0.0.1:8765`만 열었다. COM 포트를 열지 않았고, 플래시하지 않았고, `%APPDATA%\potetobot\destination.json`을 쓰지 않았다. 화면 언어와 음성 인식 언어는 `%APPDATA%\potetobot\gui.json`에 따로 저장된다. 기본은 둘 다 한국어다. 영어 음성 인식은 Whisper에 `language=en`, `fp16=false`, `temperature=0`으로 넘어간다. Orca 이름은 `orca bot list --json`에서 온다. 저장한 이름 하나는 실제 질문을 시작할 때만 목적지의 `botId`가 된다. 이번 확인은 새 Orca 질문을 보내지 않았다. Grok Bot에는 계정 봇 목록이나 답 경로 문서가 없다. Share에서 붙여 넣은 id로 `grokbot://app/v1/prompt?id=fromshare&text=hello`를 만들었다. 그 링크는 보내지 않는다. 웹훅 HTTP 200은 수락이지 답이 아니다. xAI 모델 API는 쓰지 않는다. 캐릭터 카드는 `firmware\main\frame_table.c`와 `assets\frames`를 읽는다. 보드의 포테토는 idle 3, listening 2, thinking 2, speaking 4, tilt 4, breath 1이다. 보드의 써니는 idle 2, listening 1, thinking 1, speaking 4, smile 1이다. 포테토 smile과 emphasis는 보드 프레임 0이고 미리보기 PNG는 5장과 3장이다. 이번 확인은 이미지를 넣지 않았고 이미지는 7519200바이트 그대로다. Windows는 레지스트리에서 `COM5`를 읽었고 열지 않았다. Mac과 Linux 어댑터는 이 PC에서 `not_measured`를 반환했다. `host\gui_server.py --self-test`는 종료 0이었다. 브라우저에서 정상 가상 발화는 `CUE v1 1 prepare`와 `CUE v1 1 speak virtual`을 썼고 `whisper language=en`이었다. 나쁜 줄은 `CUE v1 error`를 썼다. 둘 다 BoundedKernel이다.

```
C:\Python313\python.exe host\gui_server.py
```

## 시작은 세션 하나를 고정한다.

2026-11-24에 같은 화면에 시작과 정지가 생겼다. 시작은 `gui.json`과 `destination.json`을 쓰지 않는다. 포트, 제공자, 고른 봇, 음성 언어는 그 세션 프로세스의 인자다. 포트는 `POTETOBOT_PORT`에 복사되지 않는다. CLI에서 `--stt-language`를 빼면 한국어가 그대로다. POST는 `Content-Type: application/json`, `Host: 127.0.0.1:<port>`, `Origin: http://127.0.0.1:<port>`, 페이지 토큰이 있어야 한다. 출처가 다르면 403이고 아무것도 쓰지 않는다. 연결된 동안 두 번째 시작은 409다.

셀프 테스트는 그 시작 경로를 유지한다. 시리얼 오프너만 유한 가상 포트로, Orca CLI만 로컬 대역으로 바꾼다. 발화 하나는 `bot:Poteto`에 질문을 한 번 보냈다. `prepare`와 `speak`는 그 발화 id를 같이 썼고, 답 글은 `band-answer`였다. 빈 봇 id와 `grok_bot`은 질문을 0번 보냈다. 정지는 늦은 speak를 더하지 않았다. 실제 오프너의 `COM99`는 `serial open failed COM99`와 빈 핸들이었다. COM5는 열지 않았다.

Windows 브라우저에서는 시작 전에 포트 칸을 `COM5`에서 `COM99`로 바꿨다. 화면은 `Failed · serial open failed COM99`와 빈 핸들이었다. 검색 `zzz-no-such`는 `No search matches.`였다. 전체 목록은 `It'sMe`, `서치린`, `Poteto`, `정리린`, `버그린`, `리뷰린`으로 남았다. 칸을 비우자 여섯이 다시 보였다. 칸이 `Poteto`인 채로 새로고침해도 다른 이름은 빠지지 않았다. 민트는 미리보기 카드만이다. 보드 프레임 0, 음성 큐 없음, SEL 없음, idle PNG 하나. Grok 컨트롤은 실제 링크 `grokbot://app/v1/prompt?id=fromshare&text=hello`를 만들었고 target은 `_blank`다. 복사는 버튼을 `Copied`로 바꿨다. 그 링크를 눌러도 세션은 시작하지 않았다. 음성 언어가 한국어일 때 정상 가상 발화는 `CUE v1 1 prepare`와 `CUE v1 1 speak 가상`을 썼고 `openedPort=false`였다. 오류 발화는 `CUE v1 1 error`만 있고 speak 줄은 없었다.

이 PC에는 `os.openpty`가 없다. 매개 확인은 소켓 쌍과 바이트 `sock-byte`였다. Mac이나 Linux 장치는 재지 않았다. Windows 열기, 읽기, 쓰기, 닫기는 커널 대역의 `open_serial`였다. 115200, 데이터 8, 패리티 0, 정지 비트 0, flags 1. 장치 핸들은 만들지 않았다.

`C:\Python313\python.exe host\gui_server.py --self-test`는 종료 0이었다. `C:\Python313\python.exe host\mvp_host.py --self-test`도 종료 0이었다. 둘 다 `C:\Dev\ESP32Bot`에서 `POTETOBOT_PORT` 없이 돌았다. `%APPDATA%\potetobot\gui.json`, `destination.json`, `host-terminal.json`은 그대로다. 펌웨어는 다시 빌드하지 않았다. `firmware\build\potetobot.bin`은 7519200바이트다. SHA256은 `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`이다. 팩토리 상한은 8388608바이트 그대로다.

깨끗한 체크아웃에서 선택 파일은 `host\build` 아래에 만든다. 그 파일은 `%APPDATA%\potetobot\destination.json`이 아니다.

```
C:\Python313\python.exe -c "import json, pathlib; p=pathlib.Path('host/build'); p.mkdir(parents=True, exist_ok=True); p.joinpath('orca-dest.json').write_text(json.dumps({'provider':'orca_bots','botId':'Poteto'})+'\n', encoding='utf-8')"
```

## 프레임 팩은 표와 같다.

2026-11-24에 `1a20bc0`의 깨끗한 체크아웃에서 `host\build\test_schedule.exe`는 `schedule: poteto listening was accepted`로 실패했다. 프레임 표는 포테토 listening과 thinking을 이미 2로 세고, `frames.bin`은 예전 8089636바이트 블롭이었고, 스케줄 테스트는 그 루프를 거절했다.

`tools\gen_frames.py`가 그 블롭을 7199282바이트로 다시 썼다. SHA256은 `19401f0a6ce56bfd05e09209868b9379d05db4f323b84a852ead53734e9ab1bb`다. `firmware\main\frame_table.c`는 바뀌지 않았다. 예산 검사는 `budget ok 8068962 blob 7199282`를 찍었다. 포테토 smile과 emphasis의 팩 프레임은 0이다. 다시 만든 스케줄 테스트는 `schedule ok`를 찍었다. `tests\test_key3.py`는 `key3 ok`, `tests\test_bubble_font.py`는 기존 1325432바이트 아틀라스에 `pbf1 ok`를 찍었다.

`firmware`에서 `idf.py build`는 종료 0이었다. `firmware\build\potetobot.bin`은 7519200바이트다. SHA256은 `ea8bb1687f014dc02fbfc2454eb640363f7c195d6483e614d195d9c8213fa6ff`다. `check_sizes`는 `0x72bbe0`과 `0x800000`에서 `0xd4420`바이트가 남았다고 했다. `esptool image_info` 체크섬은 `61`이고 검증 해시는 `1b61a0a6f1d26abc2ec27ba97d4bfd57781db3604078c6fec8bb1152eef3fb26`다. 앱 버전 문자열은 `1a20bc0-dirty`다. 이미지에 `AUD v1`, `i2s rx 16000`, `key3 gpio18 armed`가 있다. 부트로더는 22288바이트, 파티션 테이블은 3072바이트다. 팩토리 상한은 8388608바이트다. 이 클린 빌드는 플래시하지 않았다. 2026-11-24에 COM5가 받은 이미지는 그 앞의 7519200바이트이고 SHA256은 `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`이다. esptool이 네 이미지를 검증했고, 다시 시작한 보드는 `ACK poteto idle`을 돌려줬다.

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

`1000/6`은 166, `1000/15`는 66, `1000/12`는 83이다. 모두 정수 나눗셈이다.

음성은 poteto만 고른다. KEY3를 누르는 동안 listening, 떼면 thinking, `CUE v1 <utt> prepare`는 tilt, `CUE v1 <utt> speak`는 speaking, `CUE v1 <utt> idle` 또는 `CUE v1 <utt> error`는 idle이다. sunny는 `SEL`로만 고른다. 다른 KEY3 누름의 큐는 얼굴을 바꾸지 않는다.

이 이름은 스케줄러에 있으나 프레임이 0장이다. 보드는 거절한다. `ACK`를 보내지 않고, 이미 나와 있는 얼굴을 바꾸지 않는다.

| 캐릭터 | 프레임이 0장인 이름 |
| --- | --- |
| poteto | emphasis, smile |
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

`SEL v1 poteto emphasis`와 `SEL v1 sunny tilt`는 문법은 맞고 프레임이 없어서 `ACK`가 없다. `SEL v1 poteto listening`은 프레임이 두 장이다.

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

스피커는 꺼져 있다. KEY3 캡처는 I2S 수신만이다. 16 kHz, mono, signed 16-bit이며 `firmware/main/key3.c`에 있다. 캡처 태스크가 그 스트림을 66 ms 그리기와 따로 읽는다. 가득 찬 조각은 `AUD v1` 줄로 나간다. 시리얼 프로토콜에는 언어 설정도 무전기도 없다. 피어에서 온 말과 짧은 답 발췌는 말풍선에 그려진다.

## 트리는 이렇게 나뉜다.

| 경로 | 들어 있는 것 |
| --- | --- |
| `firmware/` | ESP-IDF 앱, `partitions.csv`, 폰트 이미지 |
| `firmware/main/frames.bin` | 빌드가 집어넣는 RGB565 RLE 프레임 |
| `host/peer_main.c` | Windows 피어 |
| `host/mvp_host.py` | Windows 음성 진입점 |
| `host/gui_server.py` | Windows 루프백 화면 |
| `assets/frames/` | 원본 프레임, 480×480 |
| `tests/` | 피어와 프레임 예산 검사 |
