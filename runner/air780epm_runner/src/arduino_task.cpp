#include "Arduino.h"

extern "C" {
#include "common_api.h"
#include "luat_debug.h"
#include "luat_rtos.h"
}

extern "C" void air780epm_call_static_constructors(void);
extern "C" bool air780epm_static_constructors_ok(void);

static luat_rtos_task_handle arduino_task_handle;

static void arduino_task(void *param) {
    (void)param;

    air780epm_call_static_constructors();
    luat_debug_print(
        air780epm_static_constructors_ok() ? "+ARDUINO: CTOR,PASS" : "+ARDUINO: CTOR,SKIP");
    setup();

    while (1) {
        loop();
        luat_rtos_task_sleep(1);
    }
}

static void arduino_task_init(void) {
    int rc = luat_rtos_task_create(
        &arduino_task_handle,
        8 * 1024,
        50,
        "arduino",
        arduino_task,
        NULL,
        0);
    luat_debug_print("arduino_task_init: arduino_task_create rc=%d", rc);
}

INIT_TASK_EXPORT(arduino_task_init, "1");
