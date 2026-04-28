#pragma once

#include <stddef.h>

class Print;

class Printable {
public:
    virtual ~Printable() {}
    virtual size_t printTo(Print &print) const = 0;
};
