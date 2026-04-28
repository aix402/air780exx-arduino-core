#include <Arduino.h>
#include <Wire.h>
#include <stdio.h>

static const uint8_t SHT40_ADDRESS = 0x44;
static const uint8_t SHT40_MEASURE_HIGH_PRECISION = 0xFD;

static uint8_t crc8(const uint8_t *data, size_t len) {
  uint8_t crc = 0xFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x31) : static_cast<uint8_t>(crc << 1);
    }
  }
  return crc;
}

static bool readSht40(float &temperature, float &humidity) {
  Wire.beginTransmission(SHT40_ADDRESS);
  Wire.write(SHT40_MEASURE_HIGH_PRECISION);
  const uint8_t txResult = Wire.endTransmission();
  if (txResult != 0) {
    char line[40];
    snprintf(line, sizeof(line), "+ARDUINO: SHT40,TX_ERROR,%u", txResult);
    Serial.println(line);
    return false;
  }

  delay(10);

  const size_t received = Wire.requestFrom(SHT40_ADDRESS, static_cast<size_t>(6));
  if (received != 6) {
    char line[40];
    snprintf(line, sizeof(line), "+ARDUINO: SHT40,RX_ERROR,%lu", static_cast<unsigned long>(received));
    Serial.println(line);
    return false;
  }

  uint8_t data[6];
  for (size_t i = 0; i < sizeof(data); ++i) {
    data[i] = static_cast<uint8_t>(Wire.read());
  }

  if ((crc8(data, 2) != data[2]) || (crc8(data + 3, 2) != data[5])) {
    Serial.println("+ARDUINO: SHT40,CRC_ERROR");
    return false;
  }

  const uint16_t rawTemperature = (static_cast<uint16_t>(data[0]) << 8) | data[1];
  const uint16_t rawHumidity = (static_cast<uint16_t>(data[3]) << 8) | data[4];
  temperature = -45.0f + 175.0f * static_cast<float>(rawTemperature) / 65535.0f;
  humidity = -6.0f + 125.0f * static_cast<float>(rawHumidity) / 65535.0f;
  humidity = constrain(humidity, 0.0f, 100.0f);
  return true;
}

void setup() {
  Serial.begin(921600);
  Serial.println("+ARDUINO: SHT40,READY");

  Wire.begin();
  Wire.setClock(100000);
  char line[48];
  snprintf(line, sizeof(line), "+ARDUINO: WIRE,I2C0,SDA=%u,SCL=%u", SDA, SCL);
  Serial.println(line);
}

void loop() {
  float temperature = 0.0f;
  float humidity = 0.0f;
  if (readSht40(temperature, humidity)) {
    const int temperatureCenti = static_cast<int>((temperature * 100.0f) + ((temperature >= 0.0f) ? 0.5f : -0.5f));
    const int humidityCenti = static_cast<int>((humidity * 100.0f) + 0.5f);
    char line[64];
    snprintf(
      line,
      sizeof(line),
      "+ARDUINO: SHT40,PASS,T=%d.%02d,RH=%d.%02d",
      temperatureCenti / 100,
      abs(temperatureCenti % 100),
      humidityCenti / 100,
      abs(humidityCenti % 100));
    Serial.println(line);
  }
  delay(1000);
}
