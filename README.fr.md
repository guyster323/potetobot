[English](README.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md) · [한국어](README.ko.md)

# Potetobot

Potetobot est un petit appareil de bureau. L'écran rond montre un visage et, quand il y a quelque chose à lire, une bulle. Il tourne sur une carte Waveshare ESP32-S3-Touch-AMOLED-2.16. Deux personnages y vivent : poteto et sunny. Un petit programme Windows, le pair, envoie les lignes d'expression et le texte de la réponse sur la même liaison série USB.

Le micrologiciel ne répond pas à la voix. `firmware/main` n'a pas de sortie audio. La carte porte un haut-parleur ES8311 et un microphone, et cette version ne se sert ni de l'un ni de l'autre. Il n'y a pas de menu de langue et pas de talkie-walkie. Les mots envoyés par le pair sont dessinés à l'écran.

Le visage reste tel qu'il est. Poteto garde la patate douce. Sunny garde le journal et les lunettes de soleil au-dessus des yeux.

## La carte

La Waveshare ESP32-S3-Touch-AMOLED-2.16 (SKU 33969) est un module ESP32-S3R8 avec 8 Mo de PSRAM et 16 Mo de flash. La dalle est un AMOLED CO5300 de 480 par 480 sur un bus SPI à quatre lignes. Le tactile est un CST9217 (ou CST9220) sur I2C : SCL est GPIO14, SDA est GPIO15, l'interruption est GPIO11 et la remise à zéro est GPIO40. L'alimentation est un AXP2101. La touche utilisateur est GPIO18.

Sur cette machine, le port USB Serial/JTAG apparaît comme COM5. Le pair lui parle à 115200 bauds. Il ouvre le port avec DTR coupé et RTS coupé, donc lancer le pair ne bascule pas la puce en mode téléchargement.

La partition d'application factory fait 8388608 octets, de `0x10000` à `0x810000`. L'image de l'application doit tenir dedans. L'atlas des glyphes de la bulle n'y tient pas. Il occupe une partition de données à part, nommée `font`.

## Les visages

Poteto a les cheveux noirs, un peu de rouge aux joues, des lunettes rondes à monture dorée et une patate douce. Sunny a les cheveux noirs et courts, des lunettes de soleil brunes posées au-dessus des yeux et un journal avec un soleil. L'ancre du dessin est en x 255, y 271, sur un canevas de 480 par 480. Ce projet ne redessine pas ces images et ne change pas le rythme des images.

Expressions empaquetées, et images par seconde de la boucle :

| Expression | Images par seconde |
| --- | --- |
| idle | 6 |
| listening | 15 |
| thinking | 12 |
| speaking | 6 |
| tilt | 6 |
| breath | 6 |
| emphasis | 6 |
| smile | 6 |

Poteto listening et poteto thinking n'ont pas d'images empaquetées. La carte refuse ces deux sélections, ne les acquitte pas et laisse à l'écran le visage déjà affiché. Sunny a bien une image d'écoute et une image de réflexion. Parler, pour les deux personnages, est une courte boucle à 6 images par seconde. Le lecteur redessine environ toutes les 66 ms.

## SEL

Une ligne de sélection nomme le personnage et l'expression :

```
SEL v1 poteto idle
SEL v1 sunny speaking
```

Un temps facultatif, en millisecondes, peut suivre l'expression. La carte répond par une ligne :

```
ACK poteto idle
```

Le pair écrit `poteto idle` sur sa propre sortie standard et transmet la ligne d'origine sur le port série. `grok-bot` et `orca` font tous les deux cela. Ils partagent une seule boucle de lecture.

## SAY et la bulle

Le texte de la réponse emprunte une seconde ligne sur la même liaison :

```
SAY v1 thinking still working
SAY v1 answer hello potato
```

`thinking` est la ligne pendant que le bot travaille encore. `answer` est la ligne quand la réponse est arrivée. Le reste de la ligne est du texte UTF-8, conservé jusqu'à 480 octets. La carte répond :

```
ACK say thinking
ACK say answer
```

Toute phase autre que `thinking` ou `answer` est refusée. Une ligne `SAY` n'est pas un choix de visage.

Les deux phases ouvrent une bulle sur la dalle. Le rectangle est x = 12, y = 356, largeur 456, hauteur 112. Il se place sous les yeux et sous les lunettes de soleil. Le haut de la patate douce, et le soleil du journal, restent à pleine lumière au-dessus de la bulle. Les pixels sous la bulle sont assombris jusqu'à environ un quart de leur luminosité. Ils ne sont pas effacés, et la bulle n'a pas de queue. Un bord blanc arrondi, de rayon 14, encadre la zone assombrie. Le texte de réflexion est dessiné dans un blanc plus doux. Le texte de la réponse est dessiné en blanc.

Environ quatre lignes tiennent dans la bulle. Un texte plus long se coupe sur les espaces. La carte garde les huit derniers messages. Un nouveau message saute aux lignes les plus récentes. Pour lire les plus anciennes, pose un doigt dans la bulle et fais-le glisser. Monter le doigt révèle le texte du dessous. Le défilement s'arrête à la ligne la plus ancienne et à la plus récente. Un glissement qui commence hors de la bulle ne la fait pas défiler.

La bulle est peinte sur le visage déjà décodé, puis l'image part vers la dalle. Le visage, la patate douce, le journal et les lunettes de soleil sont les mêmes images qu'avant.

## L'atlas de glyphes

L'image factory n'a plus qu'environ quinze kilo-octets de libre, ce qui ne suffit pas pour une police multilingue. L'atlas est GNU Unifont 16.0.04, rangé hors de l'image factory.

`tools/gen_bubble_font.py` écrit `firmware/font/bubble_font.bin`. Le fichier de cet arbre fait 1325432 octets. La partition est déclarée dans `firmware/partitions.csv` :

```
font, data, 0x40, 0x810000, 0x200000
```

Ce sont 2097152 octets à partir de `0x810000`, juste après l'application factory. Le micrologiciel mappe cette partition au démarrage. Si la signature manque, les lettres anglaises sortent encore d'une petite table Unifont liée dans l'application, et les autres écritures non. L'attribution est dans `firmware/font/LICENSE.txt`. Ne rastérise pas une police Windows dans un fichier que tu publies. `tools/gen_font.py` n'est pas la police que lit cette carte.

## Pilotes

`host/peer_main.c` est le pair. Le pilote par défaut est `grok-bot`. Il lit `%APPDATA%\Grok Bot\desktop-status.json` et ne continue que si `signedIn` est vrai et si le numéro de processus noté est encore vivant.

`orca` cherche un `Orca.exe` en cours d'exécution. Il ne lit pas le fichier d'état de Grok Bot et ne demande pas une connexion Grok.

```
potetobot-peer.exe
potetobot-peer.exe --driver grok-bot
potetobot-peer.exe --driver orca
```

`POTETOBOT_DRIVER` choisit les mêmes noms. Quand les deux sont posés, `--driver` l'emporte. `POTETOBOT_PORT` choisit la carte, par exemple `COM5`. Les deux pilotes lisent l'entrée standard. Une ligne `SEL` change le visage. Une ligne `SAY` est transmise telle quelle, donc l'un ou l'autre peut mettre à l'écran le texte de réflexion ou celui de la réponse.

Compile le pair avec les outils x64 de Visual Studio, depuis la racine du dépôt :

```
cl /nologo /O2 /I firmware\main /Fe:host\build\potetobot-peer.exe host\peer_main.c firmware\main\schedule.c firmware\main\frame_table.c firmware\main\say.c
```

`host/build/test_schedule.exe` vérifie la boucle d'images et l'analyseur `SAY`. L'argument est la racine du dépôt. `tests/test_peer.py` a besoin d'un Grok Bot connecté. `tests/test_peer_orca.py` a besoin d'`Orca.exe`.

## Compiler le micrologiciel

Utilise ESP-IDF 5.5. Dans PowerShell :

```
. C:\Esp\esp-idf\export.ps1
cd firmware
idf.py build
```

Le paquet de visages est déjà `firmware/main/frames.bin`. Une compilation ordinaire embarque ce fichier. Elle ne repeint pas les personnages. L'image de l'application est `firmware/build/potetobot.bin`. Ne la grave pas si elle dépasse 8388608 octets.

## Graver sur COM5

Depuis `firmware\build`, avec le Python d'ESP-IDF :

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

Le bootloader, la table de partitions et l'application sont les trois images habituelles. `bubble_font.bin` est écrit à `0x810000`, qui est la partition `font` et non une partie de l'application factory. Après la remise à zéro dure, attends que le journal dise que l'application est revenue de `app_main` avant d'envoyer `SEL` ou `SAY`.
