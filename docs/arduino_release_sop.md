# Arduino Boards Manager Release SOP

这份 SOP 描述如何把一个 Arduino Core 从本地可用推进到正式可安装、可升级、可烧录的 Boards Manager 发布包。

本文以 AIR780EPM Windows 首版发布为参考，但流程适用于后续其他模组。

## 发布目标

普通用户应该能够：

- 在 Arduino IDE 中添加 package index URL。
- 通过 Boards Manager 安装平台包。
- 选择目标开发板。
- 打开示例 `Blink`。
- 点击 Verify 编译。
- 点击 Upload 烧录。
- 看到开发板按预期运行。

普通用户不应该需要：

- xmake。
- 完整 SDK 源码。
- 完整 LuatOS/RTOS 源码。
- 手工配置 linker script。
- 手工下载 toolchain。

Arduino IDE 用户编译时使用系统自带的 `powershell`。维护者执行发布打包和发布验证时
使用已验证的 `pwsh`。

## 发布资产形态

推荐把发布资产拆成多个 Arduino tool/platform 包：

| 包 | 内容 |
| --- | --- |
| platform package | `platform.txt`、`boards.txt`、core、variant、recipe scripts、examples |
| CSDK ABI tool package | `libcsdk.a`、`libair780epm_runner.a`、ABI headers、linker、pack inputs、manifest |
| GNU Arm toolchain package | `arm-none-eabi-gcc/g++/ar/objcopy` 等工具链 |
| flash tool package | `luatos-cli` 或目标芯片下载工具 |
| package index | 指向以上 zip 的 `package_air780_index.json` |

这样做的好处：

- 平台包很小，升级快。
- CSDK ABI 包可以独立更新。
- 工具链可以复用，不重复塞进 CSDK 包。
- 下载工具版本可被 Arduino CLI 管理。

## 版本策略

首版建议使用保守版本：

- `0.1.0`：第一个公开功能版本。
- `0.1.1`：发布包装、安装、脚本修复。
- `0.2.0`：新增较大 API 面或多模组支持。
- `1.0.0`：API、发布流程、硬件验证矩阵稳定后再考虑。

版本发布前要确认：

- package index 中 platform 版本和 zip 文件名一致。
- CSDK ABI 包版本和 manifest 一致。
- toolchain 版本固定。
- flash tool 版本固定。
- 升级路径从上一版验证通过。

## 包命名建议

Boards Manager 的 package/vendor 名称会进入用户本地目录，例如：

```text
Arduino15\packages\<vendor>\hardware\<architecture>\<version>
Arduino15\packages\<vendor>\tools\<tool>\<version>
```

命名原则：

- `<vendor>` 使用项目、组织或产品族中立名称。
- `<architecture>` 使用产品族或平台名，不一定等于 SoC。
- board 名称显示用户手里的开发板或模组。
- 避免使用不属于自己的厂商品牌。

AIR780EPM 当前采用：

```text
air780:air780:air780epm_dev
```

对应安装目录：

```text
Arduino15\packages\air780\hardware\air780\<version>
Arduino15\packages\air780\tools\air780epm-csdk\<version>
Arduino15\packages\air780\tools\gnu-rm\<version>
Arduino15\packages\air780\tools\luatos-cli\<version>
```

给其他模组的建议：

- 如果后续是同一产品族，可继续放在同一 package 下增加 board。
- 如果 SoC、工具链、下载协议差异很大，可以拆新 architecture 或新 package。
- 用户菜单里应显示模组/开发板名，不要只显示芯片名。

## 发布前本地门禁

发布前至少跑：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_csdk_prebuilt_arduino_flow.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_package_index_install.ps1 -Clean -KeepSmokeRoot
```

第一条验证：

- project-local Arduino CLI 可用。
- `platform.txt` recipes 使用预编译 CSDK combine/link。
- Blink 编译和打包。
- 复杂第三方库 probe。
- ArduinoJson probe。
- OTA API report。
- Sleep report。

第二条验证：

- 重新生成 release assets。
- 启动本地 HTTP server。
- 使用隔离 Arduino15-like 目录安装 package index。
- 验证 platform shape。
- 验证 CSDK tool 不含重复 toolchain。
- 编译安装后的 Blink。
- 通过 Arduino library index 安装 `PubSubClient` 并编译安装后的 `MqttsLoopback`。
- 编译实验性第三方库 probe。

有硬件时再跑：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\arduino_cli_upload.ps1 `
  -SketchPath .\examples\01.Basics\Blink `
  -ComPort COM3 `
  -Clean
```

## 生成发布候选

推荐用一个脚本生成干净目录，例如：

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\prepare_release_candidate.ps1 `
  -Clean `
  -OutputDirectory .\dist\release-v0.2.0 `
  -BaseUrl https://github.com/aix402/air780exx-arduino-core/releases/download/v0.2.0 `
  -PlatformVersion 0.2.0 `
  -CsdkVersion 0.2.0 `
  -PublishPackageIndexPath .\package_air780_index.json
```

候选目录会包含本地验证所需的 package index，例如：

```text
air780-arduino-platform-0.2.0.zip
csdk-prebuilt-air780epm-0.2.0-notoolchain-toolroot.zip
gnu-rm-10.2.1-ec718.zip
luatos-cli-1.8.0.zip
package_air780_index.json
release-candidate.manifest.json
```

注意：

- `dist` 是生成目录，不提交到源码仓库。
- release asset 的 SHA256 必须和 package index 一致。
- `package_air780_index.json` 由 `-PublishPackageIndexPath` 同步到仓库根目录，提交到
  `main`；不要把它作为 GitHub Release asset 上传。
- package index 内的 ZIP URL 必须指向最终公开下载地址，不要写入本地 `127.0.0.1` URL。

## GitHub Release 流程

推荐用公开 release 仓库承载 Arduino 包，例如：

```text
https://github.com/aix402/air780exx-arduino-core
```

流程：

1. 将根目录的 `package_air780_index.json` 提交并推送到公开 `main` 分支。
2. 创建 tag，例如 `v0.2.0`。
3. 创建 GitHub Release，上传 4 个 ZIP 和可选的 `release-candidate.manifest.json`。
4. 确认 release 不是 draft，或者明确需要 draft 测试。
5. 打开固定 package index URL，确认可下载。
6. 用浏览器或脚本下载 package index，确认其中 SHA256 和发布 ZIP 一致。

Boards Manager URL 示例：

```text
https://raw.githubusercontent.com/aix402/air780exx-arduino-core/main/package_air780_index.json
```

## Arduino IDE 验证

发布后必须用 Arduino IDE 做用户路径验证。

安装验证：

1. 打开 Preferences。
2. 添加 package index URL。
3. 打开 Boards Manager。
4. 搜索包名。
5. 安装目标版本。
6. 选择开发板。
7. 打开 `File > Examples > <board examples> > AIR780 > 01.Basics > Blink`。
8. Verify。
9. Upload。
10. 观察 LED 或串口输出。

升级验证：

1. 先安装上一版。
2. 确认上一版能编译。
3. 更换 package index 或刷新 index。
4. 升级到新版本。
5. 再编译 Blink。
6. 再上传。
7. 确认运行。

卸载/重装验证：

- 如果 IDE 曾经安装过旧版，必要时使用 Boards Manager Remove。
- 确认 `Arduino15\packages\<vendor>\hardware\<architecture>` 被清空或只剩目标版本。
- 重新安装后再验证。

## 网络和代理

GitHub release assets 在部分网络下可能出现：

- `wsarecv`
- `connection reset`
- 下载很慢
- 大文件中断

推荐优先用 Arduino CLI 配置代理，而不是改系统 WinHTTP：

```yaml
network:
  proxy: http://127.0.0.1:7897
```

也可以让用户使用稳定网络后重试。下载速度不稳定通常来自 GitHub/CDN、代理节点、运营商链路、文件大小和 Arduino CLI 重试行为共同影响。

## 上传和 boot recovery

正常运行态上传：

```text
luatos-cli flash run --soc <firmware.soc> --port <selected-port>
```

如果固件死机、运行口不存在：

1. 手动进入 boot/download mode。
2. 等 Windows 枚举新的下载口。
3. 使用 `-p auto` 或 IDE upload。
4. 让 `luatos-cli` 自动识别下载口。

发布文档必须说明：

- 正常运行口可能是 `COM3`。
- 下载口可能临时变成 `COM7` 或其他端口。
- 用户看到的 COM 号由 Windows 分配，不应写死。

## 发布后记录

每个 release 至少记录：

- package index URL。
- assets 数量、文件名、大小。
- CSDK 包是否包含 toolchain。
- IDE 安装结果。
- IDE 升级结果。
- Blink 编译结果。
- Blink 上传结果。
- 硬件运行结果。
- 已知网络问题和代理建议。

记录位置建议：

- `docs\release_install_guide.md`
- `docs\public_release_checklist.md`
- GitHub Release notes

## 主线合入

发布分支通过后，合入主线要单独验证。

推荐步骤：

1. 确认发布分支已推送。
2. 确认本地 `main` 没有未提交变更。
3. 如果可以 fast-forward，优先 fast-forward。
4. 跑 compile/release gate。
5. 有硬件时烧录 Blink。
6. 记录主线硬件 smoke。
7. 推送 `main`。

如果发布分支已经发布过 public release，但还没有合入 `main`，要明确告诉团队：发布包已验证不等于主线已合入。

## 回滚策略

如果 release 发布后发现问题：

- 小问题优先发布 `0.1.x+1` 修复。
- 不建议删除已公开 tag，除非 asset 明显错误且还未被用户使用。
- package index 可以推荐新版本，但旧版本可保留下载。
- 文档中标注不推荐版本和原因。

如果主线合入后发现问题：

- 先确认是否是发布包、源码主线、网络、IDE 缓存或硬件状态问题。
- 能补提交修复就补提交。
- 不要在不清楚根因时 `git reset --hard` 或强推。

## 发布完成标准

一次正式发布完成，需要满足：

- release assets 上传完成。
- package index URL 可下载。
- Arduino IDE 可安装或升级。
- Blink 编译通过。
- Blink 上传通过。
- AIR780EPM 或目标板运行正常。
- release gate 结果写入文档。
- 主线合入并推送。

对其他模组也是同样标准：只有 package 能安装还不够，必须至少有一块真实硬件跑通 Blink。
