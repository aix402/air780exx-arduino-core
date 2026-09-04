# Public Release Checklist

Use this before publishing or updating the public GitHub repository.

## Repository Shape

- [ ] Repository name is `air780exx-arduino-core`.
- [ ] Public repo root contains Arduino-facing source, examples, scripts, and docs.
- [ ] `deps/LuatOS` and `deps/luatos-soc-2024` are outside the git repository.
- [ ] `.gitmodules` does not include maintainer-only SDK trees.
- [ ] Generated files and large binaries are ignored.

## Documentation

- [ ] `README.md` explains normal Arduino usage.
- [ ] `docs/maintainer_notes.md` explains the sibling `deps/` layout.
- [ ] Public docs do not require local absolute paths.
- [ ] Validation status separates compile-enabled from hardware-observed.
- [ ] The license choice is confirmed and a top-level `LICENSE` file is added.

## Build Verification

Run at least:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_core.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\check_static_ctors_map.ps1
```

Confirm these outputs exist:

```text
runner/air780epm_runner/out/air780epm_runner.binpkg
runner/air780epm_runner/out/air780epm_runner_ec718pm.soc
```

## Release Assets

- [ ] Large zip/toolchain/package files are uploaded to GitHub Releases.
- [ ] Release asset SHA256 values match the package index.
- [ ] The package index URL points to the published release, not localhost.
- [ ] Static constructor map check passed after the final runner link.
- [ ] Arduino IDE or Arduino CLI install smoke has passed.
