#include <Arduino.h>
#include <MQTT.h>
#include <stdio.h>
#include <string.h>

namespace {

const char *kBrokerHost = "broker.emqx.io";
const uint16_t kBrokerPort = 1883U;

CellularClient netClient;
MQTTClient mqttClient(384);

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
           "+ARDUINO: MQTT_256DPI,FAIL,%s,LWMQTT_ERR,%d,RETURN_CODE,%d,TCPERR,%ld",
           (stage != NULL) ? stage : "",
           (int)mqttClient.lastError(),
           (int)mqttClient.returnCode(),
           (long)netClient.lastError());
  Serial.println(line);
}

void printStatus(const char *stage) {
  AIR780EPMModemStatus status;
  const bool ok = Modem.getStatus(status);
  const String ipText = status.localIPv4.toString();
  char line[160] = {0};

  snprintf(line,
           sizeof(line),
           "+ARDUINO: MQTT_256DPI,STATUS,%s,GET,%u,REGISTERED,%u,NET_READY,%u,HAS_IPV4,%u,IP,%s",
           (stage != NULL) ? stage : "",
           ok ? 1U : 0U,
           status.registered ? 1U : 0U,
           status.networkReady ? 1U : 0U,
           status.hasIPv4 ? 1U : 0U,
           ipText.c_str());
  Serial.println(line);
}

bool waitNetworkReady(unsigned long timeoutMs) {
  const unsigned long start = millis();

  while ((millis() - start) < timeoutMs) {
    AIR780EPMModemStatus status;
    if (Modem.getStatus(status) && status.networkReady && status.hasIPv4) {
      printStatus("NET_READY");
      return true;
    }

    delay(1000);
  }

  printStatus("NET_TIMEOUT");
  return false;
}

void buildIdentity() {
  AIR780EPMModemIdentity identity;
  const unsigned long salt = millis();

  if (Modem.getIdentity(identity) && identity.imeiValid) {
    snprintf(gClientId, sizeof(gClientId), "a780e256-%lu", salt);
    snprintf(gTopic, sizeof(gTopic), "/luatos/testcase/mqtt/%s/plain_256dpi", identity.imei);
  } else {
    snprintf(gClientId, sizeof(gClientId), "a780e256-%lu", salt);
    snprintf(gTopic, sizeof(gTopic), "/luatos/testcase/mqtt/fallback/plain_256dpi");
  }

  snprintf(gPayload, sizeof(gPayload), "plain-256dpi-%lu", salt);
}

void mqttCallback(String &topic, String &payload) {
  char line[208] = {0};

  snprintf(line,
           sizeof(line),
           "+ARDUINO: MQTT_256DPI,RX,%s,%s",
           topic.c_str(),
           payload.c_str());
  Serial.println(line);

  if ((strcmp(topic.c_str(), gTopic) == 0) && (strcmp(payload.c_str(), gPayload) == 0)) {
    gReceived = true;
  }
}

} // namespace

void setup() {
  Serial.begin(921600);
  delay(1500);

  printLine("+ARDUINO: MQTT_256DPI,READY");
  Modem.begin();
  printStatus("EARLY");

  if (!waitNetworkReady(60000UL)) {
    fail("NET_READY");
    return;
  }

  buildIdentity();
  {
    char line[192] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: MQTT_256DPI,CLIENT_ID,%s", gClientId);
    Serial.println(line);
    snprintf(line, sizeof(line), "+ARDUINO: MQTT_256DPI,TOPIC,%s", gTopic);
    Serial.println(line);
    snprintf(line, sizeof(line), "+ARDUINO: MQTT_256DPI,PAYLOAD,%s", gPayload);
    Serial.println(line);
  }

  netClient.setConnectTimeout(30000UL);

  mqttClient.begin(kBrokerHost, (int)kBrokerPort, netClient);
  mqttClient.onMessage(mqttCallback);
  mqttClient.setOptions(30, true, 10000);

  {
    char line[96] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: MQTT_256DPI,CONNECTING,%s,%u", kBrokerHost, (unsigned int)kBrokerPort);
    Serial.println(line);
  }

  if (!mqttClient.connect(gClientId)) {
    fail("CONNECT");
    return;
  }

  printLine("+ARDUINO: MQTT_256DPI,CONNECT,1");

  if (!mqttClient.subscribe(gTopic)) {
    fail("SUBSCRIBE");
    mqttClient.disconnect();
    return;
  }

  printLine("+ARDUINO: MQTT_256DPI,SUBSCRIBE,1");
  delay(500);

  if (!mqttClient.publish(gTopic, gPayload)) {
    fail("PUBLISH");
    mqttClient.disconnect();
    return;
  }

  printLine("+ARDUINO: MQTT_256DPI,PUBLISH,1");
  setTerminalState("WAIT_RX");

  const unsigned long start = millis();
  unsigned long lastHeartbeat = 0UL;
  while (((millis() - start) < 45000UL) && !gReceived) {
    if (!mqttClient.loop()) {
      fail("LOOP");
      mqttClient.disconnect();
      return;
    }

    const unsigned long now = millis();
    if ((now - lastHeartbeat) >= 5000UL) {
      char line[96] = {0};
      snprintf(line, sizeof(line), "+ARDUINO: MQTT_256DPI,HEARTBEAT,%s", gTerminalState);
      Serial.println(line);
      lastHeartbeat = now;
    }
    delay(10);
  }

  if (gReceived) {
    setTerminalState("PASS");
    printLine("+ARDUINO: MQTT_256DPI,PASS");
  } else {
    fail("RX_TIMEOUT");
  }

  mqttClient.disconnect();
}

void loop() {
  delay(1000);
}
