# Potetobot

**Grokbot with me anywhere.**

Un panel redondo de 480 por 480 muestra una cara, poteto o sunny, y dibuja el texto de la respuesta en un globo sobre el mismo cuadro.

[English](README.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · **Español** · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md)

[Inicio rápido](#inicio-rápido) · [El host](#el-host-pregunta-una-vez-a-un-bot-de-orca) · [La línea serie](#una-línea-serie-elige-la-cara) · [Cuadros empaquetados](#los-cuadros-empaquetados-son-la-cara)

## El panel muestra dos caras y un globo.

Poteto tiene el pelo negro largo, flequillo, gafas redondas doradas, rubor, ropa negra y un boniato humeante junto a la cara. Sunny tiene el pelo negro corto, gafas de sol marrones sobre los ojos, rubor, una cremallera oscura y un periódico con un sol dibujado. El fondo es negro. La cara llena el lienzo de 480.

| poteto en espera, cuadro f00 | sunny en espera, cuadro f00 |
| --- | --- |
| ![poteto en espera, cuadro f00](assets/frames/poteto/idle/f00.png) | ![sunny en espera, cuadro f00](assets/frames/sunny/idle/f00.png) |
| poteto sonriendo, cuadro f00 | sunny sonriendo, cuadro f00 |
| ![poteto sonriendo, cuadro f00](assets/frames/poteto/smile/f00.png) | ![sunny sonriendo, cuadro f00](assets/frames/sunny/smile/f00.png) |

Estas imágenes son cuadros empaquetados. Este repositorio no tiene un vídeo de demostración.

## Inicio rápido

1. **Construye el firmware con ESP-IDF 5.5.** En el PowerShell del banco:

```
. C:\Esp\esp-idf\export.ps1
cd firmware
idf.py build
```

El paquete de caras ya es `firmware/main/frames.bin`. Una construcción normal incrusta ese archivo. No vuelve a pintar los personajes. La imagen de la aplicación es `firmware/build/potetobot.bin`.

2. **Detente si esa imagen supera 8388608 bytes.** Ese es el tamaño de la partición factory. La imagen reconstruida el 2026-11-24 mide 7519200 bytes, así que quedan 869408 bytes. `check_sizes` informó `0x72bbe0` frente a `0x800000`, con `0xd4420` bytes libres. SHA256 es `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`. La suma de `esptool image_info` es `ac` y el hash de validación es `05873b3f41fdbb860ee18172114cb64205cc64dbe2d57d42a9b7b944e8023270`. Una imagen anterior de ese día medía 7518928 bytes.

3. **Grabar es el paso del dueño.** Este cierre no abrió un puerto y no grabó. Desde `firmware\build`, con el Python de ESP-IDF. El bootloader mide 22288 bytes en `0x0`, la tabla de particiones mide 3072 bytes en `0x8000`, la aplicación está en `0x10000` y la fuente mide 1325432 bytes en `0x810000`. Usa el puerto que tengas de verdad.

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **Arranca el host en el PC antes de la prueba de la placa.** El transporte sim no pone `POTETOBOT_PORT`. El `%APPDATA%\potetobot\destination.json` predeterminado sigue ausente. Pasa un archivo de destino. `C:\Python313\python.exe` tiene whisper, numpy y torch. El comando de abajo se ejecutó una vez el 2026-11-24. Ejecutarlo otra vez envía otra pregunta.

```
C:\Python313\python.exe host\mvp_host.py --transport sim --wav tests\fixtures\heami-capital.wav --destination host\build\orca-dest.json
```

5. **Después de grabar, la cara de voz es poteto.** Mantén KEY3 para listening, dos cuadros a 15 fps. Suelta para thinking, dos cuadros a 12 fps. Mientras la petición espera, `CUE v1 <utt> prepare` elige tilt, cuatro cuadros a 6 fps. Cuando llega la respuesta, `CUE v1 <utt> speak` elige speaking, cuatro cuadros a 6 fps, y dibuja el globo corto. `CUE v1 <utt> idle` y `CUE v1 <utt> error` eligen idle. Desplaza el globo con el toque. `host\mvp_host.py --transport serial` sigue leyendo `POTETOBOT_PORT` con la API de puerto Win32. `--inject` mete bytes de la placa y no abre un identificador. pyserial no se usa. Este cierre no grabó. Una señal de otra pulsación de KEY3 no cambia la cara. Un drop, un cuadro dañado, un fin ausente, un destino vacío, un fallo del bot o un tiempo de espera muestran error para esa frase, y la siguiente frase buena sigue. El serie sin `--inject` no lanza el simulador de la cara. Una escritura Win32 que devuelve 0 o no escribe bytes no queda como señal enviada, y una lectura fallida no es un tiempo de espera. Este pase no abrió un puerto COM.

6. **SEL, TXT y SAY siguen por el peer.** El peer es la línea manual. No es la ruta de voz.

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

El peer lee stdin. Constrúyelo primero si falta `host\build\potetobot-peer.exe`. La línea `cl` está en la sección del peer.

## El host pregunta una vez a un bot de Orca.

`host\mvp_host.py` es la entrada de Windows. El comando del paso 4 es el inicio que se ejecutó. `--transport sim` y `--transport serial --inject` comparten esa sesión: entran bytes AUD, `CUE v1 <utt> prepare` cuando se imprime el id, y `CUE v1 <utt> speak` con esa respuesta. `--transport serial` sin `--inject` abre un identificador COM desde `POTETOBOT_PORT` y sigue leyendo. pyserial no se importa. El peer abre el suyo para SEL, TXT y SAY. El modo sim trocea el WAV con `host\aud_format.exe`, que llama al mismo formato `AUD v1` que el firmware. El host guarda el PCM solo cuando coinciden el id de la frase, la secuencia, la longitud en bytes y la línea de fin. Un hueco, un duplicado, una línea hex corta, un registro mezclado, un drop o un fin ausente no se transcribe. Se ata una frase cada vez. Una respuesta tardía no inicia un segundo envío. Una frase mala escribe `CUE v1 <utt> error` para ese id y la sesión sigue. Una línea se detiene en 2048 bytes. Una frase empezada espera como máximo 8 segundos y 400 líneas AUD. Un fin perdido no se pega a la frase siguiente. Mientras Whisper o la petición están en curso, el host sigue leyendo ese mismo identificador. Guarda como máximo 16 frases, cada PCM de como máximo 192000 bytes (6.0 s). Pasado ese tope imprime `audio busy` y escribe `CUE v1 <utt> error`, y la frase siguiente sigue.

| Límite | Valor |
| --- | --- |
| PCM | 16 kHz, mono, s16, 32000 bytes cada segundo |
| Un cuadro | 640 bytes y luego una línea `AUD v1 <utt> <seq> <len> <hex>` |
| Una frase | 192000 bytes, que son 6.0 segundos |
| Pregunta | como máximo 2000 bytes UTF-8, en `--body-file` |
| Globo | el extracto de la respuesta, bajo 96 bytes UTF-8 |
| Línea | como máximo 2048 bytes |
| Frase empezada | como máximo 8 segundos y 400 líneas AUD |
| Frases en espera | como máximo 16, cada PCM como máximo 192000 bytes (6.0 s) |

La simulación del 2026-11-24 leyó `tests\fixtures\heami-capital.wav` (Microsoft Heami, 16 kHz, mono, s16). El reensamblado fue 151680 bytes en 238 líneas. Whisper base en CPU transcribió `안녕하세요. 대한민국의 수도는 어디인가요?`. La única petición fue `msg_5f8b8f406d35`. La fila de bandeja `msg_789251e073d5` vino de `orca_session_id:6176a306-e513-4dff-b903-5bed98beb33d` y su cuerpo fue `서울입니다.`. El globo fue ese mismo extracto. El destino vacío y `grok_bot` no imprimieron `request`.

La ejecución posterior `--transport serial --inject` usó el mismo WAV y no abrió un identificador COM. El PCM fue 151680 bytes. La petición fue `msg_bf26672f0e57`. Tras ese id, el cuadro de espera fue `poteto/tilt/f02.png`. La respuesta y el globo speak fueron `서울입니다.`. `from` fue `orca_session_id:1398eeac-555a-4337-bc0c-86e4e5d49458`.

El identificador del emisor queda en `%APPDATA%\potetobot\host-terminal.json`. El archivo de destino y el firmware no guardan un id de chat. `pyserial` vive en el entorno virtual de ESP-IDF y esta simulación no lo importó.

El comando de placa de abajo es el inicio que acepta el código. Este pase no lo ejecutó y no abrió un puerto COM. Arráncalo desde `C:\Dev\ESP32Bot`.

```
$env:POTETOBOT_PORT = "COM5"
C:\Python313\python.exe host\mvp_host.py --transport serial --destination host\build\orca-dest.json
```

`--inject` y `--transport sim` comparten la sesión observada y no abren un identificador. El `%APPDATA%\potetobot\destination.json` predeterminado sigue ausente. `host\build\orca-dest.json` contiene `{"provider":"orca_bots","botId":"Poteto"}`. El archivo del emisor `%APPDATA%\potetobot\host-terminal.json` ya está y guarda un identificador `term_`. Si falta, el host ejecuta `orca terminal create --title potetobot-host --json` y escribe `{"handle":"term_..."}`. Una creación medida antes usó `orca terminal create --worktree path:C:\Dev\ESP32Bot --title potetobot-host --json` y no pasó `--focus`. No pongas un id de chat en el archivo de destino ni en el firmware.

El 2026-11-24, en `C:\Dev\ESP32Bot` y sin `POTETOBOT_PORT`, `C:\Python313\python.exe host\mvp_host.py --self-test` salió con 0. La última línea fue `self-test ok`. No abrió un puerto COM y no envió una pregunta nueva a Orca. Una sesión respondió `하나` y `둘`. Un destino vacío imprimió `channel status not_sent` y ninguna línea `request`. Un tiempo de espera imprimió `audio timeout 6`, y la frase siguiente respondió `다시`. Un drop, un cuadro dañado y un fin ausente mostraron idle con el globo `error`, y la frase siguiente siguió. Tras `btn 0 0102`, `btn 1 -` y `btn 0 0506`, las señales sin etiqueta `CUE v1 speak old` y `CUE v1 1 speak old` dejaron la cara en listening y sin globo `old`. `CUE v1 2 prepare` mostró tilt `poteto/tilt/f02.png`, y `CUE v1 2 speak next` mostró speaking y el globo `next`. Una sola pulsación siguió aceptando `CUE v1 prepare` y `CUE v1 speak kept` sin etiqueta. Un `WriteFile` de Win32 que devolvió 0, y uno que falló, no quedaron en `sent`. Una escritura parcial de 4 bytes de `CUE v1 1 prepare` la terminó la escritura siguiente. En la sesión serie simulada el primer trozo fue `CUE `, `speak 하나` no se guardó, y las señales guardadas fueron `CUE v1 3 prepare`, `CUE v1 3 error`, `CUE v1 4 prepare` y `CUE v1 4 speak 둘`. Después de esos bytes, `ReadFile` falló y la sesión imprimió `link read read failed` y se detuvo. `readline` en una lectura fallida llamó a `ReadFile` dos veces y no se quedó girando. El ejecutable de la cara no corrió en ese camino simulado. Esta ejecución es distinta de `msg_bf26672f0e57` y de `msg_5f8b8f406d35`.

Ese mismo self-test siguió con un búfer virtual de recepción finito, no con un InjectLink que carga toda la entrada de antemano. Un segundo PCM de 16000 bytes (0.5 s) se volvió 32437 bytes de trama. Un búfer de 8192 bytes sin lectura tiró 24245 de esos bytes. Con el host leyendo durante la transcripción bloqueada, los bytes tirados fueron 0 y las dos respuestas fueron `하나` y `둘`. Con un solo puesto de espera, la frase de más imprimió `audio busy` y su señal de error, y la respuesta siguiente fue `회복`. El firmware no se reconstruyó. La imagen sigue en 7519200 bytes. El comando de placa de arriba no cambió. No se abrió un puerto COM.

`host\gui_server.py` es la página de bucle local. El 2026-11-24 escuchó solo en `127.0.0.1:8765`. No abrió un puerto COM, no grabó y no escribió `%APPDATA%\potetobot\destination.json`. El idioma de la pantalla y el idioma del reconocimiento de voz se guardan aparte en `%APPDATA%\potetobot\gui.json`. El coreano es el predeterminado de ambos. El reconocimiento en inglés llega a Whisper como `language=en`, con `fp16=false` y `temperature=0`. Los nombres de Orca salen de `orca bot list --json`. Un nombre guardado es el `botId` del destino solo cuando empieza una pregunta real. Esta pasada no envió una pregunta nueva a Orca. Grok Bot no tiene una lista de bots de la cuenta ni una ruta de respuesta documentadas. Un id pegado desde Share formó `grokbot://app/v1/prompt?id=fromshare&text=hello`. Ese enlace no envía. Un HTTP 200 de un webhook es aceptación, no una respuesta. La API de modelos de xAI no se usa. Las tarjetas de personaje leen `firmware\main\frame_table.c` y `assets\frames`. Poteto en la placa es idle 3, listening 2, thinking 2, speaking 4, tilt 4, breath 1. Sunny en la placa es idle 2, listening 1, thinking 1, speaking 4, smile 1. El smile y el emphasis de Poteto siguen en 0 cuadros de placa, con 5 y 3 PNG de vista previa. Esta pasada no añadió imágenes y la imagen sigue en 7519200 bytes. Windows listó `COM5` desde el registro y no lo abrió. Los adaptadores de Mac y Linux devolvieron `not_measured` en este PC. `host\gui_server.py --self-test` salió con 0. En el navegador, una frase virtual buena escribió `CUE v1 1 prepare` y `CUE v1 1 speak virtual` con `whisper language=en`. Una línea mala escribió `CUE v1 error`. Las dos usaron BoundedKernel.

```
C:\Python313\python.exe host\gui_server.py
```

## Inicio fija una sesión.

El 2026-11-24 la misma página ganó Inicio y Parada. Inicio no escribe `gui.json` ni `destination.json`. El puerto, el proveedor, el bot elegido y el idioma de voz son argumentos de ese proceso de sesión. El puerto no se copia a `POTETOBOT_PORT`. Omitir `--stt-language` en la CLI sigue usando coreano. Un POST necesita `Content-Type: application/json`, `Host: 127.0.0.1:<port>`, `Origin: http://127.0.0.1:<port>` y el token de la página. Un origen distinto devuelve 403 y no escribe nada. Un segundo Inicio mientras hay conexión devuelve 409.

La autoprueba conserva esa ruta de inicio. Solo cambia el abridor serie por un puerto virtual finito y solo la CLI de Orca por una banda local. Una frase envió una pregunta a `bot:Poteto`. `prepare` y `speak` compartieron el id de esa frase, y el texto de la respuesta fue `band-answer`. Un id de bot vacío y `grok_bot` enviaron cero preguntas. Parada no añadió un speak tardío. `COM99` por el abridor real mostró `serial open failed COM99` y ningún identificador. COM5 no se abrió.

En el navegador de Windows el campo del puerto pasó de `COM5` a `COM99` antes de Inicio. La página mostró `Failed · serial open failed COM99` y un identificador vacío. La búsqueda `zzz-no-such` mostró `No search matches.` La lista completa siguió siendo `It'sMe`, `서치린`, `Poteto`, `정리린`, `버그린`, `리뷰린`. Vaciar la caja volvió a mostrar los seis. Actualizar con la caja en `Poteto` no quitó los otros nombres. Mint es solo una tarjeta de vista previa: 0 cuadros de placa, señales de voz apagadas, SEL apagado, un PNG idle. El control de Grok construyó un enlace real `grokbot://app/v1/prompt?id=fromshare&text=hello` con target `_blank`. Copiar cambió el botón a `Copied`. Pulsar ese enlace no inició una sesión. Con el idioma de voz en coreano, la frase virtual buena escribió `CUE v1 1 prepare` y `CUE v1 1 speak 가상`, `openedPort=false`. La frase de error escribió `CUE v1 1 error` y ninguna línea speak.

Este PC no tiene `os.openpty`. La prueba de parámetros usó un par de sockets y los bytes `sock-byte`. No midió un dispositivo Mac o Linux. Abrir, leer, escribir y cerrar en Windows pasó por `open_serial` con un sustituto del kernel: 115200, 8 bits de datos, paridad 0, bits de parada 0, flags 1. No se creó un identificador de dispositivo.

`C:\Python313\python.exe host\gui_server.py --self-test` salió con 0. `C:\Python313\python.exe host\mvp_host.py --self-test` salió con 0. Ambos corrieron desde `C:\Dev\ESP32Bot` sin `POTETOBOT_PORT`. `%APPDATA%\potetobot\gui.json`, `destination.json` y `host-terminal.json` no cambiaron. El firmware no se reconstruyó. `firmware\build\potetobot.bin` tiene 7519200 bytes. El SHA256 es `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`. El tope de fábrica sigue en 8388608 bytes.

En un checkout limpio, el archivo de selección se crea bajo `host\build`. Ese archivo no es `%APPDATA%\potetobot\destination.json`.

```
C:\Python313\python.exe -c "import json, pathlib; p=pathlib.Path('host/build'); p.mkdir(parents=True, exist_ok=True); p.joinpath('orca-dest.json').write_text(json.dumps({'provider':'orca_bots','botId':'Poteto'})+'\n', encoding='utf-8')"
```

## La placa es una Waveshare ESP32-S3-Touch-AMOLED-2.16.

| Dato | Valor |
| --- | --- |
| Objetivo | ESP32-S3 a 240 MHz (`sdkconfig.defaults`) |
| Flash | 16 MB. El archivo de valores por defecto pone QIO. La escritura del banco usó `--flash_mode dio` y `--flash_freq 80m` |
| PSRAM | Octal, 80 MHz |
| Consola | USB Serial/JTAG, 115200 8N1 |
| Panel | 480×480 |
| Aplicación factory | Desplazamiento `0x10000`, tamaño `0x800000` (8388608 bytes) |
| Partición font | Nombre `font`, subtipo `0x40`, desplazamiento `0x810000`, tamaño `0x200000` (2097152 bytes) |
| Toque | Dirección `0x5A`. SCL GPIO14, SDA GPIO15, interrupción GPIO11, reinicio GPIO40 |

## Los cuadros empaquetados son la cara.

`play_task` vuelve a dibujar cada 66 ms. El índice del cuadro avanza con el paso propio de la expresión, `1000 / fps`.

| Personaje | Expresión | Cuadros | fps | Paso |
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

`1000/6` es 166, `1000/15` es 66 y `1000/12` es 83. Las tres son divisiones enteras.

La voz elige solo poteto: listening mientras KEY3 está pulsado, thinking al soltar, tilt con `CUE v1 <utt> prepare`, speaking con `CUE v1 <utt> speak`, e idle con `CUE v1 <utt> idle` o `CUE v1 <utt> error`. Sunny se elige solo con `SEL`. Una señal de otra pulsación de KEY3 no cambia la cara.

Estos nombres están en el planificador y se empaquetan con cero cuadros. La placa los rechaza. No envía `ACK`, y la cara que ya se muestra permanece.

| Personaje | Nombres con cero cuadros |
| --- | --- |
| poteto | emphasis, smile |
| sunny | tilt, breath, emphasis |
| ambos | happy, surprised, sleepy, dizzy, wink |

## Una línea serie elige la cara.

```
SEL v1 poteto idle
SEL v1 sunny smile
```

Una expresión empaquetada responde con esta forma:

```
ACK poteto idle
```

El analizador acepta un campo final de milisegundos, como en `SEL v1 poteto speaking 500`. La placa empieza entonces el bucle en el primer cuadro. Ese campo no busca una posición.

`SEL v1 poteto emphasis` y `SEL v1 sunny tilt` cumplen la gramática y no tienen cuadros, así que no hay `ACK`. `SEL v1 poteto listening` tiene dos cuadros.

## El texto de la respuesta viaja por la misma línea.

```
TXT v1 thinking
TXT v1 The potato is hot.
SAY v1 thinking The potato is hot.
SAY v1 answer The potato is hot.
```

| Línea | Fase | Texto guardado en el globo |
| --- | --- | --- |
| `TXT v1 thinking` | thinking | `...` |
| `TXT v1` más cualquier otro texto | answer | ese texto |
| `SAY v1 thinking` más texto | thinking | ese texto |
| `SAY v1 answer` más texto | answer | ese texto |

La placa responde `ACK say thinking` o `ACK say answer`. `POTETO_SAY_TEXT_MAX` es 480 bytes, incluido el NUL final, así que el texto guardado mide como máximo 479 bytes.

El rectángulo del globo es x=12, y=356, ancho 456, alto 112, con un radio de esquina de 14. Los píxeles de debajo se desplazan a la derecha dos veces, lo que deja cada canal RGB565 en cerca de un cuarto, y no se borran. El borde y los glifos de answer son `0xFFFF`. Los glifos de thinking son `0xCE79`. La placa guarda los últimos 8 mensajes. La ventana de texto mide 80 píxeles de alto y cada línea mide 18, así que caben cuatro líneas. Un arrastre vertical desplaza las líneas guardadas.

Cuando el peer acepta un `SEL` cuya expresión es `thinking`, también escribe `TXT v1 thinking`.

## El peer de Windows habla grok-bot u orca.

| Controlador | Cuándo acepta ejecutarse |
| --- | --- |
| `grok-bot` | `%APPDATA%\Grok Bot\desktop-status.json` contiene `"signedIn":true`, y el pid de ese archivo sigue en ejecución. Este es el valor por defecto. |
| `orca` | Hay un proceso cuya imagen se llama `Orca.exe`. |

```
host\build\potetobot-peer.exe --driver grok-bot
host\build\potetobot-peer.exe --driver orca
```

`--driver` gana a `POTETOBOT_DRIVER`. El nombre del puerto sale de `POTETOBOT_PORT`. El peer lo abre a 115200 8N1 con DTR y RTS apagados, así que abrir el puerto no reinicia el chip en modo de descarga. Reenvía las líneas `SEL` y las líneas que empiezan por `TXT v1`. En stderr, grok-bot imprime `driver=grok-bot pid=<pid> signedIn=true`. orca imprime `driver=orca`.

Construye el peer desde la raíz del repositorio con el compilador x64 de Visual Studio:

```
cl /nologo /O2 /I firmware\main /Fe:host\build\potetobot-peer.exe host\peer_main.c firmware\main\schedule.c firmware\main\frame_table.c firmware\main\say.c
```

`host/test_schedule.c` comprueba el bucle de cuadros. Ejecuta `host\build\test_schedule.exe` con la raíz del repositorio, o sin argumento para usar `.`. Sus comprobaciones de speaking usan 6 fps: poteto en `(1000/6)*2` espera `poteto/speaking/f02.png`, y sunny en `(1000/6)*3` espera `sunny/speaking/f03.png`. `tests/test_peer.py` necesita un Grok Bot con sesión iniciada. `tests/test_peer_orca.py` necesita `Orca.exe`. `tests/test_frame_budget.py` compara el blob empaquetado con el límite de factory.

## El atlas de glifos queda fuera de la imagen factory.

`firmware/font/bubble_font.bin` mide 1325432 bytes. Es un subconjunto de GNU Unifont 16.0.04, y `firmware/main/ascii8x16.inc` es el respaldo ASCII que `firmware/font/LICENSE.txt` nombra a su lado. La línea de grabación escribe el atlas en `0x810000`. Esa es la partición `font`, no parte de la aplicación factory de 8388608 bytes.

## Esta construcción no reproduce audio.

El altavoz sigue apagado. La captura de KEY3 es solo recepción I2S: 16 kHz, mono, 16 bits con signo, en `firmware/main/key3.c`. Una tarea de captura lee ese flujo aparte del dibujo de 66 ms. Los trozos llenos salen como líneas `AUD v1`. El protocolo serie no tiene ajuste de idioma ni walkie-talkie. Las palabras del peer y el extracto corto de la respuesta se dibujan en el globo.

## El árbol se reparte así.

| Ruta | Qué guarda |
| --- | --- |
| `firmware/` | Aplicación ESP-IDF, `partitions.csv` y la imagen de la fuente |
| `firmware/main/frames.bin` | Cuadros RGB565 RLE que la construcción incrusta |
| `host/peer_main.c` | Peer de Windows |
| `host/mvp_host.py` | Entrada de voz de Windows |
| `host/gui_server.py` | Página Windows de bucle local |
| `assets/frames/` | Cuadros de origen, 480×480 |
| `tests/` | Comprobaciones del peer y del presupuesto de cuadros |
