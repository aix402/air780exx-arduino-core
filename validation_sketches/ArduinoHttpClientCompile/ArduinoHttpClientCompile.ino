#include <Arduino.h>
#include <ArduinoHttpClient.h>

CellularClient netClient;
HttpClient httpClient(netClient, "example.com", 80);

void setup() {
  Serial.begin(921600);
  httpClient.setHttpResponseTimeout(5000);
  httpClient.setHttpWaitForDataDelay(50);
  httpClient.connectionKeepAlive();
  Serial.println(F("+ARDUINO: ARDUINO_HTTPCLIENT_COMPILE,READY"));
}

void loop() {
  delay(1000);
}
