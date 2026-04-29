#include <Arduino.h>

extern "C" {
#include "luat_pm.h"
uint32_t BSP_UsbGetVBUSWkupPad(void);
}

namespace {

const AIR780EPMSleepClass::DeepSleepTimerId kDeepSleepTimerId =
    AIR780EPMSleepClass::DEEP_SLEEP_TIMER_1;
const uint32_t kDeepSleepMs = 60000UL;
const uint32_t kHoldReportWindowMs = 15000UL;
const uint32_t kHoldReportIntervalMs = 1000UL;
const uint32_t kUsbPowerSettleMs = 300UL;
const uint32_t kPreSleepUsbOffSettleMs = 100UL;

AIR780EPMSleepClass::WakeupPad currentWakeupPad()
{
    const uint32_t pad = BSP_UsbGetVBUSWkupPad();

    if (pad <= static_cast<uint32_t>(AIR780EPMSleepClass::WAKEUP_PAD_5)) {
        return static_cast<AIR780EPMSleepClass::WakeupPad>(pad);
    }

    return AIR780EPMSleepClass::WAKEUP_PAD_1;
}

const char *wakeupReasonName(AIR780EPMSleepClass::WakeupReason reason)
{
    switch (reason) {
        case AIR780EPMSleepClass::WAKEUP_FROM_POR:
            return "POR";
        case AIR780EPMSleepClass::WAKEUP_FROM_RTC:
            return "RTC";
        case AIR780EPMSleepClass::WAKEUP_FROM_PAD:
            return "PAD";
        case AIR780EPMSleepClass::WAKEUP_FROM_LPUART:
            return "LPUART";
        case AIR780EPMSleepClass::WAKEUP_FROM_LPUSB:
            return "LPUSB";
        case AIR780EPMSleepClass::WAKEUP_FROM_PWRKEY:
            return "PWRKEY";
        case AIR780EPMSleepClass::WAKEUP_FROM_CHARGER:
            return "CHARGER";
        default:
            return "UNKNOWN";
    }
}

const char *sleepStateName(AIR780EPMSleepClass::SleepState state)
{
    switch (state) {
        case AIR780EPMSleepClass::SLEEP_STATE_ACTIVE:
            return "ACTIVE";
        case AIR780EPMSleepClass::SLEEP_STATE_IDLE:
            return "IDLE";
        case AIR780EPMSleepClass::SLEEP_STATE_LIGHT:
            return "SLEEP1";
        case AIR780EPMSleepClass::SLEEP_STATE_DEEP:
            return "SLEEP2";
        case AIR780EPMSleepClass::SLEEP_STATE_HIBERNATE:
            return "HIBERNATE";
        default:
            return "INVALID";
    }
}

void printBootReport()
{
    Serial.print("+ARDUINO: SLEEP_PAD,WAKE_REASON,");
    Serial.println(wakeupReasonName(AIR780EPMSleep.wakeupReason()));

    Serial.print("+ARDUINO: SLEEP_PAD,LAST_STATE,");
    Serial.println(sleepStateName(AIR780EPMSleep.lastSleepState()));

    Serial.print("+ARDUINO: SLEEP_PAD,LAST_MS,");
    Serial.println(AIR780EPMSleep.sleepTimeMillis());

    Serial.print("+ARDUINO: SLEEP_PAD,WAKE_BITMAP,0x");
    Serial.println(AIR780EPMSleep.wakeupPinBitmap(), HEX);
}

void holdWakeResult(AIR780EPMSleepClass::WakeupReason reason)
{
    const bool pass = (reason == AIR780EPMSleepClass::WAKEUP_FROM_PAD) ||
                      (reason == AIR780EPMSleepClass::WAKEUP_FROM_LPUSB);
    const unsigned long deadline = millis() + kHoldReportWindowMs;

    Serial.println("+ARDUINO: SLEEP_PAD,DEEP_WAKE,DETECTED");
    Serial.print("+ARDUINO: SLEEP_PAD,RESULT,");
    Serial.println(wakeupReasonName(reason));
    Serial.println(pass ? "+ARDUINO: SLEEP_PAD,PASS"
                        : "+ARDUINO: SLEEP_PAD,FAIL");
    Serial.println("+ARDUINO: SLEEP_PAD,INFO,DEEP_WAKE_RESULT_HOLD");

    while ((long)(millis() - deadline) < 0L) {
        delay(kHoldReportIntervalMs);
        Serial.print("+ARDUINO: SLEEP_PAD,HOLD,RESULT,");
        Serial.println(wakeupReasonName(reason));
        Serial.println(pass ? "+ARDUINO: SLEEP_PAD,PASS"
                            : "+ARDUINO: SLEEP_PAD,FAIL");
    }
}

void setUsbPower(bool on)
{
    luat_pm_power_ctrl(LUAT_PM_POWER_USB, on ? 1 : 0);
    delay(kUsbPowerSettleMs);
}

}  // namespace

void setup()
{
    const AIR780EPMSleepClass::WakeupPad wakeupPad = currentWakeupPad();
    const bool usePulldown = (wakeupPad == AIR780EPMSleepClass::WAKEUP_PAD_1);

    setUsbPower(true);
    Serial.begin(115200);
    delay(1500);

    Serial.println("+ARDUINO: SLEEP_PAD,READY");
    printBootReport();
    Serial.println("+ARDUINO: SLEEP_PAD,INFO,WAKEUP_PAD_X_IS_NOT_ARDUINO_GPIO");
    Serial.print("+ARDUINO: SLEEP_PAD,USB_VBUS_WAKE_PAD,");
    Serial.println(static_cast<uint8_t>(wakeupPad));
    Serial.print("+ARDUINO: SLEEP_PAD,WAKE_PULLDOWN,");
    Serial.println(usePulldown ? 1 : 0);
    Serial.print("+ARDUINO: SLEEP_PAD,DEEP_TIMER,");
    Serial.println(static_cast<uint8_t>(kDeepSleepTimerId));

    if (AIR780EPMSleep.lastSleepState() == AIR780EPMSleepClass::SLEEP_STATE_DEEP) {
        holdWakeResult(AIR780EPMSleep.wakeupReason());
        return;
    }

    // USB VBUS wake stays on both edges. WAKEUP1 is tested with pulldown first.
    if (AIR780EPMSleep.setWakeupPad(wakeupPad,
                                    AIR780EPMSleepClass::WAKEUP_EDGE_BOTH,
                                    false,
                                    usePulldown)) {
        Serial.print("+ARDUINO: SLEEP_PAD,PAD,CONFIG,OK,");
        Serial.println(static_cast<uint8_t>(wakeupPad));
    } else {
        Serial.print("+ARDUINO: SLEEP_PAD,PAD,CONFIG,FAIL,");
        Serial.println(static_cast<uint8_t>(wakeupPad));
        return;
    }

    Serial.print("+ARDUINO: SLEEP_PAD,DEEP,REQUEST,");
    Serial.print(kDeepSleepMs);
    Serial.print(",TIMER,");
    Serial.println(static_cast<uint8_t>(kDeepSleepTimerId));
    Serial.println("+ARDUINO: SLEEP_PAD,INFO,USB_WILL_DROP_BEFORE_DEEP");
    Serial.println("+ARDUINO: SLEEP_PAD,USB_POWER,OFF_BEFORE_DEEP");
    delay(kPreSleepUsbOffSettleMs);
    setUsbPower(false);

    if (AIR780EPMSleep.deepSleep(kDeepSleepMs, kDeepSleepTimerId)) {
        Serial.println("+ARDUINO: SLEEP_PAD,DEEP,ARMED");
    } else {
        Serial.println("+ARDUINO: SLEEP_PAD,DEEP,ARM,FAIL");
    }
}

void loop()
{
    delay(1000);
}
