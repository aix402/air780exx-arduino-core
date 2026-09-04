# AIR780 Arduino Core

[English](README.md) | 简体中文

> ## 本项目由社区维护，非合宙官方项目
>
> 当前仍处于早期实验阶段，仅在 Windows 和 AIR780EPM 上完成有限验证。使用本项目
> 产生的兼容性、稳定性或数据风险由使用者自行评估。

面向 OpenLuat AIR780Ex 系列模组的 Arduino Core。

首个支持目标是基于 EC718PM 芯片的 [AIR780EPM](https://docs.openluat.com/air780epm/product/)，
使用 LuatOS CSDK 的 xmake 构建流程。

## 平台支持

当前 Arduino 发布包仅在 Windows 上完成验证。macOS 和 Linux 尚未验证。

## 快速开始

普通用户只需使用 Arduino IDE：

1. 按照 [Arduino IDE 安装和烧录说明](docs/release_install_guide.md)，将 package
   index 加入 Arduino IDE 的附加开发板管理器网址。
2. 在开发板管理器中搜索并安装 `AIR780 Arduino Core`。
3. 选择 `AIR780EPM Dev Board`。
4. 打开 `File > Examples > AIR780EPM Dev Board Examples > AIR780 >
   01.Basics > Blink`，编译并上传。

Arduino Boards Manager package index：

```text
https://raw.githubusercontent.com/aix402/air780exx-arduino-core/main/package_air780_index.json
```

已在 Windows 上验证 Arduino IDE 的安装、编译、上传和 `Blink` 运行。

## USB 烧录与日志

使用 USB 连接开发板。Windows 会将正常运行的开发板枚举为三个 USB 虚拟串口：

- 其中一个 COM 口同时用于 Arduino `Serial` 日志和正常 Arduino IDE 上传。
- 不需要额外 USB 转 TTL。
- `COM3` 只是示例，应使用 Windows 当前分配的 COM 编号。

Arduino IDE 上传默认使用
[luatos-cli](https://github.com/wendal/luatos-cli)。它负责复位模组并自动识别下载口；其 EC718 自动进入 Boot 模式、多串口自动检测和 USB 接口映射说明见
[EC718 刷机协议](https://github.com/wendal/luatos-cli/blob/main/docs/ec718-flash-protocol.md)。

正常命令/日志口的选择、下载口自动识别和 Boot 模式恢复烧录，见
[USB 烧录与日志说明](docs/release_install_guide.md#upload)。

在 Arduino IDE 中选择正常命令/日志 COM 口后，下载软件通常会自动复位模组并识别下载口，用户一般无需手动操作。若自动识别失败，
可按住 `BOOT`、按下 `RESET`（或重新上电）、松开 `BOOT`，进入下载模式，Windows 会枚举独立的 EC718 下载 COM 口。

除 Arduino IDE 外，也可使用合宙
[Luatools](https://docs.openluat.com/common/Luatools/) 手动烧录。恢复烧录的具体步骤见
[Boot/下载模式恢复烧录](docs/release_install_guide.md#boot-mode-recovery-upload)。

## 文档

- [AIR780EPM 产品页](https://docs.openluat.com/air780epm/product/)
- [Arduino IDE 安装和烧录说明](docs/release_install_guide.md)
- [Luatools](https://docs.openluat.com/common/Luatools/)（可选手动烧录工具）
- [Boot/下载模式恢复烧录](docs/release_install_guide.md#boot-mode-recovery-upload)
- [维护者说明](docs/maintainer_notes.md)
- [公开发布检查清单](docs/public_release_checklist.md)
- [贡献说明](CONTRIBUTING.md)
- [安全策略](SECURITY.md)

## 已提供的基础能力

第一阶段包含：

- Arduino `setup()` / `loop()` 和 C++ 运行时基础
- `Serial` USB 日志输出
- 可编译的 `Serial1`、`Serial2`、`Serial3` 和 `HardwareSerial`
- `Print`、`Stream`、最小 `String` 与基础 Arduino 辅助 API
- `Wire`、`Wire1`、`SPI`、`SPI1` 的常用 API 形态
- 逻辑 ADC 通道 `A0..A3` 上的读取 API，以及默认 PWM 路径的 `analogWrite()`
- `delay()`、`millis()`、`micros()`、最小 GPIO 和初步第三方库源码兼容桥接

第一阶段尚不承诺完整硬件 UART TX/RX、网络、文件系统、休眠、OTA 以及广泛的
第三方 Arduino 库兼容性。

## 引脚与外设约定

Arduino 数字引脚使用 AIR780EPM 的 GPIO 编号，而非模组物理引脚编号。例如
`pinMode(27, OUTPUT)` 操作 `GPIO27`；在当前 AIR780EPM 开发板上，这是板载
LED 路径。

`A0..A3` 是逻辑 ADC 通道标识，不是 GPIO 编号：

~~~text
A0 -> ADC0 -> PIN9
A1 -> ADC1 -> PIN96
A2 -> ADC2 -> PIN77
A3 -> ADC3 -> PIN76
~~~

当前引脚约定和硬件验证待办见 [docs/pinmap.md](docs/pinmap.md)。

`Serial` 是 USB/日志通道，不占用 UART0。`Serial1` 已在 GPIO18/GPIO19 完成
TX/RX 硬件观测；`Serial2` 和 `Serial3` 尚待验证。

`Wire` 已在 I2C0 GPIO14/GPIO15 上通过仓库内 SHT40 sketch 和第三方 SparkFun
SCD4x 示例完成硬件观测。`PIN_PWM4` 已在 GPIO33/PIN26 上通过 LED 亮度探针完成
硬件观测。`A1 -> ADC1` 已在模组物理 `PIN96` 完成运行时电压观测；其余 ADC
通道仍待验证。

Core 提供低层桥接头文件 `AIR780EPM_LuatOS.h`，用于 ST7796 LCD 探针等平台验证
sketch；它尚不是稳定的高层 Arduino 显示 API。

## 公开仓库边界

本仓库包含 Arduino 相关源码和构建自动化，不内嵌完整厂商 SDK：

- 公开源码为 `core/`、`runner/`、`examples/`、`libraries/`、
  `validation_sketches/`、`scripts/` 和 `docs/`。
- 普通用户安装发布的 Boards Manager 包即可；不需要 LuatOS、
  `luatos-soc-2024` 源码或 xmake。
- LuatOS CSDK 是维护者依赖，可在仓库同级的 `deps/` 目录中存放。
- 重新生成 SDK 相关预编译产物时，维护者需要 `deps/LuatOS/` 和
  `deps/luatos-soc-2024/`。
- 发布二进制通过 GitHub Releases 提供，不提交到 Git。

~~~text
core/air780epm/              Arduino 平台文件和最小 Core
runner/air780epm_runner/     承载 setup()/loop() 的 xmake CSDK runner
examples/                    Arduino CLI 兼容示例
libraries/                   小型公开辅助库和探针
validation_sketches/         编译和运行时回归 sketch
scripts/                     构建、烧录和 Arduino CLI 桥接脚本
docs/                        公开文档、发布说明和维护者说明
tools/luatos-cli/            命令行烧录/日志工具子模块
~~~

## 维护者构建

普通 Arduino 用户不需要执行本节。Arduino IDE 的日常构建使用 Windows 自带
`powershell`；发行包打包与验证使用 `pwsh`。

构建默认 runner：

~~~powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1
~~~

默认构建启用 Arduino 静态构造桥，日志应出现 `+ARDUINO: CTOR,PASS`。设置项目
本地的 Arduino CLI 平台挂载：

~~~powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_setup.ps1
~~~

本地 FQBN 为：

~~~text
air780:air780:air780epm_dev
~~~

编译并上传 Blink：

~~~powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\01.Basics\Blink -Clean
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_upload.ps1 -SketchPath .\examples\01.Basics\Blink -ComPort COM3
~~~

发布前验证与预编译产物刷新流程见
[CSDK 预编译构建流程](docs/csdk_prebuilt_build_flow.md) 和
[维护者说明](docs/maintainer_notes.md)。

## 许可证

本仓库原创代码使用 MIT License，见 [LICENSE](LICENSE)。第三方组件仍使用各自的
许可证，组件清单及许可证位置见
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
