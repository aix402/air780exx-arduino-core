#include "ComplexDetail.h"

uint32_t air780epmComplexDetailValue(uint32_t value) {
    return (value >> 1u) + (value << 3u) + 19u;
}
