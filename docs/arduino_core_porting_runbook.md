# Arduino Core Porting Runbook

这份文档把本次 `ML307N-EC / EC718PM` Arduino Core 移植过程整理成可复用执行手册。

目标是给下一个模组移植时直接照着推进：先打通最小 C++ / Arduino 运行链路，再逐步验证硬件资源、Arduino API、第三方库、文件系统、休眠、OTA、打包发布。

## 基本原则

- 先跑通最小闭环，再扩大 API 面。
- 先做受控 sketch，再做 generic user sketch。
- 先 compile-only，再 runtime，再外设实测，再断电 / 长稳测试。
- Core 只提供基础能力，不把业务策略写进 Core。
- 每新增一个能力，都要补示例、验证 sketch、文档和回归矩阵。
- 不轻易改 SDK 公共层；能在 Core、app、script 层收口的问题优先在这些层解决。
- 对外承诺必须按验证等级写清楚，不能把“能编译”说成“已硬件验证”。

## 验证等级

| 等级 | 名称 | 含义 | 可对外口径 |
| --- | --- | --- | --- |
| `L0` | vendor-sdk-build | 厂商 SDK 原始示例能编译 | SDK / toolchain 可用 |
| `L1` | sdk-controlled-build | 仓库受控 sketch 能通过 SDK Make 后端生成镜像 | Core 构建链路可用 |
| `L2` | arduino-cli-compile | Arduino CLI / IDE backend 能编译 sketch | Arduino 构建入口可用 |
| `L3` | flash-serial-verified | 烧录后串口命中 pass regex | 板端运行路径可用 |
| `L4` | hardware-observed | 外设真实响应或人工观测通过 | 对应硬件链路已实测 |
| `L5` | persistence-or-sleep-verified | 断电、复位、深睡、网络等待等跨状态验证通过 | 状态保持 / 低功耗路径已实测 |
| `L6` | packaged-release-verified | Boards Manager / IDE / 发布门禁通过 | 可交付给普通 Arduino 用户 |

## AIR780EPM 当前进度快照（2026-04-29）

| 面向能力 | 当前等级 | 说明 |
| --- | --- | --- |
| xmake-native Arduino runner | `L3` | 已可烧录并看到 `+ARDUINO: AIR780EPM,READY` |
| `setup()` / `loop()` / `Serial` / `Blink` | `L3`~`L4` | 基础运行链路已实机打通 |
| 静态构造链路 | `L3`~`L4` | 已实机看到 `+ARDUINO: CTOR,PASS`，说明当前静态构造桥接链路可运行 |
| GPIO / `Wire` / PWM / ADC / `Serial1` | `L4` | 已有真实板级验证样例 |
| `Wire1` / `SPI` / `SPI1` / `Serial2` / `Serial3` | `L2` | 先保持 compile-enabled，等硬件接线再补 runtime |
| `Servo` | `L2` | 已接到 PWM 层并补示例，待板级验证 |
| Sleep API | `L5` | `SleepReport` 已实机跑通，`SleepTimerWakeValidation` 已实机证明 `SLP2 -> RTC/timer wake`；`SleepWakeup0Validation` 已实机证明 `WAKEUP0 -> PAD wake`；`SleepPadWakeValidation` 已实机证明 `WAKEUP_PAD_1/USB VBUS -> PAD wake`。`deepSleep()` 语义固定为 `SLP2`，wakeup pad 仍按 PMU pad ID 暴露，不等于 Arduino GPIO |
| Network status / PDP / IPv4 | `L4` | `NetworkStatusReport` 已实机验证，能看到 `REGISTERED=1`、`READY=1`、`HAS_IPV4=1` 和真实 IPv4 地址 |
| Modem identity / signal / cell 详细信息 | `L4` | `Modem.getIdentity()` 已修复；`ModemInfoReport` 现已实机跑通到运行态摘要，看到 `WAIT_OK=1`、`REGISTERED=1`、`NET_READY=1`、`HAS_IPV4=1` 和真实 IPv4，说明 `waitForNetwork()` / 后续状态路径已收口 |
| TCP / TLS / UDP smoke | `L4` | `TcpHttpGet`、`TlsHttpGet`、`UdpNtpReport` 已实机跑通；TLS 的 `-52` 根因已定位为 EC718PM mbedTLS 配置下不应再调用无效的 `ctr_drbg_seed()`，当前已改为直接走 `luat_crypto_trng()` |
| Arduino network time helpers | `L4` | `NetworkTimeReport` 已实机跑通；`configTime()` / `getLocalTime()` 已能拿到有效 epoch 和格式化本地时间，当前 `configTzTime()` 先支持固定偏移型 POSIX TZ 字符串 |
| CA 校验型 MQTTS / PubSubClient | `L4` | `MqttsPubSubClientCaSmoke` 已实机跑通；`CellularClientSecure::setCACert()` + 第三方 `PubSubClient` 能完成 TLS MQTT 连接、订阅、发布和 loopback 接收，最终看到 `+ARDUINO: MQTTS_PUBSUB_CA,PASS` |
| `EEPROM` / `Preferences` | `L4`~`L5` partial | 已实机验证可写入并在再次烧录启动后看到 `PREV/NEXT` 递增；纯 reset / 断电验证仍可补 |
| `LittleFS` | `L4` | `LittleFSReport` 已实机验证 `mkdir/open/write/read/rename/readdir/remove/rmdir` |

当前建议推进顺序：

1. `Servo` 板级收口。
2. Sleep 文档和回归矩阵收口。
3. 把已跑通的 CA / MQTTS 路径纳入 connectivity 回归。
4. 视需要再补 `EEPROM` / `Preferences` 的纯 reset / 断电验证。
5. 继续补 `Wire1` / `SPI` / `SPI1` / `Serial2` / `Serial3` 的板级验证。

## 阶段 0：准备和边界固定

交付物：

| 文件 | 作用 |
| --- | --- |
| `docs/build_upload.md` | 记录 toolchain、SDK build、下载工具、串口和已验证命令 |
| `docs/pinmap.md` | 记录 Arduino pin、GPIO、PAD、MUX、ADC、PWM 等映射 |
| `docs/resource_catalog.md` | 记录板级资源目录和 `Verified / Available / Sensitive` 分级 |
| `docs/regression_matrix.md` | 固定每个阶段至少要跑哪些 sketch |
| `docs/arduino_api_implementation.md` | 记录已补 API 和验证等级 |
| `docs/esp32_api_compat_gap.md` | 记录和 ESP32 Arduino 的差距与不承诺项 |

执行项：

- 固定 SDK 版本、toolchain 版本、FlashTools 版本、下载配置文件。
- 用厂商原始 app 编译一次，确认 SDK 本身没坏。
- 用厂商原始 app 烧录一次，确认下载工具和硬件链路可用。
- 明确应用串口、debug 串口、下载串口是否共用。
- 明确 boot/download 进入方式，确认是否能脚本化复位。
- 建立 `examples/`、`validation_sketches/`、`scripts/`、`docs/` 四类目录。

进入下一阶段条件：

- SDK 原始 app 可编译。
- SDK 原始 app 可烧录。
- 串口能看到可重复的启动或测试输出。

## 阶段 1：C++ 和 Arduino 最小运行时

目标：

- 在厂商 SDK app 中跑起 C++。
- 手动补齐 Arduino `setup()` / `loop()` 调度。
- 先让 `Blink`、`SerialEcho`、`ApiTest` 这种最小 sketch 可运行。

关键点：

- 链接脚本必须保留 `.init_array`。
- 启动阶段必须调用全局静态构造函数。
- 补齐 `new/delete`、`__cxa_pure_virtual`、基础 libc / libstdc++ 链接问题。
- 明确是否关闭异常和 RTTI；Arduino Core 通常不依赖异常。
- `main.c` 或 app 入口中只做 Core bootstrap，不写业务逻辑。

建议最小 sketch：

| sketch | 覆盖点 |
| --- | --- |
| `blink` | `setup()`、`loop()`、`pinMode()`、`digitalWrite()`、`delay()` |
| `serial_echo` | `Serial.begin()`、`Serial.print()`、串口读写 |
| `api_test` | math、bit、random、character、基础 Arduino 宏 |

本仓库命令样例：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\upload_core.ps1 -ComPort COM3
powershell -ExecutionPolicy Bypass -File .\scripts\verify_log.ps1 -RequirePass
```

进入下一阶段条件：

- `Blink` 可编译、可烧录、可看到 LED 或串口 ready。
- `SerialEcho` 可回复输入。
- 一个带全局对象构造的 C++ smoke 能证明静态构造链路有效。

## 阶段 2：构建、烧录、Arduino CLI / IDE 桥接

目标：

- 保留厂商 SDK Make 后端，同时让 Arduino CLI / IDE 能调用它。
- 把受控 examples 和 generic user sketch 分开处理。
- 构建失败时有可诊断报告。

应落地脚本：

| 脚本 | 作用 |
| --- | --- |
| `scripts/build_core.ps1` | 调 SDK 后端编译受控 app |
| `scripts/upload_core.ps1` | 调下载工具烧录 `.binpkg` |
| `scripts/verify_serial.ps1` | 串口监听和 pass regex 验收 |
| `scripts/validate_core.ps1` | 编译、烧录、串口验收一条龙 |
| `scripts/arduino_cli_setup.ps1` | 本地挂载 Arduino CLI platform |
| `scripts/arduino_cli_compile.ps1` | Arduino CLI compile 入口 |
| `scripts/arduino_cli_upload.ps1` | Arduino CLI upload 入口 |
| `scripts/arduino_cli_recipe.ps1` | `platform.txt` recipe 调度层 |

Arduino CLI source bridge 需要做到：

- 复制 sketch 到 staging 目录。
- 使用 Arduino CLI 预处理后的 `.ino.cpp` 作为主翻译单元。
- 扫描 include，找到 sketchbook 里的第三方库。
- 按 include 可达链 staging 依赖库，不要无条件编译 `depends` 里的所有 optional dependency。
- 生成 manifest 和诊断报告，例如 `ml307nec_bridge_report.json`。
- 对 user/library 源码适当放宽 unused warning，不影响 SDK 公共层门禁。

本仓库命令样例：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_setup.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\01.Basics\Blink -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_upload.ps1 -SketchPath .\examples\01.Basics\Blink -ComPort COM3
```

进入下一阶段条件：

- SDK 受控 sketch 构建通过。
- Arduino CLI 能编译仓库 example。
- Arduino IDE backend 能发现 board 并编译 `Blink`。
- 烧录脚本能自动复位并回到应用态。

## 阶段 3：PinMap 和资源目录

目标：

- 先建立硬件资源 contract，再开放 Arduino API。
- 把普通 GPIO、敏感 pin、可选 route、默认 route 分清楚。

必须固定的资源表：

| 资源 | 必填内容 |
| --- | --- |
| GPIO | Arduino pin token、GPIO 编号、PAD 编号、MUX、是否板上可见、是否敏感 |
| ADC | Arduino analog pin、ADC channel、输入安全范围、校准方式 |
| PWM | Arduino pin、PWM instance、PAD、是否 verified |
| I2C | `Wire` / `Wire1` 对象、SDA/SCL、硬件 route |
| SPI | `SPI` / `SPI1` 对象、SCK/MISO/MOSI/SS、route 边界 |
| UART | `Serial` / `Serial1` / `Serial2` / `Serial3` 对象、RX/TX、是否占用下载或 debug |
| wakeup | WakeupPad ID、物理触发源、是否和 Arduino GPIO 一一对应 |

建议验证 sketch：

| sketch | 覆盖点 |
| --- | --- |
| `PinReport` | 用户可见 pin 列表 |
| `PinCapabilities` | GPIO/PAD/MUX/PWM/ADC contract |
| `ResourceCatalogP5Report` | 统一资源目录 |
| `ResourceBoundaryP4Report` | `Verified / Available / Sensitive` 分级 |

进入下一阶段条件：

- `pin_report` 和 `pin_capabilities` 可在真机输出完整资源信息。
- LED、BOOT、模拟输入、PWM 输出至少各有一个 verified 路径。
- 文档中明确哪些 pin 不建议用户直接操作。

## 阶段 4：基础硬件资源验证

建议顺序：

| 顺序 | 能力 | 示例 / 验证 |
| --- | --- | --- |
| 1 | Digital IO | `PinReport`、`PullModeReport`、`BootKeyInterrupt` |
| 2 | Timing | `DelayMicrosecondsReport`、`millis()`、`micros()` |
| 3 | ADC | `AnalogReadReport`、`analogReadMilliVolts()` |
| 4 | PWM | `AnalogWriteLed`、`AvailablePwmProbe`、`PwmApiReport` |
| 5 | UART | `SerialEcho`、`Serial2Echo`、`Serial3Echo`、`UartApiP5Report` |
| 6 | I2C | `I2CScan`、`SHT40_Read`、`Wire1SHT40Probe`、`SoftwareWireScan` |
| 7 | SPI | `SpiSmoke`、`ILI9341Smoke`、`SPI1_ILI9341Smoke`、`SoftwareSpiILI9341Smoke` |
| 8 | Servo | `ServoPulseReport` |

本仓库回归命令样例：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile smoke -ComPort COM6
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile digital_io -ComPort COM6
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile sensor_io -ComPort COM6
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile pinmap_contract -ComPort COM6
```

进入下一阶段条件：

- 每个默认外设对象至少有一个 compile + runtime smoke。
- 每条 verified route 都能在文档中追溯到底层 GPIO/PAD/MUX。
- 如果某条路径只 compile-only，必须在文档中标注。

## 阶段 5：Arduino 标准 API 兼容层

目标：

- 让常见 Arduino sketch 和第三方库先能编译。
- 补 API 时优先补“形状兼容”和常见行为，不承诺 ESP32 / AVR 的全部底层语义。

建议分层：

| 层级 | 能力 |
| --- | --- |
| `P0` | `Arduino.h` 常量、math、bit、character、`Print`、`Stream`、`String`、`F()`、`PROGMEM` |
| `P1` | `IPAddress`、`Client`、TLS Client、`UDP`、`WiFiClient` / `WiFiUDP` 兼容别名 |
| `P2` | `HardwareSerial`、`Wire`、`SPI` 常见 ESP32 风格签名 |
| `P3` | `EEPROM`、`Preferences`、`random()`、标准 C time 入口 |
| `P4` | 资源边界、第三方库 bridge 边界 |
| `P5` | PWM、UART、ADC、资源 catalog 等板级增强 API |

本仓库命令样例：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -Sketch api_test -BuildTarget arduino_runner
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\validation_sketches\CoreApiP0Report\CoreApiP0Report.ino -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\validation_sketches\BusApiP2Report\BusApiP2Report.ino -Clean
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\validation_sketches\UtilityApiP3Report\UtilityApiP3Report.ino -Clean
```

进入下一阶段条件：

- API report sketch compile 通过。
- 关键基础 API 至少有一个 runtime smoke。
- `arduino_api_implementation.md` 写清楚已实现和已知边界。

## 阶段 6：第三方库兼容矩阵

目标：

- 用真实 Arduino 库倒逼 Core API 补齐。
- 把“库能编译”和“库能跑通外设 / 网络”分开记录。

推荐矩阵：

| 类别 | compile-only | runtime |
| --- | --- | --- |
| JSON | `ArduinoJsonCompile` | `ArduinoJsonRuntimeSmoke` |
| I2C sensor | `SHT40`、`Sensirion`、`SparkFun SCD4x` | 真实 SHT40 / SCD40 输出 |
| SPI display | `Adafruit ILI9341 graphictest` | `AdafruitILI9341Smoke` |
| OLED / RTC | `U8g2`、`Adafruit SSD1306`、`Adafruit SH110X`、`RTClib` | 后续按真实硬件补 |
| 1-Wire | `OneWire`、`DallasTemperature` | 后续接 DS18B20 补 |
| MQTT | `PubSubClient`、`ArduinoMqttClient`、`MQTT by 256dpi` | MQTTS publish / subscribe |
| HTTP / NTP | `ArduinoHttpClient`、`NTPClient` | HTTP GET、UDP NTP |

本仓库命令样例：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\validate_library_compat.ps1 -Clean -ContinueOnError
```

单库失败时排查：

- 先看 `.arduino-cli-work\build\<SketchName>\ml307nec_bridge_report.json`。
- 确认库是否被正确 staged。
- 确认 `depends` 是 resolved、declared-only-skipped 还是 missing。
- 确认失败来自 Core 缺 API、SDK `-Werror`、库源码平台判断，还是路径长度。
- 只对 staged 副本做必要兼容 patch，不修改用户 sketchbook 里的第三方库源码。

进入下一阶段条件：

- 核心 compile-only 矩阵通过。
- 至少完成一个无外设 runtime 库、一个 I2C 传感器库、一个 SPI 显示库。
- 网络类库至少完成一个 TCP/TLS 或 UDP runtime smoke。

## 阶段 7：网络、TLS、UDP、时间

目标：

- 把蜂窝模组的网络能力包装成 Arduino 常见 `Client` / `UDP` 形状。
- 不假装有 WiFi 栈；`WiFiClient` / `WiFiUDP` 只能作为类型兼容别名。

建议验证顺序：

| 能力 | 示例 |
| --- | --- |
| IP / Client API | `IPAddressReport`、`NetworkApiP1Report` |
| 模组状态 | `NetworkStatusReport`、`PdpStatus` |
| 身份和小区信息 | `ModemInfoReport` |
| TCP | `TcpHttpGet` |
| TLS | `TlsHttpGet`、MQTTS CA smoke |
| UDP | `UdpNtpReport` |
| 第三方 NTP | `NTPClientReport` |
| 系统时间 | `NetworkTimeReport`、`TimeSyncReport` |

时间 API 边界建议：

- `configTime()` 只配置 NTP server 和固定 offset，不在调用点长阻塞。
- `configTzTime()` 先支持固定偏移型 TZ 字符串。
- `getLocalTime()` 负责等待 NITZ 或 core 内置 UDP NTP fallback，并写回系统 UTC。
- 完整 DST 规则可以后置，不要在第一轮移植中扩大风险。
- EC718PM 这套 `mbedtls_ec7xx_config.h` 关闭了默认 entropy source；TLS
  客户端不要再额外依赖 `mbedtls_ctr_drbg_seed()`，应直接使用
  `luat_crypto_trng()` 提供 RNG 回调。

进入下一阶段条件：

- PDP ready、IPv4、DNS 状态可观察。
- TCP HTTP GET 可返回内容。
- TLS 默认 fail-closed，配置 CA 后可连接。
- UDP NTP 或网络授时至少一条路径跑通。

## 阶段 8：NVM、文件系统

目标：

- 区分“小容量参数存储”和“文件系统”。
- 先做 EEPROM / Preferences，再做 LittleFS。

建议顺序：

| 能力 | 示例 / 验证 |
| --- | --- |
| EEPROM / Preferences | `EepromPreferencesReport` |
| NVM 边界 | `NvmBoundaryReport` |
| LittleFS 基础 | `LittleFSReport` |
| LittleFS 深化 | `LittleFSAdvancedValidation` |
| 物理断电持久性 | `LittleFSPowerCycleValidation` |

本仓库命令样例：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run_regression_matrix.ps1 -Profile nvm -ComPort COM6
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\examples\37.FileSystem_LittleFSReport\LittleFSReport.ino -Clean
```

验收重点：

- `EEPROM` / `Preferences` 多次复位后 `PREV/NEXT` 递增。
- LittleFS 能完成 `mkdir/open/write/read/rename/readdir`。
- `openNextFile()` 和 `rewindDirectory()` 能稳定遍历。
- 至少做一次物理断电后数据仍存在的验证。

进入下一阶段条件：

- 参数存储和文件系统都能明确区分使用场景。
- 文件系统路径映射、容量、挂载点和不支持的分区写入文档。
- 断电验证通过后才提升到 runtime-verified。

## 阶段 9：休眠

目标：

- Core 提供 light sleep / deep sleep / wakeup reason / wakeup pad 的原语。
- 不在 Core 内写业务低功耗状态机。

API 边界建议：

| 概念 | 含义 |
| --- | --- |
| `WakeupPad` | PMU/AON 唤醒 pad ID，不等于 Arduino GPIO 编号 |
| `WakeupReason` | 上次唤醒来源类别，不等于 pad 编号 |
| `DeepSleepTimerId` | deep sleep RTC timer slot，和 WakeupPad 无关 |
| `wakeupPinBitmap()` | wakeup pad 原始电平 bitmap，不是唤醒原因 |

验证顺序：

| 阶段 | 验证 |
| --- | --- |
| compile | `SleepReport` Arduino CLI compile |
| light sleep | `lightSleep(ms)` 返回和 sleep time 报告 |
| timer deep sleep | 设备真实进入深睡，定时唤醒 |
| pad wake | 两阶段触发 wakeup pad |
| 干扰排查 | 避免串口 LPUART 在深睡窗口抢先唤醒 |

本仓库命令样例：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1 -Sketch sleep_report -BuildTarget arduino_runner
powershell -ExecutionPolicy Bypass -File .\scripts\arduino_cli_compile.ps1 -SketchPath .\validation_sketches\SleepPadWakeValidation\SleepPadWakeValidation.ino -Clean
```

Pad wake 人工验证建议：

- 先烧录 `SleepPadWakeValidation`。
- 看到 `DEEP,REQUEST` 或 `DEEP,ARMED` 后停止持续串口活动。
- 人工触发目标 wakeup pad，例如 USB VBUS 插拔。
- 唤醒后再打开串口读取 held result。
- 如果一直显示 LPUART 唤醒，先排除串口工具、下载工具、AT 轮询造成的活动。

进入下一阶段条件：

- 能证明设备真实进入深睡，而不是只进入普通等待。
- timer wake、`WAKEUP0` pad wake、`WAKEUP_PAD_1`/USB VBUS wake 都有实机证据。
- 文档明确 wakeup pad 与普通 GPIO 的区别。

## 阶段 10：OTA

目标：

- AIR780EPM 第一轮只做 URL 型 diff OTA，不做 full OTA。
- Core 提供 URL 型 OTA API。
- 第一轮不做完整 `Update.h` stream-to-partition 兼容。
- 先验证状态机和失败路径，再制作真实升级包做真机升级。

AIR780EPM 当前约束：

- 当前 runner flash 布局里 FOTA 窗口是 `448 KiB`，扣除 hib backup 后可用约 `352 KiB`。
- 当前 AP 镜像已经在 `1.35 MiB` 量级，full OTA 不是当前合理首目标。
- 差分升级包应由旧版 `.soc` 和新版 `.soc` 生成 `.sota`，不是直接拿 `.binpkg` 做升级包。

后端选择：

- Arduino 层可以参考 ML307N-EC 的 API 形状和 failure validation 组织方式。
- AIR780EPM 后端不复用 ML307N-EC 的 `mhttpFwdlReq` / `mhttpFwdlApplyNow` 一类 SDK 私有接口。
- AIR780EPM OTA 统一复用 LuatOS `luat_fota_init/write/done/end` 和 HTTP client 下载路径。

API 模型建议：

| API | 语义 |
| --- | --- |
| `begin(url, config)` | 启动 URL OTA 下载；v1 先服务于 diff `.sota` 包 |
| `poll()` | 推进 / 查询状态 |
| `state()` | 返回 `IDLE/STARTING/DOWNLOADING/VERIFYING/STAGED/APPLYING/ERROR` |
| `isRunning()` | 是否正在下载 / 校验 |
| `isStaged()` | 是否已有待应用升级包 |
| `downloadedBytes()` | 已下载字节数 |
| `totalBytes()` | 服务端声明的总字节数 |
| `lastError()` | 最近错误，先收敛为 Arduino 层错误码 |
| `apply()` | staged 后请求应用并重启 |
| `clear()` | 先只清理当前会话 / 错误状态；v1 不承诺清除已 staged 包 |

错误模型建议：

- 优先提供稳定的 Arduino 层错误码，如 `NONE`、`INVALID_ARGUMENT`、`INVALID_STATE`、`NETWORK_NOT_READY`、`DOWNLOAD_FAILED`、`VERIFY_FAILED`、`HTTP_STATUS_ERROR`、`INTERNAL`。
- 如果后续确实需要暴露 SDK 原始错误，再补可选的 `lastPlatformError()`，不要在 v1 就把平台细节直接泄露给草图。

验证顺序：

| 阶段 | 验证 |
| --- | --- |
| compile | `OtaApiReport` 编译通过 |
| no-url guard | 示例默认空 URL 输出 `SKIP,NO_URL`，不自动升级 |
| local guard | 空 URL、非法 URL、认证参数不成对、网络未就绪立即拒绝 |
| running guard | 运行中二次 `begin()` / `clear()` 被拒绝 |
| async error | 假 `.invalid` URL 或连接失败最终进入 `ERROR` |
| verify error | 可连通但返回非 OTA 内容，能区分 `DOWNLOAD_FAILED` 和 `VERIFY_FAILED` |
| real package | 制作 `.sota` 包，完成 HTTP/HTTPS 下载、校验、stage、apply、重启 |

当前状态（2026-04-29）：

- `AIR780EPMSleep`、`examples\12.Sleep\SleepReport`、`validation_sketches\SleepPadWakeValidation` 已通过 Arduino CLI compile。
- 当前 `deepSleep()` 固定映射到 EC718PM `SLP2`，不把 standby/hibernate 混进 Arduino 的 `deepSleep()` 语义。
- `WakeupPad` 继续按 PMU wake pad ID 暴露，v1 不做 Arduino GPIO 自动映射。
- `SleepTimerWakeValidation` 已在 AIR780EPM `COM3` 上实机命中 `Wakup Sleep2 by RTC`、`WAKE_REASON=RTC`、`LAST_STATE=SLEEP2`、有效 `LAST_MS`、`WAKE_TIMER_ID=2` 与 `+ARDUINO: SLEEP_TIMER,PASS`，说明 timer wake 已收口。
- `SleepWakeup0Validation` 已在 AIR780EPM `COM3` 上实机命中 `REASON=PAD` 与 `+ARDUINO: SLEEP_WAKEUP0,PASS`，说明 `WAKEUP0` pad wake 已收口。
- `SleepPadWakeValidation` 已在 AIR780EPM `COM3` 上实机命中 `RESULT,PAD` 与 `+ARDUINO: SLEEP_PAD,PASS`，说明 `WAKEUP_PAD_1`/USB VBUS wake 已收口。
- 本轮 root cause 已定位并修复：之前 `AIR780EPMSleep.setWakeupPad()` 只写了 `slpman` pad 配置，没有走 LuatOS 官方 wakeup GPIO 初始化路径，所以 `WAKEUP0/1` 在 Arduino 层不会真正变成可唤醒源。修复后 `setWakeupPad()/clearWakeupPad()` 已改为复用 LuatOS wakeup GPIO 初始化/关闭路径。
- `OtaApiReport` 已在 AIR780EPM `COM3` 上实机验证，日志命中 `READY`、`INITIAL,STATE,IDLE,ERR,0`、`SKIP,NO_URL`。
- `OtaFailureValidation` 已在 AIR780EPM `COM3` 上实机验证，错误矩阵命中 `INVALID_ARGUMENT=-1`、`INVALID_STATE=-2`、`NETWORK_NOT_READY=-3`、`DOWNLOAD_FAILED=-4`、`VERIFY_FAILED=-5`。
- 仍未进入真实 `.sota` 包阶段；`apply()` / 重启后版本确认还没有实机覆盖。

示例拆分建议：

- `OtaApiReport`：默认无 URL，只报告状态和能力，不自动升级。
- `OtaFailureValidation`：专门覆盖 guard、并发、下载失败、校验失败。
- 真正的 diff OTA runtime smoke 放到真实 `.sota` 制作流程明确之后再加。

进入下一阶段条件：

- API 状态机和失败路径 compile + runtime 通过。
- 示例默认不自动升级，必须显式配置 URL。
- `.sota` 制作流程、版本识别、服务器 URL、apply/reboot 语义明确后，才进入真 OTA release 承诺。

## 阶段 11：打包、安装、发布门禁

目标：

- 让普通用户可以通过 Arduino IDE / Boards Manager 使用 Core。
- 让发布前门禁可以脚本化复跑。

建议命令：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_boards_manager_package.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\install_boards_manager_package.ps1 -ResetEnvironment -RebuildPackage
powershell -ExecutionPolicy Bypass -File .\scripts\validate_boards_manager_package.ps1 -ComPort COM6
```

发布门禁：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\check_release_consistency.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\validate_release_gate.ps1 -Mode quick -ComPort COM6 -Restart
powershell -ExecutionPolicy Bypass -File .\scripts\validate_release_gate.ps1 -Mode full -ComPort COM6 -Restart
```

进入发布条件：

- 版本号格式正确。
- `smoke`、`pinmap_contract`、`release_candidate` 通过。
- Boards Manager 安装形态下能 compile、upload、serial verify。
- `examples/README.md`、`validation_sketches/README.md`、API 文档、release note 同步更新。

## 常见排查顺序

| 现象 | 先查 |
| --- | --- |
| SDK build 失败 | 先回到厂商原始 app，确认 SDK / toolchain 本身 |
| Arduino CLI compile 失败 | 看 `platform.txt` recipe 和 bridge report |
| 受控 sketch 被 generic sketch 源码污染 | 检查 SDK build 前是否清理 user sketch manifest |
| 第三方库符号冲突 | 检查 optional dependency 是否被无条件 staged |
| FlashTools 下载失败 | 检查应用串口是否响应下载前 `AT+ECRST=delay,<ms>` |
| 烧录成功但无应用输出 | 检查烧后是否执行 sysreset，串口是否被 debug 口占用 |
| 串口 pass regex 抓不到首帧 | 使用延时复位后再监听，或把验收下沉到 upload 阶段 |
| bootloader 合包失败 | 清理 stale SDK output，优先在脚本层处理 |
| 深睡无法进入 | 检查 USB sleep mask、CFUN、串口活动、业务任务是否阻止睡眠 |
| pad wake 不稳定 | 区分 WakeupPad、GPIO、LPUART、USB VBUS port monitor |
| OTA 失败 | 先跑 failure validation，再看 URL、认证、网络、FOTA staged 状态 |

## 新模组执行清单

每换一个模组，按这个清单推进：

1. 建立 SDK 原始 app 编译 / 烧录基线。
2. 新建 Arduino app 入口，补 C++ 静态构造和 `setup()/loop()`。
3. 打通 `Blink`、`SerialEcho`、`ApiTest`。
4. 写 `build_core.ps1`、`upload_core.ps1`、`verify_serial.ps1`。
5. 接入 Arduino CLI / IDE `platform.txt`、`boards.txt`、variant。
6. 固定 PinMap、ResourceCatalog、Sensitive 资源。
7. 跑 GPIO、ADC、PWM、UART、I2C、SPI、Servo 基础矩阵。
8. 补 P0 到 P5 Arduino API report。
9. 跑第三方库 compile-only 矩阵。
10. 挑选 JSON、I2C sensor、SPI display、MQTT/TLS、NTP 做 runtime smoke。
11. 补网络状态、PDP、TCP、TLS、UDP、time API。
12. 补 EEPROM / Preferences / LittleFS，并做复位和断电验证。
13. 补 Sleep API，分别验证 timer wake 和 pad wake。
14. 补 OTA API，先失败路径，后真实升级包。
15. 建 Boards Manager package，跑 quick/full release gate。
16. 同步所有文档和对外承诺等级。

## 本次 ML307N-EC 移植经验要保留

- Arduino Core 移植不是只补 `Arduino.h`，真正难点在构建、烧录、串口验收、第三方库、文档和回归矩阵的闭环。
- Generic sketch bridge 必须可诊断，否则第三方库失败会很难定位。
- `WiFiClient` 等名字可以用于库兼容，但文档必须说明底层是蜂窝网络，不是 WiFi。
- WakeupPad 不能粗暴等同于 GPIO；休眠相关 ID 必须分层命名。
- OTA 和休眠的互斥应在示例或业务层表达，Core 层只提供各自能力。
- 文件系统、NVM、FOTA 都占内部 flash 预算，扩大任何一块都要重新评估分区。
- 对 SDK 增量构建产物要保持警惕，发现 clean 不可靠时优先在仓库脚本层做有边界的清理。
