#include "Air780EpmLinkProbe.h"

uint32_t air780epmLinkProbeValue(uint32_t seed) {
    return (seed ^ 0x780E0u) + 17u;
}
