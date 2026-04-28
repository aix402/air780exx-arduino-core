#pragma once

#include <stddef.h>
#include <stdint.h>

class __FlashStringHelper;

class String {
public:
    String();
    String(const char *value);
    String(const char *value, unsigned int length);
    String(const uint8_t *value, unsigned int length);
    String(const String &value);
    String(const __FlashStringHelper *value);
    explicit String(char value);
    ~String();

    String &operator=(const String &value);
    String &operator=(const char *value);
    String &operator=(const __FlashStringHelper *value);

    const char *c_str() const;
    size_t length() const;
    bool isEmpty() const;
    void clear();
    bool reserve(size_t size);

    bool concat(const char *value);
    bool concat(const char *value, unsigned int length);
    bool concat(const uint8_t *value, unsigned int length);
    bool concat(char value);
    bool concat(const String &value);
    bool concat(const __FlashStringHelper *value);
    bool equals(const char *value) const;
    bool equals(const String &value) const;

    char charAt(size_t index) const;
    char operator[](size_t index) const;
    char &operator[](size_t index);
    operator bool() const;
    bool operator==(const char *value) const;
    bool operator==(const String &value) const;
    bool operator!=(const char *value) const;
    bool operator!=(const String &value) const;

    String &operator+=(const char *value);
    String &operator+=(char value);
    String &operator+=(const String &value);
    String &operator+=(const __FlashStringHelper *value);

private:
    bool assign(const char *value, size_t length);

    char *buffer_;
    size_t length_;
    size_t capacity_;
};

String operator+(const String &left, const String &right);
String operator+(const String &left, const char *right);
String operator+(const char *left, const String &right);
bool operator==(const char *left, const String &right);
bool operator!=(const char *left, const String &right);
