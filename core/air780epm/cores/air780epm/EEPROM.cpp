#include "EEPROM.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "arduino_nvm_io.h"
}

namespace {

const char *kEepromFileName = "arduino_eeprom";
const char *kEepromFilePrefix = "eep_";
const uint32_t kEepromMagic = 0x31504545UL;  // "EEP1", little-endian.
const uint16_t kEepromVersion = 1U;
const size_t kEepromMaxSize = 4096U;
uint8_t gInvalidEepromByte = 0xFFU;

struct EepromHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t size;
};

}  // namespace

EEPROMClass EEPROM;

EEPROMClass::EEPROMClass()
    : file_name_(),
      data_(NULL),
      size_(0U),
      dirty_(false)
{
    setFileName(NULL);
}

EEPROMClass::EEPROMClass(uint32_t sector)
    : file_name_(),
      data_(NULL),
      size_(0U),
      dirty_(false)
{
    (void)sector;
    setFileName(NULL);
}

EEPROMClass::EEPROMClass(const char *name)
    : file_name_(),
      data_(NULL),
      size_(0U),
      dirty_(false)
{
    setFileName(name);
}

EEPROMClass::~EEPROMClass()
{
    end();
}

bool EEPROMClass::begin(size_t size)
{
    uint8_t *body = NULL;
    size_t bodySize = 0U;
    uint8_t version = 0U;

    if ((size == 0U) || (size > kEepromMaxSize))
    {
        return false;
    }

    end();
    data_ = (uint8_t *)malloc(size);
    if (data_ == NULL)
    {
        return false;
    }

    memset(data_, 0xFF, size);
    size_ = size;
    dirty_ = false;

    if (arduinoCoreNvmRead(file_name_, &version, &body, &bodySize) == 0)
    {
        if (bodySize >= sizeof(EepromHeader))
        {
            const EepromHeader *header = (const EepromHeader *)body;
            if ((header->magic == kEepromMagic) && (header->version == kEepromVersion))
            {
                size_t storedSize = header->size;
                size_t copySize = (storedSize < size_) ? storedSize : size_;
                if ((sizeof(EepromHeader) + copySize) <= bodySize)
                {
                    memcpy(data_, body + sizeof(EepromHeader), copySize);
                }
            }
        }
        free(body);
    }

    return true;
}

bool EEPROMClass::commit(void)
{
    EepromHeader header = {kEepromMagic, kEepromVersion, (uint16_t)size_};
    uint8_t *body = NULL;
    size_t bodySize = 0U;
    bool ok = false;

    if (data_ == NULL)
    {
        return false;
    }

    bodySize = sizeof(EepromHeader) + size_;
    body = (uint8_t *)malloc(bodySize);
    if (body == NULL)
    {
        return false;
    }

    memcpy(body, &header, sizeof(header));
    memcpy(body + sizeof(header), data_, size_);
    ok = (arduinoCoreNvmWrite(file_name_, (uint8_t)kEepromVersion, body, bodySize) == 0);
    free(body);

    if (ok)
    {
        dirty_ = false;
    }

    return ok;
}

void EEPROMClass::end(void)
{
    if ((data_ != NULL) && dirty_)
    {
        (void)commit();
    }

    if (data_ != NULL)
    {
        free(data_);
        data_ = NULL;
    }

    size_ = 0U;
    dirty_ = false;
}

uint8_t EEPROMClass::read(int address) const
{
    if (!validAddress(address))
    {
        return 0xFFU;
    }

    return data_[address];
}

void EEPROMClass::write(int address, uint8_t value)
{
    if (!validAddress(address))
    {
        return;
    }

    if (data_[address] != value)
    {
        data_[address] = value;
        dirty_ = true;
    }
}

void EEPROMClass::update(int address, uint8_t value)
{
    if (!validAddress(address) || (data_[address] == value))
    {
        return;
    }

    data_[address] = value;
    dirty_ = true;
}

size_t EEPROMClass::length(void) const
{
    return size_;
}

bool EEPROMClass::isDirty(void) const
{
    return dirty_;
}

uint8_t *EEPROMClass::getDataPtr(void)
{
    if (data_ == NULL)
    {
        return NULL;
    }

    dirty_ = true;
    return data_;
}

uint8_t &EEPROMClass::operator[](int address)
{
    if (!validAddress(address))
    {
        return gInvalidEepromByte;
    }

    dirty_ = true;
    return data_[address];
}

uint8_t EEPROMClass::readByte(int address) const { return readValue<uint8_t>(address); }
int8_t EEPROMClass::readChar(int address) const { return readValue<int8_t>(address); }
uint8_t EEPROMClass::readUChar(int address) const { return readValue<uint8_t>(address); }
int16_t EEPROMClass::readShort(int address) const { return readValue<int16_t>(address); }
uint16_t EEPROMClass::readUShort(int address) const { return readValue<uint16_t>(address); }
int32_t EEPROMClass::readInt(int address) const { return readValue<int32_t>(address); }
uint32_t EEPROMClass::readUInt(int address) const { return readValue<uint32_t>(address); }
int32_t EEPROMClass::readLong(int address) const { return readValue<int32_t>(address); }
uint32_t EEPROMClass::readULong(int address) const { return readValue<uint32_t>(address); }
int64_t EEPROMClass::readLong64(int address) const { return readValue<int64_t>(address); }
uint64_t EEPROMClass::readULong64(int address) const { return readValue<uint64_t>(address); }
float EEPROMClass::readFloat(int address) const { return readValue<float>(address); }
double EEPROMClass::readDouble(int address) const { return readValue<double>(address); }
bool EEPROMClass::readBool(int address) const { return readValue<int8_t>(address) != 0; }

size_t EEPROMClass::readString(int address, char *value, size_t maxLen) const
{
    size_t len = 0U;

    if ((value == NULL) || (maxLen == 0U) || !validAddress(address))
    {
        return 0U;
    }

    while (((address + (int)len) < (int)size_) && (data_[address + len] != '\0'))
    {
        len++;
    }

    if (((address + (int)len) >= (int)size_) || (len >= maxLen))
    {
        return 0U;
    }

    memcpy(value, data_ + address, len);
    value[len] = '\0';
    return len;
}

String EEPROMClass::readString(int address) const
{
    size_t len = 0U;
    char *value = NULL;
    String result;

    if (!validAddress(address))
    {
        return result;
    }

    while (((address + (int)len) < (int)size_) && (data_[address + len] != '\0'))
    {
        len++;
    }

    if ((address + (int)len) >= (int)size_)
    {
        return result;
    }

    value = (char *)malloc(len + 1U);
    if (value == NULL)
    {
        return result;
    }

    memcpy(value, data_ + address, len);
    value[len] = '\0';
    result = value;
    free(value);
    return result;
}

size_t EEPROMClass::readBytes(int address, void *value, size_t maxLen) const
{
    if ((value == NULL) || (maxLen == 0U) || !validRange(address, maxLen))
    {
        return 0U;
    }

    memcpy(value, data_ + address, maxLen);
    return maxLen;
}

size_t EEPROMClass::writeByte(int address, uint8_t value) { return writeValue(address, value); }
size_t EEPROMClass::writeChar(int address, int8_t value) { return writeValue(address, value); }
size_t EEPROMClass::writeUChar(int address, uint8_t value) { return writeValue(address, value); }
size_t EEPROMClass::writeShort(int address, int16_t value) { return writeValue(address, value); }
size_t EEPROMClass::writeUShort(int address, uint16_t value) { return writeValue(address, value); }
size_t EEPROMClass::writeInt(int address, int32_t value) { return writeValue(address, value); }
size_t EEPROMClass::writeUInt(int address, uint32_t value) { return writeValue(address, value); }
size_t EEPROMClass::writeLong(int address, int32_t value) { return writeValue(address, value); }
size_t EEPROMClass::writeULong(int address, uint32_t value) { return writeValue(address, value); }
size_t EEPROMClass::writeLong64(int address, int64_t value) { return writeValue(address, value); }
size_t EEPROMClass::writeULong64(int address, uint64_t value) { return writeValue(address, value); }
size_t EEPROMClass::writeFloat(int address, float value) { return writeValue(address, value); }
size_t EEPROMClass::writeDouble(int address, double value) { return writeValue(address, value); }
size_t EEPROMClass::writeBool(int address, bool value) { int8_t stored = value ? 1 : 0; return writeValue(address, stored); }

size_t EEPROMClass::writeString(int address, const char *value)
{
    size_t len = 0U;

    if (value == NULL)
    {
        return 0U;
    }

    len = strlen(value);
    if (!validRange(address, len + 1U))
    {
        return 0U;
    }

    memcpy(data_ + address, value, len + 1U);
    dirty_ = true;
    return len;
}

size_t EEPROMClass::writeString(int address, const String &value)
{
    return writeString(address, value.c_str());
}

size_t EEPROMClass::writeBytes(int address, const void *value, size_t len)
{
    if ((value == NULL) || (len == 0U) || !validRange(address, len))
    {
        return 0U;
    }

    memcpy(data_ + address, value, len);
    dirty_ = true;
    return len;
}

void EEPROMClass::setFileName(const char *name)
{
    if ((name == NULL) || (name[0] == '\0'))
    {
        (void)snprintf(file_name_, sizeof(file_name_), "%s", kEepromFileName);
        return;
    }

    if (snprintf(file_name_, sizeof(file_name_), "%s%s", kEepromFilePrefix, name) >= (int)sizeof(file_name_))
    {
        (void)snprintf(file_name_, sizeof(file_name_), "%s", kEepromFileName);
    }
}

bool EEPROMClass::validAddress(int address) const
{
    return (data_ != NULL) && (address >= 0) && ((size_t)address < size_);
}

bool EEPROMClass::validRange(int address, size_t len) const
{
    if ((data_ == NULL) || (address < 0))
    {
        return false;
    }

    if (len == 0U)
    {
        return true;
    }

    return ((size_t)address <= size_) && (len <= (size_ - (size_t)address));
}
