# GEC6818 基础多媒体播放器

800×480 Linux framebuffer 和触摸屏工程，提供视频、音乐、BMP 相册及主菜单。
当前状态：基础功能代码已接入；本机测试不等同于开发板验收。上板待验项目见下表。

## 编译

在队友的 Linux 开发环境、工程根目录执行：

```sh
make CC=arm-linux-gcc
```

如果编译器名称不同，可用 `make CC=arm-none-linux-gnueabi-gcc`。
课程工具包的编译器位于解压后的 `usr/local/arm/5.4.0/usr/bin`，请加入 PATH
或把 CC 设置为其绝对路径。默认静态链接，生成 **bin/main**。
Makefile 会自动创建输出目录，并通过 `-lm` 链接数学库。
请使用本次编译或新版测试包中的 **bin/main**；之前的 `build/main` 不再是默认输出。

如果课程 GCC 5.4 报 `libmpfr.so.4` 缺失，请使用其自带的宿主机库：

```sh
export LD_LIBRARY_PATH=/你的工具链目录/usr/local/arm/5.4.0/usr/lib
```

这是编译电脑的工具链依赖，不是让开发板安装该库。

本机 Linux 可执行自动测试（不需要开发板）：

```sh
make test CC=gcc
```

## 开发板运行准备

将课程工具目录中的 `mplayer` 及新编译的 `bin/main` 复制到开发板。
可执行文件和多媒体资源可以分开放置。例如：

```text
/mnt/udisk/
  main
  mplayer
  video/   01.avi 02.avi
  music/   01.mp3 02.mp3
  photo/   01.bmp 02.bmp
```

在开发板终端执行：

```sh
cd /mnt/udisk
chmod +x main mplayer
MPLAYER_BIN=/mnt/udisk/mplayer ./main
```

主菜单和按钮由代码绘制，不再依赖 main.bmp 等背景图。媒体按文件名排序，
进入对应页面自动扫描该目录，不递归子目录。支持最多 1024 个文件。
媒体格式最终取决于课程 mplayer 的解码能力；测试视频优先使用已在板上播放成功的 AVI。

可配置环境变量：

| 变量 | 默认值 | 用途 |
| --- | --- | --- |
| MEDIA_ROOT | /mnt/udisk | 下含 video、music、photo 三个子目录 |
| MPLAYER_BIN | mplayer（从 PATH 查找） | 播放器可执行文件路径 |
| LCD_DEVICE | /dev/fb0 | LCD 设备 |
| TOUCH_DEVICE | /dev/input/event0 | 触摸设备，需根据板子实际节点确认 |
| MPLAYER_VO | fbdev2 | 视频输出驱动，需在板上确认可用 |

例如：`MEDIA_ROOT=/root/media TOUCH_DEVICE=/dev/input/event1 MPLAYER_BIN=/mnt/udisk/mplayer ./main`。

## 操作和基础验收

| 模块 | 已编写的操作 | 队友上板验收 |
| --- | --- | --- |
| 主菜单 | VIDEO / MUSIC / PHOTO / EXIT；子页 BACK | 连续进入返回，确认无花屏或残留声音 |
| 视频 | 播放、暂停、停止、前后切换、进度拖动；文件名、当前/总时长 | 拖动下方进度条；检查暂停、停止和再次播放；确认视频不覆盖控件 |
| 音乐 | 自动扫描，前后切歌、暂停、音量加减及音量条、进度条 | 两首音频切换；暂停后音量调节；返回应停止声音 |
| 相册 | 自动扫描 BMP，前后切换、左右滑动、轮播开关、缩放及拖动查看 | 至少两张图；轮播 3 秒换图并可关闭；ZOOM+ 后拖动；FIT 复原 |
| 异常处理 | 空目录、设备失败、坏 BMP、播放器异常退出 | 空目录仍能返回；设备错误时在终端提示；坏文件不应崩溃 |

视频进度按触摸松开位置跳转（按下点必须在进度条内），是否精确到目标时间取决于视频关键帧。
图片缩放是相对“适应屏幕”的 1×、1.5×、2×；放大后拖动平移，正常比例下左右滑动切图。
相册目前支持无压缩 24/32 位 BMP，支持行补齐和自上而下存储，单图最多 1600 万像素。

## 已知待确认事项

- 没有实物板连接，视频输出、音频设备、触摸驱动和板上稳定性均需队友验证。
- 触摸当前按单指操作，尝试从驱动读取坐标范围，支持传统 ABS_X/Y 和常见 MT_POSITION 输入。
- 未接入真实手势模块，须提供模块型号、连接方式及驱动协议。任务书关于“触摸或者手势”和“双控制”表述不同，应确认老师要求；目前完成的是触摸控制路线。
- 当前沿用原工程 framebuffer 路线，没有引入 LVGL/Qt；若老师强制指定图形库，需移植界面层。
- 内置字体为英文大写字形，中文文件名会显示问号；验收媒体先用英文或数字文件名，中文显示需补字库。
- mplayer 使用课程工具包的旧版命令协议；暂停时不轮询进度，以免旧版查询命令意外恢复播放。
- 本次不含加分功能、设计报告和演示视频。完成上板验收后再整理这些交付项。

## 文件职责

`lcd.c`、`font.c`、`ui.c`：LCD 和界面；`event.c`：触摸输入；
`media.c`：媒体扫描；`player.c`：mplayer 子进程、控制管道与进度反馈；
`app.c`：菜单和音视频页面；`photo.c`：图片与轮播；`tests/`：本机自动测试。

参考思路来自 https://github.com/lhy112233/GEC6818 的模块划分和 mplayer slave 控制方式。
为适配本任务重写了目录扫描、进程生命周期、进度查询、UI 和图片读取逻辑。
用户仓库：https://github.com/SodaDehors/GEC6818 。
