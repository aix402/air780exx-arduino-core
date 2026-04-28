# Validation Sketches

These sketches are not polished end-user demos. They are controlled probes used
to freeze board contracts, print resource reports, and support regression work.

Current first batch:

- `PinReport`
- `PinCapabilities`
- `ResourceBoundaryP4Report`
- `ArduinoJsonRuntimeSmoke`

They are expected to compile through Arduino CLI and the xmake runner. Runtime
output is useful when hardware is connected, but the compile gate is still the
first requirement.

Third-party library probes are intentionally kept in `validation_sketches`
rather than the default smoke profile. Run them through
`scripts\validate_library_compat.ps1` so missing local sketchbook libraries can
be reported as `SKIP` instead of becoming a core build failure.
