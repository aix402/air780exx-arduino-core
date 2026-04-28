#include "Stream.h"

#include "Arduino.h"

#include <ctype.h>
#include <string.h>

Stream::Stream() : timeout_(1000UL) {}

void Stream::setTimeout(unsigned long timeout) {
    timeout_ = timeout;
}

unsigned long Stream::getTimeout(void) const {
    return timeout_;
}

size_t Stream::readBytes(char *buffer, size_t length) {
    if (buffer == nullptr) {
        return 0;
    }

    size_t count = 0;
    while (count < length) {
        const int value = timedRead();
        if (value < 0) {
            break;
        }
        buffer[count++] = static_cast<char>(value);
    }
    return count;
}

size_t Stream::readBytes(uint8_t *buffer, size_t length) {
    return readBytes(reinterpret_cast<char *>(buffer), length);
}

size_t Stream::readBytesUntil(char terminator, char *buffer, size_t length) {
    if (buffer == nullptr) {
        return 0;
    }

    size_t count = 0;
    while (count < length) {
        const int value = timedRead();
        if ((value < 0) || (value == terminator)) {
            break;
        }
        buffer[count++] = static_cast<char>(value);
    }
    return count;
}

size_t Stream::readBytesUntil(char terminator, uint8_t *buffer, size_t length) {
    return readBytesUntil(terminator, reinterpret_cast<char *>(buffer), length);
}

bool Stream::find(const char *target) {
    return find(target, (target != nullptr) ? strlen(target) : 0U);
}

bool Stream::find(const uint8_t *target) {
    return find(reinterpret_cast<const char *>(target));
}

bool Stream::find(const char *target, size_t length) {
    if (target == nullptr) {
        return false;
    }

    struct MultiTarget match = {target, length, 0U};
    return findMulti(&match, 1) == 0;
}

bool Stream::find(const uint8_t *target, size_t length) {
    return find(reinterpret_cast<const char *>(target), length);
}

bool Stream::find(char target) {
    return find(&target, 1U);
}

bool Stream::findUntil(const char *target, const char *terminator) {
    return findUntil(
        target,
        (target != nullptr) ? strlen(target) : 0U,
        terminator,
        (terminator != nullptr) ? strlen(terminator) : 0U);
}

bool Stream::findUntil(const uint8_t *target, const char *terminator) {
    return findUntil(reinterpret_cast<const char *>(target), terminator);
}

bool Stream::findUntil(const char *target, size_t target_len, const char *terminator, size_t terminator_len) {
    if (target == nullptr) {
        return false;
    }
    if ((terminator == nullptr) || (terminator_len == 0U)) {
        return find(target, target_len);
    }

    struct MultiTarget targets[2] = {
        {target, target_len, 0U},
        {terminator, terminator_len, 0U},
    };
    return findMulti(targets, 2) == 0;
}

bool Stream::findUntil(const uint8_t *target, size_t target_len, const char *terminator, size_t terminator_len) {
    return findUntil(reinterpret_cast<const char *>(target), target_len, terminator, terminator_len);
}

long Stream::parseInt(LookaheadMode lookahead, char ignore) {
    bool negative = false;
    long value = 0;
    int c = peekNextDigit(lookahead, false);

    if (c < 0) {
        return 0;
    }

    do {
        if (c == '-') {
            negative = true;
        } else if (isdigit(c)) {
            value = (value * 10L) + static_cast<long>(c - '0');
        }

        (void)read();
        c = timedPeek();
    } while ((c >= 0) && ((c == ignore) || isdigit(c)));

    return negative ? -value : value;
}

float Stream::parseFloat(LookaheadMode lookahead, char ignore) {
    bool negative = false;
    bool fraction_seen = false;
    float value = 0.0F;
    float fraction = 1.0F;
    int c = peekNextDigit(lookahead, true);

    if (c < 0) {
        return 0.0F;
    }

    do {
        if (c == '-') {
            negative = true;
        } else if (c == '.') {
            fraction_seen = true;
        } else if (isdigit(c)) {
            value = (value * 10.0F) + static_cast<float>(c - '0');
            if (fraction_seen) {
                fraction *= 0.1F;
            }
        }

        (void)read();
        c = timedPeek();
    } while ((c >= 0) && ((c == ignore) || (c == '.') || isdigit(c)));

    if (fraction_seen) {
        value *= fraction;
    }
    return negative ? -value : value;
}

String Stream::readString(void) {
    String result;
    while (true) {
        const int value = timedRead();
        if (value < 0) {
            break;
        }
        (void)result.concat(static_cast<char>(value));
    }
    return result;
}

String Stream::readStringUntil(char terminator) {
    String result;
    while (true) {
        const int value = timedRead();
        if ((value < 0) || (value == terminator)) {
            break;
        }
        (void)result.concat(static_cast<char>(value));
    }
    return result;
}

int Stream::timedRead(void) {
    const unsigned long start = millis();
    do {
        const int value = read();
        if (value >= 0) {
            return value;
        }
        yield();
    } while ((millis() - start) < timeout_);

    return -1;
}

int Stream::timedPeek(void) {
    const unsigned long start = millis();
    do {
        const int value = peek();
        if (value >= 0) {
            return value;
        }
        yield();
    } while ((millis() - start) < timeout_);

    return -1;
}

int Stream::peekNextDigit(LookaheadMode lookahead, bool detect_decimal) {
    while (true) {
        const int value = timedPeek();
        if (value < 0) {
            return -1;
        }
        if ((value == '-') || isdigit(value) || (detect_decimal && (value == '.'))) {
            return value;
        }

        if (lookahead == SKIP_NONE) {
            return -1;
        }
        if ((lookahead == SKIP_WHITESPACE) && !isspace(value)) {
            return -1;
        }

        (void)read();
    }
}

long Stream::parseInt(char ignore) {
    return parseInt(SKIP_ALL, ignore);
}

float Stream::parseFloat(char ignore) {
    return parseFloat(SKIP_ALL, ignore);
}

int Stream::findMulti(struct MultiTarget *targets, int target_count) {
    if ((targets == nullptr) || (target_count <= 0)) {
        return -1;
    }

    for (int i = 0; i < target_count; ++i) {
        targets[i].index = 0U;
        if (targets[i].len == 0U) {
            return i;
        }
    }

    while (true) {
        const int value = timedRead();
        if (value < 0) {
            return -1;
        }

        for (int i = 0; i < target_count; ++i) {
            struct MultiTarget *target = &targets[i];
            if (value == target->str[target->index]) {
                target->index++;
                if (target->index == target->len) {
                    return i;
                }
            } else {
                target->index = (value == target->str[0]) ? 1U : 0U;
            }
        }
    }
}
