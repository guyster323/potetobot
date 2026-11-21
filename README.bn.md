# Potetobot

**Grokbot with me anywhere.**

৪৮০ গুণ ৪৮০ এর গোল প্যানেল poteto বা sunny-র একটি মুখ দেখায়, এবং সেই একই ফ্রেমে উত্তরের লেখা একটি বুদবুদে আঁকে।

[English](README.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · **বাংলা**

[দ্রুত শুরু](#দ্রুত-শুরু) · [একটি সিরিয়াল লাইন](#একটি-সিরিয়াল-লাইন-মুখ-বেছে-নেয়) · [প্যাক করা ফ্রেম](#প্যাক-করা-ফ্রেমই-মুখ)

## প্যানেল দুটি মুখ ও একটি বুদবুদ দেখায়।

poteto-র লম্বা কালো চুল, কপালের ছাঁট, গোল সোনালি চশমা, গালের লালিমা, কালো পোশাক, এবং মুখের পাশে ভাপ ওঠা মিষ্টি আলু থাকে। sunny-র ছোট কালো চুল, চোখের উপর বাদামি সানগ্লাস, গালের লালিমা, গাঢ় জিপার, এবং সূর্য আঁকা খবরের কাগজ থাকে। পটভূমি কালো। মুখ ৪৮০ ক্যানভাস ভরে।

| poteto অপেক্ষা, ফ্রেম f00 | sunny অপেক্ষা, ফ্রেম f00 |
| --- | --- |
| ![poteto অপেক্ষা, ফ্রেম f00](assets/frames/poteto/idle/f00.png) | ![sunny অপেক্ষা, ফ্রেম f00](assets/frames/sunny/idle/f00.png) |
| poteto হাসি, ফ্রেম f00 | sunny হাসি, ফ্রেম f00 |
| ![poteto হাসি, ফ্রেম f00](assets/frames/poteto/smile/f00.png) | ![sunny হাসি, ফ্রেম f00](assets/frames/sunny/smile/f00.png) |

এই ছবিগুলো প্যাক করা ফ্রেম। এই রিপোজিটরিতে কোনো ডেমো ভিডিও নেই।

## দ্রুত শুরু

1. **ESP-IDF 5.5 দিয়ে ফার্মওয়্যার বানান।** বেঞ্চের PowerShell-এ:

```
. C:\Esp\esp-idf\export.ps1
cd firmware
idf.py build
```

মুখের প্যাক আগে থেকেই `firmware/main/frames.bin`। সাধারণ বিল্ড সেই ফাইল বসায়। সে চরিত্র আবার আঁকে না। অ্যাপ ইমেজ `firmware/build/potetobot.bin`।

2. **ইমেজ 8388608 বাইটের বড় হলে থামুন।** সেটাই factory পার্টিশন। 2026-11-21 তারিখে বেঞ্চে বানানো ইমেজ 8377584 বাইট, তাই 11024 বাইট বাকি।

3. **বুটলোডার, পার্টিশন টেবিল, অ্যাপ এবং ফন্ট ফ্ল্যাশ করুন।** `firmware\build` থেকে ESP-IDF-এর Python দিয়ে। বেঞ্চের বোর্ড COM5-এ আসে। যে পোর্ট সত্যিই পান সেটি ব্যবহার করুন।

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **লগ যতক্ষণ না বলে যে অ্যাপ `app_main` থেকে ফিরেছে, অপেক্ষা করুন।** তারপর `SEL` বা `TXT` পাঠান।

5. **পিয়ার চালু করে একটি বাছাই, তারপর লেখার একটি লাইন পাঠান।**

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

পিয়ার stdin পড়ে। `host\build\potetobot-peer.exe` না থাকলে আগে বানান। `cl` লাইন পিয়ার অংশে আছে।

## বোর্ডটি Waveshare ESP32-S3-Touch-AMOLED-2.16।

| বিষয় | মান |
| --- | --- |
| লক্ষ্য | ESP32-S3, 240 MHz (`sdkconfig.defaults`) |
| ফ্ল্যাশ | 16 MB। ডিফল্ট ফাইল QIO রাখে। বেঞ্চের লেখায় `--flash_mode dio` এবং `--flash_freq 80m` ছিল |
| PSRAM | অক্টাল, 80 MHz |
| কনসোল | USB Serial/JTAG, 115200 8N1 |
| প্যানেল | 480×480 |
| factory অ্যাপ | অফসেট `0x10000`, আকার `0x800000` (8388608 বাইট) |
| font পার্টিশন | নাম `font`, সাবটাইপ `0x40`, অফসেট `0x810000`, আকার `0x200000` (2097152 বাইট) |
| স্পর্শ | ঠিকানা `0x5A`। SCL GPIO14, SDA GPIO15, ইন্টারাপ্ট GPIO11, রিসেট GPIO40 |

## প্যাক করা ফ্রেমই মুখ।

`play_task` প্রতি 66 ms-এ আবার আঁকে। ফ্রেম সূচক অভিব্যক্তির নিজের ধাপ `1000 / fps`-এ এগোয়।

| চরিত্র | অভিব্যক্তি | ফ্রেম | fps | ধাপ |
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

`1000/6` হলো 166, `1000/15` হলো 66, এবং `1000/12` হলো 83। তিনটিই পূর্ণসংখ্যা ভাগ।

এই নামগুলো শিডিউলারে আছে এবং শূন্য ফ্রেম নিয়ে প্যাক করা। বোর্ড এগুলো ফিরিয়ে দেয়। সে `ACK` পাঠায় না, এবং যে মুখ আগে থেকে দেখা যাচ্ছে সেটি থাকে।

| চরিত্র | শূন্য ফ্রেমের নাম |
| --- | --- |
| poteto | listening, thinking |
| sunny | tilt, breath, emphasis |
| দুজনে | happy, surprised, sleepy, dizzy, wink |

## একটি সিরিয়াল লাইন মুখ বেছে নেয়।

```
SEL v1 poteto idle
SEL v1 sunny smile
```

যে অভিব্যক্তির ফ্রেম প্যাক করা, সে এই আকারে উত্তর দেয়:

```
ACK poteto idle
```

পার্সার শেষের মিলিসেকেন্ড ঘর নেয়, যেমন `SEL v1 poteto speaking 500`। বোর্ড তারপর লুপ প্রথম ফ্রেম থেকে শুরু করে। সেই ঘর অবস্থান খোঁজে না।

`SEL v1 poteto listening` এবং `SEL v1 sunny tilt` ব্যাকরণ মেলে এবং ফ্রেম নেই, তাই `ACK` নেই।

## উত্তরের লেখা সেই একই লাইনে চড়ে।

```
TXT v1 thinking
TXT v1 The potato is hot.
SAY v1 thinking The potato is hot.
SAY v1 answer The potato is hot.
```

| লাইন | পর্যায় | বুদবুদে রাখা লেখা |
| --- | --- | --- |
| `TXT v1 thinking` | thinking | `...` |
| `TXT v1` ও অন্য যেকোনো লেখা | answer | সেই লেখা |
| `SAY v1 thinking` ও লেখা | thinking | সেই লেখা |
| `SAY v1 answer` ও লেখা | answer | সেই লেখা |

বোর্ড `ACK say thinking` বা `ACK say answer` উত্তর দেয়। `POTETO_SAY_TEXT_MAX` শেষের NUL সহ 480 বাইট, তাই রাখা লেখা সর্বোচ্চ 479 বাইট।

বুদবুদের আয়তক্ষেত্র x=12, y=356, প্রস্থ 456, উচ্চতা 112, কোণের ব্যাসার্ধ 14। নিচের পিক্সেল দুবার ডানে সরে, তাই RGB565 চ্যানেল প্রায় চারভাগের একভাগ হয়, এবং সেগুলো মুছে যায় না। কিনারা ও answer গ্লিফ `0xFFFF`। thinking গ্লিফ `0xCE79`। বোর্ড শেষ 8টি বার্তা রাখে। লেখার জানালা 80 পিক্সেল উঁচু এবং প্রতি লাইন 18 পিক্সেল, তাই চার লাইন ধরে। উল্লম্ব টানে রাখা লাইন স্ক্রোল হয়।

পিয়ার যখন `thinking` অভিব্যক্তির `SEL` নেয়, সে `TXT v1 thinking`ও লেখে।

## Windows পিয়ার grok-bot বা orca বলে।

| ড্রাইভার | কখন চলতে রাজি হয় |
| --- | --- |
| `grok-bot` | `%APPDATA%\Grok Bot\desktop-status.json`-এ `"signedIn":true` আছে, এবং সেই ফাইলের pid এখনো চলছে। এটাই ডিফল্ট। |
| `orca` | `Orca.exe` নামের একটি প্রক্রিয়া চলছে। |

```
host\build\potetobot-peer.exe --driver grok-bot
host\build\potetobot-peer.exe --driver orca
```

`--driver`, `POTETOBOT_DRIVER`-এর উপরে। পোর্টের নাম আসে `POTETOBOT_PORT` থেকে। পিয়ার সেটি 115200 8N1-এ খোলে, DTR ও RTS বন্ধ রেখে, তাই পোর্ট খুললে চিপ ডাউনলোড মোডে রিসেট হয় না। সে `SEL` লাইন এবং `TXT v1` দিয়ে শুরু হওয়া লাইন এগিয়ে দেয়। stderr-এ grok-bot ছাপে `driver=grok-bot pid=<pid> signedIn=true`। orca ছাপে `driver=orca`।

রিপোজিটরির মূল থেকে Visual Studio x64 কম্পাইলার দিয়ে পিয়ার বানান:

```
cl /nologo /O2 /I firmware\main /Fe:host\build\potetobot-peer.exe host\peer_main.c firmware\main\schedule.c firmware\main\frame_table.c firmware\main\say.c
```

`host/test_schedule.c` ফ্রেম লুপ পরীক্ষা করে। `host\build\test_schedule.exe` চালান রিপোজিটরির মূল দিয়ে, অথবা আর্গুমেন্ট বাদ দিলে `.` ব্যবহার হয়। speaking পরীক্ষা 6 fps: poteto `(1000/6)*2`-এ `poteto/speaking/f02.png` আশা করে, sunny `(1000/6)*3`-এ `sunny/speaking/f03.png`। `tests/test_peer.py`-এর সাইন-ইন করা Grok Bot লাগে। `tests/test_peer_orca.py`-এর `Orca.exe` লাগে। `tests/test_frame_budget.py` প্যাক করা ব্লবকে factory সীমার সাথে মেলোয়।

## গ্লিফ অ্যাটলাস factory ইমেজের বাইরে থাকে।

`firmware/font/bubble_font.bin` 1325432 বাইট। এটি GNU Unifont 16.0.04-এর একটি অংশ, এবং `firmware/main/ascii8x16.inc` সেই ASCII ফলব্যাক যা `firmware/font/LICENSE.txt` পাশে নাম দেয়। ফ্ল্যাশ লাইন অ্যাটলাস লেখে `0x810000`-এ। সেটি `font` পার্টিশন, 8388608 বাইটের factory অ্যাপের অংশ নয়।

## এই বিল্ড শব্দ বাজায় না।

`firmware/main`-এ অডিও আউটপুট নেই। সিরিয়াল প্রোটোকলে ভাষার সেটিং নেই এবং ওয়াকি-টকি নেই। পিয়ারের কথা বুদবুদে আঁকা হয়।

## গাছ এভাবে ভাগ।

| পথ | যা রাখে |
| --- | --- |
| `firmware/` | ESP-IDF অ্যাপ, `partitions.csv`, এবং ফন্ট ইমেজ |
| `firmware/main/frames.bin` | RGB565 RLE ফ্রেম যা বিল্ড বসায় |
| `host/peer_main.c` | Windows পিয়ার |
| `assets/frames/` | উৎস ফ্রেম, 480×480 |
| `tests/` | পিয়ার ও ফ্রেম-বাজেট পরীক্ষা |
