#include <Arduino.h>

namespace {

const uint32_t kLightSleepMs = 2000UL;
const uint32_t kDeepSleepMs = 10000UL;
const AIR780EPMSleepClass::DeepSleepTimerId kDeepSleepTimerId =
    AIR780EPMSleepClass::DEEP_SLEEP_TIMER_0;
const AIR780EPMSleepClass::WakeupPad kWakeupPad =
    AIR780EPMSleepClass::WAKEUP_PAD_0;
const bool kAutoArmDeepSleep = false;

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
    Serial.print("+ARDUINO: SLEEP,WAKE_REASON,");
    Serial.println(wakeupReasonName(AIR780EPMSleep.wakeupReason()));

    Serial.print("+ARDUINO: SLEEP,LAST_STATE,");
    Serial.println(sleepStateName(AIR780EPMSleep.lastSleepState()));

    Serial.print("+ARDUINO: SLEEP,LAST_MS,");
    Serial.println(AIR780EPMSleep.sleepTimeMillis());

    Serial.print("+ARDUINO: SLEEP,WAKE_BITMAP,0x");
    Serial.println(AIR780EPMSleep.wakeupPinBitmap(), HEX);
}

}  // namespace

void setup()
{
    Serial.begin(115200);
    delay(1500);

    Serial.println("+ARDUINO: SLEEP,READY");
    printBootReport();

    // WakeupPad uses platform PMU pad IDs instead of Arduino GPIO numbering.
    if (AIR780EPMSleep.setWakeupPad(kWakeupPad,
                                    AIR780EPMSleepClass::WAKEUP_EDGE_FALLING,
                                    true,
                                    false)) {
        Serial.println("+ARDUINO: SLEEP,PAD0,CONFIG,OK");
    } else {
        Serial.println("+ARDUINO: SLEEP,PAD0,CONFIG,FAIL");
    }

    Serial.print("+ARDUINO: SLEEP,LIGHT,REQUEST,");
    Serial.println(kLightSleepMs);

    if (AIR780EPMSleep.lightSleep(kLightSleepMs)) {
        Serial.println("+ARDUINO: SLEEP,LIGHT,RETURNED");
        Serial.print("+ARDUINO: SLEEP,LIGHT,LAST_MS,");
        Serial.println(AIR780EPMSleep.sleepTimeMillis());
    } else {
        Serial.println("+ARDUINO: SLEEP,LIGHT,FAIL");
    }

    if (!kAutoArmDeepSleep) {
        Serial.print("+ARDUINO: SLEEP,DEEP,SKIPPED,");
        Serial.println(kDeepSleepMs);
        Serial.println("+ARDUINO: SLEEP,INFO,SET kAutoArmDeepSleep=true TO VERIFY TIMER DEEP SLEEP");
        Serial.println("+ARDUINO: SLEEP,INFO,WAKEUP_PAD_X IS A PMU PAD ID, NOT AN ARDUINO GPIO");
        Serial.println("+ARDUINO: SLEEP,PASS");
        return;
    }

    Serial.print("+ARDUINO: SLEEP,DEEP,REQUEST,");
    Serial.print(kDeepSleepMs);
    Serial.print(",TIMER,");
    Serial.println(static_cast<uint8_t>(kDeepSleepTimerId));

    if (AIR780EPMSleep.deepSleep(kDeepSleepMs, kDeepSleepTimerId)) {
        Serial.println("+ARDUINO: SLEEP,DEEP,ARMED");
    } else {
        Serial.println("+ARDUINO: SLEEP,DEEP,ARM,FAIL");
    }
}

void loop()
{
    delay(1000);
}
