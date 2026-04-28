#include <Arduino.h>

namespace {

const char *kHttpsHost = "example.com";
const uint16_t kHttpsPort = 443U;

void printStatus(const char *stage) {
  AIR780EPMModemStatus status;
  const bool ok = Modem.getStatus(status);

  Serial.print("+ARDUINO: TLS_HTTP,STATUS,");
  Serial.print(stage);
  Serial.print(",GET,");
  Serial.print(ok ? 1 : 0);
  Serial.print(",REGISTERED,");
  Serial.print(status.registered ? 1 : 0);
  Serial.print(",NET_READY,");
  Serial.print(status.networkReady ? 1 : 0);
  Serial.print(",HAS_IPV4,");
  Serial.print(status.hasIPv4 ? 1 : 0);
  Serial.print(",IP,");
  Serial.println(status.localIPv4);
}

void fail(const char *stage, int32_t error) {
  Serial.print("+ARDUINO: TLS_HTTP,FAIL,");
  Serial.print(stage);
  Serial.print(",ERR,");
  Serial.println(error);
}

bool readHttpStatusLine(CellularClientSecure &client, char *line, size_t lineSize, size_t *rxBytes) {
  unsigned long start = millis();
  size_t lineLength = 0;

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

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("+ARDUINO: TLS_HTTP,READY");
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

  CellularClientSecure client;
  client.setInsecure();
  client.setConnectTimeout(30000UL);
  client.setReadTimeout(8000UL);

  Serial.print("+ARDUINO: TLS_HTTP,CONNECTING,");
  Serial.print(kHttpsHost);
  Serial.print(",");
  Serial.println(kHttpsPort);

  if (!client.connect(kHttpsHost, kHttpsPort)) {
    fail("CONNECT", client.lastError());
    return;
  }

  Serial.println("+ARDUINO: TLS_HTTP,CONNECT,1");
  client.print("GET / HTTP/1.0\r\nHost: example.com\r\nConnection: close\r\n\r\n");

  char statusLine[128];
  size_t rxBytes = 0U;
  const bool gotStatusLine = readHttpStatusLine(client, statusLine, sizeof(statusLine), &rxBytes);

  Serial.print("+ARDUINO: TLS_HTTP,RX_BYTES,");
  Serial.println(static_cast<unsigned long>(rxBytes));
  Serial.print("+ARDUINO: TLS_HTTP,STATUS_LINE,");
  Serial.println(gotStatusLine ? statusLine : "");
  Serial.print("+ARDUINO: TLS_HTTP,TLSERR,");
  Serial.println(client.lastError());

  if (!gotStatusLine || (strncmp(statusLine, "HTTP/", 5) != 0)) {
    fail("HTTP_STATUS", client.lastError());
    client.stop();
    return;
  }

  client.stop();
  Serial.println("+ARDUINO: TLS_HTTP,PASS");
}

void loop() {
  delay(1000);
}
