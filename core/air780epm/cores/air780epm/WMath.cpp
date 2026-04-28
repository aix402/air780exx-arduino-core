#include "Arduino.h"

#include <stdlib.h>

extern "C" {

long map(long x, long in_min, long in_max, long out_min, long out_max) {
    const long run = in_max - in_min;
    if (run == 0L) {
        return out_min;
    }

    const long rise = out_max - out_min;
    const long delta = x - in_min;
    return ((delta * rise) / run) + out_min;
}

}  // extern "C"

void randomSeed(unsigned long seed) {
    if (seed != 0UL) {
        srand(static_cast<unsigned int>(seed));
    }
}

long random(long howbig) {
    if (howbig == 0L) {
        return 0L;
    }
    if (howbig < 0L) {
        return random(0L, -howbig);
    }

    return static_cast<long>(rand() % howbig);
}

long random(long howsmall, long howbig) {
    if (howsmall >= howbig) {
        return howsmall;
    }

    return random(howbig - howsmall) + howsmall;
}

uint16_t makeWord(uint16_t value) {
    return value;
}

uint16_t makeWord(uint8_t high, uint8_t low) {
    return (static_cast<uint16_t>(high) << 8U) | low;
}
