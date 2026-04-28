#include "Preferences.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern "C" {
#include "osanvm.h"
}

namespace {

const uint32_t kPrefsMagic = 0x31505241UL;  // "ARP1", little-endian.
const uint16_t kPrefsVersion = 1U;
const size_t kPrefsCapacity = 4096U;
const size_t kPrefsMaxKeyLength = 31U;
const uint8_t kPrefsTypeMarker = 0x80U;

struct PreferencesHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t used;
};

struct PreferencesEntryHeader {
    uint8_t keyLength;
    uint8_t reserved;
    uint16_t dataLength;
};

uint8_t encodePreferenceType(PreferenceType type)
{
    if (type >= PT_INVALID)
    {
        return (uint8_t)(kPrefsTypeMarker | PT_INVALID);
    }

    return (uint8_t)(kPrefsTypeMarker | (uint8_t)type);
}

PreferenceType decodePreferenceType(uint8_t stored)
{
    uint8_t type = 0U;

    if ((stored & kPrefsTypeMarker) == 0U)
    {
        return PT_BLOB;
    }

    type = (uint8_t)(stored & (uint8_t)~kPrefsTypeMarker);
    return (type < PT_INVALID) ? (PreferenceType)type : PT_INVALID;
}

}  // namespace

Preferences::Preferences()
    : file_name_(),
      buffer_(NULL),
      capacity_(0U),
      used_(0U),
      opened_(false),
      read_only_(false)
{
}

Preferences::~Preferences()
{
    end();
}

bool Preferences::begin(const char *name, bool readOnly, const char *partition_label)
{
    (void)partition_label;

    if ((name == NULL) || (name[0] == '\0'))
    {
        return false;
    }

    if (snprintf(file_name_, sizeof(file_name_), "ard_%s", name) >= (int)sizeof(file_name_))
    {
        file_name_[0] = '\0';
        return false;
    }

    read_only_ = readOnly;
    opened_ = load();
    return opened_;
}

void Preferences::end(void)
{
    if (buffer_ != NULL)
    {
        free(buffer_);
        buffer_ = NULL;
    }

    capacity_ = 0U;
    used_ = 0U;
    opened_ = false;
    read_only_ = false;
    file_name_[0] = '\0';
}

bool Preferences::clear(void)
{
    if (!opened_ || read_only_)
    {
        return false;
    }

    if (!resetBuffer())
    {
        return false;
    }

    return writeBack();
}

bool Preferences::remove(const char *key)
{
    EntryInfo info = {0U, 0U, 0U, 0U};

    if (!opened_ || read_only_ || !findEntry(key, &info))
    {
        return false;
    }

    if (!eraseEntry(info))
    {
        return false;
    }

    return writeBack();
}

bool Preferences::isKey(const char *key) const
{
    return findEntry(key, NULL);
}

size_t Preferences::putBytes(const char *key, const void *value, size_t len)
{
    return putTypedBytes(key, value, len, PT_BLOB);
}

PreferenceType Preferences::getType(const char *key) const
{
    EntryInfo info = {0U, 0U, 0U, 0U};
    PreferencesEntryHeader entryHeader = {0U, 0U, 0U};

    if (!findEntry(key, &info))
    {
        return PT_INVALID;
    }

    memcpy(&entryHeader, buffer_ + info.entryOffset, sizeof(entryHeader));
    return decodePreferenceType(entryHeader.reserved);
}

size_t Preferences::freeEntries(void) const
{
    const size_t minimumEntrySize = sizeof(PreferencesEntryHeader) + 2U;

    if (!opened_ || (capacity_ <= used_))
    {
        return 0U;
    }

    return (capacity_ - used_) / minimumEntrySize;
}

size_t Preferences::putTypedBytes(const char *key, const void *value, size_t len, PreferenceType type)
{
    EntryInfo info = {0U, 0U, 0U, 0U};
    size_t keyLength = 0U;
    size_t entrySize = 0U;
    PreferencesHeader *header = NULL;
    PreferencesEntryHeader entryHeader = {0U, 0U, 0U};

    if (!opened_ || read_only_ || (value == NULL) || !validKey(key) || (len > 0xFFFFU))
    {
        return 0U;
    }

    keyLength = strlen(key);
    entrySize = sizeof(PreferencesEntryHeader) + keyLength + len;
    if (findEntry(key, &info))
    {
        if ((used_ - info.entrySize + entrySize) > capacity_)
        {
            return 0U;
        }
        (void)eraseEntry(info);
    }
    else if ((used_ + entrySize) > capacity_)
    {
        return 0U;
    }

    entryHeader.keyLength = (uint8_t)keyLength;
    entryHeader.reserved = encodePreferenceType(type);
    entryHeader.dataLength = (uint16_t)len;
    memcpy(buffer_ + used_, &entryHeader, sizeof(entryHeader));
    memcpy(buffer_ + used_ + sizeof(entryHeader), key, keyLength);
    if (len > 0U)
    {
        memcpy(buffer_ + used_ + sizeof(entryHeader) + keyLength, value, len);
    }

    used_ += entrySize;
    header = (PreferencesHeader *)buffer_;
    header->used = (uint16_t)used_;

    if (!writeBack())
    {
        return 0U;
    }

    return len;
}

size_t Preferences::getBytes(const char *key, void *buf, size_t maxLen) const
{
    EntryInfo info = {0U, 0U, 0U, 0U};
    size_t copyLength = 0U;

    if (!opened_ || (buf == NULL) || !findEntry(key, &info))
    {
        return 0U;
    }

    copyLength = (info.dataLength < maxLen) ? info.dataLength : maxLen;
    if (copyLength > 0U)
    {
        memcpy(buf, buffer_ + info.dataOffset, copyLength);
    }

    return copyLength;
}

size_t Preferences::getBytesLength(const char *key) const
{
    EntryInfo info = {0U, 0U, 0U, 0U};
    return findEntry(key, &info) ? info.dataLength : 0U;
}

size_t Preferences::putBool(const char *key, bool value) { uint8_t stored = value ? 1U : 0U; return putValue(key, stored, PT_U8); }
bool Preferences::getBool(const char *key, bool defaultValue) const { return getValue(key, defaultValue); }
size_t Preferences::putChar(const char *key, int8_t value) { return putValue(key, value, PT_I8); }
int8_t Preferences::getChar(const char *key, int8_t defaultValue) const { return getValue(key, defaultValue); }
size_t Preferences::putUChar(const char *key, uint8_t value) { return putValue(key, value, PT_U8); }
uint8_t Preferences::getUChar(const char *key, uint8_t defaultValue) const { return getValue(key, defaultValue); }
size_t Preferences::putShort(const char *key, int16_t value) { return putValue(key, value, PT_I16); }
int16_t Preferences::getShort(const char *key, int16_t defaultValue) const { return getValue(key, defaultValue); }
size_t Preferences::putUShort(const char *key, uint16_t value) { return putValue(key, value, PT_U16); }
uint16_t Preferences::getUShort(const char *key, uint16_t defaultValue) const { return getValue(key, defaultValue); }
size_t Preferences::putInt(const char *key, int32_t value) { return putValue(key, value, PT_I32); }
int32_t Preferences::getInt(const char *key, int32_t defaultValue) const { return getValue(key, defaultValue); }
size_t Preferences::putUInt(const char *key, uint32_t value) { return putValue(key, value, PT_U32); }
uint32_t Preferences::getUInt(const char *key, uint32_t defaultValue) const { return getValue(key, defaultValue); }
size_t Preferences::putLong(const char *key, int32_t value) { return putValue(key, value, PT_I32); }
int32_t Preferences::getLong(const char *key, int32_t defaultValue) const { return getValue(key, defaultValue); }
size_t Preferences::putULong(const char *key, uint32_t value) { return putValue(key, value, PT_U32); }
uint32_t Preferences::getULong(const char *key, uint32_t defaultValue) const { return getValue(key, defaultValue); }
size_t Preferences::putLong64(const char *key, int64_t value) { return putValue(key, value, PT_I64); }
int64_t Preferences::getLong64(const char *key, int64_t defaultValue) const { return getValue(key, defaultValue); }
size_t Preferences::putULong64(const char *key, uint64_t value) { return putValue(key, value, PT_U64); }
uint64_t Preferences::getULong64(const char *key, uint64_t defaultValue) const { return getValue(key, defaultValue); }
size_t Preferences::putFloat(const char *key, float value) { return putValue(key, value, PT_BLOB); }
float Preferences::getFloat(const char *key, float defaultValue) const { return getValue(key, defaultValue); }
size_t Preferences::putDouble(const char *key, double value) { return putValue(key, value, PT_BLOB); }
double Preferences::getDouble(const char *key, double defaultValue) const { return getValue(key, defaultValue); }

size_t Preferences::putString(const char *key, const char *value)
{
    if (value == NULL)
    {
        value = "";
    }

    return putTypedBytes(key, value, strlen(value) + 1U, PT_STR);
}

size_t Preferences::putString(const char *key, const String &value)
{
    return putString(key, value.c_str());
}

String Preferences::getString(const char *key, const char *defaultValue) const
{
    size_t length = getBytesLength(key);
    char *value = NULL;
    String result(defaultValue);

    if (length == 0U)
    {
        return result;
    }

    value = (char *)malloc(length + 1U);
    if (value == NULL)
    {
        return result;
    }

    memset(value, 0, length + 1U);
    if (getBytes(key, value, length) > 0U)
    {
        value[length] = '\0';
        result = value;
    }

    free(value);
    return result;
}

String Preferences::getString(const char *key, const String &defaultValue) const
{
    return getString(key, defaultValue.c_str());
}

size_t Preferences::getString(const char *key, char *value, size_t maxLen) const
{
    size_t copied = 0U;

    if ((value == NULL) || (maxLen == 0U))
    {
        return 0U;
    }

    copied = getBytes(key, value, maxLen - 1U);
    value[copied] = '\0';
    return copied;
}

bool Preferences::load(void)
{
    OsaNvmBodyInfo bodyInfo = {0};
    uint8_t version = 0U;
    bool loaded = false;

    if (buffer_ != NULL)
    {
        free(buffer_);
        buffer_ = NULL;
    }

    capacity_ = kPrefsCapacity;
    buffer_ = (uint8_t *)malloc(capacity_);
    if (buffer_ == NULL)
    {
        capacity_ = 0U;
        return false;
    }

    if (OsaNvmRead(file_name_, &version, &bodyInfo, 0U) == OSA_NVM_SUCC)
    {
        if ((bodyInfo.bodySize >= sizeof(PreferencesHeader)) && (bodyInfo.bodySize <= capacity_))
        {
            const PreferencesHeader *header = (const PreferencesHeader *)bodyInfo.pBuf;
            if ((header->magic == kPrefsMagic) && (header->version == kPrefsVersion) && (header->used <= bodyInfo.bodySize))
            {
                memcpy(buffer_, bodyInfo.pBuf, header->used);
                used_ = header->used;
                loaded = true;
            }
        }
        OsaNvmFreeBody(&bodyInfo);
    }

    if (!loaded)
    {
        return resetBuffer();
    }

    return true;
}

bool Preferences::writeBack(void)
{
    if (!opened_ || read_only_ || (buffer_ == NULL) || (used_ < sizeof(PreferencesHeader)))
    {
        return false;
    }

    return (OsaNvmWrite(file_name_, (UINT8)kPrefsVersion, buffer_, (UINT32)used_) == OSA_NVM_SUCC);
}

bool Preferences::resetBuffer(void)
{
    PreferencesHeader header = {kPrefsMagic, kPrefsVersion, (uint16_t)sizeof(PreferencesHeader)};

    if (buffer_ == NULL)
    {
        return false;
    }

    memset(buffer_, 0, capacity_);
    memcpy(buffer_, &header, sizeof(header));
    used_ = sizeof(header);
    return true;
}

bool Preferences::validKey(const char *key) const
{
    size_t keyLength = 0U;

    if (key == NULL)
    {
        return false;
    }

    keyLength = strlen(key);
    return (keyLength > 0U) && (keyLength <= kPrefsMaxKeyLength);
}

bool Preferences::findEntry(const char *key, EntryInfo *info) const
{
    size_t offset = sizeof(PreferencesHeader);
    size_t keyLength = 0U;

    if (!opened_ || !validKey(key) || (buffer_ == NULL) || (used_ < sizeof(PreferencesHeader)))
    {
        return false;
    }

    keyLength = strlen(key);
    while ((offset + sizeof(PreferencesEntryHeader)) <= used_)
    {
        PreferencesEntryHeader entryHeader;
        size_t entrySize = 0U;
        size_t entryKeyOffset = offset + sizeof(PreferencesEntryHeader);
        size_t entryDataOffset = 0U;

        memcpy(&entryHeader, buffer_ + offset, sizeof(entryHeader));
        entryDataOffset = entryKeyOffset + entryHeader.keyLength;
        entrySize = sizeof(PreferencesEntryHeader) + entryHeader.keyLength + entryHeader.dataLength;
        if ((entryHeader.keyLength == 0U) || ((offset + entrySize) > used_))
        {
            return false;
        }

        if ((entryHeader.keyLength == keyLength) && (memcmp(buffer_ + entryKeyOffset, key, keyLength) == 0))
        {
            if (info != NULL)
            {
                info->entryOffset = offset;
                info->entrySize = entrySize;
                info->dataOffset = entryDataOffset;
                info->dataLength = entryHeader.dataLength;
            }
            return true;
        }

        offset += entrySize;
    }

    return false;
}

bool Preferences::eraseEntry(const EntryInfo &info)
{
    PreferencesHeader *header = NULL;
    size_t tailOffset = info.entryOffset + info.entrySize;
    size_t tailLength = 0U;

    if ((buffer_ == NULL) || (info.entryOffset < sizeof(PreferencesHeader)) || (tailOffset > used_))
    {
        return false;
    }

    tailLength = used_ - tailOffset;
    memmove(buffer_ + info.entryOffset, buffer_ + tailOffset, tailLength);
    used_ -= info.entrySize;
    header = (PreferencesHeader *)buffer_;
    header->used = (uint16_t)used_;
    return true;
}
