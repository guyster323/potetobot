# Potetobot

**Grokbot with me anywhere.**

৪৮০ গুণ ৪৮০ এর গোল প্যানেল poteto বা sunny-র একটি মুখ দেখায়, এবং সেই একই ফ্রেমে উত্তরের লেখা একটি বুদবুদে আঁকে।

[English](README.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · **বাংলা**

[দ্রুত শুরু](#দ্রুত-শুরু) · [হোস্ট](#হোস্ট-একটি-orca-বটকে-একবার-জিজ্ঞাসা-করে) · [একটি সিরিয়াল লাইন](#একটি-সিরিয়াল-লাইন-মুখ-বেছে-নেয়) · [প্যাক করা ফ্রেম](#প্যাক-করা-ফ্রেমই-মুখ)

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

2. **ইমেজ 8388608 বাইটের বড় হলে থামুন।** সেটাই factory পার্টিশন। 2026-11-24 তারিখে আবার বানানো ইমেজ 7519200 বাইট, তাই 869408 বাইট বাকি। `check_sizes` জানিয়েছে `0x72bbe0` বনাম `0x800000`, খালি `0xd4420` বাইট। SHA256 হলো `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`। `esptool image_info` চেকসাম `ac` এবং যাচাই হ্যাশ `05873b3f41fdbb860ee18172114cb64205cc64dbe2d57d42a9b7b944e8023270`। সেই দিনের আগের ইমেজ ছিল 7518928 বাইট।

3. **ফ্ল্যাশ মালিকের ধাপ।** এই শেষ কাজ পোর্ট খোলেনি এবং ফ্ল্যাশ করেনি। `firmware\build` থেকে ESP-IDF-এর Python দিয়ে। বুটলোডার 22288 বাইট `0x0`-এ, পার্টিশন টেবিল 3072 বাইট `0x8000`-এ, অ্যাপ `0x10000`-এ, এবং ফন্ট 1325432 বাইট `0x810000`-এ। যে পোর্ট সত্যিই পান সেটি ব্যবহার করুন।

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **বোর্ড পরীক্ষার আগে PC-তে হোস্ট চালু করুন।** সিম ট্রান্সপোর্ট `POTETOBOT_PORT` রাখে না। ডিফল্ট `%APPDATA%\potetobot\destination.json` অনুপস্থিত থাকে। একটি গন্তব্য ফাইল দিন। `C:\Python313\python.exe`-এ whisper, numpy এবং torch আছে। নিচের কমান্ড 2026-11-24-এ একবার চলেছে। আবার চালালে আরেকটি প্রশ্ন যায়।

```
C:\Python313\python.exe host\mvp_host.py --transport sim --wav tests\fixtures\heami-capital.wav --destination host\build\orca-dest.json
```

5. **ফ্ল্যাশের পরে কণ্ঠের মুখ poteto।** KEY3 চেপে রাখলে listening, দুই ফ্রেম, 15 fps। ছাড়লে thinking, দুই ফ্রেম, 12 fps। অনুরোধ অপেক্ষায় `CUE v1 <utt> prepare` tilt বেছে নেয়, চার ফ্রেম, 6 fps। উত্তর এলে `CUE v1 <utt> speak` speaking বেছে নেয়, চার ফ্রেম, 6 fps, এবং ছোট বুদবুদ আঁকে। `CUE v1 <utt> idle` ও `CUE v1 <utt> error` idle বেছে নেয়। স্পর্শে বুদবুদ স্ক্রল করুন। `host\mvp_host.py --transport serial` Win32 পোর্ট API দিয়ে `POTETOBOT_PORT` পড়তে থাকে। `--inject` বোর্ডের বাইট দেয় এবং হ্যান্ডেল খোলে না। pyserial লাগে না। এই শেষ কাজ ফ্ল্যাশ করেনি। অন্য KEY3 চাপের কিউ মুখ বদলায় না। drop, ক্ষতিগ্রস্ত ফ্রেম, হারানো শেষ, খালি গন্তব্য, বট ব্যর্থতা বা টাইমআউট সেই উক্তিতে error দেখায়, এবং পরের ভালো উক্তি চলে। `--inject` ছাড়া সিরিয়াল মুখের সিমুলেটর চালায় না। Win32 লেখা যা 0 ফেরায় বা কোনো বাইট লেখে না, সেটি পাঠানো কিউ নয়, এবং ব্যর্থ পড়া টাইমআউট নয়। এই বার COM পোর্ট খোলা হয়নি।

6. **SEL, TXT এবং SAY পিয়ারেই থাকে।** পিয়ার হাতের লাইন। সেটি কণ্ঠের পথ নয়।

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

পিয়ার stdin পড়ে। `host\build\potetobot-peer.exe` না থাকলে আগে বানান। `cl` লাইন পিয়ার অংশে আছে।

## হোস্ট একটি Orca বটকে একবার জিজ্ঞাসা করে।

`host\mvp_host.py` Windows প্রবেশ। ধাপ 4-এর কমান্ডই চালানো শুরু। `--transport sim` এবং `--transport serial --inject` একই সেশন: AUD বাইট ঢোকে, অনুরোধ আইডিতে `CUE v1 <utt> prepare`, তারপর সেই উত্তরে `CUE v1 <utt> speak`। `--inject` ছাড়া `--transport serial` `POTETOBOT_PORT` থেকে একটি COM হ্যান্ডেল খুলে পড়তে থাকে। pyserial আমদানি হয় না। পিয়ার SEL, TXT, SAY-এর জন্য নিজের হ্যান্ডেল খোলে। সিম WAV কাটে `host\aud_format.exe` দিয়ে, এবং সেই প্রোগ্রাম ফার্মওয়্যারের একই `AUD v1` ফরম্যাট ডাকে। হোস্ট PCM রাখে শুধু যখন উক্তির নম্বর, ক্রম, বাইট দৈর্ঘ্য এবং শেষ লাইন মেলে। ফাঁক, নকল, ছোট hex লাইন, মেশানো লগ, drop, বা হারানো শেষ প্রতিলেখন হয় না। একবারে একটি উক্তি বাঁধা থাকে। দেরির উত্তর দ্বিতীয় পাঠানো শুরু করে না। খারাপ উক্তি সেই নম্বরের `CUE v1 <utt> error` লেখে এবং সেশন চলতে থাকে। এক লাইন 2048 বাইটে থামে। শুরু হওয়া উক্তি সর্বোচ্চ 8 সেকেন্ড এবং সর্বোচ্চ 400টি AUD লাইন অপেক্ষা করে। হারানো শেষ পরের উক্তির সঙ্গে জোড়ে না। Whisper বা অনুরোধ চলাকালীন হোস্ট সেই একই হ্যান্ডেল পড়তে থাকে। সর্বোচ্চ 16টি উক্তি রাখে, প্রতিটি PCM সর্বোচ্চ 192000 বাইট (6.0 সেকেন্ড)। সীমা ছাড়ালে `audio busy` ছাপে এবং `CUE v1 <utt> error` লেখে, এবং পরের উক্তি চলে।

| সীমা | মান |
| --- | --- |
| PCM | 16 kHz, মনো, s16, প্রতি সেকেন্ডে 32000 বাইট |
| একটি ফ্রেম | 640 বাইট, তারপর একটি লাইন `AUD v1 <utt> <seq> <len> <hex>` |
| একটি উক্তি | 192000 বাইট, যা 6.0 সেকেন্ড |
| প্রশ্ন | সর্বোচ্চ 2000 UTF-8 বাইট, `--body-file`-এ |
| বুদবুদ | উত্তরের অংশ, 96 UTF-8 বাইটের কম |
| লাইন | সর্বোচ্চ 2048 বাইট |
| শুরু হওয়া উক্তি | সর্বোচ্চ 8 সেকেন্ড এবং 400টি AUD লাইন |
| অপেক্ষমাণ উক্তি | সর্বোচ্চ 16টি, প্রতিটি PCM সর্বোচ্চ 192000 বাইট (6.0 সেকেন্ড) |

2026-11-24-এর সিম পড়েছে `tests\fixtures\heami-capital.wav` (Microsoft Heami, 16 kHz, মনো, s16)। পুনর্গঠন 151680 বাইট, 238 লাইন। CPU-তে Whisper base-এর প্রতিলেখন `안녕하세요. 대한민국의 수도는 어디인가요?`। একটি অনুরোধ `msg_5f8b8f406d35`। ইনবক্স সারি `msg_789251e073d5` এসেছে `orca_session_id:6176a306-e513-4dff-b903-5bed98beb33d` থেকে এবং তার বডি `서울입니다.`। বুদবুদ সেই একই অংশ। খালি গন্তব্য এবং `grok_bot` `request` ছাপেনি।

পরের `--transport serial --inject` একই WAV ব্যবহার করেছে এবং COM হ্যান্ডেল খোলেনি। PCM ছিল 151680 বাইট। অনুরোধ `msg_bf26672f0e57`। সেই আইডির পরে অপেক্ষার ফ্রেম `poteto/tilt/f02.png`। উত্তর এবং speak বুদবুদ `서울입니다.`। `from` ছিল `orca_session_id:1398eeac-555a-4337-bc0c-86e4e5d49458`।

প্রেরকের হ্যান্ডেল `%APPDATA%\potetobot\host-terminal.json`-এ থাকে। গন্তব্য ফাইল এবং ফার্মওয়্যার চ্যাট আইডি রাখে না। `pyserial` ESP-IDF ভার্চুয়াল পরিবেশে আছে এবং এই সিম সেটি আমদানি করেনি।

নিচের ডিভাইস কমান্ড কোড যে শুরু গ্রহণ করে। এই বার সেটি চালানো হয়নি এবং COM পোর্ট খোলা হয়নি। `C:\Dev\ESP32Bot` থেকে শুরু করুন।

```
$env:POTETOBOT_PORT = "COM5"
C:\Python313\python.exe host\mvp_host.py --transport serial --destination host\build\orca-dest.json
```

`--inject` এবং `--transport sim` পর্যবেক্ষণ সেশন ভাগ করে এবং হ্যান্ডেল খোলে না। ডিফল্ট `%APPDATA%\potetobot\destination.json` অনুপস্থিত থাকে। `host\build\orca-dest.json`-এ আছে `{"provider":"orca_bots","botId":"Poteto"}`। প্রেরক ফাইল `%APPDATA%\potetobot\host-terminal.json` আগে থেকেই আছে এবং একটি `term_` হ্যান্ডেল ধরে। ফাইল না থাকলে হোস্ট `orca terminal create --title potetobot-host --json` চালায় এবং `{"handle":"term_..."}` লেখে। আগে মাপা তৈরি ছিল `orca terminal create --worktree path:C:\Dev\ESP32Bot --title potetobot-host --json` এবং `--focus` দেয়নি। গন্তব্য ফাইল বা ফার্মওয়্যারে চ্যাট আইডি রাখবেন না।

2026-11-24 তারিখে `C:\Dev\ESP32Bot`-এ `POTETOBOT_PORT` ছাড়া `C:\Python313\python.exe host\mvp_host.py --self-test` প্রস্থান 0 দিয়েছে। শেষ লাইন ছিল `self-test ok`। COM পোর্ট খোলেনি এবং নতুন Orca প্রশ্ন যায়নি। এক সেশনের উত্তর ছিল `하나` এবং `둘`। খালি গন্তব্য `channel status not_sent` ছাপে এবং `request` লাইন ছিল না। টাইমআউট `audio timeout 6` ছাপে, এবং পরের উত্তর ছিল `다시`। drop, ক্ষতিগ্রস্ত ফ্রেম এবং হারানো শেষ idle ও বুদবুদ `error` দেখায়, এবং পরের উক্তি চলে। `btn 0 0102`, `btn 1 -`, `btn 0 0506`-এর পরে ট্যাগহীন `CUE v1 speak old` এবং `CUE v1 1 speak old` মুখ listening-এ রাখে এবং বুদবুদ `old` আসে না। `CUE v1 2 prepare` tilt `poteto/tilt/f02.png` দেখায়, এবং `CUE v1 2 speak next` speaking ও বুদবুদ `next` দেখায়। এক চাপ এখনও ট্যাগহীন `CUE v1 prepare` এবং `CUE v1 speak kept` নেয়। Win32 `WriteFile` যা 0 ফেরায়, এবং যা ব্যর্থ হয়, `sent`-এ থাকে না। `CUE v1 1 prepare`-এর 4 বাইট আংশিক লেখা পরের লেখা শেষ করে। মক সিরিয়াল সেশনের প্রথম টুকরো ছিল `CUE `, `speak 하나` জমা হয়নি। জমা কিউ ছিল `CUE v1 3 prepare`, `CUE v1 3 error`, `CUE v1 4 prepare`, এবং `CUE v1 4 speak 둘`। সেই বাইটের পরে `ReadFile` ব্যর্থ হয় এবং সেশন `link read read failed` ছাপিয়ে থামে। ব্যর্থ পড়ায় `readline` `ReadFile` দুইবার ডাকে এবং ঘোরে না। সেই মক পথে মুখের এক্সিকিউটেবল চলেনি। এই চালানো `msg_bf26672f0e57` এবং `msg_5f8b8f406d35` থেকে আলাদা।

সেই সেলফ-টেস্ট তারপর সীমিত ভার্চুয়াল রিসিভ বাফার ব্যবহার করেছে, আগে থেকে সব ইনপুট রাখা InjectLink নয়। দ্বিতীয় PCM 16000 বাইট (0.5 সেকেন্ড) 32437 ফ্রেম বাইট হয়েছে। না পড়া 8192 বাইট বাফার তার মধ্যে 24245 বাইট ফেলেছে। প্রতিলেখন আটকে থাকাকালীন হোস্ট পড়লে ফেলা বাইট 0 এবং দুই উত্তর `하나` ও `둘`। একটি অপেক্ষার স্লটে অতিরিক্ত উক্তি `audio busy` ও error কিউ লেখে, এবং পরের উত্তর `회복`। ফার্মওয়্যার আবার বানানো হয়নি। ইমেজ 7519200 বাইটই আছে। উপরের ডিভাইস কমান্ড বদলায়নি। COM পোর্ট খোলা হয়নি।

`host\gui_server.py` লোকাল লুপ পাতা। 2026-11-24 তারিখে এটি শুধু `127.0.0.1:8765` শুনেছে। এটি COM পোর্ট খোলেনি, ফ্ল্যাশ করেনি, এবং `%APPDATA%\potetobot\destination.json` লেখেনি। পর্দার ভাষা ও কণ্ঠ চেনার ভাষা আলাদা করে `%APPDATA%\potetobot\gui.json`-এ থাকে। দুটোরই মূল কোরিয়ান। ইংরেজি চেনা Whisper-এ যায় `language=en`, `fp16=false`, `temperature=0` হিসেবে। Orca নাম আসে `orca bot list --json` থেকে। সেভ করা একটি নাম তখনই গন্তব্যের `botId` হয় যখন সত্যি প্রশ্ন শুরু হয়। এই বার নতুন Orca প্রশ্ন যায়নি। Grok Bot-এর অ্যাকাউন্ট বট তালিকা বা উত্তরের পথ লেখা নেই। Share থেকে আটকানো id বানিয়েছে `grokbot://app/v1/prompt?id=fromshare&text=hello`। এই লিঙ্ক পাঠায় না। ওয়েবহুকের HTTP 200 গ্রহণ, উত্তর নয়। xAI মডেল API ব্যবহার হয়নি। চরিত্র কার্ড পড়ে `firmware\main\frame_table.c` ও `assets\frames`। বোর্ডে Poteto হল idle 3, listening 2, thinking 2, speaking 4, tilt 4, breath 1। বোর্ডে Sunny হল idle 2, listening 1, thinking 1, speaking 4, smile 1। Poteto-র smile ও emphasis বোর্ডে 0 ফ্রেম, প্রিভিউ PNG 5 ও 3। এই বার ছবি যোগ হয়নি এবং ইমেজ 7519200 বাইটই আছে। Windows রেজিস্ট্রি থেকে `COM5` দেখিয়েছে এবং খোলেনি। Mac ও Linux অ্যাডাপ্টার এই পিসিতে `not_measured` দিয়েছে। `host\gui_server.py --self-test` প্রস্থান 0। ব্রাউজারে একটি ভালো ভার্চুয়াল উক্তি লিখেছে `CUE v1 1 prepare` ও `CUE v1 1 speak virtual`, `whisper language=en` সহ। খারাপ লাইন লিখেছে `CUE v1 error`। দুটোই BoundedKernel।

```
C:\Python313\python.exe host\gui_server.py
```

## শুরু এক সেশন আটকায়।

2026-11-24 তারিখে একই পাতায় শুরু ও থামা এসেছে। শুরু `gui.json` বা `destination.json` লেখে না। পোর্ট, সরবরাহকারী, বাছা বট ও কণ্ঠের ভাষা সেই সেশন প্রক্রিয়ার আর্গুমেন্ট। পোর্ট `POTETOBOT_PORT`-এ কপি হয় না। CLI-তে `--stt-language` বাদ দিলে কোরিয়ান থাকে। POST-এর চাই `Content-Type: application/json`, `Host: 127.0.0.1:<port>`, `Origin: http://127.0.0.1:<port>` ও পাতার টোকেন। ভুল উৎস 403 দেয় এবং কিছু লেখে না। সংযুক্ত থাকাকালীন দ্বিতীয় শুরু 409 দেয়।

সেলফ-টেস্ট সেই শুরুর পথ রাখে। শুধু সিরিয়াল ওপেনার সীমিত ভার্চুয়াল পোর্ট হয় এবং শুধু Orca CLI স্থানীয় ব্যান্ড হয়। একটি উক্তি `bot:Poteto`-কে একটি প্রশ্ন পাঠিয়েছে। `prepare` ও `speak` সেই উক্তির id ভাগ করেছে, এবং উত্তরের লেখা `band-answer`। খালি বট id ও `grok_bot` শূন্য প্রশ্ন পাঠিয়েছে। থামা দেরির speak যোগ করেনি। আসল ওপেনারে `COM99` দেখিয়েছে `serial open failed COM99` এবং হ্যান্ডেল খালি। COM5 খোলা হয়নি।

Windows ব্রাউজারে শুরুর আগে পোর্ট ঘর `COM5` থেকে `COM99` করা হয়েছে। পাতা দেখিয়েছে `Failed · serial open failed COM99` ও খালি হ্যান্ডেল। খোঁজ `zzz-no-such` দেখিয়েছে `No search matches.`। পুরো তালিকা থেকেছে `It'sMe`, `서치린`, `Poteto`, `정리린`, `버그린`, `리뷰린`। ঘর খালি করলে ছয়টি আবার দেখা গেছে। ঘর `Poteto` থাকা অবস্থায় রিফ্রেশ অন্য নাম ফেলেনি। Mint শুধু প্রিভিউ কার্ড: বোর্ড ফ্রেম 0, কণ্ঠ কিউ বন্ধ, SEL বন্ধ, একটি idle PNG। Grok নিয়ন্ত্রণ আসল লিঙ্ক বানিয়েছে `grokbot://app/v1/prompt?id=fromshare&text=hello`, target `_blank`। কপি বোতামকে `Copied` করেছে। সেই লিঙ্কে ক্লিক সেশন শুরু করেনি। কণ্ঠের ভাষা কোরিয়ান হলে ভালো ভার্চুয়াল উক্তি লিখেছে `CUE v1 1 prepare` ও `CUE v1 1 speak 가상`, `openedPort=false`। ত্রুটি উক্তি লিখেছে `CUE v1 1 error` এবং speak লাইন নেই।

এই পিসিতে `os.openpty` নেই। প্যারামিটার পরীক্ষা সকেট জোড়া ও বাইট `sock-byte` ব্যবহার করেছে। Mac বা Linux যন্ত্র মাপা হয়নি। Windows খোলা, পড়া, লেখা ও বন্ধ করা হয়েছে কার্নেল স্ট্যান্ড-ইনসহ `open_serial` দিয়ে: 115200, ডেটা বিট 8, প্যারিটি 0, স্টপ বিট 0, flags 1। কোনো যন্ত্র হ্যান্ডেল তৈরি হয়নি।

`C:\Python313\python.exe host\gui_server.py --self-test` প্রস্থান 0। `C:\Python313\python.exe host\mvp_host.py --self-test` প্রস্থান 0। দুটোই `C:\Dev\ESP32Bot` থেকে চলেছে, `POTETOBOT_PORT` ছাড়া। `%APPDATA%\potetobot\gui.json`, `destination.json`, `host-terminal.json` বদলায়নি। ফার্মওয়্যার আবার বানানো হয়নি। `firmware\build\potetobot.bin` 7519200 বাইট। SHA256 `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`। কারখানার সীমা 8388608 বাইটই আছে।

পরিষ্কার চেকআউটে নির্বাচন ফাইল হয় `host\build`-এর নিচে। সেই ফাইল `%APPDATA%\potetobot\destination.json` নয়।

```
C:\Python313\python.exe -c "import json, pathlib; p=pathlib.Path('host/build'); p.mkdir(parents=True, exist_ok=True); p.joinpath('orca-dest.json').write_text(json.dumps({'provider':'orca_bots','botId':'Poteto'})+'\n', encoding='utf-8')"
```

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

`1000/6` হলো 166, `1000/15` হলো 66, এবং `1000/12` হলো 83। তিনটিই পূর্ণসংখ্যা ভাগ।

কণ্ঠ শুধু poteto বেছে নেয়: KEY3 চাপা থাকলে listening, ছাড়লে thinking, `CUE v1 <utt> prepare`-এ tilt, `CUE v1 <utt> speak`-এ speaking, এবং `CUE v1 <utt> idle` বা `CUE v1 <utt> error`-এ idle। sunny শুধু `SEL` দিয়ে বাছা হয়। অন্য KEY3 চাপের কিউ মুখ বদলায় না।

এই নামগুলো শিডিউলারে আছে এবং শূন্য ফ্রেম নিয়ে প্যাক করা। বোর্ড এগুলো ফিরিয়ে দেয়। সে `ACK` পাঠায় না, এবং যে মুখ আগে থেকে দেখা যাচ্ছে সেটি থাকে।

| চরিত্র | শূন্য ফ্রেমের নাম |
| --- | --- |
| poteto | emphasis, smile |
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

`SEL v1 poteto emphasis` এবং `SEL v1 sunny tilt` ব্যাকরণ মেলে এবং ফ্রেম নেই, তাই `ACK` নেই। `SEL v1 poteto listening`-এর দুটি ফ্রেম আছে।

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

স্পিকার বন্ধ থাকে। KEY3 ক্যাপচার শুধু I2S রিসিভ: 16 kHz, মনো, সাইনড 16-বিট, `firmware/main/key3.c`-এ। একটি ক্যাপচার টাস্ক সেই স্ট্রিম 66 ms আঁকা থেকে আলাদা পড়ে। ভরা টুকরো `AUD v1` লাইনে বেরোয়। সিরিয়াল প্রোটোকলে ভাষার সেটিং নেই এবং ওয়াকি-টকি নেই। পিয়ারের কথা এবং উত্তরের ছোট অংশ বুদবুদে আঁকা হয়।

## গাছ এভাবে ভাগ।

| পথ | যা রাখে |
| --- | --- |
| `firmware/` | ESP-IDF অ্যাপ, `partitions.csv`, এবং ফন্ট ইমেজ |
| `firmware/main/frames.bin` | RGB565 RLE ফ্রেম যা বিল্ড বসায় |
| `host/peer_main.c` | Windows পিয়ার |
| `host/mvp_host.py` | Windows কণ্ঠ প্রবেশ |
| `host/gui_server.py` | Windows লোকাল লুপ পাতা |
| `assets/frames/` | উৎস ফ্রেম, 480×480 |
| `tests/` | পিয়ার ও ফ্রেম-বাজেট পরীক্ষা |
