# Potetobot

**Grokbot with me anywhere.**

لوحة دائرية قياسها 480 في 480 تعرض وجهًا واحدًا، poteto أو sunny، وترسم نص الجواب في فقاعة على الإطار نفسه.

[English](README.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · **العربية** · [Français](README.fr.md) · [বাংলা](README.bn.md)

[البداية السريعة](#البداية-السريعة) · [المضيف](#المضيف-يسأل-بوت-orca-واحدًا-مرة-واحدة) · [سطر تسلسلي واحد](#سطر-تسلسلي-واحد-يختار-الوجه) · [الإطارات المعبأة](#الإطارات-المعبأة-هي-الوجه)

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

2. **توقف إذا كانت الصورة أكبر من 8388608 بايت.** هذا حجم قسم factory. الصورة التي أُعيد بناؤها في 2026-11-24 حجمها 7519200 بايت، فيبقى 869408 بايت. `check_sizes` أبلغ `0x72bbe0` مقابل `0x800000`، مع `0xd4420` بايت فارغة. SHA256 هو `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`. مجموع `esptool image_info` هو `ac` وهاش التحقق هو `05873b3f41fdbb860ee18172114cb64205cc64dbe2d57d42a9b7b944e8023270`. صورة أسبق في ذلك اليوم كانت 7518928 بايت.

3. **الحرق خطوة المالك.** هذا الإغلاق لم يفتح منفذًا ولم يحرق. من `firmware\build` ببايثون ESP-IDF. محمّل الإقلاع 22288 بايت عند `0x0`، وجدول الأقسام 3072 بايت عند `0x8000`، والتطبيق عند `0x10000`، والخط 1325432 بايت عند `0x810000`. استخدم المنفذ الذي ظهر فعلًا.

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **شغّل المضيف على الحاسوب قبل اختبار اللوحة.** نقل المحاكاة لا يضبط `POTETOBOT_PORT`. ملف `%APPDATA%\potetobot\destination.json` الافتراضي يبقى غائبًا. مرّر ملف وجهة واحدًا. `C:\Python313\python.exe` فيه whisper وnumpy وtorch. الأمر أدناه اشتغل مرة في 2026-11-24. تشغيله مرة أخرى يرسل سؤالًا آخر.

```
C:\Python313\python.exe host\mvp_host.py --transport sim --wav tests\fixtures\heami-capital.wav --destination host\build\orca-dest.json
```

5. **بعد الحرق، وجه الصوت هو poteto.** أبقِ KEY3 للاستماع listening، إطاران بسرعة 15 fps. أفلته للتفكير thinking، إطاران بسرعة 12 fps. بينما الطلب ينتظر، `CUE v1 <utt> prepare` يختار tilt، أربعة إطارات بسرعة 6 fps. عند وصول الجواب، `CUE v1 <utt> speak` يختار speaking، أربعة إطارات بسرعة 6 fps، ويرسم الفقاعة القصيرة. `CUE v1 <utt> idle` و`CUE v1 <utt> error` يختاران idle. مرّر الفقاعة باللمس. `host\mvp_host.py --transport serial` يواصل قراءة `POTETOBOT_PORT` بواجهة منفذ Win32. `--inject` يمرر بايتات اللوحة ولا يفتح مقبضًا. pyserial غير مستخدم. هذا الإغلاق لم يحرق. إشارة ضغطة KEY3 أخرى لا تغيّر الوجه. الإسقاط أو الإطار التالف أو النهاية الناقصة أو الوجهة الفارغة أو فشل البوت أو انتهاء المهلة يُظهر error لذلك القول، والقول السليم التالي يستمر. التسلسل بلا `--inject` لا يشغّل محاكي الوجه. كتابة Win32 التي تُرجع 0 أو لا تكتب بايتات لا تُسجَّل كإشارة مُرسلة، والقراءة الفاشلة ليست مهلة. هذا المرور لم يفتح منفذ COM.

6. **SEL وTXT وSAY تبقى عبر النظير.** النظير هو السطر اليدوي. ليس مسار الصوت.

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

النظير يقرأ stdin. ابنِه أولًا إذا غاب `host\build\potetobot-peer.exe`. سطر `cl` في قسم النظير.

## المضيف يسأل بوت Orca واحدًا مرة واحدة.

`host\mvp_host.py` هو مدخل وندوز. أمر الخطوة 4 هو البداية التي اشتغلت. `--transport sim` و`--transport serial --inject` يشتركان في الجلسة: بايتات AUD تدخل، و`CUE v1 <utt> prepare` حين يُطبع المعرّف، ثم `CUE v1 <utt> speak` بذلك الجواب. `--transport serial` بلا `--inject` يفتح مقبض COM واحدًا من `POTETOBOT_PORT` ويواصل القراءة. pyserial لا يُستورد. النظير يفتح مقبضه لـ SEL وTXT وSAY. وضع المحاكاة يقطّع WAV بـ `host\aud_format.exe`، وهو يستدعي تنسيق `AUD v1` نفسه الذي في البرنامج الثابت. المضيف يُبقي PCM فقط حين يتفق معرّف القول والتسلسل وطول البايتات وسطر النهاية. فجوة أو تكرار أو سطر hex قصير أو سجل مختلط أو drop أو نهاية ناقصة لا تُنسخ. يُربط قول واحد في كل مرة. الجواب المتأخر لا يبدأ إرسالًا ثانيًا. القول التالف يكتب `CUE v1 <utt> error` لذلك الرقم وتستمر الجلسة. يتوقف السطر عند 2048 بايت. القول الذي بدأ ينتظر 8 ثوانٍ على الأكثر و400 سطر AUD على الأكثر. النهاية المفقودة لا تُلصق بالقول التالي. بينما يعمل Whisper أو الطلب، يواصل المضيف قراءة المقبض نفسه. يخزن 16 قولاً على الأكثر، وكل PCM 192000 بايت (6.0 ثوانٍ) على الأكثر. بعد هذا الحد يطبع `audio busy` ويكتب `CUE v1 <utt> error`، ويستمر القول التالي.

| الحد | القيمة |
| --- | --- |
| PCM | 16 kHz، أحادي، s16، 32000 بايت في كل ثانية |
| إطار واحد | 640 بايت ثم سطر `AUD v1 <utt> <seq> <len> <hex>` |
| قول واحد | 192000 بايت، أي 6.0 ثوان |
| السؤال | 2000 بايت UTF-8 على الأكثر، في `--body-file` |
| الفقاعة | مقتطف الجواب تحت 96 بايت UTF-8 |
| السطر | 2048 بايت على الأكثر |
| القول الذي بدأ | 8 ثوانٍ و400 سطر AUD على الأكثر |
| الأقوال المنتظرة | 16 على الأكثر، وكل PCM 192000 بايت (6.0 ثوانٍ) على الأكثر |

محاكاة 2026-11-24 قرأت `tests\fixtures\heami-capital.wav` (Microsoft Heami، 16 kHz، أحادي، s16). إعادة التجميع كانت 151680 بايت في 238 سطرًا. Whisper base على المعالج نسخ `안녕하세요. 대한민국의 수도는 어디인가요?`. الطلب الوحيد كان `msg_5f8b8f406d35`. صف الوارد `msg_789251e073d5` جاء من `orca_session_id:6176a306-e513-4dff-b903-5bed98beb33d` ونصه `서울입니다.`. الفقاعة كانت ذلك المقتطف نفسه. الوجهة الفارغة و`grok_bot` لم يطبعا `request`.

تشغيل `--transport serial --inject` اللاحق استخدم WAV نفسه ولم يفتح مقبض COM. كان PCM ‏151680 بايت. الطلب `msg_bf26672f0e57`. بعد ذلك المعرّف كان إطار الانتظار `poteto/tilt/f02.png`. الجواب وفقاعة speak كانا `서울입니다.`. كان `from` هو `orca_session_id:1398eeac-555a-4337-bc0c-86e4e5d49458`.

مقبض المرسل يبقى في `%APPDATA%\potetobot\host-terminal.json`. ملف الوجهة والبرنامج الثابت لا يخزنان معرّف دردشة. `pyserial` في البيئة الافتراضية لـ ESP-IDF وهذه المحاكاة لم تستورده.

أمر الجهاز أدناه هو البداية التي يقبلها الكود. هذا المرور لم يشغّله ولم يفتح منفذ COM. ابدأه من `C:\Dev\ESP32Bot`.

```
$env:POTETOBOT_PORT = "COM5"
C:\Python313\python.exe host\mvp_host.py --transport serial --destination host\build\orca-dest.json
```

`--inject` و`--transport sim` يشاركان الجلسة المراقَبة ولا يفتحان مقبضًا. يبقى `%APPDATA%\potetobot\destination.json` الافتراضي غائبًا. يحتوي `host\build\orca-dest.json` على `{"provider":"orca_bots","botId":"Poteto"}`. ملف المرسل `%APPDATA%\potetobot\host-terminal.json` موجود ويحمل مقبض `term_`. إن غاب الملف، يشغّل المضيف `orca terminal create --title potetobot-host --json` ويكتب `{"handle":"term_..."}`. إنشاء قيس من قبل استخدم `orca terminal create --worktree path:C:\Dev\ESP32Bot --title potetobot-host --json` ولم يمرر `--focus`. لا تضع معرّف دردشة في ملف الوجهة أو البرنامج الثابت.

في 2026-11-24، ومن `C:\Dev\ESP32Bot` بلا `POTETOBOT_PORT`، خرج `C:\Python313\python.exe host\mvp_host.py --self-test` بالرمز 0. السطر الأخير كان `self-test ok`. لم يفتح منفذ COM ولم يرسل سؤال Orca جديدًا. جلسة واحدة أجابت `하나` و`둘`. الوجهة الفارغة طبعت `channel status not_sent` وبلا سطر `request`. المهلة طبعت `audio timeout 6`، والقول التالي أجاب `다시`. الإسقاط والإطار التالف والنهاية الناقصة أظهرت idle وفقاعة `error`، واستمر القول التالي. بعد `btn 0 0102` و`btn 1 -` و`btn 0 0506`، تركت `CUE v1 speak old` و`CUE v1 1 speak old` بلا وسم الوجه على listening وبلا فقاعة `old`. أظهر `CUE v1 2 prepare` الميل `poteto/tilt/f02.png`، وأظهر `CUE v1 2 speak next` حالة speaking وفقاعة `next`. ضغطة واحدة ما زالت تقبل `CUE v1 prepare` و`CUE v1 speak kept` بلا وسم. كتابة Win32 عبر `WriteFile` أعادت 0، وأخرى فشلت، ولم تُخزَّنا في `sent`. كتابة جزئية من 4 بايتات لـ `CUE v1 1 prepare` أكملتها الكتابة التالية. في جلسة التسلسل المحاكية كانت القطعة الأولى `CUE `، ولم يُخزَّن `speak 하나`. الإشارات المخزنة كانت `CUE v1 3 prepare` و`CUE v1 3 error` و`CUE v1 4 prepare` و`CUE v1 4 speak 둘`. بعد تلك البايتات فشل `ReadFile` وطبعت الجلسة `link read read failed` وتوقفت. استدعى `readline` عند قراءة فاشلة `ReadFile` مرتين ولم يدر. لم يعمل تنفيذ الوجه على مسار المحاكاة هذا. هذا التشغيل منفصل عن `msg_bf26672f0e57` وعن `msg_5f8b8f406d35`.

اختبار الذات نفسه تابع على مخزن استقبال افتراضي محدود، لا على InjectLink يحمّل كل الدخل مسبقًا. صار PCM ثانٍ من 16000 بايت (0.5 ثانية) 32437 بايت إطار. مخزن 8192 بايت بلا قراءة أسقط 24245 من تلك البايتات. حين واصل المضيف القراءة أثناء النسخ المحجوب، كان عدد البايتات المسقطة 0 والجوابان `하나` و`둘`. مع خانة انتظار واحدة، طبع القول الزائد `audio busy` وإشارة error، وكان الجواب التالي `회복`. لم يُعد بناء البرنامج الثابت. الصورة تبقى 7519200 بايت. أمر الجهاز أعلاه لم يتغير. لم يُفتح منفذ COM.

`host\gui_server.py` صفحة الحلقة المحلية. في 2026-11-24 استمعت إلى `127.0.0.1:8765` فقط. لم تفتح منفذ COM، ولم تحرق، ولم تكتب `%APPDATA%\potetobot\destination.json`. لغة الشاشة ولغة التعرف على الكلام تُحفظان منفصلتين في `%APPDATA%\potetobot\gui.json`. الكورية هي الافتراض للاثنتين. التعرف الإنجليزي يصل إلى Whisper كـ `language=en` مع `fp16=false` و`temperature=0`. أسماء Orca تأتي من `orca bot list --json`. الاسم المحفوظ واحد ويصير `botId` للوجهة فقط حين يبدأ سؤال حقيقي. هذا المرور لم يرسل سؤال Orca جديدًا. Grok Bot بلا قائمة حساب موثّقة وبلا مسار جواب. معرّف ملصوق من Share بنى `grokbot://app/v1/prompt?id=fromshare&text=hello`. هذا الرابط لا يرسل. HTTP 200 من webhook قبول وليس جوابًا. واجهة نماذج xAI غير مستخدمة. بطاقات الشخصية تقرأ `firmware\main\frame_table.c` و`assets\frames`. poteto على اللوحة هو idle 3 وlistening 2 وthinking 2 وspeaking 4 وtilt 4 وbreath 1. sunny على اللوحة هو idle 2 وlistening 1 وthinking 1 وspeaking 4 وsmile 1. smile وemphasis لدى poteto يبقيان 0 إطار على اللوحة، مع 5 و3 صور معاينة. هذا المرور لم يضف صورًا والصورة تبقى 7519200 بايت. Windows سرد `COM5` من السجل ولم يفتحه. محولا Mac وLinux أعادا `not_measured` على هذا الحاسوب. خرج `host\gui_server.py --self-test` بالرمز 0. في المتصفح كتب قول افتراضي سليم `CUE v1 1 prepare` و`CUE v1 1 speak virtual` مع `whisper language=en`. سطر تالف كتب `CUE v1 error`. كلاهما استخدم BoundedKernel.

```
C:\Python313\python.exe host\gui_server.py
```

## البدء يثبت جلسة واحدة.

في 2026-11-24 اكتسبت الصفحة نفسها بدءًا وإيقافًا. البدء لا يكتب `gui.json` ولا `destination.json`. المنفذ والمزوّد والبوت المختار ولغة الكلام وسائط عملية تلك الجلسة. لا يُنسخ المنفذ إلى `POTETOBOT_PORT`. حذف `--stt-language` في سطر الأوامر يبقي الكورية. يحتاج POST إلى `Content-Type: application/json` و`Host: 127.0.0.1:<port>` و`Origin: http://127.0.0.1:<port>` ورمز الصفحة. الأصل الخطأ يعيد 403 ولا يكتب شيئًا. بدء ثانٍ أثناء الاتصال يعيد 409.

اختبار الذات يبقي مسار البدء هذا. يستبدل فقط فاتح المنفذ التسلسلي بمنفذ افتراضي محدود، وواجهة Orca فقط بنطاق محلي. قول واحد أرسل سؤالًا واحدًا إلى `bot:Poteto`. اشترك `prepare` و`speak` في معرّف ذلك القول، وكان نص الجواب `band-answer`. معرّف بوت فارغ و`grok_bot` أرسلا صفر أسئلة. الإيقاف لم يضف speak متأخرًا. `COM99` عبر الفاتح الحقيقي أظهر `serial open failed COM99` وبلا مقبض. لم يُفتح COM5.

في متصفح Windows تغيّر حقل المنفذ من `COM5` إلى `COM99` قبل البدء. أظهرت الصفحة `Failed · serial open failed COM99` ومقبضًا فارغًا. بحث `zzz-no-such` أظهر `No search matches.`. بقيت القائمة الكاملة `It'sMe` و`서치린` و`Poteto` و`정리린` و`버그린` و`리뷰린`. إفراغ الصندوق أعاد الأسماء الستة. التحديث والصندوق يقول `Poteto` لم يسقط الأسماء الأخرى. Mint بطاقة معاينة فقط: 0 إطارات على اللوحة، بلا إشارات صوت، بلا SEL، وصورة idle واحدة. بنى تحكم Grok رابطًا حقيقيًا `grokbot://app/v1/prompt?id=fromshare&text=hello` وهدفه `_blank`. النسخ غيّر الزر إلى `Copied`. نقر ذلك الرابط لم يبدأ جلسة. مع لغة كلام كورية كتب القول الافتراضي السليم `CUE v1 1 prepare` و`CUE v1 1 speak 가상` و`openedPort=false`. كتب قول الخطأ `CUE v1 1 error` وبلا سطر speak.

هذا الحاسوب بلا `os.openpty`. فحص الوسيط استخدم زوج مقابس والبايتات `sock-byte`. لم يُقس جهاز Mac أو Linux. فتح Windows وقراءته وكتابته وإغلاقه مرّت عبر `open_serial` مع بديل للنواة: 115200 و8 بتات بيانات وتكافؤ 0 وبتات توقف 0 وflags 1. لم يُنشأ مقبض جهاز.

خرج `C:\Python313\python.exe host\gui_server.py --self-test` بالرمز 0. وخرج `C:\Python313\python.exe host\mvp_host.py --self-test` بالرمز 0. كلاهما من `C:\Dev\ESP32Bot` بلا `POTETOBOT_PORT`. لم تتغير `%APPDATA%\potetobot\gui.json` و`destination.json` و`host-terminal.json`. لم يُعد بناء البرنامج الثابت. `firmware\build\potetobot.bin` حجمه 7519200 بايت. SHA256 هو `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`. سقف المصنع يبقى 8388608 بايت.

في سحب نظيف يُنشأ ملف الاختيار تحت `host\build`. ذلك الملف ليس `%APPDATA%\potetobot\destination.json`.

```
C:\Python313\python.exe -c "import json, pathlib; p=pathlib.Path('host/build'); p.mkdir(parents=True, exist_ok=True); p.joinpath('orca-dest.json').write_text(json.dumps({'provider':'orca_bots','botId':'Poteto'})+'\n', encoding='utf-8')"
```

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

`1000/6` يساوي 166، و`1000/15` يساوي 66، و`1000/12` يساوي 83. الثلاثة قسمة صحيحة.

الصوت يختار poteto فقط: listening حين يبقى KEY3 مضغوطًا، وthinking عند الإفلات، وtilt مع `CUE v1 <utt> prepare`، وspeaking مع `CUE v1 <utt> speak`، وidle مع `CUE v1 <utt> idle` أو `CUE v1 <utt> error`. sunny يُختار بـ `SEL` فقط. إشارة ضغطة KEY3 أخرى لا تغيّر الوجه.

هذه الأسماء موجودة في المجدول ومعبأة بصفر إطار. اللوحة ترفضها. لا ترسل `ACK`، والوجه الظاهر يبقى.

| الشخصية | أسماء بلا إطارات |
| --- | --- |
| poteto | emphasis, smile |
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

`SEL v1 poteto emphasis` و`SEL v1 sunny tilt` يطابقان النحو وليس لهما إطارات، لذلك لا يوجد `ACK`. `SEL v1 poteto listening` له إطاران.

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

مكبر الصوت يبقى مطفأً. التقاط KEY3 هو استقبال I2S فقط: 16 kHz، أحادي، 16 بت بإشارة، في `firmware/main/key3.c`. مهمة الالتقاط تقرأ ذلك التدفق بمعزل عن الرسم كل 66 ms. القطع الممتلئة تخرج كسطور `AUD v1`. بروتوكول التسلسل بلا إعداد لغة وبلا جهاز لاسلكي. كلمات النظير والمقتطف القصير من الجواب يُرسمان في الفقاعة.

## الشجرة مقسومة هكذا.

| المسار | ما يحمله |
| --- | --- |
| `firmware/` | تطبيق ESP-IDF و`partitions.csv` وصورة الخط |
| `firmware/main/frames.bin` | إطارات RGB565 RLE التي يضمّنها البناء |
| `host/peer_main.c` | نظير وندوز |
| `host/mvp_host.py` | مدخل الصوت في وندوز |
| `host/gui_server.py` | صفحة ويندوز على الحلقة المحلية |
| `assets/frames/` | الإطارات المصدر، 480×480 |
| `tests/` | فحوص النظير وميزانية الإطارات |
