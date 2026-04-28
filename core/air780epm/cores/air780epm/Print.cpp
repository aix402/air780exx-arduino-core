#include "Print.h"

#include "WString.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
static const char kDigits[] = "0123456789ABCDEF";

unsigned long long signedMagnitude(int value) {
    return (value < 0) ? (static_cast<unsigned long long>(-(value + 1)) + 1ULL)
                       : static_cast<unsigned long long>(value);
}

unsigned long long signedMagnitude(long value) {
    return (value < 0) ? (static_cast<unsigned long long>(-(value + 1L)) + 1ULL)
                       : static_cast<unsigned long long>(value);
}

unsigned long long signedMagnitude(long long value) {
    return (value < 0) ? (static_cast<unsigned long long>(-(value + 1LL)) + 1ULL)
                       : static_cast<unsigned long long>(value);
}
}

Print::Print() : write_error_(0) {}

size_t Print::write(const uint8_t *buffer, size_t size) {
    if (buffer == nullptr) {
        return 0;
    }

    size_t written = 0;
    while (written < size) {
        const size_t count = write(buffer[written]);
        if (count == 0) {
            setWriteError();
            break;
        }
        written += count;
    }
    return written;
}

size_t Print::write(const char *str) {
    if (str == nullptr) {
        return 0;
    }
    return write(reinterpret_cast<const uint8_t *>(str), strlen(str));
}

size_t Print::write(const char *buffer, size_t size) {
    return write(reinterpret_cast<const uint8_t *>(buffer), size);
}

size_t Print::vprintf(const char *format, va_list args) {
    if (format == nullptr) {
        return 0;
    }

    char stack_buffer[128];
    va_list copy;
    va_copy(copy, args);
    const int needed = vsnprintf(stack_buffer, sizeof(stack_buffer), format, copy);
    va_end(copy);

    if (needed < 0) {
        setWriteError();
        return 0;
    }

    if (static_cast<size_t>(needed) < sizeof(stack_buffer)) {
        return write(stack_buffer, static_cast<size_t>(needed));
    }

    char *heap_buffer = static_cast<char *>(malloc(static_cast<size_t>(needed) + 1U));
    if (heap_buffer == nullptr) {
        setWriteError();
        return 0;
    }

    va_copy(copy, args);
    const int written = vsnprintf(heap_buffer, static_cast<size_t>(needed) + 1U, format, copy);
    va_end(copy);

    size_t result = 0;
    if (written >= 0) {
        result = write(heap_buffer, static_cast<size_t>(written));
    } else {
        setWriteError();
    }

    free(heap_buffer);
    return result;
}

size_t Print::printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    const size_t result = vprintf(format, args);
    va_end(args);
    return result;
}

size_t Print::printf(const __FlashStringHelper *format, ...) {
    va_list args;
    va_start(args, format);
    const size_t result = vprintf(reinterpret_cast<const char *>(format), args);
    va_end(args);
    return result;
}

int Print::availableForWrite(void) {
    return 0;
}

int Print::getWriteError(void) const {
    return write_error_;
}

void Print::clearWriteError(void) {
    setWriteError(0);
}

size_t Print::print(const char *str) {
    return write(str);
}

size_t Print::print(const String &str) {
    return write(str.c_str());
}

size_t Print::print(const Printable &value) {
    return value.printTo(*this);
}

size_t Print::print(const __FlashStringHelper *str) {
    return write(reinterpret_cast<const char *>(str));
}

size_t Print::print(char value) {
    return write(static_cast<uint8_t>(value));
}

size_t Print::print(unsigned char value, int base) {
    return printNumber(value, static_cast<uint8_t>(base));
}

size_t Print::print(int value, int base) {
    if ((base == DEC) && (value < 0)) {
        return write('-') + printNumber(signedMagnitude(value), DEC);
    }
    return printNumber(static_cast<unsigned int>(value), static_cast<uint8_t>(base));
}

size_t Print::print(unsigned int value, int base) {
    return printNumber(value, static_cast<uint8_t>(base));
}

size_t Print::print(long value, int base) {
    if ((base == DEC) && (value < 0)) {
        return write('-') + printNumber(signedMagnitude(value), DEC);
    }
    return printNumber(static_cast<unsigned long>(value), static_cast<uint8_t>(base));
}

size_t Print::print(unsigned long value, int base) {
    return printNumber(value, static_cast<uint8_t>(base));
}

size_t Print::print(long long value, int base) {
    if ((base == DEC) && (value < 0)) {
        return write('-') + printNumber(signedMagnitude(value), DEC);
    }
    return printNumber(static_cast<unsigned long long>(value), static_cast<uint8_t>(base));
}

size_t Print::print(unsigned long long value, int base) {
    return printNumber(value, static_cast<uint8_t>(base));
}

size_t Print::print(double value, int digits) {
    return printFloat(value, static_cast<uint8_t>(digits));
}

size_t Print::print(struct tm *timeinfo, const char *format) {
    if (timeinfo == nullptr) {
        return 0;
    }
    if (format == nullptr) {
        format = "%c";
    }

    char buffer[64];
    const size_t length = strftime(buffer, sizeof(buffer), format, timeinfo);
    return write(buffer, length);
}

size_t Print::println(void) {
    static const uint8_t line_end[] = {'\r', '\n'};
    return write(line_end, sizeof(line_end));
}

size_t Print::println(const char *str) {
    return print(str) + println();
}

size_t Print::println(const String &str) {
    return print(str) + println();
}

size_t Print::println(const Printable &value) {
    return print(value) + println();
}

size_t Print::println(const __FlashStringHelper *str) {
    return print(str) + println();
}

size_t Print::println(char value) {
    return print(value) + println();
}

size_t Print::println(unsigned char value, int base) {
    return print(value, base) + println();
}

size_t Print::println(int value, int base) {
    return print(value, base) + println();
}

size_t Print::println(unsigned int value, int base) {
    return print(value, base) + println();
}

size_t Print::println(long value, int base) {
    return print(value, base) + println();
}

size_t Print::println(unsigned long value, int base) {
    return print(value, base) + println();
}

size_t Print::println(long long value, int base) {
    return print(value, base) + println();
}

size_t Print::println(unsigned long long value, int base) {
    return print(value, base) + println();
}

size_t Print::println(double value, int digits) {
    return print(value, digits) + println();
}

size_t Print::println(struct tm *timeinfo, const char *format) {
    return print(timeinfo, format) + println();
}

void Print::setWriteError(int error) {
    write_error_ = error;
}

size_t Print::printNumber(unsigned long long value, uint8_t base) {
    if (base == 0U) {
        return write(static_cast<uint8_t>(value));
    }
    if ((base < 2U) || (base > 16U)) {
        base = DEC;
    }

    char buffer[65];
    size_t index = sizeof(buffer);
    buffer[--index] = '\0';

    do {
        buffer[--index] = kDigits[value % base];
        value /= base;
    } while (value != 0ULL);

    return write(&buffer[index]);
}

size_t Print::printFloat(double value, uint8_t digits) {
    if (isnan(value)) {
        return print("nan");
    }
    if (isinf(value)) {
        return print("inf");
    }
    if (digits > 10U) {
        digits = 10U;
    }

    size_t written = 0;
    if (value < 0.0) {
        written += write('-');
        value = -value;
    }

    double rounding = 0.5;
    for (uint8_t i = 0; i < digits; ++i) {
        rounding /= 10.0;
    }
    value += rounding;

    const unsigned long long integer = static_cast<unsigned long long>(value);
    double remainder = value - static_cast<double>(integer);

    written += printNumber(integer, DEC);
    if (digits > 0U) {
        written += write('.');
        while (digits-- > 0U) {
            remainder *= 10.0;
            const uint8_t digit = static_cast<uint8_t>(remainder);
            written += write(static_cast<uint8_t>('0' + digit));
            remainder -= digit;
        }
    }

    return written;
}
