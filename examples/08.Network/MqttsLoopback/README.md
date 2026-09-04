# MqttsLoopback

This example uses [PubSubClient](https://github.com/knolleary/pubsubclient) 2.8 or a compatible version. Install `PubSubClient` from Arduino IDE Library Manager before building.

It connects to the EMQX public TLS broker at `broker.emqx.io:8883`, subscribes to a unique device topic, publishes a message to that topic, and receives the message through the MQTT callback. A successful run prints:

```text
+ARDUINO: MQTTS,PASS
```

The public broker is suitable for functional testing only. Use a broker and credentials controlled by your project in production.
