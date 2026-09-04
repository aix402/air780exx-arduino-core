# Maintainer Notes

This repository is being organized so the public Arduino repo can stay
self-contained while maintainer-only dependencies live outside the published
tree.

## Recommended Local Layout

```text
WORKSPACE_ROOT/
├─ air780exx-arduino-core/   # public Arduino repo
└─ deps/
   ├─ LuatOS/
   └─ luatos-soc-2024/
```

## Dependency Policy

- `air780exx-arduino-core` should remain usable without `deps/`
- `deps/LuatOS` is for maintainer research and SDK inspection
- `deps/luatos-soc-2024` is for maintainer build and prebuilt refresh work
- normal Arduino compile/install/Blink flow should not require either tree

## Suggested Environment Variables

- `LUATOS_ROOT` -> `WORKSPACE_ROOT/deps/LuatOS`
- `LUATOS_SOC_ROOT` -> `WORKSPACE_ROOT/deps/luatos-soc-2024`

Scripts may read these values when present and fall back to repo-local paths
only for the current maintenance workspace.

## Public Repo Rule

- do not commit private SDK trees into the public repo
- do not make public docs depend on internal absolute paths
- publish large binary artifacts through GitHub Releases
- keep the public repo focused on Arduino-facing source, examples, docs, and
  automation
