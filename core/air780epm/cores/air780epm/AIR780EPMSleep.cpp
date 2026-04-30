#include "AIR780EPMSleep.h"

#include "arduino_sleep_io.h"

bool AIR780EPMSleepClass::lightSleep(uint32_t ms) const
{
    return arduinoCoreSleepLight(ms);
}

bool AIR780EPMSleepClass::deepSleep(uint32_t ms, DeepSleepTimerId timerId) const
{
    if (!validTimerId(timerId)) {
        return false;
    }

    return arduinoCoreSleepDeep(ms, static_cast<uint8_t>(timerId));
}

bool AIR780EPMSleepClass::setWakeupPad(WakeupPad pad,
                                       WakeupEdge edge,
                                       bool pullup,
                                       bool pulldown) const
{
    if (!validPad(pad) || edge == WAKEUP_EDGE_NONE || (pullup && pulldown)) {
        return false;
    }

    return arduinoCoreSleepSetWakeupPad(static_cast<uint8_t>(pad),
                                        static_cast<uint8_t>(edge),
                                        pullup ? 1U : 0U,
                                        pulldown ? 1U : 0U);
}

bool AIR780EPMSleepClass::clearWakeupPad(WakeupPad pad) const
{
    if (!validPad(pad)) {
        return false;
    }

    return arduinoCoreSleepClearWakeupPad(static_cast<uint8_t>(pad));
}

AIR780EPMSleepClass::WakeupReason AIR780EPMSleepClass::wakeupReason(void) const
{
    return static_cast<WakeupReason>(arduinoCoreSleepWakeupReason());
}

AIR780EPMSleepClass::SleepState AIR780EPMSleepClass::lastSleepState(void) const
{
    return static_cast<SleepState>(arduinoCoreSleepLastState());
}

uint32_t AIR780EPMSleepClass::sleepTimeMillis(void) const
{
    return arduinoCoreSleepTimeMillis();
}

uint8_t AIR780EPMSleepClass::wakeupPinBitmap(void) const
{
    return arduinoCoreSleepWakeupPinBitmap();
}

bool AIR780EPMSleepClass::validPad(WakeupPad pad) const
{
    return static_cast<uint8_t>(pad) <= static_cast<uint8_t>(WAKEUP_PAD_5);
}

bool AIR780EPMSleepClass::validTimerId(DeepSleepTimerId timerId) const
{
    return static_cast<uint8_t>(timerId) <= static_cast<uint8_t>(DEEP_SLEEP_TIMER_6);
}

AIR780EPMSleepClass AIR780EPMSleep;
