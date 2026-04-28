#include <Arduino.h>
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

bool gReceivedPublish = false;
char gClientId[64] = {0};
char gTopic[96] = {0};
char gPayload[64] = {0};
char gTerminalState[48] = "BOOT";
uint8_t gPublishFrame[256] = {0};

void setTerminalState(const char *state) {
  if (state == NULL) {
    gTerminalState[0] = '\0';
    return;
  }

  strncpy(gTerminalState, state, sizeof(gTerminalState) - 1U);
  gTerminalState[sizeof(gTerminalState) - 1U] = '\0';
}

void printStatus(const char *stage) {
  AIR780EPMModemStatus status;
  const bool ok = Modem.getStatus(status);
  const String ipText = status.localIPv4.toString();
  char line[160] = {0};

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

void fail(const char *stage, int32_t error) {
  char line[96] = {0};

  setTerminalState(stage);
  snprintf(line,
           sizeof(line),
           "+ARDUINO: MQTTS,FAIL,%s,ERR,%ld",
           (stage != NULL) ? stage : "",
           (long)error);
  Serial.println(line);
}

bool syncTime() {
  struct tm timeinfo;

  configTime(kGmtOffsetSeconds, kDaylightOffsetSeconds, kTimeServer1, kTimeServer2, kTimeServer3);
  if (!getLocalTime(&timeinfo, 15000UL)) {
    return false;
  }

  char buffer[32] = {0};
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  {
    char line[80] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS,LOCAL_TIME,%s", buffer);
    Serial.println(line);
  }
  return true;
}

size_t encodeRemainingLength(uint32_t value, uint8_t *out) {
  size_t count = 0U;

  do {
    uint8_t encoded = (uint8_t)(value % 128U);
    value /= 128U;
    if (value > 0U) {
      encoded |= 0x80U;
    }
    out[count++] = encoded;
  } while ((value > 0U) && (count < 4U));

  return count;
}

uint16_t appendMqttString(uint8_t *buffer, uint16_t offset, const char *value) {
  const uint16_t length = (uint16_t)strlen(value);
  buffer[offset++] = (uint8_t)(length >> 8);
  buffer[offset++] = (uint8_t)(length & 0xFF);
  memcpy(buffer + offset, value, length);
  return (uint16_t)(offset + length);
}

bool writeAll(CellularClientSecure &client, const uint8_t *buffer, size_t length) {
  return client.write(buffer, length) == length;
}

bool readFrame(CellularClientSecure &client,
               uint8_t *type,
               uint8_t *payload,
               size_t payloadCapacity,
               size_t *payloadLength,
               uint32_t timeoutMs) {
  const unsigned long start = millis();
  uint32_t remainingLength = 0U;
  uint32_t multiplier = 1U;
  size_t received = 0U;

  while ((millis() - start) < timeoutMs) {
    if (client.available() > 0) {
      const int value = client.read();
      if (value >= 0) {
        *type = (uint8_t)(value & 0xF0);
        break;
      }
    }
    if (!client.connected()) {
      return false;
    }
    delay(10);
  }

  if (!client.connected()) {
    return false;
  }

  while ((millis() - start) < timeoutMs) {
    if (client.available() > 0) {
      const int value = client.read();
      if (value < 0) {
        continue;
      }

      remainingLength += (uint32_t)(value & 127) * multiplier;
      if ((value & 128) == 0) {
        break;
      }
      multiplier *= 128U;
    } else {
      delay(10);
    }
  }

  if (remainingLength > payloadCapacity) {
    return false;
  }

  while ((millis() - start) < timeoutMs && received < remainingLength) {
    while (client.available() > 0 && received < remainingLength) {
      const int value = client.read();
      if (value < 0) {
        break;
      }
      payload[received++] = (uint8_t)value;
    }

    if (received >= remainingLength) {
      break;
    }

    if (!client.connected()) {
      return false;
    }
    delay(10);
  }

  if (payloadLength != NULL) {
    *payloadLength = received;
  }
  return received == remainingLength;
}

bool sendConnect(CellularClientSecure &client) {
  uint8_t packet[192];
  uint8_t remLen[4];
  const uint16_t clientIdLength = (uint16_t)strlen(gClientId);
  const uint16_t usernameLength = (uint16_t)strlen(kBrokerUsername);
  const uint16_t passwordLength = (uint16_t)strlen(kBrokerPassword);
  const uint32_t remainingLength = 10U + 2U + clientIdLength + 2U + usernameLength + 2U + passwordLength;
  const size_t remLenBytes = encodeRemainingLength(remainingLength, remLen);
  uint16_t offset = 0U;

  packet[offset++] = 0x10;
  memcpy(packet + offset, remLen, remLenBytes);
  offset += (uint16_t)remLenBytes;

  packet[offset++] = 0x00;
  packet[offset++] = 0x04;
  packet[offset++] = 'M';
  packet[offset++] = 'Q';
  packet[offset++] = 'T';
  packet[offset++] = 'T';
  packet[offset++] = 0x04;
  packet[offset++] = 0xC2;
  packet[offset++] = 0x00;
  packet[offset++] = 60;
  offset = appendMqttString(packet, offset, gClientId);
  offset = appendMqttString(packet, offset, kBrokerUsername);
  offset = appendMqttString(packet, offset, kBrokerPassword);

  return writeAll(client, packet, offset);
}

bool sendSubscribe(CellularClientSecure &client) {
  uint8_t packet[160];
  uint8_t remLen[4];
  const uint16_t topicLength = (uint16_t)strlen(gTopic);
  const uint32_t remainingLength = 2U + 2U + topicLength + 1U;
  const size_t remLenBytes = encodeRemainingLength(remainingLength, remLen);
  uint16_t offset = 0U;

  packet[offset++] = 0x82;
  memcpy(packet + offset, remLen, remLenBytes);
  offset += (uint16_t)remLenBytes;
  packet[offset++] = 0x00;
  packet[offset++] = 0x01;
  offset = appendMqttString(packet, offset, gTopic);
  packet[offset++] = 0x00;

  return writeAll(client, packet, offset);
}

bool sendPublish(CellularClientSecure &client) {
  uint8_t packet[224];
  uint8_t remLen[4];
  const uint16_t topicLength = (uint16_t)strlen(gTopic);
  const uint16_t payloadLength = (uint16_t)strlen(gPayload);
  const uint32_t remainingLength = 2U + topicLength + payloadLength;
  const size_t remLenBytes = encodeRemainingLength(remainingLength, remLen);
  uint16_t offset = 0U;

  packet[offset++] = 0x30;
  memcpy(packet + offset, remLen, remLenBytes);
  offset += (uint16_t)remLenBytes;
  offset = appendMqttString(packet, offset, gTopic);
  memcpy(packet + offset, gPayload, payloadLength);
  offset = (uint16_t)(offset + payloadLength);

  return writeAll(client, packet, offset);
}

bool waitForConnAck(CellularClientSecure &client) {
  uint8_t type = 0U;
  uint8_t payload[8];
  size_t payloadLength = 0U;

  if (!readFrame(client, &type, payload, sizeof(payload), &payloadLength, 15000UL)) {
    return false;
  }

  {
    char line[64] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS,FRAME,%02X,LEN,%lu",
             (unsigned int)type,
             (unsigned long)payloadLength);
    Serial.println(line);
  }

  return (type == 0x20U) && (payloadLength >= 2U) && (payload[1] == 0x00U);
}

bool waitForSubAck(CellularClientSecure &client) {
  uint8_t type = 0U;
  uint8_t payload[8];
  size_t payloadLength = 0U;

  if (!readFrame(client, &type, payload, sizeof(payload), &payloadLength, 15000UL)) {
    return false;
  }

  {
    char line[64] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS,FRAME,%02X,LEN,%lu",
             (unsigned int)type,
             (unsigned long)payloadLength);
    Serial.println(line);
  }

  return (type == 0x90U) && (payloadLength >= 3U) && (payload[2] != 0x80U);
}

bool waitForPublish(CellularClientSecure &client) {
  const unsigned long start = millis();
  uint8_t type = 0U;
  uint8_t *payload = gPublishFrame;
  size_t payloadLength = 0U;

  while ((millis() - start) < 20000UL) {
    if (!readFrame(client, &type, payload, sizeof(payload), &payloadLength, 5000UL)) {
      if (!client.connected()) {
        return false;
      }
      continue;
    }

    {
      char line[64] = {0};
      snprintf(line, sizeof(line), "+ARDUINO: MQTTS,FRAME,%02X,LEN,%lu",
               (unsigned int)type,
               (unsigned long)payloadLength);
      Serial.println(line);
    }

    if (type != 0x30U || payloadLength < 2U) {
      continue;
    }

    const uint16_t topicLength = ((uint16_t)payload[0] << 8) | payload[1];
    if ((size_t)(2U + topicLength) > payloadLength) {
      char line[80] = {0};
      snprintf(line,
               sizeof(line),
               "+ARDUINO: MQTTS,PUBLISH_PARSE,INVALID_TOPIC_LEN,%u,TOTAL,%lu",
               (unsigned int)topicLength,
               (unsigned long)payloadLength);
      Serial.println(line);
      continue;
    }

    const size_t bodyLength = payloadLength - 2U - topicLength;
    const size_t expectedTopicLength = strlen(gTopic);
    const size_t expectedPayloadLength = strlen(gPayload);

    if ((topicLength == expectedTopicLength) &&
        (bodyLength == expectedPayloadLength) &&
        (memcmp(payload + 2U, gTopic, expectedTopicLength) == 0) &&
        (memcmp(payload + 2U + topicLength, gPayload, expectedPayloadLength) == 0)) {
      gReceivedPublish = true;
      Serial.println("+ARDUINO: MQTTS,LOOPBACK_MATCH,1");
      return true;
    }

    Serial.println("+ARDUINO: MQTTS,LOOPBACK_MATCH,0");
  }

  return false;
}

void buildIdentity() {
  AIR780EPMModemIdentity identity;
  const unsigned long salt = millis();

  if (Modem.getIdentity(identity) && identity.imeiValid) {
    snprintf(gClientId, sizeof(gClientId), "air780epm-%s-%lu", identity.imei, salt);
    snprintf(gTopic, sizeof(gTopic), "/luatos/testcase/mqtt/%s/loopback", identity.imei);
  } else {
    snprintf(gClientId, sizeof(gClientId), "air780epm-%lu", salt);
    snprintf(gTopic, sizeof(gTopic), "/luatos/testcase/mqtt/fallback/loopback");
  }

  snprintf(gPayload, sizeof(gPayload), "hello-%lu", salt);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);

  setTerminalState("SETUP");
  Serial.println("+ARDUINO: MQTTS,READY");
  Modem.begin();

  printStatus("EARLY");
  if (!Modem.waitForNetwork(60000UL)) {
    printStatus("REGISTER_TIMEOUT");
    fail("REGISTER_TIMEOUT", 0);
    return;
  }

  printStatus("REGISTERED");
  if (!Modem.activatePDP(60000UL)) {
    printStatus("PDP_TIMEOUT");
    fail("PDP_TIMEOUT", 0);
    return;
  }

  printStatus("NET_READY");

  if (!syncTime()) {
    fail("TIME_SYNC", 0);
    return;
  }

  buildIdentity();
  {
    char line[160] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS,CLIENT_ID,%s", gClientId);
    Serial.println(line);
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS,TOPIC,%s", gTopic);
    Serial.println(line);
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS,PAYLOAD,%s", gPayload);
    Serial.println(line);
  }

  CellularClientSecure client;
  client.setCACert(kBrokerCa);
  client.setConnectTimeout(30000UL);
  client.setReadTimeout(8000UL);

  {
    char line[96] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS,CONNECTING,%s,%u", kBrokerHost, (unsigned int)kBrokerPort);
    Serial.println(line);
  }

  if (!client.connect(kBrokerHost, kBrokerPort)) {
    fail("TLS_CONNECT", client.lastError());
    return;
  }

  Serial.println("+ARDUINO: MQTTS,TLS_CONNECT,1");

  if (!sendConnect(client)) {
    fail("MQTT_CONNECT_WRITE", client.lastError());
    client.stop();
    return;
  }

  if (!waitForConnAck(client)) {
    fail("MQTT_CONNACK", client.lastError());
    client.stop();
    return;
  }

  Serial.println("+ARDUINO: MQTTS,CONNACK,1");

  if (!sendSubscribe(client)) {
    fail("SUBSCRIBE_WRITE", client.lastError());
    client.stop();
    return;
  }

  if (!waitForSubAck(client)) {
    fail("SUBACK", client.lastError());
    client.stop();
    return;
  }

  Serial.println("+ARDUINO: MQTTS,SUBACK,1");

  if (!sendPublish(client)) {
    fail("PUBLISH_WRITE", client.lastError());
    client.stop();
    return;
  }

  Serial.println("+ARDUINO: MQTTS,PUBLISH,1");

  if (!waitForPublish(client)) {
    fail("PUBLISH_RX", client.lastError());
    client.stop();
    return;
  }

  client.stop();
  setTerminalState("PASS");
  Serial.println("+ARDUINO: MQTTS,PASS");
}

void loop() {
  static unsigned long lastHeartbeatMs = 0U;
  const unsigned long now = millis();

  if ((now - lastHeartbeatMs) >= 3000UL) {
    char line[96] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: MQTTS,HEARTBEAT,%s", gTerminalState);
    Serial.println(line);
    lastHeartbeatMs = now;
  }

  delay(100);
}
