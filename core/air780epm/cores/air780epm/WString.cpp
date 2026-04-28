#include "WString.h"

#include <stdlib.h>
#include <string.h>

namespace {
char kEmptyString[] = "";

void formatUnsignedNumber(char *buffer, size_t bufferSize, unsigned long value, unsigned char base) {
    static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char reversed[33];
    size_t length = 0;

    if ((buffer == nullptr) || (bufferSize == 0U)) {
        return;
    }

    if ((base < 2U) || (base > 36U)) {
        base = 10U;
    }

    do {
        reversed[length++] = digits[value % base];
        value /= base;
    } while ((value != 0UL) && (length < sizeof(reversed)));

    size_t out = 0;
    while ((length > 0U) && ((out + 1U) < bufferSize)) {
        buffer[out++] = reversed[--length];
    }
    buffer[out] = '\0';
}

void formatSignedNumber(char *buffer, size_t bufferSize, long value, unsigned char base) {
    if ((buffer == nullptr) || (bufferSize == 0U)) {
        return;
    }

    if ((base == 10U) && (value < 0L)) {
        buffer[0] = '-';
        const unsigned long magnitude =
            static_cast<unsigned long>(-(value + 1L)) + 1UL;
        formatUnsignedNumber(buffer + 1, bufferSize - 1U, magnitude, base);
        return;
    }

    formatUnsignedNumber(buffer, bufferSize, (unsigned long)value, base);
}
}

String::String() : buffer_(nullptr), length_(0), capacity_(0) {}

String::String(const char *value) : buffer_(nullptr), length_(0), capacity_(0) {
    const char *source = (value != nullptr) ? value : "";
    (void)assign(source, strlen(source));
}

String::String(const char *value, unsigned int length) : buffer_(nullptr), length_(0), capacity_(0) {
    (void)assign((value != nullptr) ? value : "", (value != nullptr) ? length : 0U);
}

String::String(const uint8_t *value, unsigned int length) : buffer_(nullptr), length_(0), capacity_(0) {
    (void)assign(reinterpret_cast<const char *>(value), (value != nullptr) ? length : 0U);
}

String::String(const String &value) : buffer_(nullptr), length_(0), capacity_(0) {
    (void)assign(value.c_str(), value.length());
}

String::String(const __FlashStringHelper *value) : buffer_(nullptr), length_(0), capacity_(0) {
    const char *source = reinterpret_cast<const char *>(value);
    (void)assign((source != nullptr) ? source : "", (source != nullptr) ? strlen(source) : 0U);
}

String::String(char value) : buffer_(nullptr), length_(0), capacity_(0) {
    char text[2] = {value, '\0'};
    (void)assign(text, 1U);
}

String::String(int value, unsigned char base) : buffer_(nullptr), length_(0), capacity_(0) {
    char text[34] = {0};
    formatSignedNumber(text, sizeof(text), static_cast<long>(value), base);
    (void)assign(text, strlen(text));
}

String::String(unsigned int value, unsigned char base) : buffer_(nullptr), length_(0), capacity_(0) {
    char text[33] = {0};
    formatUnsignedNumber(text, sizeof(text), static_cast<unsigned long>(value), base);
    (void)assign(text, strlen(text));
}

String::String(long value, unsigned char base) : buffer_(nullptr), length_(0), capacity_(0) {
    char text[34] = {0};
    formatSignedNumber(text, sizeof(text), value, base);
    (void)assign(text, strlen(text));
}

String::String(unsigned long value, unsigned char base) : buffer_(nullptr), length_(0), capacity_(0) {
    char text[33] = {0};
    formatUnsignedNumber(text, sizeof(text), value, base);
    (void)assign(text, strlen(text));
}

String::~String() {
    free(buffer_);
}

String &String::operator=(const String &value) {
    if (this != &value) {
        (void)assign(value.c_str(), value.length());
    }
    return *this;
}

String &String::operator=(const char *value) {
    const char *source = (value != nullptr) ? value : "";
    (void)assign(source, strlen(source));
    return *this;
}

String &String::operator=(const __FlashStringHelper *value) {
    const char *source = reinterpret_cast<const char *>(value);
    (void)assign((source != nullptr) ? source : "", (source != nullptr) ? strlen(source) : 0U);
    return *this;
}

const char *String::c_str() const {
    return (buffer_ != nullptr) ? buffer_ : kEmptyString;
}

size_t String::length() const {
    return length_;
}

bool String::isEmpty() const {
    return length_ == 0U;
}

void String::clear() {
    length_ = 0U;
    if (buffer_ != nullptr) {
        buffer_[0] = '\0';
    }
}

bool String::reserve(size_t size) {
    if ((size + 1U) <= capacity_) {
        return true;
    }

    char *next = static_cast<char *>(realloc(buffer_, size + 1U));
    if (next == nullptr) {
        return false;
    }

    buffer_ = next;
    capacity_ = size + 1U;
    if (length_ == 0U) {
        buffer_[0] = '\0';
    }
    return true;
}

bool String::concat(const char *value) {
    const char *source = (value != nullptr) ? value : "";
    return concat(source, static_cast<unsigned int>(strlen(source)));
}

bool String::concat(const char *value, unsigned int length) {
    if (value == nullptr) {
        return true;
    }

    const size_t next_length = length_ + static_cast<size_t>(length);
    if (!reserve(next_length)) {
        return false;
    }

    memcpy(buffer_ + length_, value, length);
    length_ = next_length;
    buffer_[length_] = '\0';
    return true;
}

bool String::concat(const uint8_t *value, unsigned int length) {
    return concat(reinterpret_cast<const char *>(value), length);
}

bool String::concat(char value) {
    char text[2] = {value, '\0'};
    return concat(text, 1U);
}

bool String::concat(const String &value) {
    return concat(value.c_str(), static_cast<unsigned int>(value.length()));
}

bool String::concat(const __FlashStringHelper *value) {
    return concat(reinterpret_cast<const char *>(value));
}

bool String::equals(const char *value) const {
    const char *source = (value != nullptr) ? value : "";
    return strcmp(c_str(), source) == 0;
}

bool String::equals(const String &value) const {
    return equals(value.c_str());
}

char String::charAt(size_t index) const {
    return (index < length_) ? c_str()[index] : '\0';
}

char String::operator[](size_t index) const {
    return charAt(index);
}

char &String::operator[](size_t index) {
    static char dummy = '\0';
    if ((buffer_ == nullptr) || (index >= length_)) {
        dummy = '\0';
        return dummy;
    }
    return buffer_[index];
}

String::operator bool() const {
    return buffer_ != nullptr;
}

bool String::operator==(const char *value) const {
    return equals(value);
}

bool String::operator==(const String &value) const {
    return equals(value);
}

bool String::operator!=(const char *value) const {
    return !equals(value);
}

bool String::operator!=(const String &value) const {
    return !equals(value);
}

String &String::operator+=(const char *value) {
    (void)concat(value);
    return *this;
}

String &String::operator+=(char value) {
    (void)concat(value);
    return *this;
}

String &String::operator+=(const String &value) {
    (void)concat(value);
    return *this;
}

String &String::operator+=(const __FlashStringHelper *value) {
    (void)concat(value);
    return *this;
}

bool String::assign(const char *value, size_t length) {
    const char *source = (value != nullptr) ? value : "";
    const size_t source_length = (value != nullptr) ? length : 0U;

    if (!reserve(source_length)) {
        return false;
    }

    memcpy(buffer_, source, source_length);
    buffer_[source_length] = '\0';
    length_ = source_length;
    return true;
}

String operator+(const String &left, const String &right) {
    String result(left);
    result += right;
    return result;
}

String operator+(const String &left, const char *right) {
    String result(left);
    result += right;
    return result;
}

String operator+(const char *left, const String &right) {
    String result(left);
    result += right;
    return result;
}

bool operator==(const char *left, const String &right) {
    return right.equals(left);
}

bool operator!=(const char *left, const String &right) {
    return !right.equals(left);
}
