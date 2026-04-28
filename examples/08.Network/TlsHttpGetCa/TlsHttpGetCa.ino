#include <Arduino.h>
#include <time.h>

namespace {

const char *kHttpsHost = "valid-isrgrootx1.letsencrypt.org";
const uint16_t kHttpsPort = 443U;
const long kGmtOffsetSeconds = 8L * 3600L;
const int kDaylightOffsetSeconds = 0;
const char *kTimeServer1 = "ntp.aliyun.com";
const char *kTimeServer2 = "ntp.ntsc.ac.cn";
const char *kTimeServer3 = "time1.cloud.tencent.com";

const char kIsrgRootX1[] PROGMEM = R"PEM(
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

void printStatus(const char *stage) {
  AIR780EPMModemStatus status;
  const bool ok = Modem.getStatus(status);
  const String ipText = status.localIPv4.toString();
  char line[160] = {0};

  snprintf(line,
           sizeof(line),
           "+ARDUINO: TLS_CA_HTTP,STATUS,%s,GET,%u,REGISTERED,%u,NET_READY,%u,HAS_IPV4,%u,IP,%s",
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

  snprintf(line,
           sizeof(line),
           "+ARDUINO: TLS_CA_HTTP,FAIL,%s,ERR,%ld",
           (stage != NULL) ? stage : "",
           (long)error);
  Serial.println(line);
}

bool readHttpStatusLine(CellularClientSecure &client, char *line, size_t lineSize, size_t *rxBytes) {
  const unsigned long start = millis();
  size_t lineLength = 0U;

  if ((line == NULL) || (lineSize < 2U)) {
    return false;
  }

  line[0] = '\0';
  if (rxBytes != NULL) {
    *rxBytes = 0U;
  }

  while ((millis() - start) < 15000UL) {
    while (client.available() > 0) {
      const int value = client.read();
      if (value < 0) {
        break;
      }

      if (rxBytes != NULL) {
        (*rxBytes)++;
      }

      if ((value == '\r') || (value == '\n')) {
        if (lineLength > 0U) {
          line[lineLength] = '\0';
          return true;
        }
        continue;
      }

      if (lineLength < (lineSize - 1U)) {
        line[lineLength++] = static_cast<char>(value);
      }
    }

    if (!client.connected()) {
      break;
    }

    delay(10);
  }

  line[lineLength] = '\0';
  return lineLength > 0U;
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
    snprintf(line, sizeof(line), "+ARDUINO: TLS_CA_HTTP,LOCAL_TIME,%s", buffer);
    Serial.println(line);
  }
  return true;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("+ARDUINO: TLS_CA_HTTP,READY");
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

  CellularClientSecure client;
  client.setCACert(kIsrgRootX1);
  client.setConnectTimeout(30000UL);
  client.setReadTimeout(8000UL);

  {
    char line[128] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: TLS_CA_HTTP,CONNECTING,%s,%u", kHttpsHost, (unsigned int)kHttpsPort);
    Serial.println(line);
  }

  if (!client.connect(kHttpsHost, kHttpsPort)) {
    fail("CONNECT", client.lastError());
    return;
  }

  Serial.println("+ARDUINO: TLS_CA_HTTP,CONNECT,1");
  client.print("GET / HTTP/1.0\r\nHost: valid-isrgrootx1.letsencrypt.org\r\nConnection: close\r\n\r\n");

  char statusLine[128];
  size_t rxBytes = 0U;
  const bool gotStatusLine = readHttpStatusLine(client, statusLine, sizeof(statusLine), &rxBytes);

  {
    char line[160] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: TLS_CA_HTTP,RX_BYTES,%lu", static_cast<unsigned long>(rxBytes));
    Serial.println(line);
    snprintf(line, sizeof(line), "+ARDUINO: TLS_CA_HTTP,STATUS_LINE,%s", gotStatusLine ? statusLine : "");
    Serial.println(line);
    snprintf(line, sizeof(line), "+ARDUINO: TLS_CA_HTTP,TLSERR,%ld", (long)client.lastError());
    Serial.println(line);
  }

  if (!gotStatusLine || (strncmp(statusLine, "HTTP/", 5) != 0)) {
    fail("HTTP_STATUS", client.lastError());
    client.stop();
    return;
  }

  client.stop();
  Serial.println("+ARDUINO: TLS_CA_HTTP,PASS");
}

void loop() {
  delay(1000);
}
