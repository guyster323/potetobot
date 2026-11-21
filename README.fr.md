# Potetobot

**Grokbot with me anywhere.**

Un panneau rond de 480 par 480 montre un visage, poteto ou sunny, et dessine le texte de la réponse dans une bulle sur la même image.

[English](README.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · **Français** · [বাংলা](README.bn.md)

[Démarrage rapide](#démarrage-rapide) · [La ligne série](#une-ligne-série-choisit-le-visage) · [Images empaquetées](#les-images-empaquetées-sont-le-visage)

## Le panneau montre deux visages et une bulle.

Poteto garde de longs cheveux noirs, une frange, des lunettes rondes dorées, du rose aux joues, des vêtements noirs et une patate douce fumante à côté du visage. Sunny garde des cheveux noirs courts, des lunettes de soleil brunes sur les yeux, du rose aux joues, une fermeture sombre et un journal avec un soleil dessiné. Le fond est noir. Le visage remplit le canevas de 480.

| poteto au repos, image f00 | sunny au repos, image f00 |
| --- | --- |
| ![poteto au repos, image f00](assets/frames/poteto/idle/f00.png) | ![sunny au repos, image f00](assets/frames/sunny/idle/f00.png) |
| poteto souriant, image f00 | sunny souriant, image f00 |
| ![poteto souriant, image f00](assets/frames/poteto/smile/f00.png) | ![sunny souriant, image f00](assets/frames/sunny/smile/f00.png) |

Ces dessins sont des images empaquetées. Ce dépôt n'a pas de vidéo de démonstration.

## Démarrage rapide

1. **Construis le micrologiciel avec ESP-IDF 5.5.** Dans le PowerShell du banc :

```
. C:\Esp\esp-idf\export.ps1
cd firmware
idf.py build
```

Le paquet des visages est déjà `firmware/main/frames.bin`. Une construction ordinaire embarque ce fichier. Elle ne repeint pas les personnages. L'image de l'application est `firmware/build/potetobot.bin`.

2. **Arrête-toi si cette image dépasse 8388608 octets.** C'est la partition factory. L'image du banc, construite le 2026-11-21, fait 8377584 octets, donc il reste 11024 octets.

3. **Flashe le bootloader, la table des partitions, l'application et la police.** Depuis `firmware\build`, avec le Python d'ESP-IDF. La carte du banc s'énumère en COM5. Utilise le port que tu as vraiment.

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **Attends que le journal dise que l'application est revenue de `app_main`.** Envoie ensuite `SEL` ou `TXT`.

5. **Lance le pair et envoie une sélection, puis une ligne de texte.**

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

Le pair lit stdin. Construis-le d'abord si `host\build\potetobot-peer.exe` manque. La ligne `cl` est dans la section du pair.

## La carte est une Waveshare ESP32-S3-Touch-AMOLED-2.16.

| Élément | Valeur |
| --- | --- |
| Cible | ESP32-S3 à 240 MHz (`sdkconfig.defaults`) |
| Flash | 16 MB. Le fichier de valeurs par défaut règle QIO. L'écriture du banc a utilisé `--flash_mode dio` et `--flash_freq 80m` |
| PSRAM | Octale, 80 MHz |
| Console | USB Serial/JTAG, 115200 8N1 |
| Panneau | 480×480 |
| Application factory | Décalage `0x10000`, taille `0x800000` (8388608 octets) |
| Partition font | Nom `font`, sous-type `0x40`, décalage `0x810000`, taille `0x200000` (2097152 octets) |
| Toucher | Adresse `0x5A`. SCL GPIO14, SDA GPIO15, interruption GPIO11, réinitialisation GPIO40 |

## Les images empaquetées sont le visage.

`play_task` redessine toutes les 66 ms. L'index d'image avance selon le pas propre à l'expression, `1000 / fps`.

| Personnage | Expression | Images | fps | Pas |
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

`1000/6` vaut 166, `1000/15` vaut 66 et `1000/12` vaut 83. Les trois sont des divisions entières.

Ces noms sont dans l'ordonnanceur et sont empaquetés avec zéro image. La carte les refuse. Elle n'envoie pas `ACK`, et le visage déjà affiché reste.

| Personnage | Noms à zéro image |
| --- | --- |
| poteto | listening, thinking |
| sunny | tilt, breath, emphasis |
| les deux | happy, surprised, sleepy, dizzy, wink |

## Une ligne série choisit le visage.

```
SEL v1 poteto idle
SEL v1 sunny smile
```

Une expression empaquetée répond sous cette forme :

```
ACK poteto idle
```

L'analyseur accepte un champ final en millisecondes, comme `SEL v1 poteto speaking 500`. La carte démarre ensuite la boucle à la première image. Ce champ ne cherche pas une position.

`SEL v1 poteto listening` et `SEL v1 sunny tilt` respectent la grammaire et n'ont pas d'images, donc il n'y a pas d'`ACK`.

## Le texte de la réponse emprunte la même ligne.

```
TXT v1 thinking
TXT v1 The potato is hot.
SAY v1 thinking The potato is hot.
SAY v1 answer The potato is hot.
```

| Ligne | Phase | Texte gardé dans la bulle |
| --- | --- | --- |
| `TXT v1 thinking` | thinking | `...` |
| `TXT v1` plus un autre texte | answer | ce texte |
| `SAY v1 thinking` plus un texte | thinking | ce texte |
| `SAY v1 answer` plus un texte | answer | ce texte |

La carte répond `ACK say thinking` ou `ACK say answer`. `POTETO_SAY_TEXT_MAX` fait 480 octets, NUL final compris, donc le texte gardé mesure au plus 479 octets.

Le rectangle de la bulle est x=12, y=356, largeur 456, hauteur 112, avec un rayon de coin de 14. Les pixels en dessous sont décalés deux fois vers la droite, ce qui ramène chaque canal RGB565 à environ un quart, et ils ne sont pas effacés. Le bord et les glyphes answer valent `0xFFFF`. Les glyphes thinking valent `0xCE79`. La carte garde les 8 derniers messages. La fenêtre de texte fait 80 pixels de haut et chaque ligne fait 18 pixels, donc quatre lignes tiennent. Un glissement vertical fait défiler les lignes gardées.

Quand le pair accepte un `SEL` dont l'expression est `thinking`, il écrit aussi `TXT v1 thinking`.

## Le pair Windows parle grok-bot ou orca.

| Pilote | Quand il accepte de tourner |
| --- | --- |
| `grok-bot` | `%APPDATA%\Grok Bot\desktop-status.json` contient `"signedIn":true`, et le pid de ce fichier tourne encore. C'est le défaut. |
| `orca` | Un processus dont l'image s'appelle `Orca.exe` tourne. |

```
host\build\potetobot-peer.exe --driver grok-bot
host\build\potetobot-peer.exe --driver orca
```

`--driver` l'emporte sur `POTETOBOT_DRIVER`. Le nom du port vient de `POTETOBOT_PORT`. Le pair l'ouvre à 115200 8N1 avec DTR et RTS coupés, donc ouvrir le port ne remet pas la puce en mode téléchargement. Il transmet les lignes `SEL` et les lignes qui commencent par `TXT v1`. Sur stderr, grok-bot imprime `driver=grok-bot pid=<pid> signedIn=true`. orca imprime `driver=orca`.

Construis le pair depuis la racine du dépôt avec le compilateur x64 de Visual Studio :

```
cl /nologo /O2 /I firmware\main /Fe:host\build\potetobot-peer.exe host\peer_main.c firmware\main\schedule.c firmware\main\frame_table.c firmware\main\say.c
```

`host/test_schedule.c` vérifie la boucle d'images. Lance `host\build\test_schedule.exe` avec la racine du dépôt, ou sans argument pour utiliser `.`. Ses contrôles de speaking utilisent 6 fps : poteto à `(1000/6)*2` attend `poteto/speaking/f02.png`, et sunny à `(1000/6)*3` attend `sunny/speaking/f03.png`. `tests/test_peer.py` a besoin d'un Grok Bot connecté. `tests/test_peer_orca.py` a besoin d'`Orca.exe`. `tests/test_frame_budget.py` compare le blob empaqueté à la limite factory.

## L'atlas de glyphes reste hors de l'image factory.

`firmware/font/bubble_font.bin` fait 1325432 octets. C'est un sous-ensemble de GNU Unifont 16.0.04, et `firmware/main/ascii8x16.inc` est le repli ASCII que `firmware/font/LICENSE.txt` nomme à côté. La ligne de flash écrit l'atlas à `0x810000`. C'est la partition `font`, pas une partie de l'application factory de 8388608 octets.

## Cette construction ne joue pas de son.

`firmware/main` n'a pas de sortie audio. Le protocole série n'a pas de réglage de langue ni de talkie-walkie. Les mots du pair sont dessinés dans la bulle.

## L'arbre se répartit ainsi.

| Chemin | Ce qu'il contient |
| --- | --- |
| `firmware/` | Application ESP-IDF, `partitions.csv` et l'image de police |
| `firmware/main/frames.bin` | Images RGB565 RLE que la construction embarque |
| `host/peer_main.c` | Pair Windows |
| `assets/frames/` | Images sources, 480×480 |
| `tests/` | Contrôles du pair et du budget d'images |
