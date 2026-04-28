#include "WString.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

char kEmptyString[] = "";
const char kDigits[] = "0123456789ABCDEF";

void formatUnsignedNumber(unsigned long long value, unsigned char base, char *buffer, size_t bufferSize)
{
    size_t index = bufferSize;

    if (bufferSize == 0U)
    {
        return;
    }

    if ((base < 2U) || (base > 16U))
    {
        base = 10U;
    }

    buffer[--index] = '\0';

    do
    {
        if (index == 0U)
        {
            break;
        }

        buffer[--index] = kDigits[value % base];
        value /= base;
    } while (value != 0UL);

    if (index > 0U)
    {
        memmove(buffer, buffer + index, bufferSize - index);
    }
}

void formatSignedNumber(long long value, unsigned char base, char *buffer, size_t bufferSize)
{
    if ((base == 10U) && (value < 0LL))
    {
        if (bufferSize < 2U)
        {
            if (bufferSize > 0U)
            {
                buffer[0] = '\0';
            }
            return;
        }

        buffer[0] = '-';
        const unsigned long long magnitude = static_cast<unsigned long long>(-(value + 1LL)) + 1ULL;
        formatUnsignedNumber(magnitude, base, buffer + 1, bufferSize - 1U);
        return;
    }

    formatUnsignedNumber(static_cast<unsigned long long>(value), base, buffer, bufferSize);
}

void formatFloatNumber(double value, unsigned int decimalPlaces, char *buffer, size_t bufferSize)
{
    size_t index = 0U;

    if (bufferSize == 0U)
    {
        return;
    }

    if (isnan(value))
    {
        snprintf(buffer, bufferSize, "nan");
        return;
    }

    if (isinf(value))
    {
        snprintf(buffer, bufferSize, "inf");
        return;
    }

    if (decimalPlaces > 10U)
    {
        decimalPlaces = 10U;
    }

    if (value < 0.0)
    {
        buffer[index++] = '-';
        value = -value;
        if (index >= bufferSize)
        {
            buffer[bufferSize - 1U] = '\0';
            return;
        }
    }

    double rounding = 0.5;
    for (unsigned int i = 0U; i < decimalPlaces; i++)
    {
        rounding /= 10.0;
    }
    value += rounding;

    const unsigned long long intPart = static_cast<unsigned long long>(value);
    char intBuffer[32];
    formatUnsignedNumber(intPart, 10, intBuffer, sizeof(intBuffer));
    const size_t intLength = strlen(intBuffer);
    if ((index + intLength + 1U) > bufferSize)
    {
        buffer[0] = '\0';
        return;
    }

    memcpy(buffer + index, intBuffer, intLength);
    index += intLength;

    if (decimalPlaces > 0U)
    {
        if ((index + 2U) > bufferSize)
        {
            buffer[0] = '\0';
            return;
        }

        buffer[index++] = '.';
        double remainder = value - static_cast<double>(intPart);
        while (decimalPlaces-- > 0U)
        {
            if ((index + 1U) >= bufferSize)
            {
                break;
            }
            remainder *= 10.0;
            const unsigned int digit = static_cast<unsigned int>(remainder);
            buffer[index++] = kDigits[digit];
            remainder -= static_cast<double>(digit);
        }
    }

    buffer[index] = '\0';
}

int asciiCaseCompare(const char *left, const char *right)
{
    while ((*left != '\0') && (*right != '\0'))
    {
        const int l = tolower(static_cast<unsigned char>(*left));
        const int r = tolower(static_cast<unsigned char>(*right));
        if (l != r)
        {
            return l - r;
        }
        left++;
        right++;
    }

    return static_cast<unsigned char>(*left) - static_cast<unsigned char>(*right);
}

}  // namespace

String::String()
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
}

String::String(const char *value)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    (void)assign(value);
}

String::String(const char *value, unsigned int length)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    (void)assign(value, length);
}

String::String(const uint8_t *value, unsigned int length)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    (void)assign(reinterpret_cast<const char *>(value), length);
}

String::String(const String &value)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    (void)assign(value.c_str());
}

String::String(const __FlashStringHelper *value)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    (void)assign(reinterpret_cast<const char *>(value));
}

String::String(char value)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    char text[2] = {value, '\0'};
    (void)assign(text);
}

String::String(unsigned char value, unsigned char base)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    char text[65];
    formatUnsignedNumber(value, base, text, sizeof(text));
    (void)assign(text);
}

String::String(int value, unsigned char base)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    char text[34];
    formatSignedNumber(value, base, text, sizeof(text));
    (void)assign(text);
}

String::String(unsigned int value, unsigned char base)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    char text[65];
    formatUnsignedNumber(value, base, text, sizeof(text));
    (void)assign(text);
}

String::String(long value, unsigned char base)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    char text[34];
    formatSignedNumber(value, base, text, sizeof(text));
    (void)assign(text);
}

String::String(unsigned long value, unsigned char base)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    char text[65];
    formatUnsignedNumber(value, base, text, sizeof(text));
    (void)assign(text);
}

String::String(float value, unsigned int decimalPlaces)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    char text[48];
    formatFloatNumber(static_cast<double>(value), decimalPlaces, text, sizeof(text));
    (void)assign(text);
}

String::String(double value, unsigned int decimalPlaces)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    char text[48];
    formatFloatNumber(value, decimalPlaces, text, sizeof(text));
    (void)assign(text);
}

String::String(long long value, unsigned char base)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    char text[66];
    formatSignedNumber(value, base, text, sizeof(text));
    (void)assign(text);
}

String::String(unsigned long long value, unsigned char base)
    : buffer_(NULL),
      length_(0U),
      capacity_(0U)
{
    char text[65];
    formatUnsignedNumber(value, base, text, sizeof(text));
    (void)assign(text);
}

String::~String()
{
    free(buffer_);
}

String &String::operator=(const String &value)
{
    if (this != &value)
    {
        (void)assign(value.c_str());
    }

    return *this;
}

String &String::operator=(const char *value)
{
    (void)assign(value);
    return *this;
}

String &String::operator=(const __FlashStringHelper *value)
{
    (void)assign(reinterpret_cast<const char *>(value));
    return *this;
}

const char *String::c_str() const
{
    return (buffer_ != NULL) ? buffer_ : kEmptyString;
}

char *String::begin()
{
    return (buffer_ != NULL) ? buffer_ : kEmptyString;
}

char *String::end()
{
    return begin() + length_;
}

const char *String::begin() const
{
    return c_str();
}

const char *String::end() const
{
    return c_str() + length_;
}

size_t String::length() const
{
    return length_;
}

void String::clear()
{
    remove(0U);
}

bool String::isEmpty() const
{
    return length_ == 0U;
}

bool String::reserve(size_t size)
{
    char *newBuffer = NULL;

    if ((size + 1U) <= capacity_)
    {
        return true;
    }

    newBuffer = static_cast<char *>(realloc(buffer_, size + 1U));
    if (newBuffer == NULL)
    {
        return false;
    }

    buffer_ = newBuffer;
    capacity_ = size + 1U;
    if (length_ == 0U)
    {
        buffer_[0] = '\0';
    }

    return true;
}

bool String::concat(const char *value)
{
    const char *suffix = (value != NULL) ? value : "";
    const size_t suffixLength = strlen(suffix);
    return concat(suffix, static_cast<unsigned int>(suffixLength));
}

bool String::concat(const char *value, unsigned int length)
{
    const char *suffix = (value != NULL) ? value : "";
    const size_t suffixLength = (value != NULL) ? static_cast<size_t>(length) : 0U;
    const size_t newLength = length_ + suffixLength;

    if (!reserve(newLength))
    {
        return false;
    }

    memcpy(buffer_ + length_, suffix, suffixLength);
    length_ = newLength;
    buffer_[length_] = '\0';
    return true;
}

bool String::concat(const uint8_t *value, unsigned int length)
{
    return concat(reinterpret_cast<const char *>(value), length);
}

bool String::concat(char value)
{
    char text[2] = {value, '\0'};
    return concat(text);
}

bool String::concat(const String &value)
{
    return concat(value.c_str());
}

bool String::concat(const __FlashStringHelper *value)
{
    return concat(reinterpret_cast<const char *>(value));
}

bool String::concat(unsigned char value)
{
    return concat(String(value));
}

bool String::concat(int value)
{
    return concat(String(value));
}

bool String::concat(unsigned int value)
{
    return concat(String(value));
}

bool String::concat(long value)
{
    return concat(String(value));
}

bool String::concat(unsigned long value)
{
    return concat(String(value));
}

bool String::concat(float value)
{
    return concat(String(value));
}

bool String::concat(double value)
{
    return concat(String(value));
}

bool String::concat(long long value)
{
    return concat(String(value));
}

bool String::concat(unsigned long long value)
{
    return concat(String(value));
}

void String::remove(size_t index)
{
    if ((buffer_ == NULL) || (index >= length_))
    {
        return;
    }

    buffer_[index] = '\0';
    length_ = index;
}

void String::remove(size_t index, size_t count)
{
    if ((buffer_ == NULL) || (index >= length_) || (count == 0U))
    {
        return;
    }

    if ((index + count) >= length_)
    {
        remove(index);
        return;
    }

    memmove(buffer_ + index, buffer_ + index + count, length_ - index - count + 1U);
    length_ -= count;
}

int String::compareTo(const String &value) const
{
    return strcmp(c_str(), value.c_str());
}

bool String::equals(const String &value) const
{
    return compareTo(value) == 0;
}

bool String::equals(const char *value) const
{
    return strcmp(c_str(), (value != NULL) ? value : "") == 0;
}

bool String::equalsIgnoreCase(const String &value) const
{
    return asciiCaseCompare(c_str(), value.c_str()) == 0;
}

unsigned char String::equalsConstantTime(const String &value) const
{
    const size_t leftLength = length_;
    const size_t rightLength = value.length();
    const size_t maxLength = (leftLength > rightLength) ? leftLength : rightLength;
    unsigned char diff = static_cast<unsigned char>(leftLength ^ rightLength);

    for (size_t i = 0U; i < maxLength; i++)
    {
        const unsigned char left = (i < leftLength) ? static_cast<unsigned char>(c_str()[i]) : 0U;
        const unsigned char right = (i < rightLength) ? static_cast<unsigned char>(value.c_str()[i]) : 0U;
        diff |= static_cast<unsigned char>(left ^ right);
    }

    return diff == 0U;
}

bool String::startsWith(const String &prefix) const
{
    return startsWith(prefix, 0U);
}

bool String::startsWith(const char *prefix) const
{
    return startsWith(String(prefix));
}

bool String::startsWith(const __FlashStringHelper *prefix) const
{
    return startsWith(reinterpret_cast<const char *>(prefix));
}

bool String::startsWith(const String &prefix, size_t offset) const
{
    if ((offset > length_) || (prefix.length() > (length_ - offset)))
    {
        return false;
    }

    return strncmp(c_str() + offset, prefix.c_str(), prefix.length()) == 0;
}

bool String::endsWith(const String &suffix) const
{
    if (suffix.length() > length_)
    {
        return false;
    }

    return strcmp(c_str() + length_ - suffix.length(), suffix.c_str()) == 0;
}

bool String::endsWith(const char *suffix) const
{
    return endsWith(String(suffix));
}

bool String::endsWith(const __FlashStringHelper *suffix) const
{
    return endsWith(reinterpret_cast<const char *>(suffix));
}

int String::indexOf(char value) const
{
    return indexOf(value, 0U);
}

int String::indexOf(char value, size_t fromIndex) const
{
    if (fromIndex >= length_)
    {
        return -1;
    }

    const char *match = strchr(c_str() + fromIndex, value);
    if (match == NULL)
    {
        return -1;
    }

    return static_cast<int>(match - c_str());
}

int String::indexOf(const String &value) const
{
    return indexOf(value, 0U);
}

int String::indexOf(const String &value, size_t fromIndex) const
{
    if (fromIndex > length_)
    {
        return -1;
    }

    const char *match = strstr(c_str() + fromIndex, value.c_str());
    return (match != NULL) ? static_cast<int>(match - c_str()) : -1;
}

int String::indexOf(const char *value) const
{
    return indexOf(String(value), 0U);
}

int String::indexOf(const char *value, size_t fromIndex) const
{
    return indexOf(String(value), fromIndex);
}

int String::lastIndexOf(char value) const
{
    if (length_ == 0U)
    {
        return -1;
    }

    return lastIndexOf(value, length_ - 1U);
}

int String::lastIndexOf(char value, size_t fromIndex) const
{
    if (length_ == 0U)
    {
        return -1;
    }

    if (fromIndex >= length_)
    {
        fromIndex = length_ - 1U;
    }

    for (size_t i = fromIndex + 1U; i > 0U; i--)
    {
        if (c_str()[i - 1U] == value)
        {
            return static_cast<int>(i - 1U);
        }
    }

    return -1;
}

int String::lastIndexOf(const String &value) const
{
    if (value.length() > length_)
    {
        return -1;
    }

    return lastIndexOf(value, length_ - value.length());
}

int String::lastIndexOf(const String &value, size_t fromIndex) const
{
    if (value.length() == 0U)
    {
        return (fromIndex < length_) ? static_cast<int>(fromIndex) : static_cast<int>(length_);
    }

    if (value.length() > length_)
    {
        return -1;
    }

    if (fromIndex > (length_ - value.length()))
    {
        fromIndex = length_ - value.length();
    }

    for (size_t i = fromIndex + 1U; i > 0U; i--)
    {
        const size_t pos = i - 1U;
        if (strncmp(c_str() + pos, value.c_str(), value.length()) == 0)
        {
            return static_cast<int>(pos);
        }
    }

    return -1;
}

String String::substring(size_t beginIndex) const
{
    return substring(beginIndex, length_);
}

String String::substring(size_t beginIndex, size_t endIndex) const
{
    String result;

    if (beginIndex > endIndex)
    {
        const size_t temp = beginIndex;
        beginIndex = endIndex;
        endIndex = temp;
    }

    if (beginIndex >= length_)
    {
        return result;
    }

    if (endIndex > length_)
    {
        endIndex = length_;
    }

    const size_t resultLength = endIndex - beginIndex;
    if (!result.reserve(resultLength))
    {
        return result;
    }

    memcpy(result.buffer_, c_str() + beginIndex, resultLength);
    result.buffer_[resultLength] = '\0';
    result.length_ = resultLength;
    return result;
}

char String::charAt(size_t index) const
{
    return (index < length_) ? c_str()[index] : '\0';
}

void String::setCharAt(size_t index, char value)
{
    if ((buffer_ != NULL) && (index < length_))
    {
        buffer_[index] = value;
    }
}

void String::getBytes(unsigned char *buffer, unsigned int bufferSize, unsigned int index) const
{
    if ((buffer == NULL) || (bufferSize == 0U))
    {
        return;
    }

    if (index >= length_)
    {
        buffer[0] = '\0';
        return;
    }

    const size_t available = length_ - index;
    const size_t copyLength = (available < (static_cast<size_t>(bufferSize) - 1U)) ? available : (static_cast<size_t>(bufferSize) - 1U);
    memcpy(buffer, c_str() + index, copyLength);
    buffer[copyLength] = '\0';
}

void String::toCharArray(char *buffer, unsigned int bufferSize, unsigned int index) const
{
    getBytes(reinterpret_cast<unsigned char *>(buffer), bufferSize, index);
}

void String::replace(char find, char replace)
{
    if (buffer_ == NULL)
    {
        return;
    }

    for (size_t i = 0U; i < length_; i++)
    {
        if (buffer_[i] == find)
        {
            buffer_[i] = replace;
        }
    }
}

void String::replace(const String &find, const String &replace)
{
    if ((find.length() == 0U) || (buffer_ == NULL))
    {
        return;
    }

    String result;
    size_t cursor = 0U;

    while (cursor < length_)
    {
        const int match = indexOf(find, cursor);
        if (match < 0)
        {
            (void)result.concat(c_str() + cursor);
            break;
        }

        const size_t matchIndex = static_cast<size_t>(match);
        (void)result.concat(c_str() + cursor, static_cast<unsigned int>(matchIndex - cursor));
        (void)result.concat(replace);
        cursor = matchIndex + find.length();
    }

    *this = result;
}

void String::replace(const char *find, const String &replace)
{
    this->replace(String(find), replace);
}

void String::replace(const char *find, const char *replace)
{
    this->replace(String(find), String(replace));
}

void String::replace(const __FlashStringHelper *find, const String &replace)
{
    this->replace(reinterpret_cast<const char *>(find), replace);
}

void String::replace(const __FlashStringHelper *find, const char *replace)
{
    this->replace(reinterpret_cast<const char *>(find), String(replace));
}

void String::replace(const __FlashStringHelper *find, const __FlashStringHelper *replace)
{
    this->replace(reinterpret_cast<const char *>(find), reinterpret_cast<const char *>(replace));
}

void String::toLowerCase()
{
    if (buffer_ == NULL)
    {
        return;
    }

    for (size_t i = 0U; i < length_; i++)
    {
        buffer_[i] = static_cast<char>(tolower(static_cast<unsigned char>(buffer_[i])));
    }
}

void String::toUpperCase()
{
    if (buffer_ == NULL)
    {
        return;
    }

    for (size_t i = 0U; i < length_; i++)
    {
        buffer_[i] = static_cast<char>(toupper(static_cast<unsigned char>(buffer_[i])));
    }
}

void String::trim()
{
    if (buffer_ == NULL)
    {
        return;
    }

    size_t start = 0U;
    while ((start < length_) && isspace(static_cast<unsigned char>(buffer_[start])))
    {
        start++;
    }

    size_t end = length_;
    while ((end > start) && isspace(static_cast<unsigned char>(buffer_[end - 1U])))
    {
        end--;
    }

    if (start > 0U)
    {
        memmove(buffer_, buffer_ + start, end - start);
    }

    length_ = end - start;
    buffer_[length_] = '\0';
}

long String::toInt() const
{
    return strtol(c_str(), NULL, 10);
}

float String::toFloat() const
{
    return static_cast<float>(strtod(c_str(), NULL));
}

double String::toDouble() const
{
    return strtod(c_str(), NULL);
}

char String::operator[](size_t index) const
{
    return charAt(index);
}

char &String::operator[](size_t index)
{
    static char dummy = '\0';

    if ((buffer_ == NULL) || (index >= length_))
    {
        dummy = '\0';
        return dummy;
    }

    return buffer_[index];
}

String::operator bool() const
{
    return true;
}

bool String::operator==(const String &value) const
{
    return equals(value);
}

bool String::operator==(const char *value) const
{
    return equals(value);
}

bool String::operator!=(const String &value) const
{
    return !equals(value);
}

bool String::operator!=(const char *value) const
{
    return !equals(value);
}

bool String::operator<(const String &value) const
{
    return compareTo(value) < 0;
}

bool String::operator>(const String &value) const
{
    return compareTo(value) > 0;
}

bool String::operator<=(const String &value) const
{
    return compareTo(value) <= 0;
}

bool String::operator>=(const String &value) const
{
    return compareTo(value) >= 0;
}

String &String::operator+=(const char *value)
{
    (void)concat(value);
    return *this;
}

String &String::operator+=(char value)
{
    (void)concat(value);
    return *this;
}

String &String::operator+=(const String &value)
{
    (void)concat(value);
    return *this;
}

String &String::operator+=(const __FlashStringHelper *value)
{
    (void)concat(value);
    return *this;
}

String &String::operator+=(unsigned char value)
{
    (void)concat(value);
    return *this;
}

String &String::operator+=(int value)
{
    (void)concat(value);
    return *this;
}

String &String::operator+=(unsigned int value)
{
    (void)concat(value);
    return *this;
}

String &String::operator+=(long value)
{
    (void)concat(value);
    return *this;
}

String &String::operator+=(unsigned long value)
{
    (void)concat(value);
    return *this;
}

String &String::operator+=(float value)
{
    (void)concat(value);
    return *this;
}

String &String::operator+=(double value)
{
    (void)concat(value);
    return *this;
}

String &String::operator+=(long long value)
{
    (void)concat(value);
    return *this;
}

String &String::operator+=(unsigned long long value)
{
    (void)concat(value);
    return *this;
}

bool String::assign(const char *value)
{
    const char *source = (value != NULL) ? value : "";
    const size_t newLength = strlen(source);
    return assign(source, newLength);
}

bool String::assign(const char *value, size_t length)
{
    const char *source = (value != NULL) ? value : "";
    const size_t newLength = (value != NULL) ? length : 0U;

    if (!reserve(newLength))
    {
        return false;
    }

    memcpy(buffer_, source, newLength);
    buffer_[newLength] = '\0';
    length_ = newLength;
    return true;
}

String operator+(const String &left, const String &right)
{
    String result(left);
    result += right;
    return result;
}

String operator+(const String &left, const char *right)
{
    String result(left);
    result += right;
    return result;
}

String operator+(const char *left, const String &right)
{
    String result(left);
    result += right;
    return result;
}

String operator+(const String &left, char right)
{
    String result(left);
    result += right;
    return result;
}

String operator+(char left, const String &right)
{
    String result(left);
    result += right;
    return result;
}

String operator+(const String &left, int right)
{
    String result(left);
    result += right;
    return result;
}

String operator+(const String &left, unsigned int right)
{
    String result(left);
    result += right;
    return result;
}

String operator+(const String &left, long right)
{
    String result(left);
    result += right;
    return result;
}

String operator+(const String &left, unsigned long right)
{
    String result(left);
    result += right;
    return result;
}

String operator+(const String &left, float right)
{
    String result(left);
    result += right;
    return result;
}

String operator+(const String &left, double right)
{
    String result(left);
    result += right;
    return result;
}

String operator+(const String &left, long long right)
{
    String result(left);
    result += right;
    return result;
}

String operator+(const String &left, unsigned long long right)
{
    String result(left);
    result += right;
    return result;
}

String operator+(const String &left, const __FlashStringHelper *right)
{
    String result(left);
    result += right;
    return result;
}

String operator+(const __FlashStringHelper *left, const String &right)
{
    String result(left);
    result += right;
    return result;
}

bool operator==(const char *left, const String &right)
{
    return right.equals(left);
}

bool operator!=(const char *left, const String &right)
{
    return !right.equals(left);
}

StringSumHelper::StringSumHelper(const String &value)
    : String(value)
{
}

StringSumHelper::StringSumHelper(const char *value)
    : String(value)
{
}

StringSumHelper::StringSumHelper(char value)
    : String(value)
{
}

StringSumHelper::StringSumHelper(unsigned char value)
    : String(value)
{
}

StringSumHelper::StringSumHelper(int value)
    : String(value)
{
}

StringSumHelper::StringSumHelper(unsigned int value)
    : String(value)
{
}

StringSumHelper::StringSumHelper(long value)
    : String(value)
{
}

StringSumHelper::StringSumHelper(unsigned long value)
    : String(value)
{
}

StringSumHelper::StringSumHelper(float value)
    : String(value)
{
}

StringSumHelper::StringSumHelper(double value)
    : String(value)
{
}

StringSumHelper::StringSumHelper(long long value)
    : String(value)
{
}

StringSumHelper::StringSumHelper(unsigned long long value)
    : String(value)
{
}
