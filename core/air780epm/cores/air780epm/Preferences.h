#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <stddef.h>
#include <stdint.h>

#include "WString.h"

typedef enum {
    PT_I8,
    PT_U8,
    PT_I16,
    PT_U16,
    PT_I32,
    PT_U32,
    PT_I64,
    PT_U64,
    PT_STR,
    PT_BLOB,
    PT_INVALID
} PreferenceType;

class Preferences {
public:
    Preferences();
    ~Preferences();

    bool begin(const char *name, bool readOnly = false, const char *partition_label = NULL);
    void end(void);
    bool clear(void);
    bool remove(const char *key);
    bool isKey(const char *key) const;
    PreferenceType getType(const char *key) const;
    size_t freeEntries(void) const;

    size_t putBytes(const char *key, const void *value, size_t len);
    size_t getBytes(const char *key, void *buf, size_t maxLen) const;
    size_t getBytesLength(const char *key) const;

    size_t putBool(const char *key, bool value);
    bool getBool(const char *key, bool defaultValue = false) const;
    size_t putChar(const char *key, int8_t value);
    int8_t getChar(const char *key, int8_t defaultValue = 0) const;
    size_t putUChar(const char *key, uint8_t value);
    uint8_t getUChar(const char *key, uint8_t defaultValue = 0U) const;
    size_t putShort(const char *key, int16_t value);
    int16_t getShort(const char *key, int16_t defaultValue = 0) const;
    size_t putUShort(const char *key, uint16_t value);
    uint16_t getUShort(const char *key, uint16_t defaultValue = 0U) const;
    size_t putInt(const char *key, int32_t value);
    int32_t getInt(const char *key, int32_t defaultValue = 0) const;
    size_t putUInt(const char *key, uint32_t value);
    uint32_t getUInt(const char *key, uint32_t defaultValue = 0U) const;
    size_t putLong(const char *key, int32_t value);
    int32_t getLong(const char *key, int32_t defaultValue = 0) const;
    size_t putULong(const char *key, uint32_t value);
    uint32_t getULong(const char *key, uint32_t defaultValue = 0U) const;
    size_t putLong64(const char *key, int64_t value);
    int64_t getLong64(const char *key, int64_t defaultValue = 0) const;
    size_t putULong64(const char *key, uint64_t value);
    uint64_t getULong64(const char *key, uint64_t defaultValue = 0U) const;
    size_t putFloat(const char *key, float value);
    float getFloat(const char *key, float defaultValue = 0.0F) const;
    size_t putDouble(const char *key, double value);
    double getDouble(const char *key, double defaultValue = 0.0) const;
    size_t putString(const char *key, const char *value);
    size_t putString(const char *key, const String &value);
    String getString(const char *key, const char *defaultValue = "") const;
    String getString(const char *key, const String &defaultValue) const;
    size_t getString(const char *key, char *value, size_t maxLen) const;

private:
    struct EntryInfo {
        size_t entryOffset;
        size_t entrySize;
        size_t dataOffset;
        size_t dataLength;
    };

    bool load(void);
    bool writeBack(void);
    bool resetBuffer(void);
    bool validKey(const char *key) const;
    bool findEntry(const char *key, EntryInfo *info) const;
    bool eraseEntry(const EntryInfo &info);
    size_t putTypedBytes(const char *key, const void *value, size_t len, PreferenceType type);

    template <typename T>
    size_t putValue(const char *key, const T &value, PreferenceType type)
    {
        return putTypedBytes(key, &value, sizeof(T), type);
    }

    template <typename T>
    T getValue(const char *key, T defaultValue) const
    {
        T value = defaultValue;
        if (getBytes(key, &value, sizeof(T)) != sizeof(T)) {
            return defaultValue;
        }
        return value;
    }

    char file_name_[32];
    uint8_t *buffer_;
    size_t capacity_;
    size_t used_;
    bool opened_;
    bool read_only_;
};

#endif
