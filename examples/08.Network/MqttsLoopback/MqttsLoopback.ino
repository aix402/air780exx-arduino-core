#include <Arduino.h>
#include <PubSubClient.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace {

const char *kBrokerHost = "broker.emqx.io";
const uint16_t kBrokerPort = 8883U;
const long kGmtOffsetSeconds = 8L * 3600L;
const int kDaylightOffsetSeconds = 0;
const char *kTimeServer1 = "ntp.aliyun.com";
const char *kTimeServer2 = "ntp.ntsc.ac.cn";
const char *kTimeServer3 = "time1.cloud.tencent.com";

const char kBrokerCa[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD
QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB
CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97
nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt
43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P
T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4
gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR
TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw
DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr
hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg
06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF
PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls
YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk
CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=
-----END CERTIFICATE-----
)PEM";

CellularClientSecure secureClient;
PubSubClient mqttClient(secureClient);

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
  char line[128] = {0};

  setTerminalState(stage);
  snprintf(line,
           sizeof(line),
           "+ARDUINO: MQTTS,FAIL,%s,STATE,%d,TLSERR,%ld",
           (stage != NULL) ? stage : "",
           mqttClient.state(),
           (long)secureClient.lastError());
  Serial.println(line);
}

void printStatus(const char *stage) {
  AIR780EPMModemStatus status;
  const bool ok = Modem.getStatus(status);
  const String ipText = status.localIPv4.toString();
  char line[176] = {0};

  snprintf(line,
           sizeof(line),
           "+ARDUINO: MQTTS,STATUS,%s,GET,%u,REGISTERED,%u,NET_READY,%u,HAS_IPV4,%u,IP,%s",
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

bool syncTime() {
  struct tm timeinfo;

  configTime(kGmtOffsetSeconds, kDaylightOffsetSeconds, kTimeServer1, kTimeServer2, kTimeServer3);
  if (!getLocalTime(&timeinfo, 15000UL)) {
    return false;
  }

  char buffer[32] = {0};
  char line[96] = {0};
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  snprintf(line, sizeof(line), "+ARDUINO: MQTTS,LOCAL_TIME,%s", buffer);
  Serial.println(line);
  return true;
}

void buildIdentity() {
  AIR780EPMModemIdentity identity;
  const unsigned long salt = millis();

  if (Modem.getIdentity(identity) && identity.imeiValid) {
    snprintf(gClientId, sizeof(gClientId), "air780epm-mqtts-%s-%lu", identity.imei, salt);
    snprintf(gTopic, sizeof(gTopic), "/luatos/testcase/mqtt/%s/mqtts_loopback", identity.imei);
  } else {
    snprintf(gClientId, sizeof(gClientId), "air780epm-mqtts-%lu", salt);
    snprintf(gTopic, sizeof(gTopic), "/luatos/testcase/mqtt/fallback/mqtts_loopback");
  }

  snprintf(gPayload, sizeof(gPayload), "mqtts-loopback-%lu", salt);
}

void mqttCallback(char *topic, uint8_t *payload, unsigned int length) {
  char message[80] = {0};
  char line[192] = {0};
  const unsigned int copyLength =
      (length < (sizeof(message) - 1U)) ? length : (sizeof(message) - 1U);

  memcpy(message, payload, copyLength);
  message[copyLength] = '\0';

  snprintf(line,
           sizeof(line),
           "+ARDUINO: MQTTS,RX,%s,%s",
           (topic != NULL) ? topic : "",
           message);
  Serial.println(line);

  if ((topic != NULL) && (strcmp(topic, gTopic) == 0) && (strcmp(message, gPayload) == 0)) {
    gReceived = true;
  }
}

} // namespace

void setup() {
  Serial.begin(921600);
  delay(1500);

  printLine("+ARDUINO: MQTTS,READY");
  Modem.begin();
  printStatus("EARLY");

  if (!waitNetworkReady(60000UL)) {
    fail("NET_READY");
    return;
  }

  if (!syncTime()) {
    fail("TIME_SYNC");
    return;
  }

  buildIdentity();
  {
    char line[192] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS,CLIENT_ID,%s", gClientId);
    Serial.println(line);
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS,TOPIC,%s", gTopic);
    Serial.println(line);
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS,PAYLOAD,%s", gPayload);
    Serial.println(line);
  }

  secureClient.setCACert(kBrokerCa);
  secureClient.setConnectTimeout(30000UL);
  secureClient.setReadTimeout(10000UL);

  mqttClient.setServer(kBrokerHost, kBrokerPort);
  mqttClient.setCallback(mqttCallback);
  mqttClient.setKeepAlive(60);
  mqttClient.setSocketTimeout(12);
  mqttClient.setBufferSize(384);

  {
    char line[96] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS,CONNECTING,%s,%u", kBrokerHost, (unsigned int)kBrokerPort);
    Serial.println(line);
  }

  if (!mqttClient.connect(gClientId)) {
    fail("CONNECT");
    return;
  }

  printLine("+ARDUINO: MQTTS,CONNECT,1");

  if (!mqttClient.subscribe(gTopic)) {
    fail("SUBSCRIBE");
    mqttClient.disconnect();
    return;
  }

  printLine("+ARDUINO: MQTTS,SUBSCRIBE,1");
  delay(500);

  if (!mqttClient.publish(gTopic, gPayload)) {
    fail("PUBLISH");
    mqttClient.disconnect();
    return;
  }

  printLine("+ARDUINO: MQTTS,PUBLISH,1");
  setTerminalState("WAIT_RX");

  const unsigned long start = millis();
  unsigned long lastHeartbeat = 0UL;
  while (((millis() - start) < 45000UL) && !gReceived) {
    mqttClient.loop();
    const unsigned long now = millis();
    if ((now - lastHeartbeat) >= 5000UL) {
      char line[96] = {0};
      snprintf(line, sizeof(line), "+ARDUINO: MQTTS,HEARTBEAT,%s", gTerminalState);
      Serial.println(line);
      lastHeartbeat = now;
    }
    delay(10);
  }

  if (gReceived) {
    setTerminalState("PASS");
    printLine("+ARDUINO: MQTTS,PASS");
  } else {
    fail("RX_TIMEOUT");
  }

  mqttClient.disconnect();
}

void loop() {
  delay(1000);
}
