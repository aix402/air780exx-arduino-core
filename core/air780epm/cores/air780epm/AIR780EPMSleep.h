#ifndef AIR780EPM_SLEEP_H
#define AIR780EPM_SLEEP_H

#include <stdint.h>

class AIR780EPMSleepClass {
public:
    // Platform PMU wakeup pad IDs. These are not Arduino digital pin numbers.
    enum WakeupPad {
        WAKEUP_PAD_0 = 0,
        WAKEUP_PAD_1 = 1,
        WAKEUP_PAD_2 = 2,
        WAKEUP_PAD_3 = 3,
        WAKEUP_PAD_4 = 4,
        WAKEUP_PAD_5 = 5
    };

    enum WakeupEdge {
        WAKEUP_EDGE_NONE = 0,
        WAKEUP_EDGE_RISING = 1,
        WAKEUP_EDGE_FALLING = 2,
        WAKEUP_EDGE_BOTH = 3
    };

    enum WakeupReason {
        WAKEUP_FROM_POR = 0,
        WAKEUP_FROM_RTC = 1,
        WAKEUP_FROM_PAD = 2,
        WAKEUP_FROM_LPUART = 3,
        WAKEUP_FROM_LPUSB = 4,
        WAKEUP_FROM_PWRKEY = 5,
        WAKEUP_FROM_CHARGER = 6,
        WAKEUP_FROM_UNKNOWN = 255
    };

    enum SleepState {
        SLEEP_STATE_ACTIVE = 0,
        SLEEP_STATE_IDLE = 1,
        SLEEP_STATE_LIGHT = 2,
        SLEEP_STATE_DEEP = 3,
        SLEEP_STATE_HIBERNATE = 4,
        SLEEP_STATE_INVALID = 255
    };

    // Deep-sleep RTC timer slots used when arming SLP2 sleep.
    enum DeepSleepTimerId {
        DEEP_SLEEP_TIMER_0 = 0,
        DEEP_SLEEP_TIMER_1 = 1,
        DEEP_SLEEP_TIMER_2 = 2,
        DEEP_SLEEP_TIMER_3 = 3,
        DEEP_SLEEP_TIMER_4 = 4,
        DEEP_SLEEP_TIMER_5 = 5,
        DEEP_SLEEP_TIMER_6 = 6
    };

    bool lightSleep(uint32_t ms) const;
    bool deepSleep(uint32_t ms,
                   DeepSleepTimerId timerId = DEEP_SLEEP_TIMER_0) const;
    bool setWakeupPad(WakeupPad pad,
                      WakeupEdge edge,
                      bool pullup = false,
                      bool pulldown = false) const;
    bool clearWakeupPad(WakeupPad pad) const;
    WakeupReason wakeupReason(void) const;
    SleepState lastSleepState(void) const;
    uint32_t sleepTimeMillis(void) const;
    // Returns the raw level bitmap for WAKEUP_PAD_0..5, not Arduino GPIOs.
    uint8_t wakeupPinBitmap(void) const;

private:
    bool validPad(WakeupPad pad) const;
    bool validTimerId(DeepSleepTimerId timerId) const;
};

extern AIR780EPMSleepClass AIR780EPMSleep;

#endif
