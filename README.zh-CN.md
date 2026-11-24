# Potetobot

**Grokbot with me anywhere.**

一块 480×480 的圆屏显示 poteto 或 sunny 的一张脸，并在同一帧上用气泡画出回答文字。

[English](README.md) · [한국어](README.ko.md) · **简体中文** · [हिन्दी](README.hi.md) · [Español](README.es.md) · [العربية](README.ar.md) · [Français](README.fr.md) · [বাংলা](README.bn.md)

[快速开始](#快速开始) · [主机](#主机向一个-orca-机器人问一次) · [一行串口](#一行串口选定脸) · [打包帧](#打包帧就是脸)

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

2. **如果该镜像大于 8388608 字节就停下。** 这是 factory 分区的大小。2026-11-24 从 `1a20bc0` 加上 KEY3 与帧源重新构建的镜像是 7519200 字节，因此还剩 869408 字节。`check_sizes` 报告 `0x72bbe0` 对照 `0x800000`，空闲 `0xd4420` 字节。SHA256 是 `ea8bb1687f014dc02fbfc2454eb640363f7c195d6483e614d195d9c8213fa6ff`。`esptool image_info` 校验和是 `61`，验证哈希是 `1b61a0a6f1d26abc2ec27ba97d4bfd57781db3604078c6fec8bb1152eef3fb26`。应用版本字符串是 `1a20bc0-dirty`。较早的同大小镜像 SHA256 是 `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`。这次构建没有烧录。

3. **烧录是所有者的步骤。** 这次收尾没有打开端口，也没有烧录。在 `firmware\build` 里用 ESP-IDF 的 Python。bootloader 是 22288 字节，位于 `0x0`。分区表是 3072 字节，位于 `0x8000`。应用在 `0x10000`。字体是 1325432 字节，位于 `0x810000`。请用你实际得到的端口。

```
python -m esptool --chip esp32s3 -p COM5 -b 460800 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader\bootloader.bin 0x8000 partition_table\partition-table.bin 0x10000 potetobot.bin 0x810000 ..\font\bubble_font.bin
```

4. **板子测试之前先在 PC 上启动主机。** 模拟传输不设置 `POTETOBOT_PORT`。默认的 `%APPDATA%\potetobot\destination.json` 保持不存在。传入一个目的地文件。`C:\Python313\python.exe` 带有 whisper、numpy 和 torch。下面的命令在 2026-11-24 运行过一次。再运行会再发送一个问题。

```
C:\Python313\python.exe host\mvp_host.py --transport sim --wav tests\fixtures\heami-capital.wav --destination host\build\orca-dest.json
```

5. **烧录之后，语音脸是 poteto。** 按住 KEY3 是 listening，2 帧，15 fps。松开是 thinking，2 帧，12 fps。请求等待时 `CUE v1 <utt> prepare` 选择 tilt，4 帧，6 fps。答案到达时 `CUE v1 <utt> speak` 选择 speaking，4 帧，6 fps，并画出短气泡。`CUE v1 <utt> idle` 和 `CUE v1 <utt> error` 选择 idle。用触摸滚动气泡。`host\mvp_host.py --transport serial` 通过 Win32 端口 API 持续读取 `POTETOBOT_PORT`。`--inject` 喂入板子字节并且不打开句柄。不使用 pyserial。这次收尾没有烧录。 另一次 KEY3 按下的提示不会改变脸。drop、损坏的帧、缺少结束行、空目的地、机器人失败或超时会为该话语显示 error，下一句正常话语仍然继续。没有 `--inject` 的串口不会启动脸部模拟器。返回 0 或没有写入字节的 Win32 写入不会记成已发送的提示，失败的读取不是超时。这一次没有打开 COM 端口。

6. **SEL、TXT 和 SAY 仍走 peer。** peer 是手动行。它不是语音路径。

```
$env:POTETOBOT_PORT = "COM5"
host\build\potetobot-peer.exe
SEL v1 poteto idle
TXT v1 hello
```

peer 读取 stdin。如果没有 `host\build\potetobot-peer.exe`，先构建它。`cl` 命令在 peer 一节。

## 主机向一个 Orca 机器人问一次。

`host\mvp_host.py` 是 Windows 入口。第 4 步的命令是跑过的起点。`--transport sim` 和 `--transport serial --inject` 共用该会话：读入 AUD 字节，打印请求 id 时写 `CUE v1 <utt> prepare`，该答案到达时写 `CUE v1 <utt> speak`。没有 `--inject` 的 `--transport serial` 从 `POTETOBOT_PORT` 打开一个 COM 句柄并持续读取。不导入 pyserial。peer 为自己的 SEL、TXT、SAY 另开句柄。模拟模式用 `host\aud_format.exe` 切分 WAV，该程序调用与固件相同的 `AUD v1` 格式。主机只在话语编号、序号、字节长度和结束行一致时保留 PCM。缺口、重复、短 hex 行、混入的日志、drop 或缺少结束行都不会被转录。一次只绑定一个话语。迟到的答案不会开始第二次发送。 坏的话语写入该编号的 `CUE v1 <utt> error`，会话继续。一行停在 2048 字节。已开始的话语最多等 8 秒，最多 400 行 AUD。丢失的结束行不会接到下一句。 Whisper 或请求进行时，主机继续读同一个句柄。最多保存 16 句，每句 PCM 最多 192000 字节（6.0 秒）。超过上限时打印 `audio busy` 并写入 `CUE v1 <utt> error`，下一句仍然继续。

| 限制 | 值 |
| --- | --- |
| PCM | 16 kHz，单声道，s16，每秒 32000 字节 |
| 一帧 | 640 字节，然后一行 `AUD v1 <utt> <seq> <len> <hex>` |
| 一次话语 | 192000 字节，即 6.0 秒 |
| 问题 | 最多 2000 个 UTF-8 字节，放在 `--body-file` |
| 气泡 | 回答摘录，少于 96 个 UTF-8 字节 |
| 一行 | 最多 2048 字节 |
| 已开始的话语 | 最多 8 秒，400 行 AUD |
| 等待中的话语 | 最多 16 句，每句 PCM 最多 192000 字节（6.0 秒） |

2026-11-24 的模拟读取了 `tests\fixtures\heami-capital.wav`（Microsoft Heami，16 kHz，单声道，s16）。重组为 151680 字节、238 行。CPU 上的 Whisper base 转录为 `안녕하세요. 대한민국의 수도는 어디인가요?`。唯一的请求是 `msg_5f8b8f406d35`。收件箱行 `msg_789251e073d5` 来自 `orca_session_id:6176a306-e513-4dff-b903-5bed98beb33d`，正文是 `서울입니다.`。气泡是同一段摘录。空目的地和 `grok_bot` 没有打印 `request`。

之后的 `--transport serial --inject` 用了同一个 WAV，没有打开 COM 句柄。PCM 是 151680 字节。请求是 `msg_bf26672f0e57`。该 id 之后的等待帧是 `poteto/tilt/f02.png`。答案和 speak 气泡是 `서울입니다.`。`from` 是 `orca_session_id:1398eeac-555a-4337-bc0c-86e4e5d49458`。

发送句柄只留在 `%APPDATA%\potetobot\host-terminal.json`。目的地文件和固件不存放聊天 id。`pyserial` 在 ESP-IDF 虚拟环境里，这次模拟没有导入它。

下面的实机命令是代码接受的启动。这一次没有运行它，也没有打开 COM 端口。从 `C:\Dev\ESP32Bot` 启动。

```
$env:POTETOBOT_PORT = "COM5"
C:\Python313\python.exe host\mvp_host.py --transport serial --destination host\build\orca-dest.json
```

`--inject` 和 `--transport sim` 共用观察会话，并且不打开句柄。默认的 `%APPDATA%\potetobot\destination.json` 保持不存在。`host\build\orca-dest.json` 的内容是 `{"provider":"orca_bots","botId":"Poteto"}`。发送文件 `%APPDATA%\potetobot\host-terminal.json` 已经存在，并且持有 `term_` 句柄。如果该文件缺失，主机运行 `orca terminal create --title potetobot-host --json` 并写入 `{"handle":"term_..."}`。更早一次实测的创建用的是 `orca terminal create --worktree path:C:\Dev\ESP32Bot --title potetobot-host --json`，没有传 `--focus`。不要把聊天 id 放进目的地文件或固件。

2026-11-24，在 `C:\Dev\ESP32Bot` 且未设置 `POTETOBOT_PORT` 时，`C:\Python313\python.exe host\mvp_host.py --self-test` 退出码为 0。最后一行是 `self-test ok`。它没有打开 COM 端口，也没有发送新的 Orca 问题。同一次会话的答案是 `하나` 和 `둘`。空目的地打印 `channel status not_sent`，没有 `request` 行。超时打印 `audio timeout 6`，下一句的答案是 `다시`。drop、损坏的帧和缺少结束行显示 idle 和气泡 `error`，下一句仍然继续。在 `btn 0 0102`、`btn 1 -`、`btn 0 0506` 之后，不带话语编号的 `CUE v1 speak old` 和 `CUE v1 1 speak old` 让脸停留在 listening，没有气泡 `old`。`CUE v1 2 prepare` 显示 tilt `poteto/tilt/f02.png`，`CUE v1 2 speak next` 显示 speaking 和气泡 `next`。只按一次时，不带编号的 `CUE v1 prepare` 和 `CUE v1 speak kept` 仍然生效。返回 0 的 Win32 `WriteFile` 和失败的写入都没有进入 `sent`。`CUE v1 1 prepare` 的 4 字节部分写入由下一次写入补完。模拟串口会话的第一块是 `CUE `，`speak 하나` 没有被保存。保存的提示是 `CUE v1 3 prepare`、`CUE v1 3 error`、`CUE v1 4 prepare` 和 `CUE v1 4 speak 둘`。这些字节之后 `ReadFile` 失败，会话打印 `link read read failed` 并停止。失败读取上的 `readline` 调用了两次 `ReadFile`，没有空转。该模拟路径没有运行脸部可执行文件。这次运行与 `msg_bf26672f0e57` 和 `msg_5f8b8f406d35` 分开。

同一次自测接着使用了有限的虚拟接收缓冲区，而不是预先装入全部输入的 InjectLink。第二句 16000 字节（0.5 秒）的 PCM 变成 32437 字节的帧。不读取时，8192 字节缓冲区丢掉其中 24245 字节。转录被堵住时主机继续读取，丢掉的字节是 0，两个答案是 `하나` 和 `둘`。等待槽只有一个时，超出的话语打印 `audio busy` 并写下 error 提示，下一句的答案是 `회복`。固件没有重新构建。镜像仍是 7519200 字节。上面的实机命令没有改。没有打开 COM 端口。

`host\gui_server.py` 是回环页面。2026-11-24 它只监听 `127.0.0.1:8765`。它没有打开 COM 端口，没有烧录，也没有写 `%APPDATA%\potetobot\destination.json`。画面语言和语音识别语言分开存在 `%APPDATA%\potetobot\gui.json`。两者默认都是韩语。英语语音识别传给 Whisper 的是 `language=en`、`fp16=false`、`temperature=0`。Orca 名字来自 `orca bot list --json`。保存的一个名字只在真正发问时成为目的地的 `botId`。这一次没有发送新的 Orca 问题。Grok Bot 没有记载账户机器人列表或回答路径。粘贴的 Share id 组成 `grokbot://app/v1/prompt?id=fromshare&text=hello`。该链接不发送。Webhook 的 HTTP 200 是接受，不是回答。不使用 xAI 模型 API。角色卡读取 `firmware\main\frame_table.c` 和 `assets\frames`。板上的 Poteto 是 idle 3、listening 2、thinking 2、speaking 4、tilt 4、breath 1。板上的 Sunny 是 idle 2、listening 1、thinking 1、speaking 4、smile 1。Poteto 的 smile 和 emphasis 在板上是 0 帧，预览 PNG 分别是 5 张和 3 张。这一次没有加入图片，镜像仍是 7519200 字节。Windows 从注册表列出 `COM5` 并且没有打开它。Mac 和 Linux 适配器在这台 PC 上返回 `not_measured`。`host\gui_server.py --self-test` 退出码为 0。浏览器里一次正常的虚拟话语写下 `CUE v1 1 prepare` 和 `CUE v1 1 speak virtual`，并且 `whisper language=en`。坏行写下 `CUE v1 error`。两者都使用 BoundedKernel。

```
C:\Python313\python.exe host\gui_server.py
```

## 开始会固定一个会话。

2026-11-24 同一页面增加了开始和停止。开始不写 `gui.json` 或 `destination.json`。端口、提供方、所选机器人、语音语言是该会话进程的参数。端口不会复制到 `POTETOBOT_PORT`。CLI 省略 `--stt-language` 时仍用韩语。POST 需要 `Content-Type: application/json`、`Host: 127.0.0.1:<port>`、`Origin: http://127.0.0.1:<port>` 和页面令牌。错误来源返回 403 并且不写文件。已连接时第二次开始返回 409。

自测保留这条开始路径。只把串口打开换成有限虚拟端口，只把 Orca CLI 换成本地带。一句话语向 `bot:Poteto` 发了一个问题。`prepare` 和 `speak` 共用该话语 id，回答文本是 `band-answer`。空机器人 id 和 `grok_bot` 发送 0 次。停止没有补上迟到的 speak。真实打开器的 `COM99` 显示 `serial open failed COM99`，没有句柄。没有打开 COM5。

在 Windows 浏览器里，开始之前把端口栏从 `COM5` 改成 `COM99`。页面显示 `Failed · serial open failed COM99` 和空句柄。搜索 `zzz-no-such` 显示 `No search matches.`。完整列表仍是 `It'sMe`、`서치린`、`Poteto`、`정리린`、`버그린`、`리뷰린`。清空输入框后六个名字再次出现。输入框为 `Poteto` 时刷新没有丢掉其他名字。Mint 只是预览卡：板载帧 0，不用语音提示，不发 SEL，一张 idle PNG。Grok 控件做成真实链接 `grokbot://app/v1/prompt?id=fromshare&text=hello`，target 为 `_blank`。复制把按钮改成 `Copied`。点击该链接没有启动会话。语音语言为韩语时，正常虚拟话语写下 `CUE v1 1 prepare` 和 `CUE v1 1 speak 가상`，`openedPort=false`。错误话语写下 `CUE v1 1 error`，没有 speak 行。

这台 PC 没有 `os.openpty`。参数检查用了套接字对和字节 `sock-byte`。没有测量 Mac 或 Linux 设备。Windows 的打开、读取、写入、关闭走带内核替身的 `open_serial`：115200、8 数据位、奇偶校验 0、停止位 0、flags 1。没有创建设备句柄。

`C:\Python313\python.exe host\gui_server.py --self-test` 退出码 0。`C:\Python313\python.exe host\mvp_host.py --self-test` 退出码 0。两者都在 `C:\Dev\ESP32Bot` 运行，且未设置 `POTETOBOT_PORT`。`%APPDATA%\potetobot\gui.json`、`destination.json`、`host-terminal.json` 未改变。固件没有重新构建。`firmware\build\potetobot.bin` 为 7519200 字节。SHA256 是 `07671b8214030d477f7fb6323f43c2ab0d59f4eb7988bf5a2d784b5332d46041`。出厂上限仍是 8388608 字节。

干净检出时，选择文件建在 `host\build` 下。该文件不是 `%APPDATA%\potetobot\destination.json`。

```
C:\Python313\python.exe -c "import json, pathlib; p=pathlib.Path('host/build'); p.mkdir(parents=True, exist_ok=True); p.joinpath('orca-dest.json').write_text(json.dumps({'provider':'orca_bots','botId':'Poteto'})+'\n', encoding='utf-8')"
```

## 帧包与表一致。

2026-11-24，`1a20bc0` 的干净检出运行 `host\build\test_schedule.exe` 失败，输出 `schedule: poteto listening was accepted`。帧表已经把 poteto 的 listening 和 thinking 计为 2，而 `frames.bin` 仍是旧的 8089636 字节块，调度测试仍拒绝这些循环。

`tools\gen_frames.py` 把该块重写为 7199282 字节。SHA256 是 `19401f0a6ce56bfd05e09209868b9379d05db4f323b84a852ead53734e9ab1bb`。`firmware\main\frame_table.c` 没有变化。预算检查打印 `budget ok 8068962 blob 7199282`。Poteto 的 smile 和 emphasis 打包帧仍是 0。重新构建的调度测试打印 `schedule ok`。`tests\test_key3.py` 打印 `key3 ok`。`tests\test_bubble_font.py` 对现有 1325432 字节图集打印 `pbf1 ok`。

在 `firmware` 中 `idf.py build` 退出码 0。`firmware\build\potetobot.bin` 为 7519200 字节。SHA256 是 `ea8bb1687f014dc02fbfc2454eb640363f7c195d6483e614d195d9c8213fa6ff`。`check_sizes` 报告 `0x72bbe0` 对照 `0x800000`，空闲 `0xd4420` 字节。`esptool image_info` 校验和是 `61`，验证哈希是 `1b61a0a6f1d26abc2ec27ba97d4bfd57781db3604078c6fec8bb1152eef3fb26`。应用版本字符串是 `1a20bc0-dirty`。镜像含有 `AUD v1`、`i2s rx 16000` 和 `key3 gpio18 armed`。引导程序 22288 字节，分区表 3072 字节。出厂上限仍是 8388608 字节。这次构建没有烧录，也没有打开 COM 端口。最后写入板子的镜像仍是 2026-11-22 的 8387456 字节。

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

`1000/6` 是 166，`1000/15` 是 66，`1000/12` 是 83，都是整数除法。

语音只选择 poteto：按住 KEY3 时 listening，松开时 thinking，`CUE v1 <utt> prepare` 为 tilt，`CUE v1 <utt> speak` 为 speaking，`CUE v1 <utt> idle` 或 `CUE v1 <utt> error` 为 idle。sunny 只由 `SEL` 选择。 另一次 KEY3 按下的提示不会改变脸。

这些名字在调度器里，但打包帧数是 0。板子拒绝它们。它不发送 `ACK`，已经显示的脸保持不变。

| 角色 | 帧数为 0 的名字 |
| --- | --- |
| poteto | emphasis, smile |
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

`SEL v1 poteto emphasis` 和 `SEL v1 sunny tilt` 语法正确但没有帧，所以没有 `ACK`。`SEL v1 poteto listening` 有两帧。

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

扬声器保持关闭。KEY3 采集只有 I2S 接收：16 kHz、单声道、有符号 16 位，在 `firmware/main/key3.c`。采集任务与 66 ms 绘制分开读这条流。装满的片段以 `AUD v1` 行发出。串口协议没有语言设置，也没有对讲。来自 peer 的文字和短回答摘录画在气泡里。

## 目录是这样分的。

| 路径 | 内容 |
| --- | --- |
| `firmware/` | ESP-IDF 应用、`partitions.csv` 和字体镜像 |
| `firmware/main/frames.bin` | 构建嵌入的 RGB565 RLE 帧 |
| `host/peer_main.c` | Windows peer |
| `host/mvp_host.py` | Windows 语音入口 |
| `host/gui_server.py` | Windows 回环页面 |
| `assets/frames/` | 源帧，480×480 |
| `tests/` | peer 和帧预算检查 |
