#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <stdio.h>
#include <string.h>

namespace {

const char *kBrokerHost = "broker.emqx.io";
const uint16_t kBrokerPort = 1883U;

CellularClient netClient;
MqttClient mqttClient(netClient);

char gClientId[72] = {0};
char gTopic[128] = {0};
char gPayload[64] = {0};
char gTerminalState[48] = "BOOT";
bool gReceived = false;

void printLine(const char *line) {
  Serial.println(line);
}

void setTerminalState(const char *state) {
  if (state == NULL) {
    gTerminalState[0] = '\0';
    return;
  }

  strncpy(gTerminalState, state, sizeof(gTerminalState) - 1U);
  gTerminalState[sizeof(gTerminalState) - 1U] = '\0';
}

void fail(const char *stage) {
  char line[160] = {0};

  setTerminalState(stage);
  snprintf(line,
           sizeof(line),
           "+ARDUINO: ARDUINO_MQTTCLIENT,FAIL,%s,CONNERR,%d,TCPERR,%ld",
           (stage != NULL) ? stage : "",
           mqttClient.connectError(),
           (long)netClient.lastError());
  Serial.println(line);
}

void printStatus(const char *stage) {
  AIR780EPMModemStatus status;
  const bool ok = Modem.getStatus(status);
  const String ipText = status.localIPv4.toString();
  char line[176] = {0};

  snprintf(line,
           sizeof(line),
           "+ARDUINO: ARDUINO_MQTTCLIENT,STATUS,%s,GET,%u,REGISTERED,%u,NET_READY,%u,HAS_IPV4,%u,IP,%s",
           (stage != NULL) ? stage : "",
           ok ? 1U : 0U,
           status.registered ? 1U : 0U,
           status.networkReady ? 1U : 0U,
           status.hasIPv4 ? 1U : 0U,
           ipText.c_str());
  Serial.println(line);
}

void buildIdentity() {
  AIR780EPMModemIdentity identity;
  const unsigned long salt = millis();

  if (Modem.getIdentity(identity) && identity.imeiValid) {
    snprintf(gClientId, sizeof(gClientId), "a780eamc-%s-%lu", identity.imei, salt);
    snprintf(gTopic, sizeof(gTopic), "/luatos/testcase/mqtt/%s/arduino_mqttclient", identity.imei);
  } else {
    snprintf(gClientId, sizeof(gClientId), "a780eamc-%lu", salt);
    snprintf(gTopic, sizeof(gTopic), "/luatos/testcase/mqtt/fallback/arduino_mqttclient");
  }

  snprintf(gPayload, sizeof(gPayload), "arduino-mqttclient-%lu", salt);
}

void onMqttMessage(int messageSize) {
  const String topic = mqttClient.messageTopic();
  char message[80] = {0};
  int index = 0;

  while (mqttClient.available() > 0) {
    const int value = mqttClient.read();
    if (value < 0) {
      break;
    }

    if (index < (int)(sizeof(message) - 1U)) {
      message[index++] = (char)value;
    }
  }
  message[index] = '\0';

  char line[224] = {0};
  snprintf(line,
           sizeof(line),
           "+ARDUINO: ARDUINO_MQTTCLIENT,RX,%s,%d,%s",
           topic.c_str(),
           messageSize,
           message);
  Serial.println(line);

  if ((strcmp(topic.c_str(), gTopic) == 0) && (strcmp(message, gPayload) == 0)) {
    gReceived = true;
  }
}

} // namespace

void setup() {
  Serial.begin(921600);
  delay(1500);

  printLine("+ARDUINO: ARDUINO_MQTTCLIENT,READY");
  Modem.begin();

  printStatus("EARLY");
  if (!Modem.waitForNetwork(60000UL)) {
    printStatus("REGISTER_TIMEOUT");
    fail("REGISTER_TIMEOUT");
    return;
  }

  printStatus("REGISTERED");
  if (!Modem.activatePDP(60000UL)) {
    printStatus("PDP_TIMEOUT");
    fail("PDP_TIMEOUT");
    return;
  }

  printStatus("NET_READY");
  buildIdentity();

  {
    char line[192] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: ARDUINO_MQTTCLIENT,CLIENT_ID,%s", gClientId);
    Serial.println(line);
    snprintf(line, sizeof(line), "+ARDUINO: ARDUINO_MQTTCLIENT,TOPIC,%s", gTopic);
    Serial.println(line);
    snprintf(line, sizeof(line), "+ARDUINO: ARDUINO_MQTTCLIENT,PAYLOAD,%s", gPayload);
    Serial.println(line);
  }

  netClient.setConnectTimeout(30000UL);
  mqttClient.setId(gClientId);
  mqttClient.setCleanSession(true);
  mqttClient.setKeepAliveInterval(30000UL);
  mqttClient.setConnectionTimeout(30000UL);
  mqttClient.setTxPayloadSize(256);
  mqttClient.onMessage(onMqttMessage);

  {
    char line[96] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: ARDUINO_MQTTCLIENT,CONNECTING,%s,%u", kBrokerHost, (unsigned int)kBrokerPort);
    Serial.println(line);
  }

  if (!mqttClient.connect(kBrokerHost, kBrokerPort)) {
    fail("CONNECT");
    return;
  }

  printLine("+ARDUINO: ARDUINO_MQTTCLIENT,CONNECT,1");

  if (!mqttClient.subscribe(gTopic)) {
    fail("SUBSCRIBE");
    mqttClient.stop();
    return;
  }

  Serial.print("+ARDUINO: ARDUINO_MQTTCLIENT,SUBSCRIBE,1,QOS,");
  Serial.println(mqttClient.subscribeQoS());
  delay(500);

  if (!mqttClient.beginMessage(gTopic, (unsigned long)strlen(gPayload))) {
    fail("PUBLISH_BEGIN");
    mqttClient.stop();
    return;
  }

  mqttClient.print(gPayload);
  if (!mqttClient.endMessage()) {
    fail("PUBLISH_END");
    mqttClient.stop();
    return;
  }

  printLine("+ARDUINO: ARDUINO_MQTTCLIENT,PUBLISH,1");
  setTerminalState("WAIT_RX");

  const unsigned long start = millis();
  unsigned long lastHeartbeat = 0UL;
  while (((millis() - start) < 45000UL) && !gReceived) {
    mqttClient.poll();

    if (!mqttClient.connected() && !gReceived) {
      fail("DISCONNECTED");
      mqttClient.stop();
      return;
    }

    const unsigned long now = millis();
    if ((now - lastHeartbeat) >= 5000UL) {
      char line[96] = {0};
      snprintf(line, sizeof(line), "+ARDUINO: ARDUINO_MQTTCLIENT,HEARTBEAT,%s", gTerminalState);
      Serial.println(line);
      lastHeartbeat = now;
    }

    delay(10);
  }

  if (gReceived) {
    setTerminalState("PASS");
    printLine("+ARDUINO: ARDUINO_MQTTCLIENT,PASS");
  } else {
    fail("RX_TIMEOUT");
  }

  mqttClient.stop();
}

void loop() {
  delay(1000);
}
