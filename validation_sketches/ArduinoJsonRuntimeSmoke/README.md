# ArduinoJsonRuntimeSmoke

Runtime smoke sketch for the third-party `ArduinoJson` library.

It parses a small JSON payload, mutates it with an `F()` flash-string value, and
serializes the document back into a fixed buffer. No external hardware is
required.

Build through the Arduino CLI bridge:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\arduino_cli_compile.ps1 -SketchPath validation_sketches\ArduinoJsonRuntimeSmoke -Clean
```

A passing hardware run prints:

```text
+ARDUINO: ARDUINOJSON_RUNTIME,PASS,LEN,<n>,JSON,<payload>
```
