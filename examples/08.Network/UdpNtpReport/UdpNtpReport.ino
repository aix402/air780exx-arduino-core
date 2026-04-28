#include <Arduino.h>

namespace {

const char *kNtpHost = "pool.ntp.org";
const uint16_t kNtpPort = 123U;
const uint16_t kLocalPort = 2390U;
const size_t kNtpPacketSize = 48U;
const unsigned long kNtpEpochOffset = 2208988800UL;

CellularUDP Udp;
uint8_t Packet[kNtpPacketSize];

void printStatus(const char *stage) {
  AIR780EPMModemStatus status;
  const bool ok = Modem.getStatus(status);

  Serial.print("+ARDUINO: UDP_NTP,STATUS,");
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
  Serial.print("+ARDUINO: UDP_NTP,FAIL,");
  Serial.print(stage);
  Serial.print(",ERR,");
  Serial.println(error);
}

void prepareNtpPacket() {
  memset(Packet, 0, sizeof(Packet));
  Packet[0] = 0b11100011;
  Packet[1] = 0;
  Packet[2] = 6;
  Packet[3] = 0xEC;
  Packet[12] = 49;
  Packet[13] = 0x4E;
  Packet[14] = 49;
  Packet[15] = 52;
}

unsigned long parseEpoch() {
  const unsigned long highWord = word(Packet[40], Packet[41]);
  const unsigned long lowWord = word(Packet[42], Packet[43]);
  const unsigned long secondsSince1900 = (highWord << 16) | lowWord;
  return secondsSince1900 - kNtpEpochOffset;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("+ARDUINO: UDP_NTP,READY");
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

  if (!Udp.begin(kLocalPort)) {
    fail("BEGIN", Udp.lastError());
    return;
  }

  prepareNtpPacket();
  if (!Udp.beginPacket(kNtpHost, kNtpPort)) {
    fail("BEGIN_PACKET", Udp.lastError());
    return;
  }

  if (Udp.write(Packet, sizeof(Packet)) != sizeof(Packet)) {
    fail("WRITE", Udp.lastError());
    return;
  }

  if (!Udp.endPacket()) {
    fail("END_PACKET", Udp.lastError());
    return;
  }

  Serial.print("+ARDUINO: UDP_NTP,SENT,");
  Serial.print(kNtpHost);
  Serial.print(",");
  Serial.println(kNtpPort);

  const unsigned long start = millis();
  int packetSize = 0;
  while ((millis() - start) < 10000UL) {
    packetSize = Udp.parsePacket();
    if (packetSize >= (int)kNtpPacketSize) {
      break;
    }
    delay(50);
  }

  Serial.print("+ARDUINO: UDP_NTP,PACKET,");
  Serial.print(packetSize);
  Serial.print(",REMOTE,");
  Serial.print(Udp.remoteIP());
  Serial.print(",");
  Serial.println(Udp.remotePort());

  if (packetSize < (int)kNtpPacketSize) {
    fail("RX_TIMEOUT", Udp.lastError());
    return;
  }

  if (Udp.read(Packet, sizeof(Packet)) != (int)sizeof(Packet)) {
    fail("READ", Udp.lastError());
    return;
  }

  const unsigned long epoch = parseEpoch();
  Serial.print("+ARDUINO: UDP_NTP,EPOCH,");
  Serial.println(epoch);

  if (epoch < 1700000000UL) {
    fail("EPOCH_RANGE", 0);
    return;
  }

  Serial.println("+ARDUINO: UDP_NTP,PASS");
}

void loop() {
  delay(1000);
}
