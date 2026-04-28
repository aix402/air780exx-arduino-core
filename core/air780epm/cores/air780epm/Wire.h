#pragma once

#include <stddef.h>
#include <stdint.h>

#include "Stream.h"

#define WIRE_HAS_END 1
#define WIRE_HAS_BUFFER_SIZE 1
#define WIRE_INTERFACES_COUNT 2

#ifndef I2C_BUFFER_LENGTH
#define I2C_BUFFER_LENGTH 32U
#endif

#ifndef BUFFER_LENGTH
#define BUFFER_LENGTH I2C_BUFFER_LENGTH
#endif

class TwoWire : public Stream {
public:
    explicit TwoWire(uint8_t bus_id = 0U);

    bool begin(void);
    bool begin(uint8_t address);
    bool begin(int sda, int scl);
    bool begin(int sda, int scl, uint32_t frequency);
    bool end(void);

    bool setPins(int sda, int scl);
    bool setClock(uint32_t frequency);
    uint32_t getClock(void) const;
    size_t setBufferSize(size_t size);
    void setTimeOut(uint16_t timeout_ms);
    uint16_t getTimeOut(void) const;

    void beginTransmission(uint8_t address);
    uint8_t endTransmission(void);
    uint8_t endTransmission(bool send_stop);
    uint8_t endTransmission(uint8_t send_stop);

    size_t requestFrom(uint8_t address, size_t quantity);
    size_t requestFrom(uint8_t address, size_t quantity, bool send_stop);
    size_t requestFrom(uint8_t address, uint8_t quantity);
    size_t requestFrom(uint8_t address, uint8_t quantity, uint8_t send_stop);
    size_t requestFrom(int address, size_t quantity);
    size_t requestFrom(int address, size_t quantity, bool send_stop);
    size_t requestFrom(int address, int quantity);
    size_t requestFrom(int address, int quantity, int send_stop);

    void onReceive(void (*function)(int));
    void onRequest(void (*function)(void));

    int available(void) override;
    int read(void) override;
    int peek(void) override;
    void flush(void) override;
    size_t write(uint8_t value) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    using Print::write;

private:
    static const size_t kBufferLength = I2C_BUFFER_LENGTH;

    bool ensureInitialized(void);
    int speedCode(void) const;
    uint8_t mapTransferResult(int result) const;
    void resetTxBuffer(void);
    void resetRxBuffer(void);

    uint8_t bus_id_;
    int sda_pin_;
    int scl_pin_;
    uint32_t clock_hz_;
    uint16_t timeout_ms_;
    bool initialized_;
    bool transmitting_;
    uint8_t tx_address_;
    uint8_t tx_buffer_[kBufferLength];
    size_t tx_length_;
    uint8_t rx_buffer_[kBufferLength];
    size_t rx_length_;
    size_t rx_index_;
};

extern TwoWire Wire;
extern TwoWire Wire1;
