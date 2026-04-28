extern "C" {
#include <stddef.h>
#include <stdlib.h>
}

#if defined(ARDUINO_ENABLE_STATIC_CONSTRUCTORS)
extern "C" {
typedef void (*init_fn_t)(void);
extern init_fn_t __arduino_init_array_start[] __attribute__((weak));
extern init_fn_t __arduino_init_array_end[] __attribute__((weak));
}
#endif

static volatile unsigned long g_static_constructor_cookie = 0;

class StaticConstructorSmoke {
public:
    StaticConstructorSmoke() {
        g_static_constructor_cookie = 0x780ECAFEUL;
    }
};

static StaticConstructorSmoke g_static_constructor_smoke;

extern "C" void air780epm_call_static_constructors(void) {
#if defined(ARDUINO_ENABLE_STATIC_CONSTRUCTORS)
    static bool initialized = false;
    if (initialized) {
        return;
    }
    initialized = true;

    if (__arduino_init_array_start == 0 || __arduino_init_array_end == 0) {
        return;
    }

    for (init_fn_t *fn = __arduino_init_array_start; fn != __arduino_init_array_end; ++fn) {
        if (*fn != 0) {
            (*fn)();
        }
    }
#else
    // Recovery builds can disable this bridge while keeping setup()/loop() alive.
#endif
}

extern "C" bool air780epm_static_constructors_ok(void) {
    return g_static_constructor_cookie == 0x780ECAFEUL;
}

void *operator new(size_t size) {
    return malloc(size);
}

void *operator new[](size_t size) {
    return malloc(size);
}

void operator delete(void *ptr) noexcept {
    free(ptr);
}

void operator delete[](void *ptr) noexcept {
    free(ptr);
}

void operator delete(void *ptr, size_t size) noexcept {
    (void)size;
    free(ptr);
}

void operator delete[](void *ptr, size_t size) noexcept {
    (void)size;
    free(ptr);
}

extern "C" void __cxa_pure_virtual(void) {
    while (1) {
    }
}
