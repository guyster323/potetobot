# Potetobot

**Grokbot with me anywhere.**

一块 480×480 的圆屏显示 poteto 或 sunny 的一张脸，并在同一帧上用气泡画出回答文字。

[English](README.md) · [한국어](README.ko.md) · **简体中文** · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md)

[快速开始](#快速开始) · [一行串口](#一行串口选定脸) · [打包帧](#打包帧就是脸)

## 面板显示两张脸和一个气泡。

poteto 留着长黑发、刘海、圆金丝眼镜、腮红、黑衣服，以及脸旁边冒着热气的红薯。sunny 留着短黑发、盖在眼睛上的棕色墨镜、腮红、深色拉链，以及画着太阳的报纸。背景是黑色。脸铺满 480 的画布。

| poteto 待机，帧 f00 | sunny 待机，帧 f00 |
| --- | --- |
| ![poteto 待机，帧 f00](assets/frames/poteto/idle/f00.png) | ![sunny 待机，帧 f00](assets/frames/sunny/idle/f00.png) |
| poteto 微笑，帧 f00 | sunny 微笑，帧 f00 |
| ![poteto 微笑，帧 f00](assets/frames/poteto/smile/f00.png) | ![sunny 微笑，帧 f00](assets/frames/sunny/smile/f00.png) |

这些图是打包帧。这个仓库没有演示视频。

## 快速开始

1. **用 ESP-IDF 5.5 构建固件。** 在工作台的 PowerShell 里：

```
. C:\Esp\esp-idf\export.ps1
cd firmware
idf.py build
```

脸部数据包已经是 `firmware/main/frames.bin`。普通构建只嵌入这个文件，不会重画角色。应用镜像是 `firmware/build/potetobot.bin`。

2. **如果该镜像大于 8388608 字节就停下。** 这是 factory 分区的大小。2026-11-21 在工作台构建的镜像是 8377584 字节，因此还剩 11024 字节。

3. **烧录 bootloader、分区表、应用和字体。** 在 `firmware\build` 里用 ESP-IDF 的 Python。工作台的板子枚举为 COM5。请用你实际得到的端口。

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **等到日志写明程序已从 `app_main` 返回。** 然后再发送 `SEL` 或 `TXT`。

5. **启动 peer，先发一行选择，再发一行文字。**

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

peer 读取 stdin。如果没有 `host\build\potetobot-peer.exe`，先构建它。`cl` 命令在 peer 一节。

## 板子是 Waveshare ESP32-S3-Touch-AMOLED-2.16。

| 项目 | 值 |
| --- | --- |
| 目标 | ESP32-S3，240 MHz（`sdkconfig.defaults`） |
| 闪存 | 16 MB。默认文件设置 QIO。工作台烧录使用 `--flash_mode dio` 和 `--flash_freq 80m` |
| PSRAM | 八线，80 MHz |
| 控制台 | USB Serial/JTAG，115200 8N1 |
| 面板 | 480×480 |
| factory 应用 | 偏移 `0x10000`，大小 `0x800000`（8388608 字节） |
| font 分区 | 名称 `font`，子类型 `0x40`，偏移 `0x810000`，大小 `0x200000`（2097152 字节） |
| 触摸 | 地址 `0x5A`。SCL GPIO14，SDA GPIO15，中断 GPIO11，复位 GPIO40 |

## 打包帧就是脸。

`play_task` 每 66 ms 重画一次。帧序号按该表情自己的步进 `1000 / fps` 前进。

| 角色 | 表情 | 帧数 | fps | 步进 |
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

`1000/6` 是 166，`1000/15` 是 66，`1000/12` 是 83，都是整数除法。

这些名字在调度器里，但打包帧数是 0。板子拒绝它们。它不发送 `ACK`，已经显示的脸保持不变。

| 角色 | 帧数为 0 的名字 |
| --- | --- |
| poteto | listening, thinking |
| sunny | tilt, breath, emphasis |
| 两者 | happy, surprised, sleepy, dizzy, wink |

## 一行串口选定脸。

```
SEL v1 poteto idle
SEL v1 sunny smile
```

有打包帧的表情按这个形状回答：

```
ACK poteto idle
```

解析器接受末尾的毫秒字段，例如 `SEL v1 poteto speaking 500`。板子随后从第一帧开始循环。该字段不会跳转。

`SEL v1 poteto listening` 和 `SEL v1 sunny tilt` 语法正确但没有帧，所以没有 `ACK`。

## 回答文字走同一条线。

```
TXT v1 thinking
TXT v1 The potato is hot.
SAY v1 thinking The potato is hot.
SAY v1 answer The potato is hot.
```

| 行 | 阶段 | 气泡里保存的文字 |
| --- | --- | --- |
| `TXT v1 thinking` | thinking | `...` |
| `TXT v1` 加上其他文字 | answer | 那段文字 |
| `SAY v1 thinking` 加上文字 | thinking | 那段文字 |
| `SAY v1 answer` 加上文字 | answer | 那段文字 |

板子回答 `ACK say thinking` 或 `ACK say answer`。`POTETO_SAY_TEXT_MAX` 是 480 字节，含结尾 NUL，所以保存的文字最多 479 字节。

气泡矩形是 x=12，y=356，宽 456，高 112，圆角半径 14。底下的像素右移两次，RGB565 通道大约变成四分之一，而且不会被擦掉。边框和 answer 字形是 `0xFFFF`。thinking 字形是 `0xCE79`。板子保留最近 8 条消息。文字窗口高 80 像素，每行 18 像素，所以放得下四行。垂直拖动会滚动已保存的行。

peer 接受表情为 `thinking` 的 `SEL` 时，还会写入 `TXT v1 thinking`。

## Windows peer 说 grok-bot 或 orca。

| 驱动 | 何时同意运行 |
| --- | --- |
| `grok-bot` | `%APPDATA%\Grok Bot\desktop-status.json` 含有 `"signedIn":true`，且文件里的 pid 仍在运行。这是默认值。 |
| `orca` | 有一个映像名为 `Orca.exe` 的进程在运行。 |

```
host\build\potetobot-peer.exe --driver grok-bot
host\build\potetobot-peer.exe --driver orca
```

`--driver` 优先于 `POTETOBOT_DRIVER`。端口名来自 `POTETOBOT_PORT`。peer 以 115200 8N1 打开，并关闭 DTR 和 RTS，因此打开端口不会把芯片复位进下载模式。它转发 `SEL` 行和以 `TXT v1` 开头的行。在 stderr 上，grok-bot 打印 `driver=grok-bot pid=<pid> signedIn=true`。orca 打印 `driver=orca`。

在仓库根目录用 Visual Studio x64 编译器构建 peer：

```
cl /nologo /O2 /I firmware\main /Fe:host\build\potetobot-peer.exe host\peer_main.c firmware\main\schedule.c firmware\main\frame_table.c firmware\main\say.c
```

`host/test_schedule.c` 检查帧循环。运行 `host\build\test_schedule.exe` 时传入仓库根目录，省略参数则使用 `.`。speaking 检查使用 6 fps：poteto 在 `(1000/6)*2` 时期望 `poteto/speaking/f02.png`，sunny 在 `(1000/6)*3` 时期望 `sunny/speaking/f03.png`。`tests/test_peer.py` 需要已登录的 Grok Bot。`tests/test_peer_orca.py` 需要 `Orca.exe`。`tests/test_frame_budget.py` 把打包块和 factory 上限比较。

## 字形图集不在 factory 镜像里。

`firmware/font/bubble_font.bin` 是 1325432 字节。它是 GNU Unifont 16.0.04 的子集，`firmware/main/ascii8x16.inc` 是 `firmware/font/LICENSE.txt` 在旁边写明的 ASCII 后备。烧录行把图集写到 `0x810000`。那是 `font` 分区，不属于 8388608 字节的 factory 应用。

## 这个构建不播放声音。

`firmware/main` 没有音频输出。串口协议没有语言设置，也没有对讲。来自 peer 的文字画在气泡里。

## 目录是这样分的。

| 路径 | 内容 |
| --- | --- |
| `firmware/` | ESP-IDF 应用、`partitions.csv` 和字体镜像 |
| `firmware/main/frames.bin` | 构建嵌入的 RGB565 RLE 帧 |
| `host/peer_main.c` | Windows peer |
| `assets/frames/` | 源帧，480×480 |
| `tests/` | peer 和帧预算检查 |
