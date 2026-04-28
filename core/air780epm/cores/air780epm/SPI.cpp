#include "SPI.h"

#include <string.h>

extern "C" {
#include "luat_spi.h"
}

SPIClass SPI(0U);
SPIClass SPI1(1U);

SPISettings::SPISettings(uint32_t clock, uint8_t bit_order, uint8_t data_mode)
    : _clock(clock),
      _bitOrder(bit_order),
      _dataMode(data_mode) {
}

SPIClass::SPIClass(uint8_t bus_id)
    : bus_id_(bus_id),
      initialized_(false),
      hardware_cs_(false),
      active_settings_(),
      sck_pin_(SCK),
      miso_pin_(MISO),
      mosi_pin_(MOSI),
      ss_pin_(SS) {
}

void SPIClass::begin(void) {
    (void)ensureInitialized();
}

void SPIClass::begin(int8_t sck, int8_t miso, int8_t mosi, int8_t ss) {
    if (sck >= 0) {
        sck_pin_ = sck;
    }
    if (miso >= 0) {
        miso_pin_ = miso;
    }
    if (mosi >= 0) {
        mosi_pin_ = mosi;
    }
    if (ss >= 0) {
        ss_pin_ = ss;
    }
    begin();
}

void SPIClass::end(void) {
    if (initialized_) {
        (void)luat_spi_close(bus_id_);
    }
    initialized_ = false;
}

void SPIClass::setHwCs(bool use) {
    hardware_cs_ = use;
    if (initialized_) {
        (void)applySettings(active_settings_);
    }
}

void SPIClass::beginTransaction(SPISettings settings) {
    if (ensureInitialized()) {
        (void)applySettings(settings);
    }
}

void SPIClass::endTransaction(void) {
}

void SPIClass::transfer(void *data, uint32_t size) {
    transfer(reinterpret_cast<uint8_t *>(data), static_cast<size_t>(size));
}

uint8_t SPIClass::transfer(uint8_t data) {
    uint8_t received = 0U;
    if (!ensureInitialized()) {
        return 0U;
    }

    const int count = luat_spi_transfer(
        bus_id_,
        reinterpret_cast<const char *>(&data),
        1U,
        reinterpret_cast<char *>(&received),
        1U);
    return (count == 1) ? received : 0U;
}

uint16_t SPIClass::transfer16(uint16_t data) {
    uint16_t received = 0U;
    if (active_settings_._bitOrder == LSBFIRST) {
        received = transfer(static_cast<uint8_t>(data & 0xFFU));
        received |= static_cast<uint16_t>(transfer(static_cast<uint8_t>(data >> 8U))) << 8U;
        return received;
    }

    received = static_cast<uint16_t>(transfer(static_cast<uint8_t>(data >> 8U))) << 8U;
    received |= transfer(static_cast<uint8_t>(data & 0xFFU));
    return received;
}

uint32_t SPIClass::transfer32(uint32_t data) {
    uint32_t received = 0U;
    if (active_settings_._bitOrder == LSBFIRST) {
        received = transfer(static_cast<uint8_t>(data & 0xFFU));
        received |= static_cast<uint32_t>(transfer(static_cast<uint8_t>((data >> 8U) & 0xFFU))) << 8U;
        received |= static_cast<uint32_t>(transfer(static_cast<uint8_t>((data >> 16U) & 0xFFU))) << 16U;
        received |= static_cast<uint32_t>(transfer(static_cast<uint8_t>((data >> 24U) & 0xFFU))) << 24U;
        return received;
    }

    received = static_cast<uint32_t>(transfer(static_cast<uint8_t>((data >> 24U) & 0xFFU))) << 24U;
    received |= static_cast<uint32_t>(transfer(static_cast<uint8_t>((data >> 16U) & 0xFFU))) << 16U;
    received |= static_cast<uint32_t>(transfer(static_cast<uint8_t>((data >> 8U) & 0xFFU))) << 8U;
    received |= transfer(static_cast<uint8_t>(data & 0xFFU));
    return received;
}

void SPIClass::transfer(uint8_t *buffer, size_t len) {
    if ((buffer == nullptr) || (len == 0U) || !ensureInitialized()) {
        return;
    }

    const int count = luat_spi_transfer(
        bus_id_,
        reinterpret_cast<const char *>(buffer),
        len,
        reinterpret_cast<char *>(buffer),
        len);
    if (count < 0) {
        return;
    }
}

void SPIClass::transfer(const uint8_t *tx_buffer, uint8_t *rx_buffer, size_t len) {
    if ((len == 0U) || !ensureInitialized()) {
        return;
    }
    if (rx_buffer == nullptr) {
        if (tx_buffer != nullptr) {
            (void)luat_spi_send(bus_id_, reinterpret_cast<const char *>(tx_buffer), len);
        }
        return;
    }

    if (tx_buffer == nullptr) {
        memset(rx_buffer, 0xFF, len);
        (void)luat_spi_transfer(
            bus_id_,
            reinterpret_cast<const char *>(rx_buffer),
            len,
            reinterpret_cast<char *>(rx_buffer),
            len);
        return;
    }

    (void)luat_spi_transfer(
        bus_id_,
        reinterpret_cast<const char *>(tx_buffer),
        len,
        reinterpret_cast<char *>(rx_buffer),
        len);
}

void SPIClass::transferBytes(uint8_t *tx_buffer, uint8_t *rx_buffer, size_t len) {
    transfer(static_cast<const uint8_t *>(tx_buffer), rx_buffer, len);
}

void SPIClass::transferBytes(const uint8_t *tx_buffer, uint8_t *rx_buffer, size_t len) {
    transfer(tx_buffer, rx_buffer, len);
}

void SPIClass::write(uint8_t data) {
    if (ensureInitialized()) {
        (void)luat_spi_send(bus_id_, reinterpret_cast<const char *>(&data), 1U);
    }
}

void SPIClass::write16(uint16_t data) {
    writeByteOrdered(data, 2U);
}

void SPIClass::write32(uint32_t data) {
    writeByteOrdered(data, 4U);
}

void SPIClass::writeBytes(uint8_t *buffer, size_t len) {
    writeBytes(static_cast<const uint8_t *>(buffer), len);
}

void SPIClass::writeBytes(const uint8_t *buffer, size_t len) {
    if ((buffer != nullptr) && (len != 0U) && ensureInitialized()) {
        (void)luat_spi_send(bus_id_, reinterpret_cast<const char *>(buffer), len);
    }
}

void SPIClass::writePixels(const void *buffer, uint32_t size) {
    writeBytes(static_cast<const uint8_t *>(buffer), static_cast<size_t>(size));
}

void SPIClass::writePattern(const uint8_t *data, uint8_t size, uint32_t repeat) {
    if ((data == nullptr) || (size == 0U)) {
        return;
    }
    for (uint32_t i = 0U; i < repeat; ++i) {
        writeBytes(data, size);
    }
}

void SPIClass::setBitOrder(uint8_t bit_order) {
    active_settings_._bitOrder = bit_order;
    if (initialized_) {
        (void)applySettings(active_settings_);
    }
}

void SPIClass::setDataMode(uint8_t data_mode) {
    active_settings_._dataMode = data_mode;
    if (initialized_) {
        (void)applySettings(active_settings_);
    }
}

void SPIClass::setClockDivider(uint8_t divider) {
    if (divider == 0U) {
        return;
    }
    setFrequency(26000000UL / divider);
}

uint32_t SPIClass::getClockDivider(void) const {
    return active_settings_._clock;
}

void SPIClass::setFrequency(uint32_t frequency) {
    active_settings_._clock = normalizedClock(frequency);
    if (initialized_) {
        (void)applySettings(active_settings_);
    }
}

void SPIClass::setClock(uint32_t frequency) {
    setFrequency(frequency);
}

int8_t SPIClass::pinSS(void) const {
    return ss_pin_;
}

bool SPIClass::ensureInitialized(void) {
    if (initialized_) {
        return true;
    }
    return applySettings(active_settings_);
}

bool SPIClass::applySettings(const SPISettings &settings) {
    luat_spi_t config;
    memset(&config, 0, sizeof(config));

    active_settings_ = settings;
    active_settings_._clock = normalizedClock(active_settings_._clock);

    config.id = bus_id_;
    config.CPHA = modeCpha();
    config.CPOL = modeCpol();
    config.dataw = 8;
    config.bit_dict = (active_settings_._bitOrder == LSBFIRST) ? 0 : 1;
    config.master = 1;
    config.mode = 1;
    config.bandrate = static_cast<int>(active_settings_._clock);
    config.cs = hardware_cs_ ? ss_pin_ : -1;

    initialized_ = (luat_spi_setup(&config) == 0);
    return initialized_;
}

uint32_t SPIClass::normalizedClock(uint32_t clock) const {
    if (clock < 100000UL) {
        return 100000UL;
    }
    if (clock > 26000000UL) {
        return 26000000UL;
    }
    return clock;
}

int SPIClass::modeCpha(void) const {
    return (active_settings_._dataMode & 0x01U) ? 1 : 0;
}

int SPIClass::modeCpol(void) const {
    return (active_settings_._dataMode & 0x02U) ? 1 : 0;
}

void SPIClass::writeByteOrdered(uint32_t data, uint8_t bytes) {
    if (active_settings_._bitOrder == LSBFIRST) {
        for (uint8_t i = 0U; i < bytes; ++i) {
            write(static_cast<uint8_t>((data >> (8U * i)) & 0xFFU));
        }
        return;
    }

    for (uint8_t i = bytes; i > 0U; --i) {
        write(static_cast<uint8_t>((data >> (8U * (i - 1U))) & 0xFFU));
    }
}
