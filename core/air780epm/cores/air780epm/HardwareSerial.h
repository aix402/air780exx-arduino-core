#pragma once

#include <stddef.h>
#include <stdint.h>

#include "Stream.h"

#ifndef SERIAL_5N1
#define SERIAL_5N1 0x00
#define SERIAL_6N1 0x02
#define SERIAL_7N1 0x04
#define SERIAL_8N1 0x06
#define SERIAL_5N2 0x08
#define SERIAL_6N2 0x0A
#define SERIAL_7N2 0x0C
#define SERIAL_8N2 0x0E
#define SERIAL_5E1 0x20
#define SERIAL_6E1 0x22
#define SERIAL_7E1 0x24
#define SERIAL_8E1 0x26
#define SERIAL_5E2 0x28
#define SERIAL_6E2 0x2A
#define SERIAL_7E2 0x2C
#define SERIAL_8E2 0x2E
#define SERIAL_5O1 0x30
#define SERIAL_6O1 0x32
#define SERIAL_7O1 0x34
#define SERIAL_8O1 0x36
#define SERIAL_5O2 0x38
#define SERIAL_6O2 0x3A
#define SERIAL_7O2 0x3C
#define SERIAL_8O2 0x3E
#endif

class HardwareSerial : public Stream {
public:
    explicit HardwareSerial(int id = 0);

    void begin(unsigned long baud);
    void begin(unsigned long baud, uint16_t config);
    void end(void);

    int available(void) override;
    int availableForWrite(void) override;
    int read(void) override;
    size_t read(uint8_t *buffer, size_t size);
    size_t read(char *buffer, size_t size);
    int peek(void) override;
    void flush(void) override;

    size_t write(uint8_t value) override;
    size_t write(const uint8_t *buffer, size_t size) override;
    using Print::write;

    operator bool() const;

private:
    bool isLogPort(void) const;

    int id_;
    bool begun_;
    int peeked_;
};

extern HardwareSerial Serial;
extern HardwareSerial Serial1;
extern HardwareSerial Serial2;
extern HardwareSerial Serial3;
