#ifndef WSTRING_H
#define WSTRING_H

#include <stddef.h>
#include <stdint.h>

class __FlashStringHelper;
class StringSumHelper;

class String {
public:
    String();
    String(const char *value);
    String(const char *value, unsigned int length);
    String(const uint8_t *value, unsigned int length);
    String(const String &value);
    String(const __FlashStringHelper *value);
    explicit String(char value);
    explicit String(unsigned char value, unsigned char base = 10);
    explicit String(int value, unsigned char base = 10);
    explicit String(unsigned int value, unsigned char base = 10);
    explicit String(long value, unsigned char base = 10);
    explicit String(unsigned long value, unsigned char base = 10);
    explicit String(float value, unsigned int decimalPlaces = 2);
    explicit String(double value, unsigned int decimalPlaces = 2);
    explicit String(long long value, unsigned char base = 10);
    explicit String(unsigned long long value, unsigned char base = 10);
    ~String();

    String &operator=(const String &value);
    String &operator=(const char *value);
    String &operator=(const __FlashStringHelper *value);

    const char *c_str() const;
    char *begin();
    char *end();
    const char *begin() const;
    const char *end() const;
    size_t length() const;
    void clear();
    bool isEmpty() const;
    bool reserve(size_t size);
    bool concat(const char *value);
    bool concat(const char *value, unsigned int length);
    bool concat(const uint8_t *value, unsigned int length);
    bool concat(char value);
    bool concat(const String &value);
    bool concat(const __FlashStringHelper *value);
    bool concat(unsigned char value);
    bool concat(int value);
    bool concat(unsigned int value);
    bool concat(long value);
    bool concat(unsigned long value);
    bool concat(float value);
    bool concat(double value);
    bool concat(long long value);
    bool concat(unsigned long long value);
    void remove(size_t index);
    void remove(size_t index, size_t count);
    int compareTo(const String &value) const;
    bool equals(const String &value) const;
    bool equals(const char *value) const;
    bool equalsIgnoreCase(const String &value) const;
    unsigned char equalsConstantTime(const String &value) const;
    bool startsWith(const String &prefix) const;
    bool startsWith(const char *prefix) const;
    bool startsWith(const __FlashStringHelper *prefix) const;
    bool startsWith(const String &prefix, size_t offset) const;
    bool endsWith(const String &suffix) const;
    bool endsWith(const char *suffix) const;
    bool endsWith(const __FlashStringHelper *suffix) const;
    int indexOf(char value) const;
    int indexOf(char value, size_t fromIndex) const;
    int indexOf(const String &value) const;
    int indexOf(const String &value, size_t fromIndex) const;
    int indexOf(const char *value) const;
    int indexOf(const char *value, size_t fromIndex) const;
    int lastIndexOf(char value) const;
    int lastIndexOf(char value, size_t fromIndex) const;
    int lastIndexOf(const String &value) const;
    int lastIndexOf(const String &value, size_t fromIndex) const;
    String substring(size_t beginIndex) const;
    String substring(size_t beginIndex, size_t endIndex) const;
    char charAt(size_t index) const;
    void setCharAt(size_t index, char value);
    void getBytes(unsigned char *buffer, unsigned int bufferSize, unsigned int index = 0) const;
    void toCharArray(char *buffer, unsigned int bufferSize, unsigned int index = 0) const;
    void replace(char find, char replace);
    void replace(const String &find, const String &replace);
    void replace(const char *find, const String &replace);
    void replace(const char *find, const char *replace);
    void replace(const __FlashStringHelper *find, const String &replace);
    void replace(const __FlashStringHelper *find, const char *replace);
    void replace(const __FlashStringHelper *find, const __FlashStringHelper *replace);
    void toLowerCase();
    void toUpperCase();
    void trim();
    long toInt() const;
    float toFloat() const;
    double toDouble() const;

    char operator[](size_t index) const;
    char &operator[](size_t index);
    operator bool() const;
    bool operator==(const String &value) const;
    bool operator==(const char *value) const;
    bool operator!=(const String &value) const;
    bool operator!=(const char *value) const;
    bool operator<(const String &value) const;
    bool operator>(const String &value) const;
    bool operator<=(const String &value) const;
    bool operator>=(const String &value) const;
    String &operator+=(const char *value);
    String &operator+=(char value);
    String &operator+=(const String &value);
    String &operator+=(const __FlashStringHelper *value);
    String &operator+=(unsigned char value);
    String &operator+=(int value);
    String &operator+=(unsigned int value);
    String &operator+=(long value);
    String &operator+=(unsigned long value);
    String &operator+=(float value);
    String &operator+=(double value);
    String &operator+=(long long value);
    String &operator+=(unsigned long long value);

private:
    bool assign(const char *value);
    bool assign(const char *value, size_t length);

    char *buffer_;
    size_t length_;
    size_t capacity_;
};

String operator+(const String &left, const String &right);
String operator+(const String &left, const char *right);
String operator+(const char *left, const String &right);
String operator+(const String &left, char right);
String operator+(char left, const String &right);
String operator+(const String &left, int right);
String operator+(const String &left, unsigned int right);
String operator+(const String &left, long right);
String operator+(const String &left, unsigned long right);
String operator+(const String &left, float right);
String operator+(const String &left, double right);
String operator+(const String &left, long long right);
String operator+(const String &left, unsigned long long right);
String operator+(const String &left, const __FlashStringHelper *right);
String operator+(const __FlashStringHelper *left, const String &right);
bool operator==(const char *left, const String &right);
bool operator!=(const char *left, const String &right);

class StringSumHelper : public String {
public:
    StringSumHelper(const String &value);
    StringSumHelper(const char *value);
    StringSumHelper(char value);
    StringSumHelper(unsigned char value);
    StringSumHelper(int value);
    StringSumHelper(unsigned int value);
    StringSumHelper(long value);
    StringSumHelper(unsigned long value);
    StringSumHelper(float value);
    StringSumHelper(double value);
    StringSumHelper(long long value);
    StringSumHelper(unsigned long long value);
};

#endif
