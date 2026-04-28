#pragma once

#include "Print.h"
#include "WString.h"

enum LookaheadMode {
    SKIP_ALL,
    SKIP_NONE,
    SKIP_WHITESPACE
};

#define NO_IGNORE_CHAR '\x01'

class Stream : public Print {
public:
    Stream();

    virtual int available(void) = 0;
    virtual int read(void) = 0;
    virtual int peek(void) = 0;

    void setTimeout(unsigned long timeout);
    unsigned long getTimeout(void) const;

    virtual size_t readBytes(char *buffer, size_t length);
    virtual size_t readBytes(uint8_t *buffer, size_t length);
    size_t readBytesUntil(char terminator, char *buffer, size_t length);
    size_t readBytesUntil(char terminator, uint8_t *buffer, size_t length);

    bool find(const char *target);
    bool find(const uint8_t *target);
    bool find(const char *target, size_t length);
    bool find(const uint8_t *target, size_t length);
    bool find(char target);
    bool findUntil(const char *target, const char *terminator);
    bool findUntil(const uint8_t *target, const char *terminator);
    bool findUntil(const char *target, size_t target_len, const char *terminator, size_t terminator_len);
    bool findUntil(const uint8_t *target, size_t target_len, const char *terminator, size_t terminator_len);

    long parseInt(LookaheadMode lookahead = SKIP_ALL, char ignore = NO_IGNORE_CHAR);
    float parseFloat(LookaheadMode lookahead = SKIP_ALL, char ignore = NO_IGNORE_CHAR);
    virtual String readString(void);
    String readStringUntil(char terminator);

protected:
    int timedRead(void);
    int timedPeek(void);
    int peekNextDigit(LookaheadMode lookahead, bool detect_decimal);
    long parseInt(char ignore);
    float parseFloat(char ignore);

    struct MultiTarget {
        const char *str;
        size_t len;
        size_t index;
    };

    int findMulti(struct MultiTarget *targets, int target_count);

private:
    unsigned long timeout_;
};
