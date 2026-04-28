#include "IPAddress.h"

#include "Print.h"

#include <stdio.h>
#include <string.h>

IPAddress::IPAddress()
{
    memset(bytes_, 0, sizeof(bytes_));
}

IPAddress::IPAddress(uint8_t first, uint8_t second, uint8_t third, uint8_t fourth)
{
    bytes_[0] = first;
    bytes_[1] = second;
    bytes_[2] = third;
    bytes_[3] = fourth;
}

IPAddress::IPAddress(uint32_t address)
{
    memcpy(bytes_, &address, sizeof(bytes_));
}

IPAddress::IPAddress(const uint8_t *address)
{
    *this = address;
}

IPAddress::IPAddress(const char *address)
{
    if (!fromString(address))
    {
        memset(bytes_, 0, sizeof(bytes_));
    }
}

IPAddress::IPAddress(const IPAddress &address)
{
    *this = address;
}

bool IPAddress::fromString(const char *address)
{
    uint16_t parts[4] = {0U, 0U, 0U, 0U};
    uint8_t partIndex = 0U;
    bool sawDigit = false;

    if (address == NULL)
    {
        return false;
    }

    for (const char *cursor = address;; cursor++)
    {
        const char ch = *cursor;

        if ((ch >= '0') && (ch <= '9'))
        {
            sawDigit = true;
            parts[partIndex] = static_cast<uint16_t>((parts[partIndex] * 10U) + static_cast<uint16_t>(ch - '0'));
            if (parts[partIndex] > 255U)
            {
                return false;
            }
        }
        else if (ch == '.')
        {
            if ((!sawDigit) || (partIndex >= 3U))
            {
                return false;
            }
            partIndex++;
            sawDigit = false;
        }
        else if (ch == '\0')
        {
            if ((!sawDigit) || (partIndex != 3U))
            {
                return false;
            }
            break;
        }
        else
        {
            return false;
        }
    }

    for (uint8_t index = 0U; index < 4U; index++)
    {
        bytes_[index] = static_cast<uint8_t>(parts[index]);
    }

    return true;
}

bool IPAddress::fromString(const String &address)
{
    return fromString(address.c_str());
}

String IPAddress::toString() const
{
    char buffer[16];

    snprintf(buffer,
             sizeof(buffer),
             "%u.%u.%u.%u",
             static_cast<unsigned int>(bytes_[0]),
             static_cast<unsigned int>(bytes_[1]),
             static_cast<unsigned int>(bytes_[2]),
             static_cast<unsigned int>(bytes_[3]));
    return String(buffer);
}

size_t IPAddress::printTo(Print &print) const
{
    return print.print(toString());
}

const uint8_t *IPAddress::raw_address() const
{
    return bytes_;
}

uint8_t *IPAddress::raw_address()
{
    return bytes_;
}

uint8_t IPAddress::operator[](int index) const
{
    return bytes_[index & 3];
}

uint8_t &IPAddress::operator[](int index)
{
    return bytes_[index & 3];
}

IPAddress &IPAddress::operator=(const uint8_t *address)
{
    if (address == NULL)
    {
        memset(bytes_, 0, sizeof(bytes_));
    }
    else
    {
        memcpy(bytes_, address, sizeof(bytes_));
    }

    return *this;
}

IPAddress &IPAddress::operator=(uint32_t address)
{
    memcpy(bytes_, &address, sizeof(bytes_));
    return *this;
}

IPAddress &IPAddress::operator=(const char *address)
{
    if (!fromString(address))
    {
        memset(bytes_, 0, sizeof(bytes_));
    }

    return *this;
}

IPAddress &IPAddress::operator=(const IPAddress &address)
{
    if (this != &address)
    {
        memcpy(bytes_, address.bytes_, sizeof(bytes_));
    }

    return *this;
}

bool IPAddress::operator==(const IPAddress &address) const
{
    return memcmp(bytes_, address.bytes_, sizeof(bytes_)) == 0;
}

bool IPAddress::operator==(const uint8_t *address) const
{
    return (address != NULL) && (memcmp(bytes_, address, sizeof(bytes_)) == 0);
}

bool IPAddress::operator!=(const IPAddress &address) const
{
    return !(*this == address);
}

bool IPAddress::operator!=(const uint8_t *address) const
{
    return !(*this == address);
}

IPAddress::operator uint32_t() const
{
    uint32_t address = 0U;

    memcpy(&address, bytes_, sizeof(address));
    return address;
}
