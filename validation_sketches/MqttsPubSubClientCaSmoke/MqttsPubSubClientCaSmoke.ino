#include <Arduino.h>
#include <PubSubClient.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

namespace {

const char *kBrokerHost = "airtest.openluat.com";
const uint16_t kBrokerPort = 8888U;
const char *kBrokerUsername = "mqtt_hz_test_2";
const char *kBrokerPassword = "3bEKUb";
const long kGmtOffsetSeconds = 8L * 3600L;
const int kDaylightOffsetSeconds = 0;
const char *kTimeServer1 = "ntp.aliyun.com";
const char *kTimeServer2 = "ntp.ntsc.ac.cn";
const char *kTimeServer3 = "time1.cloud.tencent.com";

const char kBrokerCa[] PROGMEM = R"PEM(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
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
           "+ARDUINO: MQTTS_PUBSUB_CA,FAIL,%s,STATE,%d,TLSERR,%ld",
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
           "+ARDUINO: MQTTS_PUBSUB_CA,STATUS,%s,GET,%u,REGISTERED,%u,NET_READY,%u,HAS_IPV4,%u,IP,%s",
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
  snprintf(line, sizeof(line), "+ARDUINO: MQTTS_PUBSUB_CA,LOCAL_TIME,%s", buffer);
  Serial.println(line);
  return true;
}

void buildIdentity() {
  AIR780EPMModemIdentity identity;
  const unsigned long salt = millis();

  if (Modem.getIdentity(identity) && identity.imeiValid) {
    snprintf(gClientId, sizeof(gClientId), "air780epm-pubsub-ca-%s-%lu", identity.imei, salt);
    snprintf(gTopic, sizeof(gTopic), "/luatos/testcase/mqtt/%s/pubsub_ca", identity.imei);
  } else {
    snprintf(gClientId, sizeof(gClientId), "air780epm-pubsub-ca-%lu", salt);
    snprintf(gTopic, sizeof(gTopic), "/luatos/testcase/mqtt/fallback/pubsub_ca");
  }

  snprintf(gPayload, sizeof(gPayload), "pubsub-ca-%lu", salt);
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
           "+ARDUINO: MQTTS_PUBSUB_CA,RX,%s,%s",
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

  printLine("+ARDUINO: MQTTS_PUBSUB_CA,READY");
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
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS_PUBSUB_CA,CLIENT_ID,%s", gClientId);
    Serial.println(line);
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS_PUBSUB_CA,TOPIC,%s", gTopic);
    Serial.println(line);
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS_PUBSUB_CA,PAYLOAD,%s", gPayload);
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
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS_PUBSUB_CA,CONNECTING,%s,%u", kBrokerHost, (unsigned int)kBrokerPort);
    Serial.println(line);
  }

  if (!mqttClient.connect(gClientId, kBrokerUsername, kBrokerPassword)) {
    fail("CONNECT");
    return;
  }

  printLine("+ARDUINO: MQTTS_PUBSUB_CA,CONNECT,1");

  if (!mqttClient.subscribe(gTopic)) {
    fail("SUBSCRIBE");
    mqttClient.disconnect();
    return;
  }

  printLine("+ARDUINO: MQTTS_PUBSUB_CA,SUBSCRIBE,1");
  delay(500);

  if (!mqttClient.publish(gTopic, gPayload)) {
    fail("PUBLISH");
    mqttClient.disconnect();
    return;
  }

  printLine("+ARDUINO: MQTTS_PUBSUB_CA,PUBLISH,1");
  setTerminalState("WAIT_RX");

  const unsigned long start = millis();
  unsigned long lastHeartbeat = 0UL;
  while (((millis() - start) < 45000UL) && !gReceived) {
    mqttClient.loop();
    const unsigned long now = millis();
    if ((now - lastHeartbeat) >= 5000UL) {
      char line[96] = {0};
      snprintf(line, sizeof(line), "+ARDUINO: MQTTS_PUBSUB_CA,HEARTBEAT,%s", gTerminalState);
      Serial.println(line);
      lastHeartbeat = now;
    }
    delay(10);
  }

  if (gReceived) {
    setTerminalState("PASS");
    printLine("+ARDUINO: MQTTS_PUBSUB_CA,PASS");
  } else {
    fail("RX_TIMEOUT");
  }

  mqttClient.disconnect();
}

void loop() {
  delay(1000);
}
