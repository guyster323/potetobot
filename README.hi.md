# Potetobot

**Grokbot with me anywhere.**

480 गुणा 480 का गोल पैनल poteto या sunny का एक चेहरा दिखाता है, और उसी फ्रेम पर जवाब का पाठ एक भाषण बुलबुले में बनाता है।

[English](README.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · **हिन्दी** · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md)

[त्वरित शुरुआत](#त्वरित-शुरुआत) · [एक सीरियल पंक्ति](#एक-सीरियल-पंक्ति-चेहरा-चुनती-है) · [पैक किए फ्रेम](#पैक-किए-फ्रेम-ही-चेहरा-हैं)

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

2. **अगर वह छवि 8388608 बाइट से बड़ी हो तो रुकें।** यही factory विभाजन है। 2026-11-21 को बेंच पर बनी छवि 8377584 बाइट है, इसलिए 11024 बाइट बचते हैं।

3. **बूटलोडर, विभाजन तालिका, ऐप और फ़ॉन्ट फ्लैश करें।** `firmware\build` से ESP-IDF Python के साथ। बेंच बोर्ड COM5 पर आता है। जो पोर्ट वास्तव में मिले उसी का उपयोग करें।

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **लॉग के `app_main` से लौटने तक प्रतीक्षा करें।** उसके बाद `SEL` या `TXT` भेजें।

5. **पीयर चालू करें और एक चयन, फिर पाठ की एक पंक्ति भेजें।**

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

पीयर stdin पढ़ता है। `host\build\potetobot-peer.exe` न हो तो पहले उसे बनाएँ। `cl` पंक्ति पीयर वाले भाग में है।

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

`1000/6` 166 है, `1000/15` 66 है, और `1000/12` 83 है। तीनों पूर्णांक भाग हैं।

ये नाम शेड्यूलर में हैं और शून्य फ्रेम के साथ पैक हैं। बोर्ड इन्हें अस्वीकार करता है। वह `ACK` नहीं भेजता, और जो चेहरा पहले से दिख रहा है वह रहता है।

| पात्र | शून्य फ्रेम वाले नाम |
| --- | --- |
| poteto | listening, thinking |
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

`SEL v1 poteto listening` और `SEL v1 sunny tilt` व्याकरण से मेल खाते हैं और फ्रेम नहीं रखते, इसलिए `ACK` नहीं आता।

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

`firmware/main` में कोई ऑडियो आउटपुट नहीं है। सीरियल प्रोटोकॉल में भाषा सेटिंग नहीं है और वॉकी-टॉकी नहीं है। पीयर से आए शब्द बुलबुले में बनते हैं।

## पेड़ इस तरह बँटा है।

| पथ | इसमें क्या है |
| --- | --- |
| `firmware/` | ESP-IDF ऐप, `partitions.csv`, और फ़ॉन्ट छवि |
| `firmware/main/frames.bin` | RGB565 RLE फ्रेम जिन्हें बिल्ड जोड़ता है |
| `host/peer_main.c` | Windows पीयर |
| `assets/frames/` | स्रोत फ्रेम, 480×480 |
| `tests/` | पीयर और फ्रेम-बजट जाँच |
