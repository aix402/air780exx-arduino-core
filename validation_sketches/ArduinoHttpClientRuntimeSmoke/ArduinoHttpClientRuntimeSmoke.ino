#include <Arduino.h>
#include <ArduinoHttpClient.h>
#include <stdio.h>

namespace {

const char *kHttpHost = "example.com";
const uint16_t kHttpPort = 80U;
const char *kHttpPath = "/";

CellularClient netClient;
HttpClient httpClient(netClient, kHttpHost, kHttpPort);

void printLine(const char *line) {
  Serial.println(line);
}

void printStatus(const char *stage) {
  AIR780EPMModemStatus status;
  const bool ok = Modem.getStatus(status);
  const String ipText = status.localIPv4.toString();
  char line[176] = {0};

  snprintf(line,
           sizeof(line),
           "+ARDUINO: ARDUINO_HTTPCLIENT,STATUS,%s,GET,%u,REGISTERED,%u,NET_READY,%u,HAS_IPV4,%u,IP,%s",
           (stage != NULL) ? stage : "",
           ok ? 1U : 0U,
           status.registered ? 1U : 0U,
           status.networkReady ? 1U : 0U,
           status.hasIPv4 ? 1U : 0U,
           ipText.c_str());
  Serial.println(line);
}

void fail(const char *stage, int error) {
  char line[128] = {0};

  snprintf(line,
           sizeof(line),
           "+ARDUINO: ARDUINO_HTTPCLIENT,FAIL,%s,ERR,%d,TCPERR,%ld",
           (stage != NULL) ? stage : "",
           error,
           (long)netClient.lastError());
  Serial.println(line);
  httpClient.stop();
}

} // namespace

void setup() {
  Serial.begin(921600);
  delay(1500);

  printLine("+ARDUINO: ARDUINO_HTTPCLIENT,READY");
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

  netClient.setConnectTimeout(30000UL);
  httpClient.setHttpResponseTimeout(15000UL);
  httpClient.setHttpWaitForDataDelay(50UL);

  {
    char line[96] = {0};
    snprintf(line, sizeof(line), "+ARDUINO: ARDUINO_HTTPCLIENT,GET,%s,%u,%s", kHttpHost, (unsigned int)kHttpPort, kHttpPath);
    Serial.println(line);
  }

  const int requestResult = httpClient.get(kHttpPath);
  Serial.print("+ARDUINO: ARDUINO_HTTPCLIENT,GET_RESULT,");
  Serial.println(requestResult);
  if (requestResult != HTTP_SUCCESS) {
    fail("GET", requestResult);
    return;
  }

  const int statusCode = httpClient.responseStatusCode();
  Serial.print("+ARDUINO: ARDUINO_HTTPCLIENT,STATUS_CODE,");
  Serial.println(statusCode);
  if (statusCode < 100) {
    fail("STATUS_CODE", statusCode);
    return;
  }

  const int skipResult = httpClient.skipResponseHeaders();
  Serial.print("+ARDUINO: ARDUINO_HTTPCLIENT,SKIP_HEADERS,");
  Serial.println(skipResult);
  if (skipResult != HTTP_SUCCESS) {
    fail("SKIP_HEADERS", skipResult);
    return;
  }

  const long contentLength = httpClient.contentLength();
  Serial.print("+ARDUINO: ARDUINO_HTTPCLIENT,CONTENT_LENGTH,");
  Serial.println(contentLength);

  const String body = httpClient.responseBody();
  const uint32_t bodyBytes = (uint32_t)body.length();

  Serial.print("+ARDUINO: ARDUINO_HTTPCLIENT,BODY_BYTES,");
  Serial.println((unsigned long)bodyBytes);

  if (bodyBytes == 0UL) {
    fail("BODY_EMPTY", netClient.lastError());
    return;
  }

  httpClient.stop();
  printLine("+ARDUINO: ARDUINO_HTTPCLIENT,PASS");
}

void loop() {
  delay(1000);
}
