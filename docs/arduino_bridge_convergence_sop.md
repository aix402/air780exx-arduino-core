# Arduino Bridge Convergence SOP

这份文档描述 Arduino Core 从“桥接能跑”收敛到“边界稳定、可发布、可维护”的流程。

核心观点：桥接不一定都要删除。应该删除的是临时桥接、泄漏 SDK 私有细节的桥接、阻碍 Arduino CLI 标准编译的桥接。应该保留的是稳定的 runner ABI 边界。

## 为什么要收敛桥接

早期移植时，最快路径通常是：

```text
Arduino API
    |
临时 bridge / SDK include
    |
厂商 SDK / RTOS
```

这个阶段能快速证明功能，但长期会带来问题：

- Arduino core 直接依赖 SDK 私有头文件。
- 普通用户编译 sketch 时需要完整 SDK。
- 第三方库编译容易被 SDK 编译参数污染。
- SDK 目录结构、头文件路径、linker 模板变化会打断 Arduino 用户构建。
- 私有 SDK 难以随公开 Arduino 包发布。

正式发布前，目标形态应变成：

```text
Arduino sketch / libraries
    |
Arduino core source
    |
stable arduino_xxx_io ABI headers
    |
prebuilt runner + SDK static libraries
    |
vendor SDK / RTOS internals
```

## 桥接分类

盘点时把桥接分成四类。

| 类型 | 特征 | 处理建议 |
| --- | --- | --- |
| 稳定 ABI 桥接 | Arduino core 只调用 `arduino_xxx_io`，SDK 细节在 runner 内部 | 保留并文档化 |
| 临时 source bridge | 把 sketch/library 源码复制进 SDK 工程编译 | 迁移到 Arduino CLI 标准编译后删除 |
| SDK 头文件泄漏 | core 直接 include SDK 私有头文件 | 下沉到 runner，core 改调 ABI |
| 发布包兼容残留 | 只为打包暂时保留的 SDK 子目录或工具 | 标记用途，逐步缩小 |

## 收敛目标

完成收敛后应该满足：

- Arduino CLI 编译 sketch、core、第三方库。
- CSDK/LuatOS/厂商 SDK 预编译为 `.a` 或等价二进制。
- Arduino core 不需要完整 SDK 源码。
- 公开发布包不包含完整私有 SDK。
- 公开头文件只暴露 Arduino API 和稳定 runner ABI。
- ABI manifest 明确记录编译、链接、合包所需参数。

## 盘点清单

每次准备收敛一个模块时先做盘点：

```powershell
rg -n "#include .*luat|#include .*osasys|#include .*mem_map|#include .*sdk|extern .*luat|luat_" core runner
rg -n "arduino_.*_io|bridge|manifest|sdk" core runner scripts
```

记录：

- 哪些 core 文件直接 include SDK 头文件。
- 哪些 runner 文件调用 SDK。
- 哪些头文件必须给 Arduino core 使用。
- 哪些头文件只应该给 runner 使用。
- 哪些 link 参数来自 SDK。
- 哪些工具或 pack 输入来自 SDK。

## 模块收敛流程

每次只收敛一组能力，例如 NVM、network、sleep、OTA、pin。

1. 固定当前行为

- 找到已有 example 和 validation sketch。
- 先跑 compile gate。
- 如果硬件可用，先跑一遍现状硬件 smoke。

2. 定义 Arduino 侧 contract

- Arduino core 需要哪些函数？
- 返回值如何表示成功、失败、busy、unsupported？
- 是否允许阻塞？
- 是否需要 callback？
- 是否需要暴露 SDK enum？

3. 设计 runner ABI

示例：

```c
int arduino_nvm_get(const char *namespace_name, const char *key, void *buffer, size_t length);
int arduino_nvm_set(const char *namespace_name, const char *key, const void *buffer, size_t length);
```

原则：

- ABI header 使用 C 风格类型和稳定结构体。
- 不把 SDK 私有 enum、struct、handle 暴露给 core。
- 大对象和状态机留在 runner 内部。
- Arduino core 负责 Arduino API 语义，runner 负责 SDK 适配。

4. 下沉 SDK include

把 SDK 私有 include 从：

```text
core\air780epm\cores\air780epm\*.cpp
```

下沉到：

```text
runner\air780epm_runner\src\arduino_xxx_io.c
```

core 只 include：

```text
runner\air780epm_runner\inc\arduino_xxx_io.h
```

5. 更新预编译导出

- 确认新增 runner 源码进入 `libair780epm_runner.a`。
- 确认 ABI header 被导出。
- 删除不再需要公开的 SDK header。
- 更新 manifest 或 fingerprint 输入。

6. 跑验证

至少跑：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_csdk_prebuilt_arduino_flow.ps1
```

如果影响发布包，再跑：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_package_index_install.ps1 -Clean -KeepSmokeRoot
```

如果影响硬件行为，再烧录对应 example 或至少 Blink。

## 什么时候不能删除桥接

以下桥接通常应该保留：

- pin、time、sleep、NVM、network、TLS、OTA 等需要 SDK 服务的底层能力。
- RTOS task、lock、timer、event callback 的适配层。
- flash pack、bootloader、linker、memory map 的固件合包边界。
- 跨 C/C++ 边界的稳定 C ABI。

这些桥接的目标不是删除，而是变稳定、变薄、变可测试。

## 什么时候应该删除桥接

以下桥接应该逐步删除：

- 为了早期跑通而复制 sketch 源码到 SDK app 的 source bridge。
- 让 Arduino core 直接 include SDK 私有头文件的 bridge。
- 只给一个验证 sketch 使用、没有长期 API 价值的临时函数。
- 与 Arduino CLI 标准 library discovery 冲突的 staging 逻辑。
- 暴露 SDK memory map、NVM 内部结构、network private struct 给用户侧编译的头文件。

## AIR780EPM 已采用的收敛经验

本仓库已经验证过几类收敛方式：

- `mem_map.h` / `mem_map_csdk_*.h` 不再作为用户编译所需头文件导出，发布包使用预处理后的 `abi\package\mem_map.txt`。
- `osanvm.h` 不再暴露给 Arduino `EEPROM` / `Preferences`，core 改调 `arduino_nvm_io`。
- network 私有头文件从 Arduino-facing classes 中移除，core 改调 `arduino_tcp_io`、`arduino_udp_io`、`arduino_tls_io`、`arduino_modem_io`。
- CSDK/runner 预编译为 `libcsdk.a` 和 `libair780epm_runner.a`，Arduino CLI 只编译 sketch、core 和 libraries。
- 发布包中的 `external\LuatOS` / `external\luatos-soc-2024` 只保留 ABI 必要子集，不代表完整源码发布。

## 其他模组移植建议

对新模组，不要一开始就追求 ABI 最终形态。建议顺序：

1. 用桥接跑通最小链路。
2. 用桥接跑通关键硬件能力。
3. 建立回归矩阵。
4. 找出直接 SDK include 和临时 bridge。
5. 每次收敛一个模块。
6. 收敛后生成预编译 ABI 包。
7. 用 Arduino CLI 标准编译验证第三方库。

如果新模组和现有模组共用同一 SoC：

- 优先复用 runner ABI。
- 把差异放进 variant、board config、manifest。
- 不要把某个开发板 pinmap 写死成 SoC 默认。

如果新模组使用不同 SoC：

- 重新确认 linker、memory map、bootloader、cp image、download protocol。
- 不要假设 `.a` 的 ABI 可复用。
- 可以复用 SOP 和脚本结构，但不能跳过 SDK 原始 app 基线。

## 收敛完成标准

一个模块可以认为完成桥接收敛，需要满足：

- Arduino core 不直接 include 该模块的 SDK 私有头文件。
- runner ABI header 可读、稳定、无 SDK 私有类型泄漏。
- compile gate 通过。
- 如果模块涉及硬件或状态机，硬件 smoke 通过。
- 预编译发布包可安装并编译 Blink。
- 文档记录该模块的边界和验证等级。
