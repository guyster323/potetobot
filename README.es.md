# Potetobot

**Grokbot with me anywhere.**

Un panel redondo de 480 por 480 muestra una cara, poteto o sunny, y dibuja el texto de la respuesta en un globo sobre el mismo cuadro.

[English](README.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · **Español** · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md)

[Inicio rápido](#inicio-rápido) · [La línea serie](#una-línea-serie-elige-la-cara) · [Cuadros empaquetados](#los-cuadros-empaquetados-son-la-cara)

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

2. **Detente si esa imagen supera 8388608 bytes.** Ese es el tamaño de la partición factory. La imagen del banco, construida el 2026-11-21, mide 8377584 bytes, así que quedan 11024 bytes.

3. **Graba el bootloader, la tabla de particiones, la aplicación y la fuente.** Desde `firmware\build`, con el Python de ESP-IDF. La placa del banco aparece como COM5. Usa el puerto que tengas de verdad.

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **Espera a que el registro diga que la aplicación ha vuelto de `app_main`.** Después envía `SEL` o `TXT`.

5. **Arranca el peer y envía una selección y luego una línea de texto.**

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

El peer lee stdin. Constrúyelo primero si falta `host\build\potetobot-peer.exe`. La línea `cl` está en la sección del peer.

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

`1000/6` es 166, `1000/15` es 66 y `1000/12` es 83. Las tres son divisiones enteras.

Estos nombres están en el planificador y se empaquetan con cero cuadros. La placa los rechaza. No envía `ACK`, y la cara que ya se muestra permanece.

| Personaje | Nombres con cero cuadros |
| --- | --- |
| poteto | listening, thinking |
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

`SEL v1 poteto listening` y `SEL v1 sunny tilt` cumplen la gramática y no tienen cuadros, así que no hay `ACK`.

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

`firmware/main` no tiene salida de audio. El protocolo serie no tiene ajuste de idioma ni walkie-talkie. Las palabras del peer se dibujan en el globo.

## El árbol se reparte así.

| Ruta | Qué guarda |
| --- | --- |
| `firmware/` | Aplicación ESP-IDF, `partitions.csv` y la imagen de la fuente |
| `firmware/main/frames.bin` | Cuadros RGB565 RLE que la construcción incrusta |
| `host/peer_main.c` | Peer de Windows |
| `assets/frames/` | Cuadros de origen, 480×480 |
| `tests/` | Comprobaciones del peer y del presupuesto de cuadros |
