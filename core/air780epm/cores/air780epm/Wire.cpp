#include "Wire.h"

#include "pins_arduino.h"

#include <string.h>

extern "C" {
#include "luat_i2c.h"
}

TwoWire Wire(0U);
TwoWire Wire1(1U);

TwoWire::TwoWire(uint8_t bus_id)
    : bus_id_(bus_id),
      sda_pin_(SDA),
      scl_pin_(SCL),
      clock_hz_(100000UL),
      timeout_ms_(1000U),
      initialized_(false),
      transmitting_(false),
      tx_address_(0U),
      tx_length_(0U),
      rx_length_(0U),
      rx_index_(0U) {
}

bool TwoWire::begin(void) {
    if (luat_i2c_setup(bus_id_, speedCode()) == 0) {
        initialized_ = true;
        resetTxBuffer();
        resetRxBuffer();
        return true;
    }
    initialized_ = false;
    return false;
}

bool TwoWire::begin(uint8_t address) {
    (void)address;
    return false;
}

bool TwoWire::begin(int sda, int scl) {
    return begin(sda, scl, clock_hz_);
}

bool TwoWire::begin(int sda, int scl, uint32_t frequency) {
    (void)setPins(sda, scl);
    clock_hz_ = (frequency != 0UL) ? frequency : clock_hz_;
    return begin();
}

bool TwoWire::end(void) {
    const bool was_initialized = initialized_;
    if (initialized_) {
        (void)luat_i2c_close(bus_id_);
    }
    initialized_ = false;
    resetTxBuffer();
    resetRxBuffer();
    return was_initialized;
}

bool TwoWire::setPins(int sda, int scl) {
    sda_pin_ = sda;
    scl_pin_ = scl;
    return true;
}

bool TwoWire::setClock(uint32_t frequency) {
    if (frequency == 0UL) {
        return false;
    }
    clock_hz_ = frequency;
    if (initialized_) {
        return luat_i2c_setup(bus_id_, speedCode()) == 0;
    }
    return true;
}

uint32_t TwoWire::getClock(void) const {
    return clock_hz_;
}

size_t TwoWire::setBufferSize(size_t size) {
    (void)size;
    return kBufferLength;
}

void TwoWire::setTimeOut(uint16_t timeout_ms) {
    timeout_ms_ = timeout_ms;
    setTimeout(timeout_ms);
}

uint16_t TwoWire::getTimeOut(void) const {
    return timeout_ms_;
}

void TwoWire::beginTransmission(uint8_t address) {
    tx_address_ = address;
    transmitting_ = true;
    resetTxBuffer();
}

uint8_t TwoWire::endTransmission(void) {
    return endTransmission(true);
}

uint8_t TwoWire::endTransmission(bool send_stop) {
    if (!transmitting_) {
        return 4U;
    }

    if (!ensureInitialized()) {
        resetTxBuffer();
        transmitting_ = false;
        return 4U;
    }

    if (tx_length_ == 0U) {
        transmitting_ = false;
        return 4U;
    }

    const int result = luat_i2c_send(
        bus_id_,
        tx_address_,
        tx_buffer_,
        tx_length_,
        send_stop ? 1U : 0U);

    resetTxBuffer();
    transmitting_ = false;
    return mapTransferResult(result);
}

uint8_t TwoWire::endTransmission(uint8_t send_stop) {
    return endTransmission(send_stop != 0U);
}

size_t TwoWire::requestFrom(uint8_t address, size_t quantity) {
    return requestFrom(address, quantity, true);
}

size_t TwoWire::requestFrom(uint8_t address, size_t quantity, bool send_stop) {
    (void)send_stop;
    resetRxBuffer();

    if (quantity > kBufferLength) {
        quantity = kBufferLength;
    }
    if ((quantity == 0U) || !ensureInitialized()) {
        return 0U;
    }

    const int result = luat_i2c_recv(bus_id_, address, rx_buffer_, quantity);
    if (result != 0) {
        resetRxBuffer();
        return 0U;
    }

    rx_length_ = quantity;
    rx_index_ = 0U;
    return rx_length_;
}

size_t TwoWire::requestFrom(uint8_t address, uint8_t quantity) {
    return requestFrom(address, static_cast<size_t>(quantity), true);
}

size_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t send_stop) {
    return requestFrom(address, static_cast<size_t>(quantity), send_stop != 0U);
}

size_t TwoWire::requestFrom(int address, size_t quantity) {
    return requestFrom(static_cast<uint8_t>(address), quantity, true);
}

size_t TwoWire::requestFrom(int address, size_t quantity, bool send_stop) {
    return requestFrom(static_cast<uint8_t>(address), quantity, send_stop);
}

size_t TwoWire::requestFrom(int address, int quantity) {
    return requestFrom(static_cast<uint8_t>(address), static_cast<size_t>(quantity), true);
}

size_t TwoWire::requestFrom(int address, int quantity, int send_stop) {
    return requestFrom(static_cast<uint8_t>(address), static_cast<size_t>(quantity), send_stop != 0);
}

void TwoWire::onReceive(void (*function)(int)) {
    (void)function;
}

void TwoWire::onRequest(void (*function)(void)) {
    (void)function;
}

int TwoWire::available(void) {
    return static_cast<int>(rx_length_ - rx_index_);
}

int TwoWire::read(void) {
    if (rx_index_ >= rx_length_) {
        return -1;
    }
    return rx_buffer_[rx_index_++];
}

int TwoWire::peek(void) {
    if (rx_index_ >= rx_length_) {
        return -1;
    }
    return rx_buffer_[rx_index_];
}

void TwoWire::flush(void) {
}

size_t TwoWire::write(uint8_t value) {
    return write(&value, 1U);
}

size_t TwoWire::write(const uint8_t *buffer, size_t size) {
    if ((buffer == nullptr) || (size == 0U) || !transmitting_) {
        return 0U;
    }

    const size_t available = kBufferLength - tx_length_;
    const size_t copied = (size < available) ? size : available;
    if (copied > 0U) {
        memcpy(tx_buffer_ + tx_length_, buffer, copied);
        tx_length_ += copied;
    }
    return copied;
}

bool TwoWire::ensureInitialized(void) {
    if (initialized_) {
        return true;
    }
    return begin();
}

int TwoWire::speedCode(void) const {
    if (clock_hz_ <= 100000UL) {
        return I2C_SPEED_SLOW;
    }
    if (clock_hz_ <= 400000UL) {
        return I2C_SPEED_FAST;
    }
    return I2C_SPEED_PLUS;
}

uint8_t TwoWire::mapTransferResult(int result) const {
    return (result == 0) ? 0U : 4U;
}

void TwoWire::resetTxBuffer(void) {
    tx_length_ = 0U;
}

void TwoWire::resetRxBuffer(void) {
    rx_length_ = 0U;
    rx_index_ = 0U;
}
