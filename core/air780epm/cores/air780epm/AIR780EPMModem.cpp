#include "AIR780EPMModem.h"

#include <string.h>

#include "arduino_modem_io.h"

extern "C" unsigned long millis(void);
extern "C" void delay(unsigned long ms);

AIR780EPMModem Modem;

AIR780EPMModemStatus::AIR780EPMModemStatus() :
    psReady(false),
    ceregState(0U),
    registered(false),
    netInfoValid(false),
    netMgrResult(1U),
    netStatus(0U),
    ipType(0U),
    networkReady(false),
    ipv4Cid(0xFFU),
    ipv6Cid(0xFFU),
    hasIPv4(false),
    localIPv4(),
    dnsCount(0U)
{
}

AIR780EPMModemIdentity::AIR780EPMModemIdentity() :
    imeiValid(false),
    imsiValid(false),
    iccidValid(false),
    iccidRawValid(false),
    phoneNumberValid(false),
    snValid(false),
    simIdValid(false),
    imei(),
    imsi(),
    iccid(),
    iccidRaw(),
    phoneNumber(),
    sn(),
    phoneNumberType(0U),
    simId(0U)
{
}

AIR780EPMSignalQuality::AIR780EPMSignalQuality() :
    valid(false),
    csq(99U),
    rssiValid(false),
    rssiDbm(0),
    rsrpValid(false),
    rsrpRaw(127),
    rsrpDbm(0),
    rsrqValid(false),
    rsrqRaw(127),
    rsrqDb(0),
    snrValid(false),
    snrDb(0)
{
}

AIR780EPMCellInfo::AIR780EPMCellInfo() :
    valid(false),
    mcc(0U),
    mnc(0U),
    mncDigits(0U),
    tac(0U),
    cid(0U),
    cellId(0U),
    earfcn(0U),
    physicalCellId(0U),
    band(0U),
    snrValid(false),
    snrDb(0),
    rsrpDbm(0),
    rsrqDb(0),
    neighborCount(0U)
{
}

AIR780EPMTimeStatus::AIR780EPMTimeStatus() :
    valid(false),
    synced(false),
    nitzSynced(false),
    epoch(0U),
    milliseconds(0U),
    timezoneQuarterHours(0),
    year(0U),
    month(0U),
    day(0U),
    hour(0U),
    minute(0U),
    second(0U)
{
}

bool AIR780EPMModem::begin()
{
    return true;
}

bool AIR780EPMModem::getStatus(AIR780EPMModemStatus &status) const
{
    ArduinoCoreModemStatus nativeStatus;
    memset(&nativeStatus, 0, sizeof(nativeStatus));

    if (arduinoCoreModemGetStatus(&nativeStatus) != 0)
    {
        status = AIR780EPMModemStatus();
        return false;
    }

    status.psReady = (nativeStatus.psReady != 0U);
    status.ceregState = nativeStatus.ceregState;
    status.registered = (nativeStatus.registered != 0U);
    status.netInfoValid = (nativeStatus.netInfoValid != 0U);
    status.netMgrResult = nativeStatus.netMgrResult;
    status.netStatus = nativeStatus.netStatus;
    status.ipType = nativeStatus.ipType;
    status.networkReady = (nativeStatus.networkReady != 0U);
    status.ipv4Cid = nativeStatus.ipv4Cid;
    status.ipv6Cid = nativeStatus.ipv6Cid;
    status.hasIPv4 = (nativeStatus.hasIPv4 != 0U);
    status.localIPv4 = nativeStatus.ipv4;
    status.dnsCount = nativeStatus.dnsCount;
    if (status.dnsCount > 2U)
    {
        status.dnsCount = 2U;
    }

    for (uint8_t index = 0U; index < status.dnsCount; index++)
    {
        status.dns[index] = nativeStatus.dns[index];
    }

    return true;
}

bool AIR780EPMModem::getIdentity(AIR780EPMModemIdentity &identity) const
{
    ArduinoCoreModemIdentity nativeIdentity;
    memset(&nativeIdentity, 0, sizeof(nativeIdentity));

    if (arduinoCoreModemGetIdentity(&nativeIdentity) != 0)
    {
        identity = AIR780EPMModemIdentity();
        return false;
    }

    identity = AIR780EPMModemIdentity();
    identity.imeiValid = (nativeIdentity.imeiValid != 0U);
    identity.imsiValid = (nativeIdentity.imsiValid != 0U);
    identity.iccidValid = (nativeIdentity.iccidValid != 0U);
    identity.iccidRawValid = (nativeIdentity.iccidRawValid != 0U);
    identity.phoneNumberValid = (nativeIdentity.phoneNumberValid != 0U);
    identity.snValid = (nativeIdentity.snValid != 0U);
    identity.simIdValid = (nativeIdentity.simIdValid != 0U);
    strncpy(identity.imei, nativeIdentity.imei, sizeof(identity.imei) - 1U);
    strncpy(identity.imsi, nativeIdentity.imsi, sizeof(identity.imsi) - 1U);
    strncpy(identity.iccid, nativeIdentity.iccid, sizeof(identity.iccid) - 1U);
    strncpy(identity.iccidRaw, nativeIdentity.iccidRaw, sizeof(identity.iccidRaw) - 1U);
    strncpy(identity.phoneNumber, nativeIdentity.phoneNumber, sizeof(identity.phoneNumber) - 1U);
    strncpy(identity.sn, nativeIdentity.sn, sizeof(identity.sn) - 1U);
    identity.phoneNumberType = nativeIdentity.phoneNumberType;
    identity.simId = nativeIdentity.simId;

    return identity.imeiValid || identity.imsiValid || identity.iccidValid ||
           identity.iccidRawValid || identity.phoneNumberValid ||
           identity.snValid || identity.simIdValid;
}

bool AIR780EPMModem::getSignalQuality(AIR780EPMSignalQuality &signal) const
{
    ArduinoCoreModemSignalQuality nativeSignal;
    memset(&nativeSignal, 0, sizeof(nativeSignal));

    if (arduinoCoreModemGetSignalQuality(&nativeSignal) != 0)
    {
        signal = AIR780EPMSignalQuality();
        return false;
    }

    signal.valid = (nativeSignal.valid != 0U);
    signal.csq = nativeSignal.csq;
    signal.rssiValid = (nativeSignal.rssiValid != 0U);
    signal.rssiDbm = nativeSignal.rssiDbm;
    signal.rsrpValid = (nativeSignal.rsrpValid != 0U);
    signal.rsrpRaw = nativeSignal.rsrpRaw;
    signal.rsrpDbm = nativeSignal.rsrpDbm;
    signal.rsrqValid = (nativeSignal.rsrqValid != 0U);
    signal.rsrqRaw = nativeSignal.rsrqRaw;
    signal.rsrqDb = nativeSignal.rsrqDb;
    signal.snrValid = (nativeSignal.snrValid != 0U);
    signal.snrDb = nativeSignal.snrDb;

    return signal.valid;
}

bool AIR780EPMModem::getCellInfo(AIR780EPMCellInfo &cell) const
{
    ArduinoCoreModemCellInfo nativeCell;
    memset(&nativeCell, 0, sizeof(nativeCell));

    if (arduinoCoreModemGetCellInfo(&nativeCell) != 0)
    {
        cell = AIR780EPMCellInfo();
        return false;
    }

    cell.valid = (nativeCell.valid != 0U);
    cell.mcc = nativeCell.mcc;
    cell.mnc = nativeCell.mnc;
    cell.mncDigits = nativeCell.mncDigits;
    cell.tac = nativeCell.tac;
    cell.cid = nativeCell.cellId;
    cell.cellId = nativeCell.cellId;
    cell.earfcn = nativeCell.earfcn;
    cell.physicalCellId = nativeCell.physicalCellId;
    cell.band = nativeCell.band;
    cell.snrValid = (nativeCell.snrValid != 0U);
    cell.snrDb = nativeCell.snrDb;
    cell.rsrpDbm = nativeCell.rsrpDbm;
    cell.rsrqDb = nativeCell.rsrqDb;
    cell.neighborCount = nativeCell.neighborCount;

    return cell.valid;
}

bool AIR780EPMModem::getTimeStatus(AIR780EPMTimeStatus &status) const
{
    ArduinoCoreModemTimeStatus nativeStatus;
    memset(&nativeStatus, 0, sizeof(nativeStatus));

    if (arduinoCoreModemGetTimeStatus(&nativeStatus) != 0)
    {
        status = AIR780EPMTimeStatus();
        return false;
    }

    status.valid = (nativeStatus.valid != 0U);
    status.synced = (nativeStatus.synced != 0U);
    status.nitzSynced = (nativeStatus.nitzSynced != 0U);
    status.epoch = nativeStatus.epoch;
    status.milliseconds = nativeStatus.milliseconds;
    status.timezoneQuarterHours = nativeStatus.timezoneQuarterHours;
    status.year = nativeStatus.year;
    status.month = nativeStatus.month;
    status.day = nativeStatus.day;
    status.hour = nativeStatus.hour;
    status.minute = nativeStatus.minute;
    status.second = nativeStatus.second;

    return true;
}

bool AIR780EPMModem::getAPN(char *out, size_t outSize, uint8_t cid) const
{
    if ((out == NULL) || (outSize == 0U))
    {
        return false;
    }

    out[0] = '\0';
    return arduinoCoreModemGetAPN(cid, out, outSize) == 0;
}

bool AIR780EPMModem::setAPN(const char *apn, uint8_t cid, uint8_t pdnType)
{
    return arduinoCoreModemSetAPN(cid, apn, pdnType) == 0;
}

bool AIR780EPMModem::getPhoneNumber(char *out, size_t outSize) const
{
    if ((out == NULL) || (outSize == 0U))
    {
        return false;
    }

    out[0] = '\0';

    AIR780EPMModemIdentity identity;
    if (!getIdentity(identity) || !identity.phoneNumberValid)
    {
        return false;
    }

    strncpy(out, identity.phoneNumber, outSize - 1U);
    out[outSize - 1U] = '\0';
    return true;
}

bool AIR780EPMModem::isRegistered() const
{
    AIR780EPMModemStatus status;
    return getStatus(status) && status.registered;
}

bool AIR780EPMModem::isNetworkReady() const
{
    AIR780EPMModemStatus status;
    return getStatus(status) && status.networkReady;
}

bool AIR780EPMModem::isTimeSynced() const
{
    AIR780EPMTimeStatus status;
    return getTimeStatus(status) && status.valid && status.synced;
}

bool AIR780EPMModem::isNetworkTimeSynced() const
{
    AIR780EPMTimeStatus status;
    return getTimeStatus(status) && status.valid && status.nitzSynced;
}

bool AIR780EPMModem::waitForNetwork(uint32_t timeoutMs) const
{
    const unsigned long start = millis();

    do
    {
        if (isRegistered())
        {
            return true;
        }

        delay(500UL);
    } while ((millis() - start) < timeoutMs);

    return isRegistered();
}

bool AIR780EPMModem::waitForTimeSync(uint32_t timeoutMs) const
{
    const unsigned long start = millis();

    do
    {
        if (isTimeSynced())
        {
            return true;
        }

        delay(250UL);
    } while ((millis() - start) < timeoutMs);

    return isTimeSynced();
}

bool AIR780EPMModem::waitForNetworkTime(uint32_t timeoutMs) const
{
    const unsigned long start = millis();

    do
    {
        if (isNetworkTimeSynced())
        {
            return true;
        }

        delay(250UL);
    } while ((millis() - start) < timeoutMs);

    return isNetworkTimeSynced();
}

bool AIR780EPMModem::activatePDP(uint32_t timeoutMs)
{
    const unsigned long start = millis();

    do
    {
        if (isNetworkReady())
        {
            return true;
        }

        delay(500UL);
    } while ((millis() - start) < timeoutMs);

    return isNetworkReady();
}

uint8_t AIR780EPMModem::ceregState() const
{
    AIR780EPMModemStatus status;
    if (!getStatus(status))
    {
        return 0U;
    }

    return status.ceregState;
}

IPAddress AIR780EPMModem::localIP() const
{
    AIR780EPMModemStatus status;
    if (!getStatus(status) || !status.hasIPv4)
    {
        return IPAddress();
    }

    return status.localIPv4;
}

uint32_t AIR780EPMModem::epochTime() const
{
    AIR780EPMTimeStatus status;
    if (!getTimeStatus(status) || !status.valid)
    {
        return 0U;
    }

    return status.epoch;
}

int8_t AIR780EPMModem::timeZoneQuarterHours() const
{
    AIR780EPMTimeStatus status;
    if (!getTimeStatus(status) || !status.valid)
    {
        return 0;
    }

    return status.timezoneQuarterHours;
}

int AIR780EPMModem::csq() const
{
    AIR780EPMSignalQuality signal;
    if (!getSignalQuality(signal))
    {
        return 99;
    }

    return signal.csq;
}

int AIR780EPMModem::rssi() const
{
    AIR780EPMSignalQuality signal;
    if (!getSignalQuality(signal) || !signal.rssiValid)
    {
        return 0;
    }

    return signal.rssiDbm;
}

String AIR780EPMModem::imei() const
{
    AIR780EPMModemIdentity identity;
    if (!getIdentity(identity) || !identity.imeiValid)
    {
        return String();
    }

    return String(identity.imei);
}

String AIR780EPMModem::imsi() const
{
    AIR780EPMModemIdentity identity;
    if (!getIdentity(identity) || !identity.imsiValid)
    {
        return String();
    }

    return String(identity.imsi);
}

String AIR780EPMModem::iccid() const
{
    AIR780EPMModemIdentity identity;
    if (!getIdentity(identity) || !identity.iccidValid)
    {
        return String();
    }

    return String(identity.iccid);
}

String AIR780EPMModem::iccidRaw() const
{
    AIR780EPMModemIdentity identity;
    if (!getIdentity(identity) || !identity.iccidRawValid)
    {
        return String();
    }

    return String(identity.iccidRaw);
}

String AIR780EPMModem::phoneNumber() const
{
    AIR780EPMModemIdentity identity;
    if (!getIdentity(identity) || !identity.phoneNumberValid)
    {
        return String();
    }

    return String(identity.phoneNumber);
}

String AIR780EPMModem::sn() const
{
    AIR780EPMModemIdentity identity;
    if (!getIdentity(identity) || !identity.snValid)
    {
        return String();
    }

    return String(identity.sn);
}

String AIR780EPMModem::apn(uint8_t cid) const
{
    char value[101];
    memset(value, 0, sizeof(value));

    if (!getAPN(value, sizeof(value), cid))
    {
        return String();
    }

    return String(value);
}
