[English](README.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md) · [한국어](README.ko.md)

# Potetobot

Potetobot es un aparato de escritorio. En la pantalla redonda hay una cara y, cuando hay algo que leer, un globo de texto. Funciona en una Waveshare ESP32-S3-Touch-AMOLED-2.16. En el panel viven dos personajes: poteto y sunny. Un programa pequeño de Windows, el peer, envía las líneas de expresión y el texto de la respuesta por la misma conexión serie USB.

El firmware no contesta con la voz. `firmware/main` no tiene salida de audio. La placa lleva un altavoz ES8311 y un micrófono, y esta compilación no usa ninguno de los dos. No hay menú de idioma ni walkie-talkie. Las palabras que manda el peer se dibujan en la pantalla.

El aspecto de las caras se queda como está. Poteto sigue con el boniato. Sunny sigue con el periódico y las gafas de sol por encima de los ojos.

## La placa

La Waveshare ESP32-S3-Touch-AMOLED-2.16 (SKU 33969) es un módulo ESP32-S3R8 con 8 MB de PSRAM y 16 MB de flash. El panel es un AMOLED CO5300 de 480 por 480 en un bus SPI de cuatro líneas. El tacto es un CST9217 (o CST9220) en I2C: SCL es GPIO14, SDA es GPIO15, la interrupción es GPIO11 y el reset es GPIO40. La alimentación es un AXP2101. La tecla de usuario es GPIO18.

En este ordenador el puerto USB Serial/JTAG aparece como COM5. El peer habla con él a 115200 baudios. Abre el puerto con DTR desactivado y RTS desactivado, así que abrir el peer no mete el chip en modo de descarga.

La partición de aplicación factory ocupa 8388608 bytes, de `0x10000` a `0x810000`. La imagen de la aplicación tiene que caber ahí. El atlas de glifos del globo no cabe. Va en una partición de datos aparte, llamada `font`.

## Las caras

Poteto tiene el pelo negro, un poco de rubor, gafas redondas de montura dorada y un boniato. Sunny tiene el pelo negro y corto, gafas de sol marrones apoyadas encima de los ojos y un periódico con un sol. El ancla del dibujo está en x 255, y 271, sobre un lienzo de 480 por 480. Este proyecto no vuelve a dibujar esas imágenes y no cambia la velocidad de los fotogramas.

Expresiones empaquetadas y fotogramas por segundo del bucle:

| Expresión | Fotogramas por segundo |
| --- | --- |
| idle | 6 |
| listening | 15 |
| thinking | 12 |
| speaking | 6 |
| tilt | 6 |
| breath | 6 |
| emphasis | 6 |
| smile | 6 |

Poteto listening y poteto thinking no tienen fotogramas empaquetados. La placa rechaza esas dos selecciones, no las confirma y deja en pantalla la cara que ya estaba. Sunny sí tiene un fotograma de listening y uno de thinking. Hablar, en los dos personajes, es un bucle corto a 6 fotogramas por segundo. El reproductor redibuja más o menos cada 66 ms.

## SEL

Una línea de selección nombra el personaje y la expresión:

```
SEL v1 poteto idle
SEL v1 sunny speaking
```

Después de la expresión puede ir un tiempo opcional en milisegundos. La placa responde con una línea:

```
ACK poteto idle
```

El peer escribe `poteto idle` en su propia salida estándar y reenvía la línea original por el puerto serie. `grok-bot` y `orca` hacen lo mismo. Comparten un solo bucle de lectura.

## SAY y el globo

El texto de la respuesta usa una segunda línea en esa misma conexión:

```
SAY v1 thinking still working
SAY v1 answer hello potato
```

`thinking` es la línea de mientras el bot sigue trabajando. `answer` es la línea de cuando la respuesta ya ha llegado. El resto de la línea es texto UTF-8, conservado hasta 480 bytes. La placa responde:

```
ACK say thinking
ACK say answer
```

Cualquier fase que no sea `thinking` ni `answer` se rechaza. Una línea `SAY` no es una selección de cara.

Las dos fases abren un globo en el panel. El rectángulo es x = 12, y = 356, ancho 456, alto 112. Queda por debajo de los ojos y por debajo de las gafas de sol. La parte alta del boniato, y el sol del periódico, siguen a pleno brillo por encima del globo. Los píxeles que quedan debajo se atenúan hasta cerca de un cuarto de su brillo. No se borran, y el globo no tiene rabito. Un borde blanco redondeado, de radio 14, enmarca la zona atenuada. El texto de pensamiento se dibuja en un blanco más suave. El texto de la respuesta se dibuja en blanco.

En el globo caben unas cuatro líneas. El texto más largo se parte en los espacios. La placa guarda los últimos ocho mensajes. Un mensaje nuevo salta a las líneas más recientes. Para leer las anteriores, apoya un dedo dentro del globo y arrastra. Subir el dedo descubre el texto de abajo. El desplazamiento se detiene en la línea más antigua y en la más nueva. Un arrastre que empieza fuera del globo no lo mueve.

El globo se pinta sobre la cara ya decodificada y después el fotograma sale hacia el panel. La cara, el boniato, el periódico y las gafas de sol son las mismas imágenes de antes.

## El atlas de glifos

A la imagen factory le sobran unos quince kilobytes, y eso no alcanza para una fuente multilingüe. El atlas es GNU Unifont 16.0.04 y vive fuera de la imagen factory.

`tools/gen_bubble_font.py` escribe `firmware/font/bubble_font.bin`. El archivo de este árbol ocupa 1325432 bytes. La partición está declarada en `firmware/partitions.csv`:

```
font, data, 0x40, 0x810000, 0x200000
```

Son 2097152 bytes a partir de `0x810000`, justo después de la aplicación factory. El firmware mapea esa partición al arrancar. Si falta la marca mágica, las letras inglesas siguen saliendo de una tabla pequeña de Unifont enlazada en la aplicación, y el resto de escrituras no. La atribución está en `firmware/font/LICENSE.txt`. No rasterices una fuente de Windows en un archivo que vayas a publicar. `tools/gen_font.py` no es la fuente que lee esta placa.

## Controladores

`host/peer_main.c` es el peer. El controlador predeterminado es `grok-bot`. Lee `%APPDATA%\Grok Bot\desktop-status.json` y solo sigue si `signedIn` es verdadero y el identificador de proceso anotado sigue vivo.

`orca` busca un `Orca.exe` en ejecución. No lee el archivo de estado de Grok Bot y no pide un inicio de sesión en Grok.

```
potetobot-peer.exe
potetobot-peer.exe --driver grok-bot
potetobot-peer.exe --driver orca
```

`POTETOBOT_DRIVER` elige los mismos nombres. Si están los dos, gana `--driver`. `POTETOBOT_PORT` elige la placa, por ejemplo `COM5`. Los dos controladores leen la entrada estándar. Una línea `SEL` cambia la cara. Una línea `SAY` se reenvía tal cual, así que cualquiera de los dos puede poner en el panel el texto de pensamiento o el de la respuesta.

Compila el peer con las herramientas x64 de Visual Studio, desde la raíz del repositorio:

```
cl /nologo /O2 /I firmware\main /Fe:host\build\potetobot-peer.exe host\peer_main.c firmware\main\schedule.c firmware\main\frame_table.c firmware\main\say.c
```

`host/build/test_schedule.exe` comprueba el bucle de fotogramas y el analizador de `SAY`. El argumento es la raíz del repositorio. `tests/test_peer.py` necesita un Grok Bot con la sesión iniciada. `tests/test_peer_orca.py` necesita `Orca.exe`.

## Compilar el firmware

Usa ESP-IDF 5.5. En PowerShell:

```
. C:\Esp\esp-idf\export.ps1
cd firmware
idf.py build
```

El paquete de caras ya es `firmware/main/frames.bin`. Una compilación normal incrusta ese archivo. No vuelve a pintar los personajes. La imagen de la aplicación es `firmware/build/potetobot.bin`. No la grabes si pasa de 8388608 bytes.

## Grabar en COM5

Desde `firmware\build`, con el Python de ESP-IDF:

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

El bootloader, la tabla de particiones y la aplicación son las tres imágenes de siempre. `bubble_font.bin` se escribe en `0x810000`, que es la partición `font` y no forma parte de la aplicación factory. Después del reinicio duro, espera a que el registro diga que la aplicación ha vuelto de `app_main` antes de enviar `SEL` o `SAY`.
