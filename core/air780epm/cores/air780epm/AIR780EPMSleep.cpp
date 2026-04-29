#include "AIR780EPMSleep.h"

extern "C" {
#include "luat_gpio.h"
#include "luat_pm.h"
#include "platform_define.h"
#include "slpman.h"

typedef int32_t CmsRetId;

CmsRetId appSetCFUN(uint8_t fun);
CmsRetId appGetCFUN(uint8_t *pOutCfun);
}

extern "C" void delay(unsigned long ms);

namespace {

static const uint8_t kDeepSleepTimerMax = 6U;
static const uint32_t kInvalidRemainMs = 0xFFFFFFFFUL;
static const uint8_t kCfunOff = 0U;
static const uint32_t kCfunSettleMs = 2000UL;
static const CmsRetId kCmsRetSucc = 0;

static APmuWakeupPad_e toPlatformPad(AIR780EPMSleepClass::WakeupPad pad)
{
    return static_cast<APmuWakeupPad_e>(static_cast<uint8_t>(pad));
}

static slpManTimerID_e toPlatformTimer(AIR780EPMSleepClass::DeepSleepTimerId timerId)
{
    return static_cast<slpManTimerID_e>(static_cast<uint8_t>(timerId));
}

static int toHalWakeupPin(AIR780EPMSleepClass::WakeupPad pad)
{
    return HAL_WAKEUP_0 + static_cast<int>(static_cast<uint8_t>(pad));
}

static uint8_t toLuatIrqType(AIR780EPMSleepClass::WakeupEdge edge)
{
    switch (edge) {
        case AIR780EPMSleepClass::WAKEUP_EDGE_RISING:
            return LUAT_GPIO_RISING_IRQ;
        case AIR780EPMSleepClass::WAKEUP_EDGE_FALLING:
            return LUAT_GPIO_FALLING_IRQ;
        case AIR780EPMSleepClass::WAKEUP_EDGE_BOTH:
            return LUAT_GPIO_BOTH_IRQ;
        default:
            return LUAT_GPIO_NO_IRQ;
    }
}

static APmuWakeupPadSettings_t makePadSettings(AIR780EPMSleepClass::WakeupEdge edge,
                                               bool pullup,
                                               bool pulldown)
{
    APmuWakeupPadSettings_t settings = {false, false, false, false};

    settings.posEdgeEn = (edge == AIR780EPMSleepClass::WAKEUP_EDGE_RISING) ||
                         (edge == AIR780EPMSleepClass::WAKEUP_EDGE_BOTH);
    settings.negEdgeEn = (edge == AIR780EPMSleepClass::WAKEUP_EDGE_FALLING) ||
                         (edge == AIR780EPMSleepClass::WAKEUP_EDGE_BOTH);
    settings.pullUpEn = pullup;
    settings.pullDownEn = pulldown;

    return settings;
}

static bool padSettingsEqual(const APmuWakeupPadSettings_t &lhs,
                             const APmuWakeupPadSettings_t &rhs)
{
    return lhs.posEdgeEn == rhs.posEdgeEn &&
           lhs.negEdgeEn == rhs.negEdgeEn &&
           lhs.pullUpEn == rhs.pullUpEn &&
           lhs.pullDownEn == rhs.pullDownEn;
}

static void clearOtherDeepSleepTimers(uint8_t keepTimerId)
{
    uint8_t timerId = 0U;

    for (timerId = 0U; timerId <= kDeepSleepTimerMax; ++timerId) {
        if (timerId == keepTimerId) {
            continue;
        }

        if (slpManDeepSlpTimerIsRunning(timerId)) {
            slpManDeepSlpTimerDel(timerId);
        }
    }
}

static bool setCfunOffBeforeDeepSleep(void)
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

    delay(kCfunSettleMs);
    return true;
}

}  // namespace

bool AIR780EPMSleepClass::lightSleep(uint32_t ms) const
{
    if (ms == 0U) {
        return false;
    }

    if (luat_pm_request(LUAT_PM_SLEEP_MODE_LIGHT) != 0) {
        return false;
    }

    delay(static_cast<unsigned long>(ms));
    (void)luat_pm_release(LUAT_PM_SLEEP_MODE_LIGHT);
    return true;
}

bool AIR780EPMSleepClass::deepSleep(uint32_t ms, DeepSleepTimerId timerId) const
{
    uint32_t remainMs = 0U;
    const uint8_t rawTimerId = static_cast<uint8_t>(timerId);

    if (ms == 0U || !validTimerId(timerId)) {
        return false;
    }

    if (!setCfunOffBeforeDeepSleep()) {
        return false;
    }

    slpManSetPmuSleepMode(true, SLP_SLP2_STATE, false);
    clearOtherDeepSleepTimers(rawTimerId);

    if (slpManDeepSlpTimerIsRunning(rawTimerId)) {
        slpManDeepSlpTimerDel(rawTimerId);
    }

    slpManDeepSlpTimerStart(toPlatformTimer(timerId), ms);
    remainMs = slpManDeepSlpTimerRemainMs(rawTimerId);
    if (remainMs == 0U || remainMs == kInvalidRemainMs) {
        return false;
    }

    if (luat_pm_force(LUAT_PM_SLEEP_MODE_DEEP) != 0) {
        slpManDeepSlpTimerDel(rawTimerId);
        return false;
    }

    delay(1UL);
    return true;
}

bool AIR780EPMSleepClass::setWakeupPad(WakeupPad pad,
                                       WakeupEdge edge,
                                       bool pullup,
                                       bool pulldown) const
{
    APmuWakeupPadSettings_t requested;
    APmuWakeupPadSettings_t actual;
    luat_gpio_cfg_t gpioCfg;
    bool enabled = false;

    if (!validPad(pad) || edge == WAKEUP_EDGE_NONE || (pullup && pulldown)) {
        return false;
    }

    luat_gpio_set_default_cfg(&gpioCfg);
    gpioCfg.pin = toHalWakeupPin(pad);
    gpioCfg.mode = LUAT_GPIO_IRQ;
    gpioCfg.pull = pullup ? LUAT_GPIO_PULLUP :
                   (pulldown ? LUAT_GPIO_PULLDOWN : LUAT_GPIO_DEFAULT);
    gpioCfg.irq_type = toLuatIrqType(edge);
    gpioCfg.irq_cb = NULL;
    gpioCfg.irq_args = NULL;
    gpioCfg.output_level = 0;

    if (luat_gpio_open(&gpioCfg) != 0) {
        return false;
    }

    requested = makePadSettings(edge, pullup, pulldown);
    actual = makePadSettings(WAKEUP_EDGE_NONE, false, false);
    slpManGetWakeupPadCfg(toPlatformPad(pad), &enabled, &actual);

    return enabled && padSettingsEqual(requested, actual);
}

bool AIR780EPMSleepClass::clearWakeupPad(WakeupPad pad) const
{
    APmuWakeupPadSettings_t requested = makePadSettings(WAKEUP_EDGE_NONE, false, false);
    APmuWakeupPadSettings_t actual = requested;
    bool enabled = true;

    if (!validPad(pad)) {
        return false;
    }

    luat_gpio_close(toHalWakeupPin(pad));
    slpManGetWakeupPadCfg(toPlatformPad(pad), &enabled, &actual);

    return !enabled && padSettingsEqual(requested, actual);
}

AIR780EPMSleepClass::WakeupReason AIR780EPMSleepClass::wakeupReason(void) const
{
    switch (slpManGetWakeupSrc()) {
        case WAKEUP_FROM_POR:
            return WAKEUP_FROM_POR;
        case WAKEUP_FROM_RTC:
            return WAKEUP_FROM_RTC;
        case WAKEUP_FROM_PAD:
            return WAKEUP_FROM_PAD;
        case WAKEUP_FROM_LPUART:
            return WAKEUP_FROM_LPUART;
        case WAKEUP_FROM_LPUSB:
            return WAKEUP_FROM_LPUSB;
        case WAKEUP_FROM_PWRKEY:
            return WAKEUP_FROM_PWRKEY;
        case WAKEUP_FROM_CHARG:
            return WAKEUP_FROM_CHARGER;
        default:
            return WAKEUP_FROM_UNKNOWN;
    }
}

AIR780EPMSleepClass::SleepState AIR780EPMSleepClass::lastSleepState(void) const
{
    switch (slpManGetLastSlpState()) {
        case SLP_ACTIVE_STATE:
            return SLEEP_STATE_ACTIVE;
        case SLP_IDLE_STATE:
            return SLEEP_STATE_IDLE;
        case SLP_SLP1_STATE:
            return SLEEP_STATE_LIGHT;
        case SLP_SLP2_STATE:
            return SLEEP_STATE_DEEP;
        case SLP_HIB_STATE:
            return SLEEP_STATE_HIBERNATE;
        default:
            return SLEEP_STATE_INVALID;
    }
}

uint32_t AIR780EPMSleepClass::sleepTimeMillis(void) const
{
    const slpManSlpState_t state = slpManGetLastSlpState();

    if (state == SLP_ACTIVE_STATE || state == SLP_IDLE_STATE) {
        return 0U;
    }

    return slpManGetSleepTime();
}

uint8_t AIR780EPMSleepClass::wakeupPinBitmap(void) const
{
    return slpManGetWakeupPinValue();
}

bool AIR780EPMSleepClass::validPad(WakeupPad pad) const
{
    return static_cast<uint8_t>(pad) <= static_cast<uint8_t>(WAKEUP_PAD_5);
}

bool AIR780EPMSleepClass::validTimerId(DeepSleepTimerId timerId) const
{
    return static_cast<uint8_t>(timerId) <= kDeepSleepTimerMax;
}

AIR780EPMSleepClass AIR780EPMSleep;
