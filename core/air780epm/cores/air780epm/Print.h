#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>

#include "Printable.h"

#ifndef DEC
#define DEC 10
#endif
#ifndef HEX
#define HEX 16
#endif
#ifndef OCT
#define OCT 8
#endif
#ifndef BIN
#define BIN 2
#endif

class String;
class __FlashStringHelper;

class Print {
public:
    Print();
    virtual ~Print() {}

    virtual size_t write(uint8_t value) = 0;
    virtual size_t write(const uint8_t *buffer, size_t size);

    size_t write(const char *str);
    size_t write(const char *buffer, size_t size);

    size_t vprintf(const char *format, va_list args);
    size_t printf(const char *format, ...) __attribute__((format(printf, 2, 3)));
    size_t printf(const __FlashStringHelper *format, ...);

    virtual int availableForWrite(void);

    int getWriteError(void) const;
    void clearWriteError(void);

    size_t print(const char *str);
    size_t print(const String &str);
    size_t print(const Printable &value);
    size_t print(const __FlashStringHelper *str);
    size_t print(char value);
    size_t print(unsigned char value, int base = DEC);
    size_t print(int value, int base = DEC);
    size_t print(unsigned int value, int base = DEC);
    size_t print(long value, int base = DEC);
    size_t print(unsigned long value, int base = DEC);
    size_t print(long long value, int base = DEC);
    size_t print(unsigned long long value, int base = DEC);
    size_t print(double value, int digits = 2);
    size_t print(struct tm *timeinfo, const char *format = nullptr);

    size_t println(void);
    size_t println(const char *str);
    size_t println(const String &str);
    size_t println(const Printable &value);
    size_t println(const __FlashStringHelper *str);
    size_t println(char value);
    size_t println(unsigned char value, int base = DEC);
    size_t println(int value, int base = DEC);
    size_t println(unsigned int value, int base = DEC);
    size_t println(long value, int base = DEC);
    size_t println(unsigned long value, int base = DEC);
    size_t println(long long value, int base = DEC);
    size_t println(unsigned long long value, int base = DEC);
    size_t println(double value, int digits = 2);
    size_t println(struct tm *timeinfo, const char *format = nullptr);

    virtual void flush(void) {}

protected:
    void setWriteError(int error = 1);
    size_t printNumber(unsigned long long value, uint8_t base);
    size_t printFloat(double value, uint8_t digits);

private:
    int write_error_;
};
