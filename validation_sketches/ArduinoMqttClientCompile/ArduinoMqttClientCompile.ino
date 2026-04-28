#include <Arduino.h>
#include <ArduinoMqttClient.h>

CellularClient netClient;
MqttClient mqttClient(netClient);

void setup() {
  Serial.begin(921600);

  mqttClient.setId("air780epm-compile");
  mqttClient.setUsernamePassword("user", "pass");
  mqttClient.setCleanSession(true);
  mqttClient.setKeepAliveInterval(30);
  mqttClient.setConnectionTimeout(5000);
  mqttClient.setTxPayloadSize(256);

  Serial.println(F("+ARDUINO: ARDUINO_MQTTCLIENT_COMPILE,READY"));
}

void loop() {
  delay(1000);
}
