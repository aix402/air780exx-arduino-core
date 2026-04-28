#ifndef IPADDRESS_H
#define IPADDRESS_H

#include <stdint.h>

#include "Printable.h"
#include "WString.h"

class IPAddress : public Printable {
public:
    IPAddress();
    IPAddress(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth);
    IPAddress(uint32_t address);
    explicit IPAddress(const uint8_t *address);
    explicit IPAddress(const char *address);
    IPAddress(const IPAddress &address);

    bool fromString(const char *address);
    bool fromString(const String &address);

    String toString() const;
    size_t printTo(Print &print) const override;
    const uint8_t *raw_address() const;
    uint8_t *raw_address();

    uint8_t operator[](int index) const;
    uint8_t &operator[](int index);

    IPAddress &operator=(const uint8_t *address);
    IPAddress &operator=(uint32_t address);
    IPAddress &operator=(const char *address);
    IPAddress &operator=(const IPAddress &address);

    bool operator==(const IPAddress &address) const;
    bool operator==(const uint8_t *address) const;
    bool operator!=(const IPAddress &address) const;
    bool operator!=(const uint8_t *address) const;
    operator uint32_t() const;

private:
    uint8_t bytes_[4];
};

#endif
