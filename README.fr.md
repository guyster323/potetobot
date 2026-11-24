# Potetobot

**Grokbot with me anywhere.**

Un panneau rond de 480 par 480 montre un visage, poteto ou sunny, et dessine le texte de la réponse dans une bulle sur la même image.

[English](README.md) · [한국어](README.ko.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · **Français** · [বাংলা](README.bn.md)

[Démarrage rapide](#démarrage-rapide) · [L'hôte](#lhôte-demande-une-fois-à-un-bot-orca) · [La ligne série](#une-ligne-série-choisit-le-visage) · [Images empaquetées](#les-images-empaquetées-sont-le-visage)

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

2. **Arrête-toi si cette image dépasse 8388608 octets.** C'est la partition factory. L'image reconstruite le 2026-11-24 fait 7519200 octets, donc il reste 869408 octets. `check_sizes` a indiqué `0x72bbe0` face à `0x800000`, avec `0xd4420` octets libres. SHA256 est `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`. La somme `esptool image_info` est `ac` et le hachage de validation est `05873b3f41fdbb860ee18172114cb64205cc64dbe2d57d42a9b7b944e8023270`. Une image plus tôt ce jour-là faisait 7518928 octets.

3. **Le flash est l'étape du propriétaire.** Cette fin n'a pas ouvert de port et n'a pas flashé. Depuis `firmware\build`, avec le Python d'ESP-IDF. Le bootloader fait 22288 octets à `0x0`, la table des partitions fait 3072 octets à `0x8000`, l'application est à `0x10000`, et la police fait 1325432 octets à `0x810000`. Utilise le port que tu as vraiment.

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **Démarre l'hôte sur le PC avant l'essai de la carte.** Le transport sim ne pose pas `POTETOBOT_PORT`. Le `%APPDATA%\potetobot\destination.json` par défaut reste absent. Passe un fichier de destination. `C:\Python313\python.exe` a whisper, numpy et torch. La commande ci-dessous a tourné une fois le 2026-11-24. La relancer envoie une autre question.

```
C:\Python313\python.exe host\mvp_host.py --transport sim --wav tests\fixtures\heami-capital.wav --destination host\build\orca-dest.json
```

5. **Après le flash, le visage vocal est poteto.** Tiens KEY3 pour listening, deux images à 15 fps. Relâche pour thinking, deux images à 12 fps. Pendant l'attente de la demande, `CUE v1 <utt> prepare` choisit tilt, quatre images à 6 fps. Quand la réponse arrive, `CUE v1 <utt> speak` choisit speaking, quatre images à 6 fps, et dessine la bulle courte. `CUE v1 <utt> idle` et `CUE v1 <utt> error` choisissent idle. Fais défiler la bulle au toucher. `host\mvp_host.py --transport serial` continue de lire `POTETOBOT_PORT` par l'API de port Win32. `--inject` fournit les octets de la carte et n'ouvre pas de handle. pyserial n'est pas utilisé. Cette fin n'a pas flashé. Un signal d'une autre pression KEY3 ne change pas le visage. Un drop, une image abîmée, une fin absente, une destination vide, un échec du bot ou un délai montrent error pour cet énoncé, et l'énoncé suivant qui est bon continue. Le série sans `--inject` ne lance pas le simulateur de visage. Une écriture Win32 qui renvoie 0 ou n'écrit aucun octet n'est pas un signal envoyé, et une lecture en échec n'est pas un délai. Ce passage n'a pas ouvert de port COM.

6. **SEL, TXT et SAY passent encore par le pair.** Le pair est la ligne manuelle. Ce n'est pas le chemin vocal.

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

Le pair lit stdin. Construis-le d'abord si `host\build\potetobot-peer.exe` manque. La ligne `cl` est dans la section du pair.

## L'hôte demande une fois à un bot Orca.

`host\mvp_host.py` est l'entrée Windows. La commande de l'étape 4 est le départ qui a tourné. `--transport sim` et `--transport serial --inject` partagent cette session : octets AUD, `CUE v1 <utt> prepare` quand l'identifiant s'imprime, puis `CUE v1 <utt> speak` avec cette réponse. `--transport serial` sans `--inject` ouvre un handle COM depuis `POTETOBOT_PORT` et continue de lire. pyserial n'est pas importé. Le pair ouvre le sien pour SEL, TXT et SAY. Le mode sim découpe le WAV avec `host\aud_format.exe`, qui appelle le même format `AUD v1` que le firmware. L'hôte ne garde le PCM que si l'identifiant d'énoncé, la séquence, la longueur en octets et la ligne de fin concordent. Un trou, un doublon, une ligne hex courte, un journal mélangé, un drop ou une fin absente n'est pas transcrit. Un énoncé est lié à la fois. Une réponse tardive ne lance pas un second envoi. Un énoncé mauvais écrit `CUE v1 <utt> error` pour cet identifiant et la session continue. Une ligne s'arrête à 2048 octets. Un énoncé commencé attend au plus 8 secondes et 400 lignes AUD. Une fin perdue n'est pas collée à l'énoncé suivant. Pendant que Whisper ou la demande tourne, l'hôte continue de lire ce même handle. Il garde 16 énoncés au plus, chaque PCM à 192000 octets (6,0 s) au plus. Au-delà, il imprime `audio busy` et écrit `CUE v1 <utt> error`, et l'énoncé suivant continue.

| Limite | Valeur |
| --- | --- |
| PCM | 16 kHz, mono, s16, 32000 octets chaque seconde |
| Une image | 640 octets, puis une ligne `AUD v1 <utt> <seq> <len> <hex>` |
| Un énoncé | 192000 octets, soit 6.0 secondes |
| Question | au plus 2000 octets UTF-8, dans `--body-file` |
| Bulle | l'extrait de la réponse, sous 96 octets UTF-8 |
| Ligne | 2048 octets au plus |
| Énoncé commencé | 8 secondes et 400 lignes AUD au plus |
| Énoncés en attente | 16 au plus, chaque PCM 192000 octets (6,0 s) au plus |

La simulation du 2026-11-24 a lu `tests\fixtures\heami-capital.wav` (Microsoft Heami, 16 kHz, mono, s16). Le réassemblage faisait 151680 octets en 238 lignes. Whisper base sur CPU a transcrit `안녕하세요. 대한민국의 수도는 어디인가요?`. La seule demande était `msg_5f8b8f406d35`. La ligne de boîte `msg_789251e073d5` venait de `orca_session_id:6176a306-e513-4dff-b903-5bed98beb33d` et son corps était `서울입니다.`. La bulle était ce même extrait. La destination vide et `grok_bot` n'ont pas imprimé `request`.

L'exécution suivante `--transport serial --inject` a utilisé le même WAV et n'a pas ouvert de handle COM. Le PCM faisait 151680 octets. La demande était `msg_bf26672f0e57`. Après cet identifiant, l'image d'attente était `poteto/tilt/f02.png`. La réponse et la bulle speak étaient `서울입니다.`. `from` était `orca_session_id:1398eeac-555a-4337-bc0c-86e4e5d49458`.

Le handle d'envoi reste dans `%APPDATA%\potetobot\host-terminal.json`. Le fichier de destination et le firmware ne stockent pas d'identifiant de chat. `pyserial` vit dans l'environnement virtuel ESP-IDF et cette simulation ne l'a pas importé.

La commande de carte ci-dessous est le départ que le code accepte. Ce passage ne l'a pas exécutée et n'a pas ouvert de port COM. Lance-la depuis `C:\Dev\ESP32Bot`.

```
$env:POTETOBOT_PORT = "COM5"
C:\Python313\python.exe host\mvp_host.py --transport serial --destination host\build\orca-dest.json
```

`--inject` et `--transport sim` partagent la session observée et n'ouvrent pas de handle. Le `%APPDATA%\potetobot\destination.json` par défaut reste absent. `host\build\orca-dest.json` contient `{"provider":"orca_bots","botId":"Poteto"}`. Le fichier d'envoi `%APPDATA%\potetobot\host-terminal.json` est déjà là et tient un handle `term_`. S'il manque, l'hôte lance `orca terminal create --title potetobot-host --json` et écrit `{"handle":"term_..."}`. Une création mesurée plus tôt a utilisé `orca terminal create --worktree path:C:\Dev\ESP32Bot --title potetobot-host --json` et n'a pas passé `--focus`. Ne mets pas d'identifiant de chat dans le fichier de destination ni dans le firmware.

Le 2026-11-24, depuis `C:\Dev\ESP32Bot` et sans `POTETOBOT_PORT`, `C:\Python313\python.exe host\mvp_host.py --self-test` est sorti avec 0. La dernière ligne était `self-test ok`. Il n'a pas ouvert de port COM et n'a pas envoyé de nouvelle question Orca. Une session a répondu `하나` et `둘`. Une destination vide a imprimé `channel status not_sent` et aucune ligne `request`. Un délai a imprimé `audio timeout 6`, et l'énoncé suivant a répondu `다시`. Un drop, une image abîmée et une fin absente ont montré idle avec la bulle `error`, et l'énoncé suivant a continué. Après `btn 0 0102`, `btn 1 -` et `btn 0 0506`, les signaux sans étiquette `CUE v1 speak old` et `CUE v1 1 speak old` ont laissé le visage sur listening, sans bulle `old`. `CUE v1 2 prepare` a montré tilt `poteto/tilt/f02.png`, et `CUE v1 2 speak next` a montré speaking et la bulle `next`. Une seule pression acceptait encore `CUE v1 prepare` et `CUE v1 speak kept` sans étiquette. Un `WriteFile` Win32 qui a renvoyé 0, et un qui a échoué, ne sont pas restés dans `sent`. Une écriture partielle de 4 octets de `CUE v1 1 prepare` a été finie par l'écriture suivante. Dans la session série simulée, le premier morceau était `CUE `, `speak 하나` n'a pas été gardé, et les signaux gardés étaient `CUE v1 3 prepare`, `CUE v1 3 error`, `CUE v1 4 prepare` et `CUE v1 4 speak 둘`. Après ces octets, `ReadFile` a échoué et la session a imprimé `link read read failed` puis s'est arrêtée. `readline` sur une lecture en échec a appelé `ReadFile` deux fois et n'a pas tourné en boucle. L'exécutable du visage n'a pas tourné sur ce chemin simulé. Cette exécution est séparée de `msg_bf26672f0e57` et de `msg_5f8b8f406d35`.

Le même auto-test a ensuite utilisé un tampon de réception virtuel fini, pas un InjectLink qui charge toute l'entrée d'avance. Un second PCM de 16000 octets (0,5 s) est devenu 32437 octets de trame. Un tampon de 8192 octets non lu a perdu 24245 de ces octets. Avec l'hôte qui lit pendant la transcription bloquée, les octets perdus étaient 0 et les deux réponses étaient `하나` et `둘`. Avec un seul créneau d'attente, l'énoncé en trop a imprimé `audio busy` et son signal d'erreur, et la réponse suivante était `회복`. Le firmware n'a pas été reconstruit. L'image reste à 7519200 octets. La commande de carte ci-dessus n'a pas changé. Aucun port COM n'a été ouvert.

`host\gui_server.py` est la page en boucle locale. Le 2026-11-24 elle n'écoutait que `127.0.0.1:8765`. Elle n'a pas ouvert de port COM, n'a pas flashé, et n'a pas écrit `%APPDATA%\potetobot\destination.json`. La langue de l'écran et la langue de reconnaissance vocale sont stockées à part dans `%APPDATA%\potetobot\gui.json`. Le coréen est le défaut des deux. La reconnaissance anglaise est passée à Whisper comme `language=en`, avec `fp16=false` et `temperature=0`. Les noms Orca viennent de `orca bot list --json`. Un nom enregistré devient le `botId` de destination seulement quand une vraie question part. Ce passage n'a pas envoyé de nouvelle question Orca. Grok Bot n'a pas de liste de bots du compte ni de chemin de réponse documentés. Un id collé depuis Share a construit `grokbot://app/v1/prompt?id=fromshare&text=hello`. Ce lien n'envoie pas. Un HTTP 200 de webhook est une acceptation, pas une réponse. L'API de modèles xAI n'est pas utilisée. Les cartes de personnage lisent `firmware\main\frame_table.c` et `assets\frames`. Poteto sur la carte est idle 3, listening 2, thinking 2, speaking 4, tilt 4, breath 1. Sunny sur la carte est idle 2, listening 1, thinking 1, speaking 4, smile 1. Le smile et l'emphasis de Poteto restent à 0 image sur la carte, avec 5 et 3 PNG d'aperçu. Ce passage n'a pas ajouté d'images et l'image reste à 7519200 octets. Windows a listé `COM5` depuis le registre et ne l'a pas ouvert. Les adaptateurs Mac et Linux ont renvoyé `not_measured` sur ce PC. `host\gui_server.py --self-test` est sorti avec 0. Dans le navigateur, un énoncé virtuel correct a écrit `CUE v1 1 prepare` et `CUE v1 1 speak virtual` avec `whisper language=en`. Une mauvaise ligne a écrit `CUE v1 error`. Les deux ont utilisé BoundedKernel.

```
C:\Python313\python.exe host\gui_server.py
```

## Le démarrage fixe une session.

Le 2026-11-24 la même page a gagné Démarrer et Arrêter. Démarrer n'écrit pas `gui.json` ni `destination.json`. Le port, le fournisseur, le bot choisi et la langue de parole sont des arguments de ce processus de session. Le port n'est pas copié dans `POTETOBOT_PORT`. Omettre `--stt-language` sur la CLI garde le coréen. Un POST exige `Content-Type: application/json`, `Host: 127.0.0.1:<port>`, `Origin: http://127.0.0.1:<port>` et le jeton de la page. Une origine fausse renvoie 403 et n'écrit rien. Un second démarrage pendant la connexion renvoie 409.

L'auto-test garde ce chemin de démarrage. Il ne remplace que l'ouvreur série par un port virtuel fini et que la CLI Orca par une bande locale. Un énoncé a envoyé une question à `bot:Poteto`. `prepare` et `speak` ont partagé l'id de cet énoncé, et le texte de la réponse était `band-answer`. Un id de bot vide et `grok_bot` ont envoyé zéro question. L'arrêt n'a pas ajouté de speak tardif. `COM99` par l'ouvreur réel a montré `serial open failed COM99` et aucun handle. COM5 n'a pas été ouvert.

Dans le navigateur Windows le champ du port est passé de `COM5` à `COM99` avant Démarrer. La page a montré `Failed · serial open failed COM99` et un handle vide. La recherche `zzz-no-such` a montré `No search matches.` La liste complète est restée `It'sMe`, `서치린`, `Poteto`, `정리린`, `버그린`, `리뷰린`. Vider la case a réaffiché les six. Actualiser pendant que la case disait `Poteto` n'a pas retiré les autres noms. Mint n'est qu'une carte d'aperçu : 0 image sur la carte, signaux vocaux coupés, SEL coupé, un PNG idle. Le contrôle Grok a construit un vrai lien `grokbot://app/v1/prompt?id=fromshare&text=hello` avec target `_blank`. Copier a changé le bouton en `Copied`. Cliquer ce lien n'a pas démarré de session. Avec la langue de parole en coréen, le bon énoncé virtuel a écrit `CUE v1 1 prepare` et `CUE v1 1 speak 가상`, `openedPort=false`. L'énoncé d'erreur a écrit `CUE v1 1 error` et aucune ligne speak.

Ce PC n'a pas `os.openpty`. Le contrôle de paramètre a utilisé une paire de sockets et les octets `sock-byte`. Il n'a pas mesuré un appareil Mac ou Linux. L'ouverture, la lecture, l'écriture et la fermeture Windows sont passées par `open_serial` avec un substitut du noyau : 115200, 8 bits de données, parité 0, bits d'arrêt 0, flags 1. Aucun handle d'appareil n'a été créé.

`C:\Python313\python.exe host\gui_server.py --self-test` est sorti avec 0. `C:\Python313\python.exe host\mvp_host.py --self-test` est sorti avec 0. Les deux ont tourné depuis `C:\Dev\ESP32Bot` sans `POTETOBOT_PORT`. `%APPDATA%\potetobot\gui.json`, `destination.json` et `host-terminal.json` n'ont pas changé. Le firmware n'a pas été reconstruit. `firmware\build\potetobot.bin` fait 7519200 octets. Le SHA256 est `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`. Le plafond d'usine reste 8388608 octets.

Dans un checkout propre, le fichier de sélection se crée sous `host\build`. Ce fichier n'est pas `%APPDATA%\potetobot\destination.json`.

```
C:\Python313\python.exe -c "import json, pathlib; p=pathlib.Path('host/build'); p.mkdir(parents=True, exist_ok=True); p.joinpath('orca-dest.json').write_text(json.dumps({'provider':'orca_bots','botId':'Poteto'})+'\n', encoding='utf-8')"
```

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

`1000/6` vaut 166, `1000/15` vaut 66 et `1000/12` vaut 83. Les trois sont des divisions entières.

La voix choisit seulement poteto : listening tant que KEY3 est tenu, thinking au relâchement, tilt sur `CUE v1 <utt> prepare`, speaking sur `CUE v1 <utt> speak`, et idle sur `CUE v1 <utt> idle` ou `CUE v1 <utt> error`. Sunny se choisit seulement par `SEL`. Un signal d'une autre pression KEY3 ne change pas le visage.

Ces noms sont dans l'ordonnanceur et sont empaquetés avec zéro image. La carte les refuse. Elle n'envoie pas `ACK`, et le visage déjà affiché reste.

| Personnage | Noms à zéro image |
| --- | --- |
| poteto | emphasis, smile |
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

`SEL v1 poteto emphasis` et `SEL v1 sunny tilt` respectent la grammaire et n'ont pas d'images, donc il n'y a pas d'`ACK`. `SEL v1 poteto listening` a deux images.

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

Le haut-parleur reste éteint. La capture KEY3 est une réception I2S seule : 16 kHz, mono, 16 bits signés, dans `firmware/main/key3.c`. Une tâche de capture lit ce flux à part du dessin de 66 ms. Les morceaux pleins sortent en lignes `AUD v1`. Le protocole série n'a pas de réglage de langue ni de talkie-walkie. Les mots du pair et le court extrait de la réponse sont dessinés dans la bulle.

## L'arbre se répartit ainsi.

| Chemin | Ce qu'il contient |
| --- | --- |
| `firmware/` | Application ESP-IDF, `partitions.csv` et l'image de police |
| `firmware/main/frames.bin` | Images RGB565 RLE que la construction embarque |
| `host/peer_main.c` | Pair Windows |
| `host/mvp_host.py` | Entrée vocale Windows |
| `host/gui_server.py` | Page Windows en boucle locale |
| `assets/frames/` | Images sources, 480×480 |
| `tests/` | Contrôles du pair et du budget d'images |
