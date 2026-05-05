# Arduino Core Porting SOP

这份 SOP 总结 AIR780EPM / EC718PM Arduino Core 移植和发布过程，目标是给后续其他模组或其他芯片平台移植 Arduino 时复用。

这不是单个模组的流水账，而是一套推进方法：先证明最小运行链路，再扩大 Arduino API 面，随后收敛桥接边界，最后做可交付的 Boards Manager 发布包。

## 适用范围

适用于以下场景：

- 厂商已有 C SDK、RTOS、xmake/make/cmake 工程或专用下载工具。
- 目标是让 Arduino CLI / Arduino IDE 按标准流程编译 sketch、Arduino core 和第三方库。
- 需要兼顾维护者源码构建和普通用户安装使用。
- 可能存在非公开 SDK，需要把发布包边界设计清楚。
- 后续可能扩展到多个模组、多个开发板或多个芯片变体。

不适合用这份 SOP 解决的问题：

- 只想把 Arduino API 做成厂商 SDK 示例里的一个单独 demo。
- 不准备支持 Arduino CLI / IDE。
- 不准备提供版本化发布包或可复现安装流程。

## 文档拆分

建议把 Arduino 移植经验拆成三份文档维护：

| 文档 | 作用 |
| --- | --- |
| `docs\arduino_porting_sop.md` | 总流程，适合立项、排期、复盘和移植其他模组时使用 |
| `docs\arduino_bridge_convergence_sop.md` | 桥接层收敛、SDK 头文件隔离、ABI 边界设计 |
| `docs\arduino_release_sop.md` | Boards Manager 包制作、发布、安装、升级和硬件验证 |

旧的 `docs\arduino_core_porting_runbook.md` 保留为本仓库 ML307N-EC / AIR780EPM 移植历史和阶段经验。新项目启动时优先读本 SOP，再按旧 runbook 查细节案例。

## 核心原则

- 先闭环，再扩面：先让 `Blink` 编译、烧录、运行，再谈完整 API。
- 先分级验证，再对外承诺：compile-only、烧录、硬件观测、断电/休眠/网络长状态要分开记录。
- Arduino core 保持 Arduino 视角：不要让用户代码直接依赖厂商 SDK 私有头文件。
- 维护者构建和用户构建分开：维护者可以需要 SDK、xmake、私有源码；普通 Arduino 用户不应需要这些。
- 桥接是阶段工具，不是最终目的：先通过桥接跑通，再把临时桥接收敛成稳定 ABI。
- 每个能力都要有示例、验证 sketch、文档和回归矩阵位置。
- 包发布不是打 zip：必须验证安装、升级、编译、上传和硬件运行。

## 名词和边界

| 名词 | 建议含义 |
| --- | --- |
| SoC | 芯片平台，例如 `ec718pm` |
| Module | 模组型号，例如 `air780epm` |
| Board | 开发板或用户可选板型，例如 `AIR780EPM Dev Board` |
| Arduino platform package | Boards Manager 中的 `vendor:architecture` 包，例如 `air780:air780` |
| Variant | 同一 core 下不同板型的 pins、board config、upload 参数 |
| Runner | 嵌入厂商 SDK app 的 Arduino 运行时入口和 SDK 适配层 |
| ABI package | 发布给 Arduino 用户的预编译 SDK/runner 二进制和头文件边界 |

给其他模组移植时，名称要尽早固定：

- 包名优先代表产品族或组织中立名称，不要误用不属于自己的厂商品牌。
- board 名称应该面向用户，例如 `AIR780EPM Dev Board`。
- FQBN 应保持可扩展，例如 `air780:air780:air780epm_dev`，后续可加 `air780xxx_dev`。
- SoC 名称可以出现在内部 manifest、linker、toolchain 配置中，但用户菜单优先显示模组/开发板名称。

## 阶段 0：立项和基线

目标是确认 SDK、工具链和硬件不是坏的。

必须完成：

- 固定 SDK 版本、LuatOS/RTOS 版本、工具链版本、下载工具版本。
- 编译厂商原始示例。
- 烧录厂商原始示例。
- 确认运行串口、日志串口、下载串口和 boot 模式进入方式。
- 记录 Windows 设备管理器中正常运行口和下载口枚举行为。
- 建立 `docs\build_upload.md`、`docs\pinmap.md`、`docs\regression_matrix.md`。

进入下一阶段条件：

- 原始 SDK app 可编译。
- 原始 SDK app 可烧录。
- 开发板能稳定进入运行态和下载态。

给其他模组的建议：

- 不要一开始就写 Arduino API。先证明厂商 SDK 和下载链路可靠。
- 如果下载口是动态枚举的，尽早确定是否能 `auto` 探测，不能就先写清楚人工 boot 流程。
- 如果芯片相同但模组不同，仍要独立确认 pinmap、板载 LED、BOOT、VBUS、UART route。

## 阶段 1：最小 C++ / Arduino 运行链路

目标是让厂商 SDK app 能调用 Arduino 风格的 `setup()` / `loop()`。

必须完成：

- C++ 编译和链接可用。
- `.init_array` 被保留，全局静态构造函数能执行。
- 补齐 `new/delete`、`__cxa_pure_virtual` 等基础符号。
- 建立 Arduino app/runner 入口。
- 跑通 `Blink`、`Serial` ready log、一个静态构造 smoke。

进入下一阶段条件：

- `Blink` 可编译、可烧录、LED 或日志可观测。
- `Serial.print()` 可输出。
- 全局对象构造已验证。

给其他模组的建议：

- 这阶段不要追求 API 完整，目标是证明运行时模型正确。
- 如果 SDK 自带任务调度或 RTOS，先明确 `loop()` 是独立任务、timer 回调还是 app 主线程。
- 不要把业务逻辑写进 runner，runner 只负责 Arduino 生命周期和 SDK 适配。

## 阶段 2：Arduino CLI 标准编译链路

目标是让 Arduino CLI / IDE 按标准方式编译 sketch、core 和 libraries。

推荐目标形态：

```text
sketch + Arduino core + third-party libraries
        |
        | Arduino CLI standard recipes
        v
object files + core.a
        |
        | final link against SDK/runner ABI libraries
        v
flashable firmware
```

必须完成：

- `platform.txt` 和 `boards.txt` 能被 Arduino CLI 识别。
- `.ino` 由 Arduino CLI 预处理。
- core 源码由 Arduino CLI 编译并打包成 `core.a`。
- 第三方库由 Arduino CLI 正常发现和编译。
- 最终链接接入 SDK/runner 产物。

进入下一阶段条件：

- `arduino-cli compile -b <fqbn> Blink` 通过。
- 一个包含 `.c/.cpp/.h` 多文件第三方库的 probe 通过。
- 一个真实第三方库，例如 ArduinoJson，编译通过。

给其他模组的建议：

- 不要长期依赖“复制 sketch 到 SDK 工程再编译”的 source bridge；它可以作为早期方案，但正式发布应尽量回到 Arduino CLI 标准编译。
- 如果 SDK 无法直接被 Arduino CLI 编译，优先把 SDK 预编译为静态库，再让 Arduino CLI 负责编译用户侧源码。
- 记录所有必须导出的编译参数、include、link group、linker script、pack 工具和固件合包输入。

## 阶段 3：PinMap 和资源目录

目标是先建立硬件资源 contract，再开放 API。

必须固定：

- Arduino pin token。
- SoC GPIO / PAD / mux。
- ADC channel 和输入范围。
- PWM instance 和 route。
- I2C/SPI/UART 默认对象和可选对象。
- wakeup pad 是否等同于普通 GPIO。
- 敏感 pin、保留 pin、下载口和日志口占用。

进入下一阶段条件：

- `pinmap.md` 写清楚用户可用资源。
- 至少 LED、普通 GPIO、一个 UART、一个 timing smoke 通过。
- 敏感资源有明确标注。

给其他模组的建议：

- 同一 SoC 的不同模组可能 pinout 完全不同，不能复用开发板 pinmap 当作模块 pinmap。
- WakeupPad、ADC channel、PMU pad、GPIO 编号不要强行合并成一个概念。
- `variant` 是承载板型差异的好位置，不要把所有板差异写死在 core。

## 阶段 4：基础 Arduino API

建议按风险从低到高推进：

| 顺序 | 能力 | 验证 |
| --- | --- | --- |
| 1 | `pinMode` / `digitalRead` / `digitalWrite` | Blink、PinReport |
| 2 | `millis` / `micros` / `delay` | Timing report |
| 3 | `Serial` / extra UART | Echo、API report |
| 4 | ADC / PWM | Analog/PWM report |
| 5 | Wire / SPI | scan、sensor、display smoke |
| 6 | Servo | pulse report 或示波器观察 |

进入下一阶段条件：

- 基础 API 至少达到 compile + hardware smoke。
- `docs\arduino_api_implementation.md` 记录 API 状态。
- `docs\regression_matrix.md` 有对应条目。

给其他模组的建议：

- 用户看重 Arduino API 的一致性，但底层硬件能力不一致时要诚实标注。
- ESP32 兼容 API 可以提升第三方库适配率，但不能把蜂窝网络伪装成 WiFi 硬件能力。
- API 名称、行为、阻塞语义和错误返回要尽早稳定。

## 阶段 5：网络、NVM、文件系统、休眠、OTA

这些能力跨越 SDK 内部状态，建议单独分批做。

推荐顺序：

1. Network status / modem identity / PDP。
2. TCP / UDP / TLS。
3. Arduino 兼容网络类和第三方库，例如 PubSubClient、ArduinoHttpClient、NTPClient。
4. EEPROM / Preferences。
5. LittleFS 或等价文件系统。
6. Sleep API，先 light sleep，再 timer wake，再 pad wake。
7. OTA API，先 failure path，再真实升级。

进入下一阶段条件：

- 每个能力有 report 示例。
- 至少一个真实第三方库 runtime smoke 通过。
- 状态型能力记录 L4/L5 验证差异。

给其他模组的建议：

- 网络 API 要把“蜂窝已注册”“PDP ready”“socket 可用”分开。
- NVM、文件系统、OTA 都吃 flash 分区，任何分区调整都要重新评估合包和升级风险。
- Sleep 需要真实硬件验证，compile-only 没有太大意义。
- OTA 先验证失败路径，防止示例一上来就自动升级。

## 阶段 6：桥接收敛

早期为了跑通功能，Arduino core 可能会直接包含 SDK 头文件或通过临时 bridge 调 SDK。正式发布前要收敛边界。

目标：

- Arduino core 面向 Arduino API 和稳定 runner ABI。
- SDK 私有头文件尽量只出现在 runner `.c/.cpp` 内。
- 用户发布包不暴露非必要 SDK 源码。

详细步骤见 `docs\arduino_bridge_convergence_sop.md`。

进入下一阶段条件：

- 直接 SDK 私有头文件依赖已被盘点。
- 临时 bridge 被删除或转为稳定 ABI。
- 预编译 ABI 包能支持 Arduino CLI 标准编译。

## 阶段 7：预编译 SDK/runner ABI

目标是把稳定的 SDK/runner 侧预编译为 `.a`，把用户侧留给 Arduino CLI 编译。

维护者模式需要：

- 完整 SDK / LuatOS / RTOS 源码。
- xmake/make/cmake。
- GNU Arm 工具链。
- runner 源码。

用户模式只需要：

- Arduino platform package。
- CSDK/runner ABI package。
- GNU Arm 工具链 package。
- 下载工具 package。

进入下一阶段条件：

- `libcsdk.a` 和 `libair780epm_runner.a` 可生成。
- manifest 记录 compile/link/package 参数。
- Arduino CLI 可以链接预编译 `.a` 生成可烧录固件。
- SDK 改动后可以通过 stamp/fingerprint 判断是否需要重新预编译。

给其他模组的建议：

- 预编译不是为了让维护者不能编 SDK，而是为了让普通 Arduino 用户不需要 SDK。
- 如果 SDK 私有，发布包应尽量只包含 ABI 必要文件、头文件和工具，不发布完整私有源码。
- 头文件不是二进制，不能用“放进 ABI 包”来隐藏私有 API；真正的隔离要靠 wrapper 和边界设计。

## 阶段 8：正式发布

目标是让普通用户从 Arduino IDE / Boards Manager 安装、编译、上传。

必须完成：

- 版本号固定。
- package index 生成。
- platform archive、ABI archive、toolchain archive、flash tool archive 生成。
- 本地隔离 Arduino15 目录安装烟测。
- GitHub Release 或其他公开下载地址可下载。
- Arduino IDE 安装/升级、Blink 编译、上传、运行验证。

详细步骤见 `docs\arduino_release_sop.md`。

进入发布条件：

- 本地 release gate 通过。
- 远端 release asset SHA256 和 package index 一致。
- IDE 从旧版本升级到新版本通过。
- 至少 `Blink` 硬件运行通过。

## 阶段 9：多模组扩展

当第一个模组跑通后，不要直接复制粘贴第二个 core。优先判断差异应该落在哪里。

推荐结构：

| 差异 | 建议落点 |
| --- | --- |
| 同芯片、不同板载 pin | `variants\<board>` |
| 同芯片、不同模组封装 | variant + pin/resource catalog |
| 同 SDK、不同 SoC 小差异 | runner 条件编译 + 独立 manifest |
| 不同 SoC 或不同工具链 | 新 ABI package 或新 architecture |
| 不同下载协议 | upload tool recipe 分支或新 tool package |

扩展前先回答：

- 新模组是否共用同一个 SoC？
- 是否共用同一个 SDK build target？
- flash 分区、bootloader、cp image 是否一致？
- 下载口枚举和 boot 模式是否一致？
- Arduino pin map 是否能用 variant 表达？
- 是否需要独立 Boards Manager package？

给其他模组的建议：

- 第一个模组是模板，不是约束。不要为了复用而隐藏真实硬件差异。
- 多模组发布时，用户菜单要清楚，不要只显示芯片名。
- 每个 board 都至少有自己的 Blink 烧录记录。

## 最小发布门禁

每次准备发布前至少跑：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_csdk_prebuilt_arduino_flow.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_package_index_install.ps1 -Clean -KeepSmokeRoot
```

有硬件时再跑：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_upload.ps1 `
  -SketchPath .\examples\01.Basics\Blink `
  -ComPort COM3 `
  -Clean
```

对外只承诺已验证等级。编译通过不等于硬件可用，硬件可用也不等于断电、休眠、OTA 长状态可靠。
