#include <Arduino.h>

extern "C" {
#include "luat_pm.h"
}

namespace {

const AIR780EPMSleepClass::DeepSleepTimerId kDeepSleepTimerId =
    AIR780EPMSleepClass::DEEP_SLEEP_TIMER_2;
const uint32_t kDeepSleepMs = 60000UL;
const uint32_t kHoldReportWindowMs = 15000UL;
const uint32_t kHoldReportIntervalMs = 1000UL;

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

void printRawPmReport()
{
    int rawLastState = -1;
    int rawRtcOrPad = -1;
    int rawWakeTimerId = -1;
    const int rawPowerOnReason = luat_pm_get_poweron_reason();
    const int wakeTimerRc = luat_pm_dtimer_wakeup_id(&rawWakeTimerId);
    (void)luat_pm_last_state(&rawLastState, &rawRtcOrPad);

    Serial.print("+ARDUINO: SLEEP_TIMER,RAW,LAST_STATE,");
    Serial.print(rawLastState);
    Serial.print(",RTC_OR_PAD,");
    Serial.print(rawRtcOrPad);
    Serial.print(",WAKE_TIMER_RC,");
    Serial.print(wakeTimerRc);
    Serial.print(",WAKE_TIMER_ID,");
    Serial.print(rawWakeTimerId);
    Serial.print(",POWERON_REASON,");
    Serial.println(rawPowerOnReason);
}

void printBootReport()
{
    Serial.print("+ARDUINO: SLEEP_TIMER,WAKE_REASON,");
    Serial.println(wakeupReasonName(AIR780EPMSleep.wakeupReason()));

    Serial.print("+ARDUINO: SLEEP_TIMER,LAST_STATE,");
    Serial.println(sleepStateName(AIR780EPMSleep.lastSleepState()));

    Serial.print("+ARDUINO: SLEEP_TIMER,LAST_MS,");
    Serial.println(AIR780EPMSleep.sleepTimeMillis());

    Serial.print("+ARDUINO: SLEEP_TIMER,WAKE_BITMAP,0x");
    Serial.println(AIR780EPMSleep.wakeupPinBitmap(), HEX);

    printRawPmReport();
}

void holdWakeResult()
{
    const AIR780EPMSleepClass::SleepState state = AIR780EPMSleep.lastSleepState();
    const AIR780EPMSleepClass::WakeupReason reason = AIR780EPMSleep.wakeupReason();
    const bool pass = (state == AIR780EPMSleepClass::SLEEP_STATE_DEEP) &&
                      (reason == AIR780EPMSleepClass::WAKEUP_FROM_RTC);
    const unsigned long deadline = millis() + kHoldReportWindowMs;

    Serial.println("+ARDUINO: SLEEP_TIMER,DEEP_WAKE,DETECTED");
    Serial.print("+ARDUINO: SLEEP_TIMER,RESULT,STATE,");
    Serial.print(sleepStateName(state));
    Serial.print(",REASON,");
    Serial.println(wakeupReasonName(reason));
    Serial.println(pass ? "+ARDUINO: SLEEP_TIMER,PASS"
                        : "+ARDUINO: SLEEP_TIMER,FAIL");
    Serial.println("+ARDUINO: SLEEP_TIMER,INFO,DEEP_WAKE_RESULT_HOLD");

    while ((long)(millis() - deadline) < 0L) {
        delay(kHoldReportIntervalMs);
        Serial.print("+ARDUINO: SLEEP_TIMER,HOLD,STATE,");
        Serial.print(sleepStateName(state));
        Serial.print(",REASON,");
        Serial.println(wakeupReasonName(reason));
        Serial.println(pass ? "+ARDUINO: SLEEP_TIMER,PASS"
                            : "+ARDUINO: SLEEP_TIMER,FAIL");
    }
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    delay(1500);

    Serial.println("+ARDUINO: SLEEP_TIMER,READY");
    printBootReport();

    if (AIR780EPMSleep.lastSleepState() == AIR780EPMSleepClass::SLEEP_STATE_DEEP) {
        holdWakeResult();
        return;
    }

    Serial.print("+ARDUINO: SLEEP_TIMER,DEEP,REQUEST,");
    Serial.print(kDeepSleepMs);
    Serial.print(",TIMER,");
    Serial.println(static_cast<uint8_t>(kDeepSleepTimerId));

    if (AIR780EPMSleep.deepSleep(kDeepSleepMs, kDeepSleepTimerId)) {
        Serial.println("+ARDUINO: SLEEP_TIMER,DEEP,ARMED");
    } else {
        Serial.println("+ARDUINO: SLEEP_TIMER,DEEP,ARM,FAIL");
    }
}

void loop()
{
    delay(1000);
}
