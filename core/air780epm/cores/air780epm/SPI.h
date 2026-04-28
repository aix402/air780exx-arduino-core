#pragma once

#include <stddef.h>
#include <stdint.h>

#include "Arduino.h"

#define SPI_HAS_TRANSACTION 1
#define SPI_INTERFACES_COUNT 2

#ifndef SPI_MODE0
#define SPI_MODE0 0x00U
#define SPI_MODE1 0x01U
#define SPI_MODE2 0x02U
#define SPI_MODE3 0x03U
#endif

#ifndef SPI_CLOCK_DIV2
#define SPI_CLOCK_DIV2 2U
#endif

#define SPI_LSBFIRST LSBFIRST
#define SPI_MSBFIRST MSBFIRST

typedef uint8_t BitOrder;

class SPISettings {
public:
    SPISettings(uint32_t clock = 4000000UL, uint8_t bit_order = MSBFIRST, uint8_t data_mode = SPI_MODE0);

    uint32_t _clock;
    uint8_t _bitOrder;
    uint8_t _dataMode;
};

class SPIClass {
public:
    explicit SPIClass(uint8_t bus_id = 0U);

    void begin(void);
    void begin(int8_t sck, int8_t miso = -1, int8_t mosi = -1, int8_t ss = -1);
    void end(void);

    void setHwCs(bool use);
    void beginTransaction(SPISettings settings);
    void endTransaction(void);

    void transfer(void *data, uint32_t size);
    uint8_t transfer(uint8_t data);
    uint16_t transfer16(uint16_t data);
    uint32_t transfer32(uint32_t data);
    void transfer(uint8_t *buffer, size_t len);
    void transfer(const uint8_t *tx_buffer, uint8_t *rx_buffer, size_t len);
    void transferBytes(uint8_t *tx_buffer, uint8_t *rx_buffer, size_t len);
    void transferBytes(const uint8_t *tx_buffer, uint8_t *rx_buffer, size_t len);

    void write(uint8_t data);
    void write16(uint16_t data);
    void write32(uint32_t data);
    void writeBytes(uint8_t *buffer, size_t len);
    void writeBytes(const uint8_t *buffer, size_t len);
    void writePixels(const void *buffer, uint32_t size);
    void writePattern(const uint8_t *data, uint8_t size, uint32_t repeat);

    void setBitOrder(uint8_t bit_order);
    void setDataMode(uint8_t data_mode);
    void setClockDivider(uint8_t divider);
    uint32_t getClockDivider(void) const;
    void setFrequency(uint32_t frequency);
    void setClock(uint32_t frequency);
    int8_t pinSS(void) const;

private:
    bool ensureInitialized(void);
    bool applySettings(const SPISettings &settings);
    uint32_t normalizedClock(uint32_t clock) const;
    int modeCpha(void) const;
    int modeCpol(void) const;
    void writeByteOrdered(uint32_t data, uint8_t bytes);

    uint8_t bus_id_;
    bool initialized_;
    bool hardware_cs_;
    SPISettings active_settings_;
    int8_t sck_pin_;
    int8_t miso_pin_;
    int8_t mosi_pin_;
    int8_t ss_pin_;
};

extern SPIClass SPI;
extern SPIClass SPI1;
