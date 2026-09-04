# Contributing

Thanks for helping improve the AIR780EXX Arduino Core.

## Project Boundary

The public repository should stay focused on Arduino-facing source, examples,
documentation, scripts, and release automation. Maintainer-only dependency
trees live outside this repository under a sibling `deps/` directory.

```text
WORKSPACE_ROOT/
├─ air780exx-arduino-core/
└─ deps/
   ├─ LuatOS/
   └─ luatos-soc-2024/
```

Normal Arduino usage should not require `deps/`.

## Before Sending Changes

- keep generated files out of git
- do not commit private SDK trees or local build caches
- update docs when public behavior, scripts, pins, or validation status change
- run the smallest relevant verification command before submitting changes

Recommended baseline:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1
```

For release or package work, also run the focused package verification scripts
documented under `docs/`.

## Pull Request Notes

Please include:

- what changed
- which board or module was tested
- commands run
- whether hardware validation was done
- any known limitations or deferred validation
