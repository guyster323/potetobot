# Potetobot

**Grokbot with me anywhere.**

لوحة دائرية قياسها 480 في 480 تعرض وجهًا واحدًا، poteto أو sunny، وترسم نص الجواب في فقاعة على الإطار نفسه.

[English](README.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · **العربية** · [Français](README.fr.md) · [বাংলা](README.bn.md)

[البداية السريعة](#البداية-السريعة) · [سطر تسلسلي واحد](#سطر-تسلسلي-واحد-يختار-الوجه) · [الإطارات المعبأة](#الإطارات-المعبأة-هي-الوجه)

## اللوحة تعرض وجهين وفقاعة.

poteto يبقي شعرًا أسود طويلًا وغرة ونظارة ذهبية مستديرة واحمرار خدود وملابس سوداء وبطاطا حلوة يتصاعد منها البخار بجانب الوجه. sunny يبقي شعرًا أسود قصيرًا ونظارة شمسية بنية فوق العينين واحمرار خدود وسحابًا داكنًا وصحيفة مرسوم عليها شمس. الخلفية سوداء. الوجه يملأ لوحة 480.

| poteto في الانتظار، الإطار f00 | sunny في الانتظار، الإطار f00 |
| --- | --- |
| ![poteto في الانتظار، الإطار f00](assets/frames/poteto/idle/f00.png) | ![sunny في الانتظار، الإطار f00](assets/frames/sunny/idle/f00.png) |
| poteto مبتسم، الإطار f00 | sunny مبتسم، الإطار f00 |
| ![poteto مبتسم، الإطار f00](assets/frames/poteto/smile/f00.png) | ![sunny مبتسم، الإطار f00](assets/frames/sunny/smile/f00.png) |

هذه الصور إطارات معبأة. هذا المستودع بلا فيديو عرض.

## البداية السريعة

1. **ابنِ البرنامج الثابت بـ ESP-IDF 5.5.** من PowerShell على منصة الاختبار:

```
. C:\Esp\esp-idf\export.ps1
cd firmware
idf.py build
```

حزمة الوجوه موجودة سلفًا في `firmware/main/frames.bin`. البناء العادي يضمّن هذا الملف. هو لا يعيد رسم الشخصيات. صورة التطبيق هي `firmware/build/potetobot.bin`.

2. **توقف إذا كانت الصورة أكبر من 8388608 بايت.** هذا حجم قسم factory. صورة المنصة التي بُنيت في 2026-11-21 حجمها 8377584 بايت، فيبقى 11024 بايت.

3. **احرق محمّل الإقلاع وجدول الأقسام والتطبيق والخط.** من `firmware\build` ببايثون ESP-IDF. لوحة المنصة تظهر على COM5. استخدم المنفذ الذي ظهر فعلًا.

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **انتظر حتى يقول السجل إن التطبيق عاد من `app_main`.** ثم أرسل `SEL` أو `TXT`.

5. **شغّل النظير وأرسل اختيارًا ثم سطر نص.**

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

النظير يقرأ stdin. ابنِه أولًا إذا غاب `host\build\potetobot-peer.exe`. سطر `cl` في قسم النظير.

## اللوحة Waveshare ESP32-S3-Touch-AMOLED-2.16.

| البند | القيمة |
| --- | --- |
| الهدف | ESP32-S3 بتردد 240 MHz (`sdkconfig.defaults`) |
| الذاكرة الوميضية | 16 MB. ملف الإعدادات يضبط QIO. كتابة المنصة استخدمت `--flash_mode dio` و`--flash_freq 80m` |
| PSRAM | ثماني، 80 MHz |
| الطرفية | USB Serial/JTAG، 115200 8N1 |
| اللوحة | 480×480 |
| تطبيق factory | الإزاحة `0x10000`، الحجم `0x800000` (8388608 بايت) |
| قسم font | الاسم `font`، النوع الفرعي `0x40`، الإزاحة `0x810000`، الحجم `0x200000` (2097152 بايت) |
| اللمس | العنوان `0x5A`. SCL على GPIO14، وSDA على GPIO15، والمقاطعة على GPIO11، وإعادة الضبط على GPIO40 |

## الإطارات المعبأة هي الوجه.

`play_task` يعيد الرسم كل 66 ms. فهرس الإطار يتقدم بخطوة التعبير نفسه، `1000 / fps`.

| الشخصية | التعبير | الإطارات | fps | الخطوة |
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

`1000/6` يساوي 166، و`1000/15` يساوي 66، و`1000/12` يساوي 83. الثلاثة قسمة صحيحة.

هذه الأسماء موجودة في المجدول ومعبأة بصفر إطار. اللوحة ترفضها. لا ترسل `ACK`، والوجه الظاهر يبقى.

| الشخصية | أسماء بلا إطارات |
| --- | --- |
| poteto | listening, thinking |
| sunny | tilt, breath, emphasis |
| الاثنان | happy, surprised, sleepy, dizzy, wink |

## سطر تسلسلي واحد يختار الوجه.

```
SEL v1 poteto idle
SEL v1 sunny smile
```

التعبير المعبأ يجيب بهذا الشكل:

```
ACK poteto idle
```

المحلّل يقبل حقل ملي ثانية في الآخر، مثل `SEL v1 poteto speaking 500`. اللوحة بعد ذلك تبدأ الحلقة من الإطار الأول. هذا الحقل لا يقفز إلى موضع.

`SEL v1 poteto listening` و`SEL v1 sunny tilt` يطابقان النحو وليس لهما إطارات، لذلك لا يوجد `ACK`.

## نص الجواب يركب الخط نفسه.

```
TXT v1 thinking
TXT v1 The potato is hot.
SAY v1 thinking The potato is hot.
SAY v1 answer The potato is hot.
```

| السطر | الطور | النص المحفوظ في الفقاعة |
| --- | --- | --- |
| `TXT v1 thinking` | thinking | `...` |
| `TXT v1` مع أي نص آخر | answer | ذلك النص |
| `SAY v1 thinking` مع نص | thinking | ذلك النص |
| `SAY v1 answer` مع نص | answer | ذلك النص |

اللوحة تجيب `ACK say thinking` أو `ACK say answer`. `POTETO_SAY_TEXT_MAX` هو 480 بايتًا مع NUL الختامي، فالنص المحفوظ 479 بايتًا على الأكثر.

مستطيل الفقاعة هو x=12 وy=356 والعرض 456 والارتفاع 112، ونصف قطر الزاوية 14. البكسلات تحتها تُزاح يمينًا مرتين، فيصير كل قناة RGB565 نحو الربع، وهي لا تُمسح. الحافة ورموز answer هي `0xFFFF`. رموز thinking هي `0xCE79`. اللوحة تحتفظ بآخر 8 رسائل. نافذة النص ارتفاعها 80 بكسلًا وكل سطر 18 بكسلًا، فتتسع أربعة أسطر. السحب الرأسي يمرر الأسطر المحفوظة.

حين يقبل النظير `SEL` تعبيره `thinking`، يكتب أيضًا `TXT v1 thinking`.

## نظير وندوز يتكلم grok-bot أو orca.

| المشغّل | متى يوافق على العمل |
| --- | --- |
| `grok-bot` | `%APPDATA%\Grok Bot\desktop-status.json` يحوي `"signedIn":true`، وpid ذلك الملف ما زال يعمل. هذا هو الافتراضي. |
| `orca` | عملية صورتها `Orca.exe` تعمل. |

```
host\build\potetobot-peer.exe --driver grok-bot
host\build\potetobot-peer.exe --driver orca
```

`--driver` يغلب `POTETOBOT_DRIVER`. اسم المنفذ يأتي من `POTETOBOT_PORT`. النظير يفتحه على 115200 8N1 مع إطفاء DTR وRTS، ففتح المنفذ لا يعيد ضبط الشريحة إلى وضع التنزيل. هو يمرر أسطر `SEL` والأسطر التي تبدأ بـ `TXT v1`. على stderr يطبع grok-bot `driver=grok-bot pid=<pid> signedIn=true`. ويطبع orca `driver=orca`.

ابنِ النظير من جذر المستودع بمترجم Visual Studio x64:

```
cl /nologo /O2 /I firmware\main /Fe:host\build\potetobot-peer.exe host\peer_main.c firmware\main\schedule.c firmware\main\frame_table.c firmware\main\say.c
```

`host/test_schedule.c` يفحص حلقة الإطارات. شغّل `host\build\test_schedule.exe` مع جذر المستودع، أو بلا وسيط لاستخدام `.`. فحوص speaking تستخدم 6 fps: poteto عند `(1000/6)*2` يتوقع `poteto/speaking/f02.png`، وsunny عند `(1000/6)*3` يتوقع `sunny/speaking/f03.png`. `tests/test_peer.py` يحتاج Grok Bot مسجّل الدخول. `tests/test_peer_orca.py` يحتاج `Orca.exe`. `tests/test_frame_budget.py` يقارن الكتلة المعبأة بحد factory.

## أطلس الرموز يبقى خارج صورة factory.

`firmware/font/bubble_font.bin` حجمه 1325432 بايتًا. هو مجموعة جزئية من GNU Unifont 16.0.04، و`firmware/main/ascii8x16.inc` هو احتياط ASCII الذي يسميه `firmware/font/LICENSE.txt` إلى جانبه. سطر الحرق يكتب الأطلس عند `0x810000`. ذلك قسم `font`، وليس جزءًا من تطبيق factory ذي 8388608 بايت.

## هذا البناء لا يشغّل صوتًا.

`firmware/main` بلا خرج صوتي. بروتوكول التسلسل بلا إعداد لغة وبلا جهاز لاسلكي. كلمات النظير تُرسم في الفقاعة.

## الشجرة مقسومة هكذا.

| المسار | ما يحمله |
| --- | --- |
| `firmware/` | تطبيق ESP-IDF و`partitions.csv` وصورة الخط |
| `firmware/main/frames.bin` | إطارات RGB565 RLE التي يضمّنها البناء |
| `host/peer_main.c` | نظير وندوز |
| `assets/frames/` | الإطارات المصدر، 480×480 |
| `tests/` | فحوص النظير وميزانية الإطارات |
