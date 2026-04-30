#include "arduino_sleep_io.h"

#include <stddef.h>

#include "arduino_time_io.h"
#include "luat_gpio.h"
#include "luat_pm.h"
#include "platform_define.h"
#include "slpman.h"

typedef int32_t CmsRetId;

CmsRetId appSetCFUN(uint8_t fun);
CmsRetId appGetCFUN(uint8_t *pOutCfun);

enum {
    ARDUINO_SLEEP_WAKEUP_EDGE_NONE = 0,
    ARDUINO_SLEEP_WAKEUP_EDGE_RISING = 1,
    ARDUINO_SLEEP_WAKEUP_EDGE_FALLING = 2,
    ARDUINO_SLEEP_WAKEUP_EDGE_BOTH = 3
};

enum {
    ARDUINO_SLEEP_WAKEUP_FROM_POR = 0,
    ARDUINO_SLEEP_WAKEUP_FROM_RTC = 1,
    ARDUINO_SLEEP_WAKEUP_FROM_PAD = 2,
    ARDUINO_SLEEP_WAKEUP_FROM_LPUART = 3,
    ARDUINO_SLEEP_WAKEUP_FROM_LPUSB = 4,
    ARDUINO_SLEEP_WAKEUP_FROM_PWRKEY = 5,
    ARDUINO_SLEEP_WAKEUP_FROM_CHARGER = 6,
    ARDUINO_SLEEP_WAKEUP_FROM_UNKNOWN = 255
};

enum {
    ARDUINO_SLEEP_STATE_ACTIVE = 0,
    ARDUINO_SLEEP_STATE_IDLE = 1,
    ARDUINO_SLEEP_STATE_LIGHT = 2,
    ARDUINO_SLEEP_STATE_DEEP = 3,
    ARDUINO_SLEEP_STATE_HIBERNATE = 4,
    ARDUINO_SLEEP_STATE_INVALID = 255
};

static const uint8_t kWakeupPadMax = 5U;
static const uint8_t kDeepSleepTimerMax = 6U;
static const uint32_t kInvalidRemainMs = 0xFFFFFFFFUL;
static const uint8_t kCfunOff = 0U;
static const uint32_t kCfunSettleMs = 2000UL;
static const CmsRetId kCmsRetSucc = 0;

static APmuWakeupPad_e arduinoSleepToPlatformPad(uint8_t pad)
{
    return (APmuWakeupPad_e)pad;
}

static slpManTimerID_e arduinoSleepToPlatformTimer(uint8_t timerId)
{
    return (slpManTimerID_e)timerId;
}

static int arduinoSleepToHalWakeupPin(uint8_t pad)
{
    return HAL_WAKEUP_0 + (int)pad;
}

static uint8_t arduinoSleepToLuatIrqType(uint8_t edge)
{
    switch (edge) {
        case ARDUINO_SLEEP_WAKEUP_EDGE_RISING:
            return LUAT_GPIO_RISING_IRQ;
        case ARDUINO_SLEEP_WAKEUP_EDGE_FALLING:
            return LUAT_GPIO_FALLING_IRQ;
        case ARDUINO_SLEEP_WAKEUP_EDGE_BOTH:
            return LUAT_GPIO_BOTH_IRQ;
        default:
            return LUAT_GPIO_NO_IRQ;
    }
}

static APmuWakeupPadSettings_t arduinoSleepMakePadSettings(uint8_t edge,
                                                           uint8_t pullup,
                                                           uint8_t pulldown)
{
    APmuWakeupPadSettings_t settings = {false, false, false, false};

    settings.posEdgeEn = (edge == ARDUINO_SLEEP_WAKEUP_EDGE_RISING) ||
                         (edge == ARDUINO_SLEEP_WAKEUP_EDGE_BOTH);
    settings.negEdgeEn = (edge == ARDUINO_SLEEP_WAKEUP_EDGE_FALLING) ||
                         (edge == ARDUINO_SLEEP_WAKEUP_EDGE_BOTH);
    settings.pullUpEn = pullup != 0U;
    settings.pullDownEn = pulldown != 0U;

    return settings;
}

static bool arduinoSleepPadSettingsEqual(const APmuWakeupPadSettings_t *lhs,
                                         const APmuWakeupPadSettings_t *rhs)
{
    return lhs->posEdgeEn == rhs->posEdgeEn &&
           lhs->negEdgeEn == rhs->negEdgeEn &&
           lhs->pullUpEn == rhs->pullUpEn &&
           lhs->pullDownEn == rhs->pullDownEn;
}

static void arduinoSleepClearOtherDeepSleepTimers(uint8_t keepTimerId)
{
    uint8_t timerId;

    for (timerId = 0U; timerId <= kDeepSleepTimerMax; ++timerId) {
        if (timerId == keepTimerId) {
            continue;
        }

        if (slpManDeepSlpTimerIsRunning(timerId)) {
            slpManDeepSlpTimerDel(timerId);
        }
    }
}

static bool arduinoSleepSetCfunOffBeforeDeepSleep(void)
{
    uint8_t cfun = 0U;
    CmsRetId ret = appGetCFUN(&cfun);

    if (ret != kCmsRetSucc) {
        return false;
    }

    if (cfun == kCfunOff) {
        return true;
    }

    ret = appSetCFUN(kCfunOff);
    if (ret != kCmsRetSucc) {
        return false;
    }

    arduinoCoreDelayMs(kCfunSettleMs);
    return true;
}

bool arduinoCoreSleepLight(uint32_t milliseconds)
{
    if (milliseconds == 0U) {
        return false;
    }

    if (luat_pm_request(LUAT_PM_SLEEP_MODE_LIGHT) != 0) {
        return false;
    }

    arduinoCoreDelayMs(milliseconds);
    (void)luat_pm_release(LUAT_PM_SLEEP_MODE_LIGHT);
    return true;
}

bool arduinoCoreSleepDeep(uint32_t milliseconds, uint8_t timerId)
{
    uint32_t remainMs = 0U;

    if (milliseconds == 0U || timerId > kDeepSleepTimerMax) {
        return false;
    }

    if (!arduinoSleepSetCfunOffBeforeDeepSleep()) {
        return false;
    }

    slpManSetPmuSleepMode(true, SLP_SLP2_STATE, false);
    arduinoSleepClearOtherDeepSleepTimers(timerId);

    if (slpManDeepSlpTimerIsRunning(timerId)) {
        slpManDeepSlpTimerDel(timerId);
    }

    slpManDeepSlpTimerStart(arduinoSleepToPlatformTimer(timerId), milliseconds);
    remainMs = slpManDeepSlpTimerRemainMs(timerId);
    if (remainMs == 0U || remainMs == kInvalidRemainMs) {
        return false;
    }

    if (luat_pm_force(LUAT_PM_SLEEP_MODE_DEEP) != 0) {
        slpManDeepSlpTimerDel(timerId);
        return false;
    }

    arduinoCoreDelayMs(1U);
    return true;
}

bool arduinoCoreSleepSetWakeupPad(uint8_t pad,
                                  uint8_t edge,
                                  uint8_t pullup,
                                  uint8_t pulldown)
{
    APmuWakeupPadSettings_t requested;
    APmuWakeupPadSettings_t actual;
    luat_gpio_cfg_t gpioCfg;
    bool enabled = false;

    if (pad > kWakeupPadMax || edge == ARDUINO_SLEEP_WAKEUP_EDGE_NONE ||
        (pullup != 0U && pulldown != 0U)) {
        return false;
    }

    luat_gpio_set_default_cfg(&gpioCfg);
    gpioCfg.pin = arduinoSleepToHalWakeupPin(pad);
    gpioCfg.mode = LUAT_GPIO_IRQ;
    gpioCfg.pull = (pullup != 0U) ? LUAT_GPIO_PULLUP :
                   ((pulldown != 0U) ? LUAT_GPIO_PULLDOWN : LUAT_GPIO_DEFAULT);
    gpioCfg.irq_type = arduinoSleepToLuatIrqType(edge);
    gpioCfg.irq_cb = NULL;
    gpioCfg.irq_args = NULL;
    gpioCfg.output_level = 0;

    if (luat_gpio_open(&gpioCfg) != 0) {
        return false;
    }

    requested = arduinoSleepMakePadSettings(edge, pullup, pulldown);
    actual = arduinoSleepMakePadSettings(ARDUINO_SLEEP_WAKEUP_EDGE_NONE, 0U, 0U);
    slpManGetWakeupPadCfg(arduinoSleepToPlatformPad(pad), &enabled, &actual);

    return enabled && arduinoSleepPadSettingsEqual(&requested, &actual);
}

bool arduinoCoreSleepClearWakeupPad(uint8_t pad)
{
    APmuWakeupPadSettings_t requested = arduinoSleepMakePadSettings(ARDUINO_SLEEP_WAKEUP_EDGE_NONE, 0U, 0U);
    APmuWakeupPadSettings_t actual = requested;
    bool enabled = true;

    if (pad > kWakeupPadMax) {
        return false;
    }

    luat_gpio_close(arduinoSleepToHalWakeupPin(pad));
    slpManGetWakeupPadCfg(arduinoSleepToPlatformPad(pad), &enabled, &actual);

    return !enabled && arduinoSleepPadSettingsEqual(&requested, &actual);
}

uint8_t arduinoCoreSleepWakeupReason(void)
{
    switch (slpManGetWakeupSrc()) {
        case WAKEUP_FROM_POR:
            return ARDUINO_SLEEP_WAKEUP_FROM_POR;
        case WAKEUP_FROM_RTC:
            return ARDUINO_SLEEP_WAKEUP_FROM_RTC;
        case WAKEUP_FROM_PAD:
            return ARDUINO_SLEEP_WAKEUP_FROM_PAD;
        case WAKEUP_FROM_LPUART:
            return ARDUINO_SLEEP_WAKEUP_FROM_LPUART;
        case WAKEUP_FROM_LPUSB:
            return ARDUINO_SLEEP_WAKEUP_FROM_LPUSB;
        case WAKEUP_FROM_PWRKEY:
            return ARDUINO_SLEEP_WAKEUP_FROM_PWRKEY;
        case WAKEUP_FROM_CHARG:
            return ARDUINO_SLEEP_WAKEUP_FROM_CHARGER;
        default:
            return ARDUINO_SLEEP_WAKEUP_FROM_UNKNOWN;
    }
}

uint8_t arduinoCoreSleepLastState(void)
{
    switch (slpManGetLastSlpState()) {
        case SLP_ACTIVE_STATE:
            return ARDUINO_SLEEP_STATE_ACTIVE;
        case SLP_IDLE_STATE:
            return ARDUINO_SLEEP_STATE_IDLE;
        case SLP_SLP1_STATE:
            return ARDUINO_SLEEP_STATE_LIGHT;
        case SLP_SLP2_STATE:
            return ARDUINO_SLEEP_STATE_DEEP;
        case SLP_HIB_STATE:
            return ARDUINO_SLEEP_STATE_HIBERNATE;
        default:
            return ARDUINO_SLEEP_STATE_INVALID;
    }
}

uint32_t arduinoCoreSleepTimeMillis(void)
{
    const slpManSlpState_t state = slpManGetLastSlpState();

    if (state == SLP_ACTIVE_STATE || state == SLP_IDLE_STATE) {
        return 0U;
    }

    return slpManGetSleepTime();
}

uint8_t arduinoCoreSleepWakeupPinBitmap(void)
{
    return slpManGetWakeupPinValue();
}
