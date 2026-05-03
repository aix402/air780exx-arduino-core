# CSDK Prebuilt Release Merge Plan

This note records the merge boundary for `codex/abi-only-wrapper-experiment`
after `v0.1.0-rc4` passed public GitHub release install, Arduino IDE `Blink`
compile, and upload on AIR780EPM hardware.

## Merge Target

- Source branch: `codex/abi-only-wrapper-experiment`.
- Current target branch: `codex/air780epm-sleep-ota-bringup`.
- The source branch is a direct descendant of the target branch, so the merge is
  expected to be a fast-forward or clean descendant merge rather than a
  conflict-heavy parallel merge.
- Do not touch the separate main worktree by copying generated files manually.

## Changes To Merge

Runtime and ABI wrapper changes:

- Arduino core sources now call runner ABI wrappers for GPIO, PWM, ADC, NVM,
  sleep, debug/time paths that must not expose private SDK headers to normal
  Arduino compilation.
- Runner wrapper headers and C implementations under
  `runner\air780epm_runner\inc` and `runner\air780epm_runner\src`.
- `runner\air780epm_runner\xmake.lua` support for building prebuilt CSDK and
  runner static libraries, plus external Arduino CLI object linking.

Arduino build and package recipe changes:

- `core\air780epm\platform.txt` uses standard Arduino CLI compile/archive
  recipes and links against the packaged CSDK ABI tool.
- `scripts\arduino_cli_recipe.ps1`, `scripts\link_arduino_with_csdk.ps1`, and
  `scripts\export_arduino_direct_link.ps1` own compile, archive, direct link,
  `binpkg`, and `.soc` packaging from Arduino CLI build outputs.
- `scripts\upload_core.ps1` supports the Boards Manager installed
  `air780:luatos-cli` tool path and retains development fallbacks.

Distribution and release tooling:

- CSDK prebuild stamp/fingerprint:
  `scripts\csdk_prebuild_stamp.ps1`.
- ABI distribution export and packaging:
  `scripts\export_csdk_prebuilt_distribution.ps1` and
  `scripts\package_csdk_prebuilt_distribution.ps1`.
- Arduino package index and release candidate scripts:
  `scripts\package_arduino_platform.ps1`,
  `scripts\package_gnu_rm_toolchain.ps1`,
  `scripts\package_luatos_cli_tool.ps1`,
  `scripts\generate_package_index_draft.ps1`, and
  `scripts\prepare_release_candidate.ps1`.
- Package install verification:
  `scripts\verify_package_index_install.ps1`.

Validation assets:

- `examples\99.Experimental` probe sketches remain source-controlled validation
  inputs, but are intentionally not bundled in the release platform package.
- `libraries\Air780EpmComplexLibProbe` and `libraries\Air780EpmLinkProbe`
  remain source-controlled test libraries, but are intentionally not bundled in
  the release platform package.

Documentation:

- `docs\csdk_prebuilt_a_research.md`
- `docs\csdk_prebuilt_build_flow.md`
- `docs\release_install_guide.md`
- Existing README and validation docs updated to describe the prebuilt release
  path.

## Do Not Merge Generated Artifacts

These are intentionally ignored and should not be committed or copied into the
target branch by hand:

- `dist\`
- `.arduino-cli-work\`, `.arduino-cli-data\`, `.arduino-cli-config\`,
  `.arduino-cli-downloads\`
- `.tmp_*` smoke directories
- `runner\air780epm_runner\generated\`
- `runner\air780epm_runner\build\`
- `runner\air780epm_runner\out\`
- `runner\air780epm_runner\.xmake\`
- `tools\arduino-cli-release\`
- `tools\luatos-cli-release\`
- `hardware\`
- `libraries\ArduinoJson\`
- `log\` and `logs\`

The release files for `v0.1.0-rc4` live under `dist\release-candidate`, but
they are generated release assets, not source.

## Release Package Boundary

The public platform package must contain:

- `platform.txt` and `boards.txt`.
- `cores\air780epm`.
- `variants\air780epm_dev`.
- platform-local scripts under `tools`.
- example library `libraries\AIR780\examples`.

The public platform package must not contain:

- top-level `examples`.
- `examples\99.Experimental`.
- local test libraries such as `Air780EpmComplexLibProbe`,
  `Air780EpmLinkProbe`, or `ArduinoJson`.
- full SDK/LuatOS source trees.

The Boards Manager tool packages provide:

- `air780:air780epm-csdk`
- `air780:gnu-rm`
- `air780:luatos-cli`

Normal users should not need xmake, LuatOS source, or `luatos-soc-2024` source
for sketch compile/link/upload.

## Validated Gates

Already passed on this branch:

- `scripts\verify_csdk_prebuilt_arduino_flow.ps1`
  - Blink
  - ComplexLibraryProbe
  - ArduinoJsonProbe
  - OtaApiReport
  - SleepReport
- `scripts\verify_package_index_install.ps1 -Clean -KeepSmokeRoot`
- Exact `v0.1.0-rc4` release-candidate local package-index install smoke.
- Public GitHub `v0.1.0-rc4` Arduino IDE install.
- Arduino IDE `AIR780 > 01.Basics > Blink` compile.
- Arduino IDE upload to AIR780EPM hardware.

## Recommended Merge Procedure

1. Keep the target worktree clean except for ignored local files.
2. Merge `codex/abi-only-wrapper-experiment` into
   `codex/air780epm-sleep-ota-bringup`.
3. Confirm `git status --short --ignored` shows only ignored generated
   directories, not tracked release assets.
4. Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_csdk_prebuilt_arduino_flow.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File .\scripts\verify_package_index_install.ps1 -Clean -KeepSmokeRoot
```

5. If hardware is available, use Arduino IDE or Arduino CLI to compile and
   upload `AIR780 > 01.Basics > Blink` from the installed package index.

## Residual Risks

- Windows is the only supported release host for the first public package.
- The GNU Arm toolchain package is large and can still be affected by user
  network/proxy reliability.
- Full CSDK prebuild refresh still requires xmake plus the private
  `luatos-soc-2024` source tree on the maintainer machine.
- The example menu can still show Arduino IDE built-in or user-installed
  libraries with broad architecture compatibility; this is expected Arduino IDE
  behavior, not package pollution.
