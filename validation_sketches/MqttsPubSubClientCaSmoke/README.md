# MqttsPubSubClientCaSmoke

Runtime validation sketch for `PubSubClient` over `CellularClientSecure` with CA verification enabled.

It connects to the LuatOS public MQTT TLS test broker at `airtest.openluat.com:8888`, loads the ISRG Root X1 CA with `setCACert()`, subscribes to a unique device topic, publishes a payload to that same topic, and passes only after receiving the message through the PubSubClient callback.

This sketch validates the Arduino `Client`/TLS stream contract used by common third-party MQTT libraries. It does not add an MQTT business API to the core.

Build:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build_core.ps1 -SketchPath validation_sketches\MqttsPubSubClientCaSmoke
```

Flash on the AIR780EPM USB download port:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\upload_core.ps1 -ComPort COM3
```

Hardware validation uses the USB log port at `921600` baud. A passing run prints:

```text
+ARDUINO: MQTTS_PUBSUB_CA,CONNECT,1
+ARDUINO: MQTTS_PUBSUB_CA,SUBSCRIBE,1
+ARDUINO: MQTTS_PUBSUB_CA,PUBLISH,1
+ARDUINO: MQTTS_PUBSUB_CA,RX,/luatos/testcase/mqtt/<imei>/pubsub_ca,<payload>
+ARDUINO: MQTTS_PUBSUB_CA,PASS
```
