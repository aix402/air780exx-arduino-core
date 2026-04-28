#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

void setup() {
  Serial.begin(115200);

  IPAddress ipFromOctets(1, 2, 3, 4);
  IPAddress ipFromString("5.6.7.8");
  IPAddress assigned;
  uint8_t raw[4] = {9, 10, 11, 12};
  assigned = raw;

  const bool ipOk =
      ipFromOctets.toString().equals("1.2.3.4") &&
      ipFromString[0] == 5 &&
      assigned == raw &&
      assigned.fromString(String("13.14.15.16")) &&
      assigned.toString().equals("13.14.15.16");

  WiFiClient client;
  client.setConnectionTimeout(1234);
  const bool clientOk =
      (client.connectTimeout() == 1234U) &&
      (client.write_P(PSTR("abc"), 3) == 0U) &&
      (client.connected() == 0U) &&
      (client.getNoDelay() == false);

  WiFiClientSecure secureClient;
  secureClient.setConnectionTimeout(2345);
  secureClient.setReadTimeout(3456);
  secureClient.setInsecure();
  const bool secureOk =
      (secureClient.connectTimeout() == 2345U) &&
      (secureClient.readTimeout() == 3456U) &&
      (secureClient.write_P(PSTR("tls"), 3) == 0U) &&
      (secureClient.connected() == 0U);

  WiFiUDP udp;
  const bool udpOk =
      (udp.remotePort() == 0U) &&
      udp.remoteIP().toString().equals("0.0.0.0");

  Serial.printf(
      "+ARDUINO: NETWORK_API_P1,IP,%d,CLIENT,%d,TLS,%d,UDP,%d\n",
      ipOk ? 1 : 0,
      clientOk ? 1 : 0,
      secureOk ? 1 : 0,
      udpOk ? 1 : 0);
  Serial.println((ipOk && clientOk && secureOk && udpOk) ? "+ARDUINO: NETWORK_API_P1,PASS"
                                                          : "+ARDUINO: NETWORK_API_P1,FAIL");
}

void loop() {
  delay(1000);
}
