#include <Air780EpmComplexLibProbe.h>

#include "detail/ComplexDetail.h"

extern "C" uint32_t air780epm_complex_probe_c_step(uint32_t value);

uint32_t air780epmComplexProbeValue(uint32_t seed) {
    const uint32_t cValue = air780epm_complex_probe_c_step(seed);
    return air780epmComplexDetailValue(cValue) ^ 0x780E0u;
}
