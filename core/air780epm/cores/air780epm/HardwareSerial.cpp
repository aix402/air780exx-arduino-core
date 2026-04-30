#include "HardwareSerial.h"

#include <string.h>

extern "C" {
#include "arduino_debug_io.h"
#include "luat_uart.h"
}

HardwareSerial Serial(0);
HardwareSerial Serial1(1);
HardwareSerial Serial2(2);
HardwareSerial Serial3(3);

HardwareSerial::HardwareSerial(int id) : id_(id), begun_(false), peeked_(-1) {}

void HardwareSerial::begin(unsigned long baud) {
    begin(baud, SERIAL_8N1);
}

void HardwareSerial::begin(unsigned long baud, uint16_t config) {
    if (isLogPort()) {
        begun_ = true;
        return;
    }

    luat_uart_t uart;
    memset(&uart, 0, sizeof(uart));
    uart.id = id_;
    uart.baud_rate = static_cast<int>(baud);
    uart.data_bits = static_cast<uint8_t>(((config & 0x06U) >> 1U) + 5U);
    uart.stop_bits = (config & 0x08U) ? 2U : 1U;
    uart.bit_order = 0;
    uart.parity = 0;
    if ((config & 0x30U) == 0x20U) {
        uart.parity = LUAT_PARITY_EVEN;
    } else if ((config & 0x30U) == 0x30U) {
        uart.parity = LUAT_PARITY_ODD;
    }
    uart.bufsz = 2048U;
    uart.pin485 = 0xffffffffUL;
    uart.delay = 0;
    uart.rx_level = 0;
    uart.debug_enable = 0;
    uart.error_drop = 0;

    begun_ = (luat_uart_setup(&uart) == 0);
    peeked_ = -1;
}

void HardwareSerial::end(void) {
    if (!isLogPort() && begun_) {
        luat_uart_close(id_);
    }
    begun_ = false;
    peeked_ = -1;
}

int HardwareSerial::available(void) {
    if (isLogPort() || !begun_) {
        return 0;
    }
    int count = luat_uart_read(id_, nullptr, 0);
    if (count < 0) {
        count = 0;
    }
    return count + ((peeked_ >= 0) ? 1 : 0);
}

int HardwareSerial::availableForWrite(void) {
    return isLogPort() || begun_ ? 1 : 0;
}

int HardwareSerial::read(void) {
    if (isLogPort() || !begun_) {
        return -1;
    }
    if (peeked_ >= 0) {
        const int value = peeked_;
        peeked_ = -1;
        return value;
    }

    uint8_t value = 0;
    const int count = luat_uart_read(id_, &value, 1U);
    return (count == 1) ? value : -1;
}

size_t HardwareSerial::read(uint8_t *buffer, size_t size) {
    if ((buffer == nullptr) || (size == 0U) || isLogPort() || !begun_) {
        return 0;
    }

    size_t copied = 0;
    if (peeked_ >= 0) {
        buffer[copied++] = static_cast<uint8_t>(peeked_);
        peeked_ = -1;
    }

    if (copied < size) {
        const int count = luat_uart_read(id_, buffer + copied, size - copied);
        if (count > 0) {
            copied += static_cast<size_t>(count);
        }
    }

    return copied;
}

size_t HardwareSerial::read(char *buffer, size_t size) {
    return read(reinterpret_cast<uint8_t *>(buffer), size);
}

int HardwareSerial::peek(void) {
    if (peeked_ >= 0) {
        return peeked_;
    }
    peeked_ = read();
    return peeked_;
}

void HardwareSerial::flush(void) {
}

bool HardwareSerial::isLogPort(void) const {
    return id_ == 0;
}

size_t HardwareSerial::write(uint8_t value) {
    return write(&value, 1);
}

size_t HardwareSerial::write(const uint8_t *buffer, size_t size) {
    if (buffer == nullptr || size == 0) {
        return 0;
    }
    if (isLogPort()) {
        return arduinoCoreDebugWrite(buffer, size);
    }
    if (!begun_) {
        return 0;
    }

    const int written = luat_uart_write(id_, const_cast<uint8_t *>(buffer), size);
    return (written > 0) ? static_cast<size_t>(written) : 0U;
}

HardwareSerial::operator bool() const {
    return true;
}
