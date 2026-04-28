#pragma once

#include "Arduino.h"

#include <ctype.h>

inline boolean isAlphaNumeric(int c) {
    return isalnum(c) != 0;
}

inline boolean isAlpha(int c) {
    return isalpha(c) != 0;
}

inline boolean isAscii(int c) {
    return static_cast<unsigned int>(c) <= 0177U;
}

inline boolean isWhitespace(int c) {
    return isblank(c) != 0;
}

inline boolean isControl(int c) {
    return iscntrl(c) != 0;
}

inline boolean isDigit(int c) {
    return isdigit(c) != 0;
}

inline boolean isGraph(int c) {
    return isgraph(c) != 0;
}

inline boolean isLowerCase(int c) {
    return islower(c) != 0;
}

inline boolean isPrintable(int c) {
    return isprint(c) != 0;
}

inline boolean isPunct(int c) {
    return ispunct(c) != 0;
}

inline boolean isSpace(int c) {
    return isspace(c) != 0;
}

inline boolean isUpperCase(int c) {
    return isupper(c) != 0;
}

inline boolean isHexadecimalDigit(int c) {
    return isxdigit(c) != 0;
}

inline int toAscii(int c) {
    return c & 0177;
}

inline int toLowerCase(int c) {
    return tolower(c);
}

inline int toUpperCase(int c) {
    return toupper(c);
}
