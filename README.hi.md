# Potetobot

**Grokbot with me anywhere.**

480 गुणा 480 का गोल पैनल poteto या sunny का एक चेहरा दिखाता है, और उसी फ्रेम पर जवाब का पाठ एक भाषण बुलबुले में बनाता है।

[English](README.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · **हिन्दी** · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md)

[त्वरित शुरुआत](#त्वरित-शुरुआत) · [होस्ट](#होस्ट-एक-orca-बॉट-से-एक-बार-पूछता-है) · [एक सीरियल पंक्ति](#एक-सीरियल-पंक्ति-चेहरा-चुनती-है) · [पैक किए फ्रेम](#पैक-किए-फ्रेम-ही-चेहरा-हैं)

## पैनल दो चेहरे और एक बुलबुला दिखाता है।

poteto के लंबे काले बाल, माथे पर लटें, गोल सुनहरे चश्मा, गालों पर लालिमा, काले कपड़े, और चेहरे के पास भाप छोड़ता शकरकंद रहता है। sunny के छोटे काले बाल, आँखों पर भूरे धूप के चश्मे, गालों पर लालिमा, गहरा ज़िप, और सूरज बना अखबार रहता है। पृष्ठभूमि काली है। चेहरा 480 के कैनवास को भरता है।

| poteto प्रतीक्षा, फ्रेम f00 | sunny प्रतीक्षा, फ्रेम f00 |
| --- | --- |
| ![poteto प्रतीक्षा, फ्रेम f00](assets/frames/poteto/idle/f00.png) | ![sunny प्रतीक्षा, फ्रेम f00](assets/frames/sunny/idle/f00.png) |
| poteto मुस्कान, फ्रेम f00 | sunny मुस्कान, फ्रेम f00 |
| ![poteto मुस्कान, फ्रेम f00](assets/frames/poteto/smile/f00.png) | ![sunny मुस्कान, फ्रेम f00](assets/frames/sunny/smile/f00.png) |

ये चित्र पैक किए फ्रेम हैं। इस रिपॉजिटरी में कोई डेमो वीडियो नहीं है।

## त्वरित शुरुआत

1. **ESP-IDF 5.5 से फ़र्मवेयर बनाएँ।** बेंच के PowerShell में:

```
. C:\Esp\esp-idf\export.ps1
cd firmware
idf.py build
```

चेहरे का पैक पहले से `firmware/main/frames.bin` है। सामान्य बिल्ड उस फ़ाइल को जोड़ता है। वह पात्रों को फिर से नहीं रंगता। ऐप छवि `firmware/build/potetobot.bin` है।

2. **अगर वह छवि 8388608 बाइट से बड़ी हो तो रुकें।** यही factory विभाजन है। 2026-11-24 को दोबारा बनी छवि 7519200 बाइट है, इसलिए 869408 बाइट बचते हैं। `check_sizes` ने `0x72bbe0` को `0x800000` से मिलाया और `0xd4420` बाइट खाली बताए। SHA256 `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041` है। `esptool image_info` का चेकसम `ac` है और वैलिडेशन हैश `05873b3f41fdbb860ee18172114cb64205cc64dbe2d57d42a9b7b944e8023270` है। उसी दिन की पहले वाली छवि 7518928 बाइट थी।

3. **फ्लैश मालिक का कदम है।** इस समापन ने पोर्ट नहीं खोला और फ्लैश नहीं किया। `firmware\build` से ESP-IDF Python के साथ। बूटलोडर 22288 बाइट पर `0x0` है, विभाजन तालिका 3072 बाइट पर `0x8000` है, ऐप `0x10000` पर है, और फ़ॉन्ट 1325432 बाइट पर `0x810000` है। जो पोर्ट वास्तव में मिले उसी का उपयोग करें।

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **बोर्ड जाँच से पहले PC पर होस्ट शुरू करें।** सिम ट्रांसपोर्ट `POTETOBOT_PORT` नहीं रखता। डिफ़ॉल्ट `%APPDATA%\potetobot\destination.json` अनुपस्थित रहता है। एक गंतव्य फ़ाइल दें। `C:\Python313\python.exe` में whisper, numpy और torch हैं। नीचे का आदेश 2026-11-24 को एक बार चला। फिर चलाने से एक और प्रश्न जाता है।

```
C:\Python313\python.exe host\mvp_host.py --transport sim --wav tests\fixtures\heami-capital.wav --destination host\build\orca-dest.json
```

5. **फ्लैश के बाद आवाज़ का चेहरा poteto है।** KEY3 दबाए रखने पर listening, दो फ्रेम, 15 fps। छोड़ने पर thinking, दो फ्रेम, 12 fps। अनुरोध की प्रतीक्षा में `CUE v1 <utt> prepare` tilt चुनता है, चार फ्रेम, 6 fps। उत्तर आने पर `CUE v1 <utt> speak` speaking चुनता है, चार फ्रेम, 6 fps, और छोटा बुलबुला बनाता है। `CUE v1 <utt> idle` और `CUE v1 <utt> error` idle चुनते हैं। स्पर्श से बुलबुला स्क्रॉल करें। `host\mvp_host.py --transport serial` Win32 पोर्ट API से `POTETOBOT_PORT` को लगातार पढ़ता है। `--inject` बोर्ड बाइट देता है और हैंडल नहीं खोलता। pyserial नहीं लगता। इस समापन ने फ्लैश नहीं किया। दूसरे KEY3 दबाने का क्यू चेहरा नहीं बदलता। drop, क्षतिग्रस्त फ्रेम, गायब अंत, खाली गंतव्य, बॉट विफलता या टाइमआउट उस उक्ति पर error दिखाता है, और अगली सही उक्ति चलती रहती है। `--inject` के बिना सीरियल चेहरा सिम्युलेटर नहीं चलाता। Win32 लेखन जो 0 लौटाए या कोई बाइट न लिखे उसे भेजा गया क्यू नहीं माना जाता, और असफल पठन टाइमआउट नहीं है। इस बार COM पोर्ट नहीं खोला गया।

6. **SEL, TXT और SAY पीयर पर रहते हैं।** पीयर मैनुअल पंक्ति है। वह आवाज़ का रास्ता नहीं है।

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

पीयर stdin पढ़ता है। `host\build\potetobot-peer.exe` न हो तो पहले उसे बनाएँ। `cl` पंक्ति पीयर वाले भाग में है।

## होस्ट एक Orca बॉट से एक बार पूछता है।

`host\mvp_host.py` Windows प्रवेश है। चरण 4 का आदेश वह शुरुआत है जो चली। `--transport sim` और `--transport serial --inject` वही सत्र बाँटते हैं: AUD बाइट आते हैं, अनुरोध id पर `CUE v1 <utt> prepare`, और उसी उत्तर पर `CUE v1 <utt> speak`। `--inject` के बिना `--transport serial` `POTETOBOT_PORT` से एक COM हैंडल खोलकर पढ़ता रहता है। pyserial आयात नहीं होता। पीयर SEL, TXT, SAY के लिए अपना हैंडल खोलता है। सिम WAV को `host\aud_format.exe` से काटता है, और वह प्रोग्राम फ़र्मवेयर वाला ही `AUD v1` प्रारूप बुलाता है। होस्ट PCM तभी रखता है जब उक्ति क्रमांक, क्रम, बाइट लंबाई और अंत पंक्ति मिलें। कमी, दोहराव, छोटी hex पंक्ति, मिला लॉग, drop, या गायब अंत लिप्यंतरित नहीं होता। एक बार में एक उक्ति बँधती है। देर से आए उत्तर के लिए दूसरा प्रेषण नहीं होता। खराब उक्ति उस क्रमांक का `CUE v1 <utt> error` लिखती है और सत्र जारी रहता है। एक पंक्ति 2048 बाइट पर रुकती है। शुरू हुई उक्ति अधिकतम 8 सेकंड और अधिकतम 400 AUD पंक्तियाँ प्रतीक्षा करती है। खोया हुआ अंत अगली उक्ति से नहीं जुड़ता। Whisper या अनुरोध के दौरान होस्ट उसी हैंडल को पढ़ता रहता है। अधिकतम 16 उक्तियाँ रखता है, प्रत्येक PCM अधिकतम 192000 बाइट (6.0 सेकंड)। सीमा पार होने पर `audio busy` छपता है और `CUE v1 <utt> error` लिखा जाता है, और अगली उक्ति चलती रहती है।

| सीमा | मान |
| --- | --- |
| PCM | 16 kHz, मोनो, s16, हर सेकंड 32000 बाइट |
| एक फ्रेम | 640 बाइट, फिर एक पंक्ति `AUD v1 <utt> <seq> <len> <hex>` |
| एक उक्ति | 192000 बाइट, यानी 6.0 सेकंड |
| प्रश्न | अधिकतम 2000 UTF-8 बाइट, `--body-file` में |
| बुलबुला | उत्तर का अंश, 96 UTF-8 बाइट से कम |
| पंक्ति | अधिकतम 2048 बाइट |
| शुरू हुई उक्ति | अधिकतम 8 सेकंड और 400 AUD पंक्तियाँ |
| प्रतीक्षारत उक्तियाँ | अधिकतम 16, प्रत्येक PCM अधिकतम 192000 बाइट (6.0 सेकंड) |

2026-11-24 के सिम ने `tests\fixtures\heami-capital.wav` पढ़ा (Microsoft Heami, 16 kHz, मोनो, s16)। फिर से जोड़ना 151680 बाइट और 238 पंक्तियाँ था। CPU पर Whisper base का लिप्यंतरण `안녕하세요. 대한민국의 수도는 어디인가요?` था। एक अनुरोध `msg_5f8b8f406d35` था। इनबॉक्स पंक्ति `msg_789251e073d5` `orca_session_id:6176a306-e513-4dff-b903-5bed98beb33d` से आई और उसका पाठ `서울입니다.` था। बुलबुला वही अंश था। खाली गंतव्य और `grok_bot` ने `request` नहीं छापा।

बाद का `--transport serial --inject` वही WAV था और COM हैंडल नहीं खुला। PCM 151680 बाइट था। अनुरोध `msg_bf26672f0e57` था। उस id के बाद प्रतीक्षा फ्रेम `poteto/tilt/f02.png` था। उत्तर और speak बुलबुला `서울입니다.` थे। `from` था `orca_session_id:1398eeac-555a-4337-bc0c-86e4e5d49458`।

प्रेषक हैंडल `%APPDATA%\potetobot\host-terminal.json` में रहता है। गंतव्य फ़ाइल और फ़र्मवेयर चैट id नहीं रखते। `pyserial` ESP-IDF वर्चुअल वातावरण में है और इस सिम ने उसे आयात नहीं किया।

नीचे का डिवाइस आदेश वह शुरुआत है जिसे कोड स्वीकार करता है। इस बार वह नहीं चला और COM पोर्ट नहीं खुला। इसे `C:\Dev\ESP32Bot` से शुरू करें।

```
$env:POTETOBOT_PORT = "COM5"
C:\Python313\python.exe host\mvp_host.py --transport serial --destination host\build\orca-dest.json
```

`--inject` और `--transport sim` देखा जाने वाला सत्र बाँटते हैं और हैंडल नहीं खोलते। डिफ़ॉल्ट `%APPDATA%\potetobot\destination.json` अनुपस्थित रहता है। `host\build\orca-dest.json` में `{"provider":"orca_bots","botId":"Poteto"}` है। प्रेषक फ़ाइल `%APPDATA%\potetobot\host-terminal.json` पहले से है और उसमें `term_` हैंडल है। फ़ाइल न हो तो होस्ट `orca terminal create --title potetobot-host --json` चलाकर `{"handle":"term_..."}` लिखता है। पहले मापी गई रचना `orca terminal create --worktree path:C:\Dev\ESP32Bot --title potetobot-host --json` थी और `--focus` नहीं दिया गया। गंतव्य फ़ाइल या फ़र्मवेयर में चैट id न रखें।

2026-11-24 को `C:\Dev\ESP32Bot` में `POTETOBOT_PORT` के बिना `C:\Python313\python.exe host\mvp_host.py --self-test` निकास 0 रहा। अंतिम पंक्ति `self-test ok` थी। COM पोर्ट नहीं खुला और कोई नया Orca प्रश्न नहीं गया। एक सत्र के उत्तर `하나` और `둘` थे। खाली गंतव्य ने `channel status not_sent` छापा और `request` पंक्ति नहीं थी। टाइमआउट ने `audio timeout 6` छापा, और अगली उक्ति का उत्तर `다시` था। drop, क्षतिग्रस्त फ्रेम और गायब अंत ने idle और बुलबुला `error` दिखाया, और अगली उक्ति चलती रही। `btn 0 0102`, `btn 1 -`, `btn 0 0506` के बाद बिना टैग वाला `CUE v1 speak old` और `CUE v1 1 speak old` चेहरा listening पर छोड़ गए और बुलबुला `old` नहीं आया। `CUE v1 2 prepare` ने tilt `poteto/tilt/f02.png` दिखाया, और `CUE v1 2 speak next` ने speaking और बुलबुला `next` दिखाया। एक दबाव ने बिना टैग `CUE v1 prepare` और `CUE v1 speak kept` को अभी भी माना। Win32 `WriteFile` जो 0 लौटा, और जो विफल रहा, `sent` में नहीं रहा। `CUE v1 1 prepare` का 4 बाइट आंशिक लेखन अगले लेखन ने पूरा किया। मॉक सीरियल सत्र का पहला टुकड़ा `CUE ` था, `speak 하나` सहेजा नहीं गया। सहेजे क्यू थे `CUE v1 3 prepare`, `CUE v1 3 error`, `CUE v1 4 prepare`, और `CUE v1 4 speak 둘`। उन बाइट के बाद `ReadFile` विफल हुआ और सत्र ने `link read read failed` छापकर रुक गया। विफल पठन पर `readline` ने `ReadFile` दो बार बुलाया और घूमा नहीं। उस मॉक पथ पर चेहरा निष्पादन फ़ाइल नहीं चली। यह चलन `msg_bf26672f0e57` और `msg_5f8b8f406d35` से अलग है।

वही सेल्फ-टेस्ट फिर एक सीमित आभासी रिसीव बफर पर चला, पहले से पूरी इनपुट रखने वाले InjectLink पर नहीं। दूसरा PCM 16000 बाइट (0.5 सेकंड) 32437 फ्रेम बाइट बना। बिना पढ़े 8192 बाइट बफर ने उनमें से 24245 बाइट छोड़े। ट्रांसक्रिप्शन रुका होने पर होस्ट पढ़ता रहा, तो छोड़े गए बाइट 0 थे और दोनों उत्तर `하나` और `둘` थे। एक प्रतीक्षा स्लॉट पर अतिरिक्त उक्ति ने `audio busy` और error क्यू लिखा, और अगला उत्तर `회복` था। फ़र्मवेयर दोबारा नहीं बना। छवि 7519200 बाइट ही है। ऊपर का डिवाइस आदेश नहीं बदला। COM पोर्ट नहीं खुला।

`host\gui_server.py` लूपबैक पृष्ठ है। 2026-11-24 को यह केवल `127.0.0.1:8765` पर सुना। इसने COM पोर्ट नहीं खोला, फ्लैश नहीं किया, और `%APPDATA%\potetobot\destination.json` नहीं लिखा। स्क्रीन भाषा और वाक् पहचान की भाषा `%APPDATA%\potetobot\gui.json` में अलग सहेजी जाती हैं। दोनों का मूल कोरियाई है। अंग्रेज़ी पहचान Whisper को `language=en`, `fp16=false`, `temperature=0` के रूप में जाती है। Orca नाम `orca bot list --json` से आते हैं। सहेजा एक नाम तभी गंतव्य का `botId` बनता है जब असली प्रश्न शुरू होता है। इस बार नया Orca प्रश्न नहीं गया। Grok Bot का खाता-बॉट सूची या उत्तर पथ दर्ज नहीं है। Share से चिपकाया id बना `grokbot://app/v1/prompt?id=fromshare&text=hello`। वह कड़ी नहीं भेजती। वेबहुक का HTTP 200 स्वीकृति है, उत्तर नहीं। xAI मॉडल API इस्तेमाल नहीं हुई। चरित्र कार्ड `firmware\main\frame_table.c` और `assets\frames` पढ़ते हैं। बोर्ड पर Poteto है idle 3, listening 2, thinking 2, speaking 4, tilt 4, breath 1। बोर्ड पर Sunny है idle 2, listening 1, thinking 1, speaking 4, smile 1। Poteto के smile और emphasis बोर्ड पर 0 फ्रेम हैं, झलक PNG 5 और 3। इस बार चित्र नहीं जोड़े गए और छवि 7519200 बाइट ही है। Windows ने रजिस्ट्री से `COM5` पढ़ा और नहीं खोला। Mac और Linux अडैप्टर ने इस PC पर `not_measured` दिया। `host\gui_server.py --self-test` निकास 0 रहा। ब्राउज़र में एक अच्छी आभासी उक्ति ने `CUE v1 1 prepare` और `CUE v1 1 speak virtual` लिखा, `whisper language=en` के साथ। खराब पंक्ति ने `CUE v1 error` लिखा। दोनों ने BoundedKernel इस्तेमाल किया।

```
C:\Python313\python.exe host\gui_server.py
```

## शुरू एक सत्र को टिकाता है।

2026-11-24 को उसी पृष्ठ पर Start और Stop आए। Start `gui.json` या `destination.json` नहीं लिखता। पोर्ट, प्रदाता, चुना बॉट और वाक् भाषा उस सत्र प्रक्रिया के तर्क हैं। पोर्ट `POTETOBOT_PORT` में नहीं जाता। CLI पर `--stt-language` छोड़ने से कोरियाई रहता है। POST को `Content-Type: application/json`, `Host: 127.0.0.1:<port>`, `Origin: http://127.0.0.1:<port>` और पृष्ठ टोकन चाहिए। गलत मूल 403 देता है और कुछ नहीं लिखता। जुड़े रहने पर दूसरा Start 409 देता है।

सेल्फ-टेस्ट वही शुरू पथ रखता है। केवल सीरियल ओपनर सीमित आभासी पोर्ट बनता है और केवल Orca CLI स्थानीय बैंड बनता है। एक उक्ति ने `bot:Poteto` को एक प्रश्न भेजा। `prepare` और `speak` ने वही उक्ति id बाँटी, और उत्तर पाठ `band-answer` था। खाली बॉट id और `grok_bot` ने शून्य प्रश्न भेजे। Stop ने बाद का speak नहीं जोड़ा। असली ओपनर पर `COM99` ने `serial open failed COM99` दिखाया और हैंडल खाली रहा। COM5 नहीं खुला।

Windows ब्राउज़र में Start से पहले पोर्ट क्षेत्र `COM5` से `COM99` किया गया। पृष्ठ ने `Failed · serial open failed COM99` और खाली हैंडल दिखाया। खोज `zzz-no-such` पर `No search matches.` दिखा। पूरी सूची `It'sMe`, `서치린`, `Poteto`, `정리린`, `버그린`, `리뷰린` रही। क्षेत्र खाली करने पर छह फिर दिखे। क्षेत्र `Poteto` रहते रिफ्रेश ने बाकी नाम नहीं हटाए। Mint केवल झलक कार्ड है: बोर्ड फ्रेम 0, वाक् संकेत बंद, SEL बंद, एक idle PNG। Grok नियंत्रण ने असली कड़ी `grokbot://app/v1/prompt?id=fromshare&text=hello` बनाई, target `_blank`। कॉपी ने बटन को `Copied` किया। उस कड़ी का क्लिक सत्र शुरू नहीं करता। वाक् भाषा कोरियाई होने पर अच्छी आभासी उक्ति ने `CUE v1 1 prepare` और `CUE v1 1 speak 가상` लिखा, `openedPort=false`। त्रुटि उक्ति ने `CUE v1 1 error` लिखा और speak पंक्ति नहीं।

इस PC पर `os.openpty` नहीं है। पैरामीटर जाँच ने सॉकेट जोड़ी और बाइट `sock-byte` इस्तेमाल किए। Mac या Linux उपकरण नहीं मापा गया। Windows खोलना, पढ़ना, लिखना और बंद करना कर्नेल स्टैंड-इन वाले `open_serial` से हुआ: 115200, 8 डेटा बिट, पैरिटी 0, स्टॉप बिट 0, flags 1। कोई उपकरण हैंडल नहीं बना।

`C:\Python313\python.exe host\gui_server.py --self-test` निकास 0। `C:\Python313\python.exe host\mvp_host.py --self-test` निकास 0। दोनों `C:\Dev\ESP32Bot` से चले, `POTETOBOT_PORT` बिना। `%APPDATA%\potetobot\gui.json`, `destination.json` और `host-terminal.json` नहीं बदले। फ़र्मवेयर दोबारा नहीं बना। `firmware\build\potetobot.bin` 7519200 बाइट है। SHA256 `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041` है। फ़ैक्टरी सीमा 8388608 बाइट ही है।

साफ चेकआउट में चयन फ़ाइल `host\build` के नीचे बनती है। वह फ़ाइल `%APPDATA%\potetobot\destination.json` नहीं है।

```
C:\Python313\python.exe -c "import json, pathlib; p=pathlib.Path('host/build'); p.mkdir(parents=True, exist_ok=True); p.joinpath('orca-dest.json').write_text(json.dumps({'provider':'orca_bots','botId':'Poteto'})+'\n', encoding='utf-8')"
```

## बोर्ड Waveshare ESP32-S3-Touch-AMOLED-2.16 है।

| मद | मान |
| --- | --- |
| लक्ष्य | ESP32-S3, 240 MHz (`sdkconfig.defaults`) |
| फ्लैश | 16 MB। डिफ़ॉल्ट फ़ाइल QIO रखती है। बेंच लेखन ने `--flash_mode dio` और `--flash_freq 80m` इस्तेमाल किया |
| PSRAM | ऑक्टल, 80 MHz |
| कंसोल | USB Serial/JTAG, 115200 8N1 |
| पैनल | 480×480 |
| factory ऐप | ऑफ़सेट `0x10000`, आकार `0x800000` (8388608 बाइट) |
| font विभाजन | नाम `font`, उपप्रकार `0x40`, ऑफ़सेट `0x810000`, आकार `0x200000` (2097152 बाइट) |
| स्पर्श | पता `0x5A`। SCL GPIO14, SDA GPIO15, इंटरप्ट GPIO11, रीसेट GPIO40 |

## पैक किए फ्रेम ही चेहरा हैं।

`play_task` हर 66 ms पर फिर बनाता है। फ्रेम सूचकांक भाव के अपने कदम `1000 / fps` पर बढ़ता है।

| पात्र | भाव | फ्रेम | fps | कदम |
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

`1000/6` 166 है, `1000/15` 66 है, और `1000/12` 83 है। तीनों पूर्णांक भाग हैं।

आवाज़ केवल poteto चुनती है: KEY3 दबाते listening, छोड़ते thinking, `CUE v1 <utt> prepare` पर tilt, `CUE v1 <utt> speak` पर speaking, और `CUE v1 <utt> idle` या `CUE v1 <utt> error` पर idle। sunny केवल `SEL` से चुना जाता है। दूसरे KEY3 दबाने का क्यू चेहरा नहीं बदलता।

ये नाम शेड्यूलर में हैं और शून्य फ्रेम के साथ पैक हैं। बोर्ड इन्हें अस्वीकार करता है। वह `ACK` नहीं भेजता, और जो चेहरा पहले से दिख रहा है वह रहता है।

| पात्र | शून्य फ्रेम वाले नाम |
| --- | --- |
| poteto | emphasis, smile |
| sunny | tilt, breath, emphasis |
| दोनों | happy, surprised, sleepy, dizzy, wink |

## एक सीरियल पंक्ति चेहरा चुनती है।

```
SEL v1 poteto idle
SEL v1 sunny smile
```

जिस भाव के फ्रेम पैक हैं वह इस आकार में उत्तर देता है:

```
ACK poteto idle
```

पार्सर अंत का मिलीसेकंड क्षेत्र स्वीकार करता है, जैसे `SEL v1 poteto speaking 500`। बोर्ड फिर लूप पहले फ्रेम से शुरू करता है। वह क्षेत्र जगह नहीं खोजता।

`SEL v1 poteto emphasis` और `SEL v1 sunny tilt` व्याकरण से मेल खाते हैं और फ्रेम नहीं रखते, इसलिए `ACK` नहीं आता। `SEL v1 poteto listening` के दो फ्रेम हैं।

## उत्तर का पाठ उसी लाइन पर आता है।

```
TXT v1 thinking
TXT v1 The potato is hot.
SAY v1 thinking The potato is hot.
SAY v1 answer The potato is hot.
```

| पंक्ति | चरण | बुलबुले में सहेजा पाठ |
| --- | --- | --- |
| `TXT v1 thinking` | thinking | `...` |
| `TXT v1` और कोई अन्य पाठ | answer | वही पाठ |
| `SAY v1 thinking` और पाठ | thinking | वही पाठ |
| `SAY v1 answer` और पाठ | answer | वही पाठ |

बोर्ड `ACK say thinking` या `ACK say answer` उत्तर देता है। `POTETO_SAY_TEXT_MAX` समाप्ति NUL सहित 480 बाइट है, इसलिए सहेजा पाठ अधिक से अधिक 479 बाइट है।

बुलबुले का आयत x=12, y=356, चौड़ाई 456, ऊँचाई 112 है, और कोने की त्रिज्या 14 है। नीचे के पिक्सेल दो बार दाएँ खिसकते हैं, जिससे RGB565 चैनल लगभग चौथाई हो जाते हैं, और वे मिटते नहीं। किनारा और answer ग्लिफ `0xFFFF` हैं। thinking ग्लिफ `0xCE79` हैं। बोर्ड अंतिम 8 संदेश रखता है। पाठ खिड़की 80 पिक्सेल ऊँची है और हर पंक्ति 18 पिक्सेल है, इसलिए चार पंक्तियाँ समाती हैं। ऊर्ध्वाधर खींचने से सहेजी पंक्तियाँ स्क्रॉल होती हैं।

जब पीयर `thinking` भाव वाली `SEL` स्वीकार करता है, वह `TXT v1 thinking` भी लिखता है।

## Windows पीयर grok-bot या orca बोलता है।

| ड्राइवर | कब चलने देता है |
| --- | --- |
| `grok-bot` | `%APPDATA%\Grok Bot\desktop-status.json` में `"signedIn":true` है, और उस फ़ाइल का pid अभी चल रहा है। यही डिफ़ॉल्ट है। |
| `orca` | `Orca.exe` नाम की प्रक्रिया छवि चल रही है। |

```
host\build\potetobot-peer.exe --driver grok-bot
host\build\potetobot-peer.exe --driver orca
```

`--driver`, `POTETOBOT_DRIVER` पर भारी है। पोर्ट का नाम `POTETOBOT_PORT` से आता है। पीयर उसे 115200 8N1 पर खोलता है, DTR और RTS बंद रखकर, इसलिए पोर्ट खुलने से चिप डाउनलोड मोड में रीसेट नहीं होता। वह `SEL` पंक्तियाँ और `TXT v1` से शुरू होने वाली पंक्तियाँ आगे भेजता है। stderr पर grok-bot `driver=grok-bot pid=<pid> signedIn=true` छापता है। orca `driver=orca` छापता है।

रिपॉजिटरी की जड़ से Visual Studio x64 कंपाइलर से पीयर बनाएँ:

```
cl /nologo /O2 /I firmware\main /Fe:host\build\potetobot-peer.exe host\peer_main.c firmware\main\schedule.c firmware\main\frame_table.c firmware\main\say.c
```

`host/test_schedule.c` फ्रेम लूप जाँचता है। `host\build\test_schedule.exe` को रिपॉजिटरी की जड़ दें, या तर्क छोड़ें तो वह `.` उपयोग करता है। speaking जाँच 6 fps पर है: poteto `(1000/6)*2` पर `poteto/speaking/f02.png` की अपेक्षा करता है, और sunny `(1000/6)*3` पर `sunny/speaking/f03.png` की। `tests/test_peer.py` को साइन-इन Grok Bot चाहिए। `tests/test_peer_orca.py` को `Orca.exe` चाहिए। `tests/test_frame_budget.py` पैक किए ब्लॉब को factory सीमा से मिलाता है।

## ग्लिफ एटलस factory छवि के बाहर रहता है।

`firmware/font/bubble_font.bin` 1325432 बाइट है। यह GNU Unifont 16.0.04 का उपसमूह है, और `firmware/main/ascii8x16.inc` वह ASCII फ़ॉलबैक है जिसे `firmware/font/LICENSE.txt` उसके पास नाम देता है। फ्लैश पंक्ति एटलस को `0x810000` पर लिखती है। वह `font` विभाजन है, 8388608 बाइट के factory ऐप का हिस्सा नहीं।

## यह बिल्ड ध्वनि नहीं बजाता।

स्पीकर बंद रहता है। KEY3 कैप्चर केवल I2S रिसीव है: 16 kHz, मोनो, साइन 16-बिट, `firmware/main/key3.c` में। कैप्चर टास्क उस स्ट्रीम को 66 ms के चित्र से अलग पढ़ता है। भरे टुकड़े `AUD v1` पंक्तियों में निकलते हैं। सीरियल प्रोटोकॉल में भाषा सेटिंग नहीं है और वॉकी-टॉकी नहीं है। पीयर के शब्द और छोटे उत्तर का अंश बुलबुले में बनते हैं।

## पेड़ इस तरह बँटा है।

| पथ | इसमें क्या है |
| --- | --- |
| `firmware/` | ESP-IDF ऐप, `partitions.csv`, और फ़ॉन्ट छवि |
| `firmware/main/frames.bin` | RGB565 RLE फ्रेम जिन्हें बिल्ड जोड़ता है |
| `host/peer_main.c` | Windows पीयर |
| `host/mvp_host.py` | Windows आवाज़ प्रवेश |
| `host/gui_server.py` | Windows लूपबैक पृष्ठ |
| `assets/frames/` | स्रोत फ्रेम, 480×480 |
| `tests/` | पीयर और फ्रेम-बजट जाँच |
