[English](README.md) · [简体中文](README.zh-CN.md) · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md) · [한국어](README.ko.md)

# Potetobot

Potetobot 是一个放在桌上的小装置。圆屏上有一张脸。有字要读的时候，脸上会出现一个对话气泡。它跑在微雪 Waveshare ESP32-S3-Touch-AMOLED-2.16 上。屏幕里有两个角色：poteto 和 sunny。Windows 上有一个很小的对端程序，叫做 peer。它用同一条 USB 串口，把表情行和回答文字发给板子。

固件不用声音回答。`firmware/main` 里没有音频输出。板子上有 ES8311 喇叭和麦克风，这一版固件两个都不用。没有语言菜单，也没有对讲机。peer 送来的字画在屏幕上。

脸的样子保持原样。Poteto 仍然抱着红薯。Sunny 仍然拿着报纸，眼睛上方仍然是那副太阳镜。

## 这块板子

微雪 ESP32-S3-Touch-AMOLED-2.16（SKU 33969）用的是 ESP32-S3R8，带 8 MB PSRAM 和 16 MB 闪存。屏幕是 480×480 的 CO5300 AMOLED，走四线 SPI。触摸是 CST9217（或 CST9220），挂在 I2C 上：SCL 是 GPIO14，SDA 是 GPIO15，中断是 GPIO11，复位是 GPIO40。电源管理是 AXP2101。用户按键是 GPIO18。

在这台电脑上，USB Serial/JTAG 口枚举成 COM5。peer 用 115200 波特和它说话。打开串口时 DTR 和 RTS 都关掉，所以启动 peer 不会把芯片复位进下载模式。

factory 应用分区是 8388608 字节，从 `0x10000` 到 `0x810000`。应用程序必须放进这个分区。气泡用的字库放不进去。字库是单独的数据分区，名字叫 `font`。

## 脸

Poteto 是黑发、带一点腮红、圆的金边眼镜，手里是一根红薯。Sunny 是短黑发，棕色太阳镜架在眼睛上方，手里是一份印着太阳的报纸。画面锚点在 480×480 画布的 x 255、y 271。这个工程不重画这些图，也不改帧速。

已经打进固件的表情，以及循环使用的帧率：

| 表情 | 每秒帧数 |
| --- | --- |
| idle | 6 |
| listening | 15 |
| thinking | 12 |
| speaking | 6 |
| tilt | 6 |
| breath | 6 |
| emphasis | 6 |
| smile | 6 |

Poteto 的 listening 和 thinking 没有打包进去的帧。板子拒绝这两条选择，不应答，屏幕上原来的脸留着。Sunny 有 listening 和 thinking 的帧。两个人说话都是每秒 6 帧的短循环。播放任务大约每 66 毫秒重画一次。

## SEL

选择行写出角色和表情：

```
SEL v1 poteto idle
SEL v1 sunny speaking
```

表情后面可以再跟一个可选的毫秒时间。板子回一行：

```
ACK poteto idle
```

peer 在自己的标准输出上打印 `poteto idle`，并把原来的那一行从串口转发出去。`grok-bot` 和 `orca` 都这样做。它们共用同一个读取循环。

## SAY 和对话气泡

回答的文字走同一条连接上的第二行：

```
SAY v1 thinking still working
SAY v1 answer hello potato
```

`thinking` 是机器人还在想的时候。`answer` 是回答已经到了的时候。这一行剩下的部分是 UTF-8 文字，最多保留 480 字节。板子回答：

```
ACK say thinking
ACK say answer
```

不是 `thinking` 也不是 `answer` 的行会被拒绝。`SAY` 行不会换成一张脸。

两个阶段都会在屏幕上打开气泡。矩形是 x = 12，y = 356，宽 456，高 112。它在眼睛下面，也在太阳镜下面。红薯的上半段，以及报纸上的太阳，在气泡上方保持原来的亮度。气泡盖住的像素被压暗到大约四分之一，不是擦成黑色。气泡没有小尾巴。一圈白色圆角边框，半径 14，框住这块变暗的区域。思考中的字用偏暗的白色。回答的字用白色。

气泡里大约放得下四行。更长的文字在空格处换行。板子留下最近八条消息。新消息会跳到最新的几行。要读更早的字，把手指按在气泡里面再拖。手指往上移，下面的字就出来。滚动停在最旧的一行，也停在最新的一行。从气泡外面开始的拖动不会滚动它。

气泡画在已经解码的脸上，然后这一帧才送到屏幕。脸、红薯、报纸和太阳镜还是原来的那些图。

## 字库

factory 镜像只剩下大约一万五千字节的空位，装不下一套多语言字库。字库是 GNU Unifont 16.0.04，放在 factory 镜像外面。

`tools/gen_bubble_font.py` 写出 `firmware/font/bubble_font.bin`。这棵树里的文件是 1325432 字节。分区写在 `firmware/partitions.csv`：

```
font, data, 0x40, 0x810000, 0x200000
```

也就是从 `0x810000` 开始的 2097152 字节，紧挨在 factory 应用后面。固件启动时映射这个分区。如果魔数不对，应用里仍有一张很小的 Unifont 表，可以画英文字母，其他文字不行。署名在 `firmware/font/LICENSE.txt`。不要把 Windows 字体光栅化以后放进要公开的文件。`tools/gen_font.py` 不是这块板子读取的字库。

## 驱动

`host/peer_main.c` 就是 peer。默认驱动是 `grok-bot`。它读取 `%APPDATA%\Grok Bot\desktop-status.json`，只有 `signedIn` 为真，而且记录的进程号还活着，才会继续。

`orca` 查找正在运行的 `Orca.exe`。它不读 Grok Bot 的状态文件，也不要求登录 Grok。

```
potetobot-peer.exe
potetobot-peer.exe --driver grok-bot
potetobot-peer.exe --driver orca
```

`POTETOBOT_DRIVER` 用同样的名字。两边都设置时，`--driver` 优先。`POTETOBOT_PORT` 选择板子，例如 `COM5`。两个驱动都读标准输入。`SEL` 行换脸。`SAY` 行原样转发，所以任何一个驱动都能把思考中的字或回答的字送到屏幕上。

在仓库根目录，用 Visual Studio 的 x64 工具编译 peer：

```
cl /nologo /O2 /I firmware\main /Fe:host\build\potetobot-peer.exe host\peer_main.c firmware\main\schedule.c firmware\main\frame_table.c firmware\main\say.c
```

`host/build/test_schedule.exe` 检查表情循环和 `SAY` 解析。参数是仓库根目录。`tests/test_peer.py` 需要已登录的 Grok Bot。`tests/test_peer_orca.py` 需要 `Orca.exe`。

## 编译固件

使用 ESP-IDF 5.5。在 PowerShell 里：

```
. C:\Esp\esp-idf\export.ps1
cd firmware
idf.py build
```

脸的打包文件已经是 `firmware/main/frames.bin`。平常的编译把这个文件嵌进去，不会重画角色。应用镜像是 `firmware/build/potetobot.bin`。如果它大于 8388608 字节，不要烧录。

## 在 COM5 上烧录

在 `firmware\build` 里，用 ESP-IDF 的 Python：

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

bootloader、分区表和应用是通常的三份镜像。`bubble_font.bin` 写在 `0x810000`，那是 `font` 分区，不属于 factory 应用。硬复位之后，等到日志说应用已经从 `app_main` 返回，再发送 `SEL` 或 `SAY`。
