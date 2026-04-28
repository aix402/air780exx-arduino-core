# Mqtt256dpiSmoke

Runtime validation sketch for plain MQTT using the third-party `MQTT` library
by 256dpi over the AIR780EPM `CellularClient` TCP compatibility layer.

It connects to the public EMQX plain MQTT test broker at `broker.emqx.io:1883`,
subscribes to a unique device topic, publishes a payload to that same topic,
and passes only after receiving the message through the 256dpi callback.

This sketch validates the Arduino `Client` stream contract for a second common
third-party MQTT library. It does not add an MQTT business API to the core.

Build through the Arduino CLI bridge:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_core.ps1 -SketchPath validation_sketches\Mqtt256dpiSmoke
```

A passing hardware run prints:

```text
+ARDUINO: MQTT_256DPI,CONNECT,1
+ARDUINO: MQTT_256DPI,SUBSCRIBE,1
+ARDUINO: MQTT_256DPI,PUBLISH,1
+ARDUINO: MQTT_256DPI,RX,/luatos/testcase/mqtt/<imei>/plain_256dpi,<payload>
+ARDUINO: MQTT_256DPI,PASS
```
