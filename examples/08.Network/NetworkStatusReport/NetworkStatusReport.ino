#include <Arduino.h>

static void printStatus(const AIR780EPMModemStatus &status) {
  Serial.print("+ARDUINO: MODEM_STATUS,PS_READY,");
  Serial.println(status.psReady ? 1 : 0);

  Serial.print("+ARDUINO: MODEM_STATUS,CEREG,");
  Serial.print(status.ceregState);
  Serial.print(",REGISTERED,");
  Serial.println(status.registered ? 1 : 0);

  Serial.print("+ARDUINO: MODEM_STATUS,NET,VALID,");
  Serial.print(status.netInfoValid ? 1 : 0);
  Serial.print(",RESULT,");
  Serial.print(status.netMgrResult);
  Serial.print(",STATUS,");
  Serial.print(status.netStatus);
  Serial.print(",IPTYPE,");
  Serial.print(status.ipType);
  Serial.print(",READY,");
  Serial.println(status.networkReady ? 1 : 0);

  Serial.print("+ARDUINO: MODEM_STATUS,IPV4,HAS,");
  Serial.print(status.hasIPv4 ? 1 : 0);
  Serial.print(",ADDR,");
  Serial.println(status.localIPv4);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("+ARDUINO: MODEM_STATUS,READY");
  Modem.begin();
}

void loop() {
  AIR780EPMModemStatus status;
  const bool ok = Modem.getStatus(status);

  Serial.print("+ARDUINO: MODEM_STATUS,GET,");
  Serial.println(ok ? 1 : 0);

  if (ok) {
    printStatus(status);
  }

  delay(5000);
}
