#include <stdint.h>

uint32_t air780epm_complex_probe_c_step(uint32_t value) {
    return (value * 33u) + 7u;
}
