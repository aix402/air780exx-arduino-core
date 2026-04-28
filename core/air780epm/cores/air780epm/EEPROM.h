#ifndef EEPROM_H
#define EEPROM_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "WString.h"

class EEPROMClass {
public:
    EEPROMClass();
    explicit EEPROMClass(uint32_t sector);
    explicit EEPROMClass(const char *name);
    ~EEPROMClass();

    bool begin(size_t size);
    bool commit(void);
    void end(void);
    uint8_t read(int address) const;
    void write(int address, uint8_t value);
    void update(int address, uint8_t value);
    size_t length(void) const;
    bool isDirty(void) const;
    uint8_t *getDataPtr(void);

    uint8_t &operator[](int address);

    uint8_t readByte(int address) const;
    int8_t readChar(int address) const;
    uint8_t readUChar(int address) const;
    int16_t readShort(int address) const;
    uint16_t readUShort(int address) const;
    int32_t readInt(int address) const;
    uint32_t readUInt(int address) const;
    int32_t readLong(int address) const;
    uint32_t readULong(int address) const;
    int64_t readLong64(int address) const;
    uint64_t readULong64(int address) const;
    float readFloat(int address) const;
    double readDouble(int address) const;
    bool readBool(int address) const;
    size_t readString(int address, char *value, size_t maxLen) const;
    String readString(int address) const;
    size_t readBytes(int address, void *value, size_t maxLen) const;

    size_t writeByte(int address, uint8_t value);
    size_t writeChar(int address, int8_t value);
    size_t writeUChar(int address, uint8_t value);
    size_t writeShort(int address, int16_t value);
    size_t writeUShort(int address, uint16_t value);
    size_t writeInt(int address, int32_t value);
    size_t writeUInt(int address, uint32_t value);
    size_t writeLong(int address, int32_t value);
    size_t writeULong(int address, uint32_t value);
    size_t writeLong64(int address, int64_t value);
    size_t writeULong64(int address, uint64_t value);
    size_t writeFloat(int address, float value);
    size_t writeDouble(int address, double value);
    size_t writeBool(int address, bool value);
    size_t writeString(int address, const char *value);
    size_t writeString(int address, const String &value);
    size_t writeBytes(int address, const void *value, size_t len);

    template <typename T>
    T &get(int address, T &value)
    {
        if (!validRange(address, sizeof(T))) {
            return value;
        }
        memcpy(&value, data_ + address, sizeof(T));
        return value;
    }

    template <typename T>
    const T &put(int address, const T &value)
    {
        if (!validRange(address, sizeof(T))) {
            return value;
        }
        memcpy(data_ + address, &value, sizeof(T));
        dirty_ = true;
        return value;
    }

private:
    void setFileName(const char *name);
    bool validAddress(int address) const;
    bool validRange(int address, size_t len) const;

    template <typename T>
    T readValue(int address) const
    {
        T value = T();
        if (validRange(address, sizeof(T))) {
            memcpy(&value, data_ + address, sizeof(T));
        }
        return value;
    }

    template <typename T>
    size_t writeValue(int address, const T &value)
    {
        if (!validRange(address, sizeof(T))) {
            return 0U;
        }
        memcpy(data_ + address, &value, sizeof(T));
        dirty_ = true;
        return sizeof(T);
    }

    char file_name_[32];
    uint8_t *data_;
    size_t size_;
    bool dirty_;
};

extern EEPROMClass EEPROM;

#endif
