#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

static uint8_t txBuffer[4] = {0x9F, 0x00, 0x00, 0x00};
static uint8_t rxBuffer[4] = {0};

static void compileOnlyBusApi() {
  TwoWire *wire = &Wire;
  wire->setPins(SDA, SCL);
  wire->setClock(400000);
  wire->setBufferSize(64);
  wire->setTimeOut(25);
  (void)wire->getTimeOut();
  (void)wire->getClock();
  (void)wire->begin();
  wire->beginTransmission(0x3C);
  (void)wire->write((uint8_t)0x00);
  (void)wire->write(txBuffer, sizeof(txBuffer));
  (void)wire->endTransmission(false);
  (void)wire->requestFrom(0x3C, (size_t)2, true);
  (void)wire->available();
  (void)wire->peek();
  (void)wire->read();
  (void)wire->end();

  SPISettings settings(1000000, MSBFIRST, SPI_MODE0);
  SPI.begin();
  SPI.begin(SCK, MISO, MOSI, SS);
  SPI.setHwCs(false);
  SPI.beginTransaction(settings);
  (void)SPI.transfer((uint8_t)0xAA);
  (void)SPI.transfer16(0xA55A);
  (void)SPI.transfer32(0x11223344);
  SPI.transfer(txBuffer, rxBuffer, sizeof(txBuffer));
  SPI.transferBytes(txBuffer, rxBuffer, sizeof(txBuffer));
  SPI.write((uint8_t)0x55);
  SPI.write16(0x1234);
  SPI.write32(0x12345678);
  SPI.writeBytes(txBuffer, sizeof(txBuffer));
  SPI.writePixels(txBuffer, sizeof(txBuffer));
  SPI.writePattern(txBuffer, 2, 2);
  SPI.setBitOrder(LSBFIRST);
  SPI.setDataMode(SPI_MODE3);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.setFrequency(2000000);
  (void)SPI.getClockDivider();
  (void)SPI.pinSS();
  SPI.endTransaction();
  SPI.end();
}

void setup() {
  Serial.begin(921600);
  Serial.println(F("+ARDUINO: BUS_API_P2,COMPILE"));

  if (false) {
    compileOnlyBusApi();
  }
}

void loop() {
  delay(1000);
}
