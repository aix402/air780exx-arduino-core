# NTPClientReport

Runtime validation sketch for the third-party `NTPClient` Arduino library over
the AIR780EPM cellular UDP compatibility layer.

Verified on AIR780EPM hardware with `NTPClient` 3.2.1 from the Arduino
sketchbook library folder.

It uses the normal Arduino-style `WiFiUDP` type alias, constructs an
`NTPClient`, waits for cellular registration and PDP readiness, then requests
time from `pool.ntp.org`. A passing run prints:

```text
+ARDUINO: NTPCLIENT,EPOCH,<epoch>
+ARDUINO: NTPCLIENT,FORMATTED,<hh:mm:ss>
+ARDUINO: NTPCLIENT,TIME_SET,1
+ARDUINO: NTPCLIENT,PASS
```

This sketch verifies third-party library compatibility. The core's
`configTime()` / `getLocalTime()` fallback remains an internal UDP NTP
implementation.
