# TESTLIST

2026-11-21. PBF1 말풍선 폰트와 목적지 파서를 고친 뒤 호스트 테스트와 실기를 다시 봤다. 같은 요청의 왕복 전송은 없다.

| 기능 | 확인 방법 | 결과 | 근거 |
| --- | --- | --- | --- |
| PBF1 헤더와 글리프 | `tests/test_bubble_font.py`가 실제 `firmware/font/bubble_font.bin`을 연다. ASCII, 한글, 범위 밖 오프셋. | 통과 | 종료 코드 0. `pbf1 ok ascii=U+0041 width=8 hangul=U+ACE0 width=16 oor=rejected bytes=1325432`. 손작성 헤더가 아니다. |
| 목적지가 기본으로 비어 있다 | `tests/test_destination.py`. 오버라이드가 없고 `%APPDATA%\potetobot\destination.json`도 없으면 `not_sent`. | 통과 | 종료 코드 0. 출력 `destination ok`. 기본 경로의 `status=not_sent`, `provider`와 `bot_id`는 빈 문자열. `tests/test_peer.py`는 `POTETOBOT_PORT`를 지운 뒤 종료 코드 0이고, 그 테스트는 `driver=grok-bot`과 `signedIn=true`를 요구한다. `tests/test_peer_orca.py` 0. |
| 빈 값은 보내지 않고 양쪽을 시도하지 않는다 | 같은 테스트. 없는 파일, 빈 제공자와 빈 봇 아이디, `grok_bot`에 빈 봇 아이디, 빈 제공자, `grok-bot`, 앞의 빈 키 뒤에 `orca_bots`. | 통과 | 종료 코드 0. 모두 `status=not_sent`. 앞의 빈 `provider`는 뒤의 `orca_bots`로 바뀌지 않았다. 연 경로는 임시 목적지 파일이다. |
| 한 제공자는 다른 쪽을 보지 않는다 | `grok_bot`+`bot-g`와 `orca_bots`+`bot-o`를 두 번씩. `host/destination.c`에 프로세스 조회 문자열이 없는지 검사. | 통과 | 종료 코드 0. 둘 다 `status=blocked`. 다시 읽어도 같다. `desktop-status`, `Orca.exe`, `CreateToolhelp32Snapshot`, `OpenProcess`는 소스에 없다. 제출 경로는 없다. |
| 기본 파일과 오버라이드 | 유효한 기본 파일, 둘 다 있을 때 오버라이드, 없는 오버라이드는 기본 파일로 내려가지 않음. | 통과 | 종료 코드 0. 기본 파일은 `blocked`, `grok_bot`, `from-default`. 오버라이드는 `grok_bot`, `from-override`. 없는 오버라이드는 `not_sent`이고 제공자와 봇 아이디가 비며, 기본 파일의 `orca_bots` / `from-default`가 아니다. |
| 중첩 키, 이스케이프, 잘린 문서, 유니코드 경로 | 중첩 객체보다 최상위 키, `grok\u005fbot`과 `bot\u002do`, 닫히지 않은 JSON, 8192바이트를 넘는 파일, U+0100 경로. | 통과 | 종료 코드 0. 중첩 앞의 문서는 `blocked`, `grok_bot`, `top`. 중첩만 있으면 `not_sent`이고 필드가 비다. 이스케이프는 `grok_bot`, `bot-o`. 닫히지 않은 문서, 뒤에 객체가 더 있는 문서, 9000칸을 붙인 문서는 `not_sent`이고 `grok_bot`이 새지 않는다. U+0100 경로의 파일은 `blocked`, `grok_bot`, `uni`이고 기본 파일로 넘어가지 않았다. |
| 펌웨어 NVS에 목적지가 없다 | `firmware`의 C/H에서 `nvs_flash`, `nvs_open`, `destination`, `botId`, `grok_bot` 검색. | 통과 | 일치 없음. `firmware/partitions.csv`의 nvs 파티션 `0x9000`은 그대로고, 펌웨어 소스는 그 파티션을 열지 않는다. |
| 팩토리 이미지 크기 | `firmware\build\potetobot.bin` 길이. 상한 8388608. | 통과 | 8379376바이트 (`0x7fdbf0`). `check_sizes.py`는 파티션 `0x800000`에서 `0x2410`바이트 남았다고 했다. 빌드 종료 코드 0. `tests/test_frame_budget.py` 종료 코드 0, `budget ok 8100660 blob 8089636`. |
| 플래시 | COM5, `USB\VID_303A&PID_1001`, MAC `44:bd:8d:77:a9:f8`. 지정한 esptool write_flash. | 통과 | 종료 코드 0. `Wrote 8379376 bytes ... Hash of data verified.` 폰트 파일은 이번 변경이 아니고 파티션에 쓴 바이트는 그 파일과 같다. `Wrote 1325432 bytes ... Hash of data verified.` `Hard resetting via RTS pin...` |
| poteto ACK | 115200, DTR 끔, RTS 끔. `SEL v1 poteto idle`, `SEL v1 poteto smile`. | 통과 | 보드: `ACK poteto idle` / `I (3968) potetobot: loop poteto idle` / `ACK poteto smile` / `I (5688) potetobot: loop poteto smile`. |
| sunny ACK | `SEL v1 sunny idle`, `SEL v1 sunny smile`. | 통과 | 보드: `ACK sunny idle` / `I (7388) potetobot: loop sunny idle` / `ACK sunny smile` / `I (9088) potetobot: loop sunny smile`. |
| TXT 말풍선 ACK | `TXT v1 thinking`, `TXT v1 고구마`. | 통과 | 보드: `ACK say thinking` / `I (10798) potetobot: say thinking` / `ACK say answer` / `I (12308) potetobot: say answer`. ACK는 픽셀이 보였다는 증거가 아니다. |
| 얼굴, 고구마, 신문, 말풍선 픽셀 | 부팅 로그와 패널. | 막힘 | 부팅은 `I (3218) bubble: bubble 12,356 456x112 font=partition`이다. `font atlas does not fit`와 `font=missing`은 없다. 유리는 보지 못했다. `Get-PnpDevice`의 Camera와 Image, 그리고 이름에 camera가 있는 PnP 장치는 없었다. 통과로 적지 않는다. |
| 터치 스크롤 | 패널을 손가락으로 밀어야 한다. | 막힘 | 터치를 하지 않았다. 보낸 명령은 SEL과 TXT뿐이다. |
| 같은 요청의 왕복 | 제출과 그 요청의 최종 답 회수. | 막힘 | 전송 코드는 없다. 요청 아이디에 묶인 최종 답은 없다. |
| 접수/진행/최종/실패 | 보드 단계로 만들지 않는다. 호스트 상태는 `not_sent`와 `blocked`만. | 막힘 | 네 구분을 firmware에 넣지 않았다. 왕복이 없으므로 이 행은 통과가 아니다. |

부팅에서 본 줄: `I (1788) main_task: Calling app_main()` 다음 `I (3348) main_task: Returned from app_main()`. `I (1783) app_init: Project name:     potetobot`. `I (1784) app_init: Compile time:     Nov 21 2026 23:43:50`. `I (1784) app_init: ESP-IDF:          v5.5.5`. 이미지에서 `Nov 17 2026`과 `21:30:24`는 없었다.

통과 12. 실패 0. 막힘 4.

## 2026-11-22 KEY3로 ES7210 열기

이전 행의 결과 문장은 그대로다. 아래는 이번 이미지와 이번 실기만 적는다. 심볼만 넣었던 8383840은 이번 크기로 쓰지 않았다.

| 기능 | 확인 방법 | 결과 | 근거 |
| --- | --- | --- | --- |
| KEY3와 ES7210 초기화가 들어간 팩토리 이미지 | `firmware\build\potetobot.bin` 길이. 상한 8388608. | 통과 | 링크 전 8379376. 링크 후 8384992 (`0x7ff1e0`). 상한보다 3616바이트 아래. `check_sizes.py`는 `0xe20`바이트가 남았다고 했다. 빌드 종료 코드 0. 상한을 넘지 않아 되돌리지 않았다. |
| 플래시 | COM5, MAC `44:bd:8d:77:a9:f8`. 앱 `0x10000`, 폰트 `0x810000`. | 통과 | 종료 코드 0. `Wrote 8384992 bytes ... Hash of data verified.` `Wrote 1325432 bytes ... Hash of data verified.` 폰트는 기존 `firmware/font/bubble_font.bin`. |
| 부팅에 마이크를 열지 않음 | 115200, DTR 끔, RTS 펄스 후 부팅 로그. | 통과 | `I (3219) key3: key3 gpio18 armed` 다음 `I (3349) main_task: Returned from app_main()`. 그 로그에 `es7210 open`과 `es7210 absent`는 없다. |
| SEL, TXT v1 | `SEL v1 poteto idle`, `SEL v1 sunny idle`, `TXT v1 thinking`, `TXT v1 고구마`. | 통과 | `ACK poteto idle`, `ACK sunny idle`, `ACK say thinking`, `ACK say answer`. ACK는 픽셀이 보였다는 증거가 아니다. |
| 목적지 | `tests/test_destination.py`. | 통과 | 종료 코드 0. 출력 `destination ok`. 이번 펌웨어 변경은 목적지 파일을 건드리지 않았다. |
| KEY3를 눌렀을 때의 말풍선 | GPIO18을 누르고 유리의 글자를 본다. | 막힘 | 버튼을 누르지 않았다. 유리를 보지 못했다. 이미지 안에 `마이크`와 `마이크 없음` 문자열은 있다. 누르는 결과는 통과로 적지 않는다. |
| 부팅 얼굴, 고구마, 신문 픽셀 | 패널. | 막힘 | 유리를 보지 못했다. |
| 터치 스크롤 | 패널을 손가락으로 민다. | 막힘 | 터치를 하지 않았다. |

이번 추가분 통과 5. 실패 0. 막힘 3.

## 2026-11-22 KEY3 마무리

이전 절의 8384992는 클린 전 이미지다. 이번 크기로 쓰지 않는다. 파티션은 그대로다.

| 기능 | 확인 방법 | 결과 | 근거 |
| --- | --- | --- | --- |
| KEY3가 진행 중인 TXT thinking을 덮지 않음 | `tests/test_key3.py`. thinking, 마이크, answer 순서. | 통과 | 종료 코드 0. 출력 `key3 ok`. 슬롯은 3개이고 첫 칸은 phase `thinking`, 텍스트 `...`이다. thinking 다음에 answer만 넣으면 한 칸으로 바뀌는 대조도 같은 실행에서 통과했다. |
| 정상 ACK, 주소 NACK, SDA 고정, SCL 고정 | 같은 테스트의 가짜 버스. 주소 `0x80`과 레지스터, 데이터 3바이트. | 통과 | 종료 코드 0. 네 경우를 서로 다른 상태로 나눈다. ACK의 캡션은 `마이크`이고 나머지 셋은 `마이크 없음`이다. SDA가 낮게 고정된 경우를 ACK로 세지 않는다. 보드에서 코덱이 ACK한 열기는 버튼을 누르지 않아 보지 못했다. |
| 짧은 탭이 66ms 샘플 사이에 빠지지 않음 | 같은 테스트의 하강 에지. | 통과 | 종료 코드 0. 부팅 첫 샘플은 열지 않는다. 부팅 중 누른 채로 시작했다가 떼고 다시 누르면 한 번 연다. 누르고 있는 동안은 반복하지 않는다. 다시 누름은 한 번이다. 레벨이 아직 1인 짧은 탭은 한 번 열고 반복하지 않는다. 실기 버튼은 누르지 않았다. |
| 팩토리 이미지 | `firmware\build\potetobot.bin`. 상한 8388608. | 통과 | 8387456바이트 (`0x7ffb80`). 상한보다 1152바이트 아래. 빌드의 `check_sizes`는 파티션 `0x800000`에서 `0x480`바이트가 남았다고 했다. 파티션을 늘리지 않았다. |
| 플래시 | COM5, MAC `44:bd:8d:77:a9:f8`. 앱 `0x10000`, 폰트 `0x810000`. | 통과 | 종료 코드 0. `Wrote 8387456 bytes ... Hash of data verified.` `Wrote 1325432 bytes ... Hash of data verified.` 폰트는 새로 만들지 않은 `firmware/font/bubble_font.bin`이다. 디스크 이미지의 ELF SHA256은 `414ef7994641b019…`이고 부팅 로그의 `ELF file SHA256: 414ef7994…`와 같다. `image_info` 세그먼트 길이가 부팅의 `esp_image` 세그먼트와 같다. |
| 부팅에 마이크를 열지 않음 | 115200, DTR 끔, RTS를 연 뒤 0.1초 만에 놓음. | 통과 | `I (3220) bubble: bubble 12,356 456x112 font=partition`. `I (3220) key3: key3 gpio18 armed`. `I (3350) main_task: Returned from app_main()`. 그 로그에 `es7210`은 없다. |
| SEL, TXT v1 | `SEL v1 poteto idle`, `SEL v1 sunny idle`, `TXT v1 thinking`, `TXT v1 고구마`. | 통과 | `ACK poteto idle`, `ACK sunny idle`, `ACK say thinking`, `ACK say answer`. ACK는 픽셀이 보였다는 증거가 아니다. |
| KEY3를 눌렀을 때의 말풍선 | GPIO18을 누르고 유리의 글자를 본다. | 막힘 | 버튼을 누르지 않았다. 유리를 보지 못했다. 보드에서 `es7210 open`은 보지 못했다. |
| 부팅 얼굴, 고구마, 신문 픽셀 | 패널. | 막힘 | 유리를 보지 못했다. |
| 터치 스크롤 | 패널을 손가락으로 민다. | 막힘 | 터치를 하지 않았다. |

이번 추가분 통과 7. 실패 0. 막힘 3.

## 2026-11-23 호스트 시뮬레이션

실기기 포트는 열지 않았다. COM5에 쓰지 않았다. 유리 픽셀은 증거로 쓰지 않는다. 이전 행의 8387456은 이번 이미지가 아니다.

| 기능 | 확인 방법 | 결과 | 근거 |
| --- | --- | --- | --- |
| 포테토 한 얼굴의 버튼 전이 | `tests/test_companion_sim.py`. 호스트 `sim_companion.exe`. | 통과 | `expression idle -> listening -> thinking -> tilt f02 -> speaking f00 -> speaking f02`. 프레임은 `poteto/listening/f00.png`, `poteto/thinking/f00.png`, 332ms에 `poteto/tilt/f02.png`, 0ms `poteto/speaking/f00.png`, 332ms `poteto/speaking/f02.png`. |
| 말풍선은 짧은 글 | 같은 실행. PC에만 둔 긴 문장과 보드 출력을 비교. | 통과 | 말풍선 `알겠어`. 긴 문장 `전체 답은 PC에만 남는다. 이 문장은 봇이 돌려준 답이 아니다.`는 보드 출력에 없었다. |
| 입력이 끝난 바이트가 PC 파서에 도착 | 같은 실행이 `voice_pc.exe`에 MIC 줄을 넘김. | 통과 | 업링크 6바이트 `aabbccdd1122`. PC 출력 `pc aabbccdd1122`. 빈 샘플로 떼면 `mic -`. 이 6바이트는 테스트가 넣은 값이다. 마이크 녹음이 아니다. |
| 봇 회수 | 같은 실행의 `channel`. 제출 함수는 답을 만들지 않는다. | 미충족 | `channel -1`, `answer -`. 한 줄을 넣고 그 요청의 최종 답을 회수하는 경로는 없다. |
| 팩토리 이미지 | `firmware\build\potetobot.bin`. 상한 8388608. | 통과 | 8182960바이트. 남은 205648. 빌드 `check_sizes`는 `0x7cdcb0`, 파티션 `0x800000`에서 `0x32350`바이트가 남았다고 했다. `frames.bin` 7883928. listening RLE 279522, thinking RLE 279640. `tests/test_frame_budget.py`는 `budget ok 8089576 blob 7883928`. 파티션은 그대로다. |
| 고구마와 신문 그림 | 팩 목록과 디스크 파일. | 통과 | 포테토 idle, speaking, tilt와 써니 idle, thinking이 팩에 있다. emphasis PNG와 초안 PNG는 디스크에 있다. emphasis는 팩에서만 빠졌다. |

## 2026-11-24 호스트 왕복과 음성 시뮬레이션

2026-11-22 NextPlan의 「경로 없음」과 2026-11-23의 `channel -1`은 그 날짜의 결과다. 아래는 이번 빌드만 적는다. COM5는 열지 않았고 플래시하지 않았다. 기본 `%APPDATA%\potetobot\destination.json`은 없다.

| 기능 | 확인 방법 | 결과 | 근거 |
| --- | --- | --- | --- |
| 같은 요청의 최종 답 | 세션 변수를 지운 뒤 `orca terminal create`. 접수 `msg_2787a1c295c8`. 수신함 `msg_b3ad688ccb09`. 이어서 `host/ask_main.exe`가 전사문을 한 번 보냄. | 통과 | 첫 본문 `391`. `thread_id`와 `requestId`가 `msg_2787a1c295c8`. `from_handle`은 `orca_session_id:b4c49b90-88f3-4c72-b0a5-9e9d1a4c1957`이고 그 채팅의 `agent read`는 `state done`. 제품 접수는 `msg_2cad93bb6706`, 본문 `391`. 빈 설정과 `grok_bot`은 `request`를 찍지 않았다. |
| 음성 PCM과 전사 | `tests/fixtures/seventeen.wav`. Zira, 16 kHz, mono, s16. Whisper tiny가 PCM 배열을 전사. `voice_pc.exe`가 MIC 줄을 다시 읽음. | 통과 | PCM 91520바이트가 업링크와 PC 출력에서 같았다. 전사 `What is 17 times 23?`. 그 문장이 위 제품 요청이다. 6바이트 표식은 쓰지 않았다. |
| 표정 여러 프레임 | `tests/test_companion_sim.py`. | 통과 | listening 66ms에 f01, 중심 237.4,263.9에서 245.4,263.9. thinking 83ms에 f01, 중심 237.3,263.8에서 237.3,255.8. tilt 332ms에 f02. speaking 332ms에 f02. 말풍선 `391`. |
| 팩토리 이미지 | `firmware\build\potetobot.bin`. | 통과 | 7517184바이트. 남은 871424. `frames.bin` 7199282. `tests/test_frame_budget.py`는 `budget ok 8070706 blob 7199282`. 파티션 `0x10000`,`0x800000`은 그대로다. 이미지에 `i2s rx 16000`이 있다. |
| 고구마와 신문 | 팩 목록과 디스크. | 통과 | 포테토 idle, speaking, tilt, listening, thinking과 써니 idle, thinking이 팩에 있다. emphasis와 smile PNG는 디스크에 있고 팩에는 없다. |

실기기에 남은 것: COM5 플래시, KEY3로 실제 마이크를 녹음하는지, 유리의 표정과 말풍선, 터치 스크롤. Grok Bot 목적지는 여전히 보내지 않는다.

## 2026-11-24 마무리 한국어 왕복과 새 이미지

위 표의 영어 `391`과 이미지 7517184바이트는 그 앞 빌드의 결과다. 아래는 소스를 고친 뒤의 결과만 적는다. COM은 열지 않았고 플래시하지 않았다. 기본 `%APPDATA%\potetobot\destination.json`은 쓰지 않았다. 이 표의 질문은 다시 보내지 않는다.

| 기능 | 확인 방법 | 결과 | 근거 |
| --- | --- | --- | --- |
| 한 시작 경로의 경계 | `C:\Python313\python.exe host\mvp_host.py --self-test` | 통과 | `match ok`, `bind ok`, `self-test ok`. 빈 목적지는 `not_sent`, `grok_bot`은 `blocked`. 둘 다 `request`가 없다. 잘린 줄, 중복 순서, 짧은 hex, 섞인 로그, drop overflow, 용량 초과, 한 발화 바인딩을 거절했다. |
| 한국어 PCM과 같은 요청의 답 | `C:\Python313\python.exe host\mvp_host.py --transport sim --wav tests\fixtures\heami-capital.wav --destination host\build\orca-dest.json` | 통과 | PCM 151680바이트, AUD 238줄, 재조립이 원본과 같았다. Whisper base, CPU, 전사 `안녕하세요. 대한민국의 수도는 어디인가요?`. 요청 `msg_5f8b8f406d35` 한 번. 수신함 `msg_789251e073d5`. `from_handle` `orca_session_id:6176a306-e513-4dff-b903-5bed98beb33d`. 본문과 말풍선 `서울입니다.`. |
| 표정 | 같은 실행의 `sim_companion.exe` | 통과 | listening 66ms에 f01. thinking 83ms에 f01. tilt 332ms에 f02. speaking은 f00 다음 332ms에 f02. 말풍선 `서울입니다.`. `CUE v1 error`는 `error`. `CUE v1 idle`은 말풍선을 비웠다. `sim ok`. 버튼 표본 `aabb`는 전사가 아니다. |
| 팩토리 이미지 | 소스 수정 뒤 `firmware`에서 `idf.py build`. `firmware\build\potetobot.bin`. | 통과 | 7518928바이트. 남은 869680. `check_sizes`는 `0x72bad0`, 여유 `0xd4530`. SHA256 `07a787267d6cddf8a78e8016d68b1774bd2c701bdf57b71bfd2bd9ec34ab9545`. `esptool image_info` 체크섬 `b9`, 검증 해시 `701bce00b5b79931534e777e0efe73717679f8c148b3e80d0f73da2fbcf105d7`. 부트로더 22288바이트 `0x0`. 파티션 테이블 3072바이트 `0x8000`. 앱 `0x10000`. 폰트 1325432바이트 `0x810000`. `frames.bin` 7199282. `tests/test_frame_budget.py`는 `budget ok 8068962 blob 7199282`. 이미지에 `AUD v1`과 `i2s rx 16000`이 있다. 앱 버전 문자열은 `5d27f42-dirty`. 파티션 `0x10000`,`0x800000`은 그대로다. |

실기기에 남은 것: 이 이미지의 플래시, KEY3 캡처, ES7210과 I2S 샘플, 유리의 표정과 말풍선, 터치 스크롤, AUD 줄의 USB 타이밍.

## 2026-11-24 발화 회복과 Win32 실패

위 표의 7518928바이트, `msg_5f8b8f406d35`, `msg_bf26672f0e57`는 그 앞 결과다. 아래는 발화 회복과 Win32 반환 확인을 넣은 뒤의 결과만 적는다. COM은 열지 않았고 플래시하지 않았다. 새 Orca 질문은 보내지 않았다. `POTETOBOT_PORT`는 비웠다.

| 기능 | 확인 방법 | 결과 | 근거 |
| --- | --- | --- | --- |
| drop이 프로세스를 끝내지 않음 | `C:\Python313\python.exe host\mvp_host.py --self-test` | 통과 | 종료 0. 마지막 줄 `self-test ok`. `AUD v1 1 drop queue`는 `("error", 1, "queue")`이고 다음 줄은 발화가 아니다. 줄 3000바이트, 부분 줄 0.2초, AUD 401줄은 각각 길이, 타임아웃, overflow로 끊기고 다음 drop은 따로 회복된다. |
| 두 입력과 답, 손상 뒤 회복 | 같은 실행의 한 세션 | 통과 | 답 `하나`, `둘`. 빈 목적지는 `channel status not_sent`이고 `request`가 없다. `audio timeout 6` 다음 답 `다시`. 보낸 큐는 `CUE v1 error`, `CUE v1 1 error`, `CUE v1 error`, `CUE v1 2 error`, `CUE v1 3 prepare`, `CUE v1 3 speak 하나`, `CUE v1 4 prepare`, `CUE v1 4 speak 둘`, `CUE v1 5 error`, `CUE v1 6 error`, `CUE v1 7 prepare`, `CUE v1 7 speak 다시`. 성공 3, 오류 6. 경과 15초 미만. |
| 늦은 이전 큐 | 같은 실행의 `sim_companion` | 통과 | `btn 0 0102`, `btn 1 -`, `btn 0 0506` 뒤 태그 없는 `CUE v1 speak old`와 `CUE v1 1 speak old`는 listening을 유지하고 말풍선 `old`가 없다. `CUE v1 2 prepare`는 tilt `poteto/tilt/f02.png`. `CUE v1 2 speak next`는 speaking과 말풍선 `next`. 한 번 누름의 태그 없는 `CUE v1 prepare`와 `CUE v1 speak kept`는 그대로 적용된다. |
| Win32 실패와 부분 쓰기 | 같은 실행의 KernelMock. 얼굴 exe 없음 | 통과 | 잘못된 핸들은 닫지 않는다. `GetCommState`와 `SetCommTimeouts` 실패는 핸들 42를 닫는다. `WriteFile` 0과 실패는 `sent`에 없다. 4바이트 짧은 쓰기 다음은 나머지로 `CUE v1 1 prepare`가 완성된다. `ReadFile` 실패는 `readline`이 두 번 부르고 끝난다. 0바이트 성공 읽기는 빈 타임아웃이다. 모의 시리얼 세션의 첫 조각은 `CUE `이고 `speak 하나`는 없다. 저장된 큐는 `CUE v1 3 prepare`, `CUE v1 3 error`, `CUE v1 4 prepare`, `CUE v1 4 speak 둘`. 그 뒤 `link read read failed`로 멈춘다. `face_runs`는 늘지 않았다. |
| 팩토리 이미지 | `firmware`에서 `idf.py build`. `firmware\build\potetobot.bin` | 통과 | 7519200바이트. 남은 869408. `check_sizes`는 `0x72bbe0`, 여유 `0xd4420`. SHA256 `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`. `esptool image_info` 체크섬 `ac`, 검증 해시 `05873b3f41fdbb860ee18172114cb64205cc64dbe2d57d42a9b7b944e8023270`. 부트로더 22288바이트 `0x0`. 파티션 테이블 3072바이트 `0x8000`. 앱 `0x10000`. 폰트 1325432바이트 `0x810000`. `frames.bin` 7199282. 이미지에 `AUD v1`과 `i2s rx 16000`이 있다. 플래시하지 않았다. |

실기기에 남은 것: 이 7519200바이트 이미지의 플래시, KEY3 캡처, ES7210과 I2S 샘플, 유리의 표정과 말풍선, 터치 스크롤, AUD 줄의 USB 타이밍. 호스트 세 결함은 위 셀프 테스트로 확인했다. 실기만 남았는지는 보드를 보기 전에 다시 가른다.

## 2026-11-24 요청 대기 중 수신 배수

정책: Whisper나 Orca 요청이 도는 동안 같은 Win32 핸들의 RX를 별도 스레드가 계속 읽는다. 소프트웨어 큐는 대기 발화 16개, 각 PCM은 192000바이트(6.0초)까지다. 한도를 넘는 발화의 PCM은 저장하지 않고, `audio busy`와 `CUE v1 <utt> error`만 남긴다. 다음 발화는 그대로 돈다. 펌웨어의 busy 잠금은 쓰지 않았다. 발화 ID, 늦은 큐 필터, SEL/TXT/SAY, ReadFile 오류 처리는 그대로다. 펌웨어는 다시 빌드하지 않았다. 이미지는 7519200바이트 그대로다. 실기 시작 명령은 바꾸지 않았다. COM은 열지 않았다.

| 기능 | 확인 방법 | 결과 | 근거 |
| --- | --- | --- | --- |
| 유한 수신에서 둘째 발화 | `C:\Python313\python.exe host\mvp_host.py --self-test`의 BoundedKernel 8192. InjectLink가 아니다. | 통과 | 둘째 PCM 16000바이트(0.5초), 프레임 32437바이트. 아무도 읽지 않으면 24245바이트가 떨어진다. 첫 전사가 막힌 동안 512바이트씩 넣자 떨어진 바이트는 0. 답 `하나`, `둘`. `finite rx ok`. |
| 대기 한도 초과와 회복 | 같은 실행, 대기 슬롯 1 | 통과 | 넘는 발화는 `audio busy 3`과 `CUE v1 3 error`. 다음 답 `회복`. `busy cap ok`. 마지막 줄 `self-test ok`. 종료 0. |
| 잇츠미 주입 | 잇츠미가 수정본에서 `--transport serial --inject` | 통과 | 요청 `msg_44c090720654`, 수신 `msg_6017f249373f`, 본문 `서울입니다.`. 이 표의 셀프 테스트와 따로다. |

## 2026-11-24 호스트 루프백 화면

`host\gui_server.py`는 `127.0.0.1:8765`만 연다. 화면 언어와 음성 인식 언어는 `%APPDATA%\potetobot\gui.json`에 따로 둔다. 기본은 둘 다 한국어다. 펌웨어와 `frames.bin`은 바꾸지 않았다. 이미지는 7519200바이트 그대로다. `%APPDATA%\potetobot\destination.json`은 만들지 않았다. COM은 열지 않았다. 새 Orca 질문은 없다.

| 기능 | 확인 방법 | 결과 | 근거 |
| --- | --- | --- | --- |
| 영어가 Whisper 인수에 닿음 | `C:\Python313\python.exe host\gui_server.py --self-test`와 브라우저의 인식 언어 확인 | 통과 | 기본 호출은 `language=ko`. 영어는 `language=en`, `fp16=false`, `temperature=0`, 텍스트 `virtual`. 가중치는 올리지 않았다. |
| Orca 목록과 저장 | `orca bot list --json`, 브라우저에서 Poteto 선택 후 새로고침 | 통과 | 이름 It'sMe, 서치린, Poteto, 정리린, 버그린, 리뷰린. 검색 `Poteto`는 Poteto만. 가짜 이름은 저장이 400. 저장 파일은 `gui.json`이고 `botId`는 `Poteto`. |
| Grok Bot 경계 | 공식 문서와 화면의 앱 링크 | 통과 | 계정 목록 API와 왕복 경로가 없다. 봇 목록은 비어 있다. `grokbot://app/v1/prompt?id=fromshare&text=hello`는 보내지 않는다. 웹훅 HTTP 200은 답이 아니다. 모델 API로 대체하지 않았다. |
| 캐릭터 | `frame_table.c`와 `assets\frames` 미리보기 | 통과 | 포테토 보드: idle 3, listening 2, thinking 2, speaking 4, tilt 4, breath 1. smile·emphasis는 보드 0, PNG 5·3. 써니 보드: idle 2, listening 1, thinking 1, speaking 4, smile 1. 미리보기는 480×480. 이미지를 펌웨어에 넣지 않았다. |
| 가상 시리얼 | 브라우저에서 정상 발화와 오류 발화. BoundedKernel | 통과 | `CUE v1 1 prepare`, `CUE v1 1 speak virtual`, `whisper language=en`, `ask=local_stub`, `openedPort=false`. 오류는 `CUE v1 error`. COM을 열지 않았다. |
| 다른 OS 포트 | Windows에서 Mac·Linux 어댑터 호출 | 측정 안 함 | Windows는 레지스트리에서 `COM5`를 읽고 열지 않았다. Mac과 Linux는 `not_measured`와 빈 목록. 그 기기는 재지 않았다. |
| 기존 호스트 | `C:\Python313\python.exe host\mvp_host.py --self-test` | 통과 | 종료 0. 마지막 줄 `self-test ok`. |

## 2026-11-24 시작이 세션을 고정한다

시작과 정지는 같은 `host\gui_server.py`다. 포트, 제공자, 고른 봇, 음성 언어는 세션 프로세스 인자다. `POTETOBOT_PORT`에 넣지 않는다. POST는 로컬 Origin과 페이지 토큰이 있어야 한다. 시리얼만 유한 가상 포트로, Orca CLI만 로컬 대역으로 바꿨다. 펌웨어는 다시 빌드하지 않았다. 이미지 7519200바이트, SHA256 `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`. 상한 8388608. COM5는 열지 않았다. `%APPDATA%\potetobot\gui.json`, `destination.json`, `host-terminal.json` 해시는 그대로다.

| 기능 | 확인 방법 | 결과 | 근거 |
| --- | --- | --- | --- |
| 가상 포트로 발화당 1회 | `host\gui_server.py --self-test`의 `prove_session` | 통과 | `bot:Poteto`에 질문 1회, 같은 발화 id의 `prepare`와 `speak`, 답 `band-answer`. 둘째 발화도 1회. 빈 봇과 `grok_bot`은 0회. 정지는 늦은 speak가 없다. 종료 0. |
| COM99 실제 열기 | 같은 테스트와 Windows 브라우저 시작 버튼 | 통과 | `serial open failed COM99`, 핸들 없음. 브라우저 문구 `Failed · serial open failed COM99`. 시작 전에 칸을 COM5에서 COM99로 바꿨다. |
| 출처와 검색 | 잘못된 Origin, 브라우저 검색·새로고침 | 통과 | 잘못된 Origin은 403 `origin`이고 설정을 쓰지 않는다. 중복 시작은 409. 검색 `zzz-no-such`는 결과 없음. 지운 뒤 여섯 이름이 돌아왔다. 검색 중 새로고침도 전체 목록을 유지했다. |
| Grok 링크와 민트 | 브라우저에서 링크 만들기, 복사, 카드 | 통과 | `grokbot://app/v1/prompt?id=fromshare&text=hello`, target `_blank`. 복사 버튼은 `Copied`. 링크 클릭은 세션을 시작하지 않았다. 민트는 보드 0, 음성 큐 없음, SEL 없음. |
| 시리얼 경계 | 소켓 쌍과 커널 대역 | 측정 범위만 통과 | `os.openpty` 없음. 소켓 쌍 `sock-byte`. Mac·Linux 장치는 재지 않았다. 대역 열기는 115200 8N1 flags 1. 장치 핸들 없음. |
| 가상 버튼 | 브라우저 정상·오류 발화 | 통과 | 한국어 인식에서 `CUE v1 1 speak 가상`, `openedPort=false`. 오류는 `CUE v1 1 error`이고 speak가 없다. |
| 호스트 회귀 | `host\mvp_host.py --self-test` | 통과 | 종료 0. 마지막 줄 `self-test ok`. |
