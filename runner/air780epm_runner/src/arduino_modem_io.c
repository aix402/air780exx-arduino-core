#include "arduino_modem_io.h"

#include <string.h>
#include <time.h>

#include "arduino_time_io.h"
#include "cmips.h"
#include "osasys.h"
#include "ps_lib_api.h"
#include "lwip/ip4_addr.h"
#include "lwip_config_cat.h"
#include "networkmgr.h"
#include "psdial.h"
#include "psdial_ps_ctrl.h"

#define ARDUINO_MODEM_TIME_VALID_EPOCH_MIN 1577836800UL

extern CmsRetId appSetEpsBeaerParamSync(SetPsBearerParams *pSetEpsParams);
extern void soc_mobile_get_sim_number(uint8_t *buf);
extern void soc_mobile_get_sim_id(uint8_t *sim_id, uint8_t *is_auto);

static uint8_t arduinoModemStatusHasIpv4(uint8_t ipType)
{
    return (ipType == NM_NET_TYPE_IPV4) ||
           (ipType == NM_NET_TYPE_IPV4V6) ||
           (ipType == NM_NET_TYPE_IPV4_IPV6preparing);
}

static uint8_t arduinoModemStatusIsRegistered(uint8_t ceregState)
{
    return (ceregState == CMI_PS_REG_HOME) || (ceregState == CMI_PS_REG_ROAMING);
}

static void arduinoModemCopyIpv4(uint8_t out[4], const ip4_addr_t *address)
{
    out[0] = ip4_addr1(address);
    out[1] = ip4_addr2(address);
    out[2] = ip4_addr3(address);
    out[3] = ip4_addr4(address);
}

static uint8_t arduinoModemStringHasAnyChar(const char *value)
{
    return (value != NULL && value[0] != '\0') ? 1U : 0U;
}

static void arduinoModemTerminatePrintable(char *value, size_t valueSize)
{
    size_t index;

    if ((value == NULL) || (valueSize == 0U))
    {
        return;
    }

    value[valueSize - 1U] = '\0';
    for (index = 0U; index < valueSize; index++)
    {
        const unsigned char ch = (unsigned char)value[index];
        if (ch == '\0')
        {
            return;
        }

        if ((ch < 0x20U) || (ch > 0x7EU))
        {
            value[index] = '\0';
            return;
        }
    }
}

static void arduinoModemCopyBoundedString(char *out, size_t outSize, const char *in)
{
    if ((out == NULL) || (outSize == 0U))
    {
        return;
    }

    memset(out, 0, outSize);
    if (in != NULL)
    {
        strncpy(out, in, outSize - 1U);
        arduinoModemTerminatePrintable(out, outSize);
    }
}

static uint8_t arduinoModemValidateApn(const char *apn, size_t *apnLength)
{
    size_t index = 0U;

    if (apn == NULL)
    {
        return 0U;
    }

    while (apn[index] != '\0')
    {
        const unsigned char ch = (unsigned char)apn[index];
        const uint8_t isAlpha = (((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z'))) ? 1U : 0U;
        const uint8_t isDigit = ((ch >= '0') && (ch <= '9')) ? 1U : 0U;
        const uint8_t isAllowedSymbol = ((ch == '.') || (ch == '-') || (ch == '_')) ? 1U : 0U;

        if (index >= CMI_PS_MAX_APN_LEN)
        {
            return 0U;
        }

        if ((isAlpha == 0U) && (isDigit == 0U) && (isAllowedSymbol == 0U))
        {
            return 0U;
        }

        index++;
    }

    if (apnLength != NULL)
    {
        *apnLength = index;
    }

    return 1U;
}

static uint8_t arduinoModemPdnTypeIsValid(uint8_t pdnType)
{
    return ((pdnType == CMI_PS_PDN_TYPE_IP_V4) ||
            (pdnType == CMI_PS_PDN_TYPE_IP_V6) ||
            (pdnType == CMI_PS_PDN_TYPE_IP_V4V6) ||
            (pdnType == CMI_PS_PDN_TYPE_NON_IP)) ? 1U : 0U;
}

static uint8_t arduinoModemEpochLooksValid(uint32_t epoch)
{
    return (epoch >= ARDUINO_MODEM_TIME_VALID_EPOCH_MIN) ? 1U : 0U;
}

static int arduinoModemGetSubscriberNumber(char *number, size_t numberSize, uint8_t *numberType)
{
    uint8_t scratch[24] = {0};

    if ((number == NULL) || (numberSize == 0U))
    {
        return -1;
    }

    memset(number, 0, numberSize);
    if (numberType != NULL)
    {
        *numberType = 0U;
    }

    soc_mobile_get_sim_number(scratch);
    arduinoModemCopyBoundedString(number, numberSize, (const char *)scratch);
    return arduinoModemStringHasAnyChar(number) ? 0 : -4;
}

static uint16_t arduinoModemBcd3ToNumber(uint16_t value)
{
    return (uint16_t)((((value >> 8) & 0x0FU) * 100U) +
                      (((value >> 4) & 0x0FU) * 10U) +
                      (value & 0x0FU));
}

static uint16_t arduinoModemBcdMncToNumber(uint16_t mncWithAddInfo, uint8_t *digits)
{
    const uint16_t pureMnc = CAM_GET_PURE_MNC(mncWithAddInfo);

    if (CAM_IS_2_DIGIT_MNC(mncWithAddInfo))
    {
        if (digits != NULL)
        {
            *digits = 2U;
        }
        return (uint16_t)((((pureMnc >> 4) & 0x0FU) * 10U) +
                          (pureMnc & 0x0FU));
    }

    if (digits != NULL)
    {
        *digits = 3U;
    }
    return arduinoModemBcd3ToNumber(pureMnc);
}

static uint8_t arduinoModemRsrpRawIsValid(int8_t rsrpRaw)
{
    return ((rsrpRaw >= -17) && (rsrpRaw <= 97)) ? 1U : 0U;
}

static uint8_t arduinoModemRsrqRawIsValid(int8_t rsrqRaw)
{
    return ((rsrqRaw > -30) && (rsrqRaw <= 46)) ? 1U : 0U;
}

static int16_t arduinoModemRsrpRawToDbm(int8_t rsrpRaw)
{
    return (int16_t)rsrpRaw - 141;
}

static int16_t arduinoModemRsrqRawToDb(int8_t rsrqRaw)
{
    if (rsrqRaw <= 34)
    {
        return (int16_t)(((int16_t)rsrqRaw - 40) / 2);
    }

    return (int16_t)(((int16_t)rsrqRaw - 41) / 2);
}

static uint8_t arduinoModemCsqRssiIsValid(uint8_t csq)
{
    return (csq <= 31U) ? 1U : 0U;
}

static int16_t arduinoModemCsqToRssiDbm(uint8_t csq)
{
    if (csq == 0U)
    {
        return -113;
    }

    if (csq >= 31U)
    {
        return -51;
    }

    return (int16_t)(-113 + ((int16_t)csq * 2));
}

int arduinoCoreModemGetStatus(ArduinoCoreModemStatus *status)
{
    NmAtiNetifInfo netInfo;
    NmResult netResult;

    if (status == NULL)
    {
        return -1;
    }

    memset(status, 0, sizeof(*status));
    status->netMgrResult = NM_FAIL;
    status->ipv4Cid = LWIP_PS_INVALID_CID;
    status->ipv6Cid = LWIP_PS_INVALID_CID;

    status->psReady = (psDialPsIsReady() != 0) ? 1U : 0U;
    status->ceregState = psDialGetCeregState();
    status->registered = arduinoModemStatusIsRegistered(status->ceregState);

    memset(&netInfo, 0, sizeof(netInfo));
    netResult = NetMgrGetNetInfo(LWIP_PS_INVALID_CID, &netInfo);
    status->netMgrResult = (uint8_t)netResult;

    if (netResult != NM_SUCCESS)
    {
        return 0;
    }

    status->netInfoValid = 1U;
    status->netStatus = netInfo.netStatus;
    status->ipType = netInfo.ipType;
    status->ipv4Cid = netInfo.ipv4Cid;
    status->ipv6Cid = netInfo.ipv6Cid;
    status->networkReady = (netInfo.netStatus == NM_NETIF_ACTIVATED) ? 1U : 0U;

    if ((status->networkReady != 0U) &&
        (arduinoModemStatusHasIpv4(netInfo.ipType) != 0U) &&
        (netInfo.ipv4Cid != LWIP_PS_INVALID_CID))
    {
        status->hasIPv4 = 1U;
        arduinoModemCopyIpv4(status->ipv4, &netInfo.ipv4Info.ipv4Addr);

        status->dnsCount = netInfo.ipv4Info.dnsNum;
        if (status->dnsCount > 2U)
        {
            status->dnsCount = 2U;
        }

        for (uint8_t index = 0U; index < status->dnsCount; index++)
        {
            arduinoModemCopyIpv4(status->dns[index], &netInfo.ipv4Info.dns[index]);
        }
    }

    return 0;
}

int arduinoCoreModemGetIdentity(ArduinoCoreModemIdentity *identity)
{
    char scratch[CMI_PS_MAX_APN_LEN + 1U];
    uint8_t simId = 0xFFU;
    uint8_t simAuto = 0U;

    if (identity == NULL)
    {
        return -1;
    }

    memset(identity, 0, sizeof(*identity));

    memset(scratch, 0, sizeof(scratch));
    if (appGetImeiNumSync(scratch) == CMS_RET_SUCC)
    {
        arduinoModemCopyBoundedString(identity->imei, sizeof(identity->imei), scratch);
        identity->imeiValid = arduinoModemStringHasAnyChar(identity->imei);
    }

    memset(scratch, 0, sizeof(scratch));
    if (appGetImsiNumSync(scratch) == CMS_RET_SUCC)
    {
        arduinoModemCopyBoundedString(identity->imsi, sizeof(identity->imsi), scratch);
        identity->imsiValid = arduinoModemStringHasAnyChar(identity->imsi);
    }

    memset(scratch, 0, sizeof(scratch));
    if (appGetIccidNumSync(scratch) == CMS_RET_SUCC)
    {
        arduinoModemCopyBoundedString(identity->iccid, sizeof(identity->iccid), scratch);
        arduinoModemCopyBoundedString(identity->iccidRaw, sizeof(identity->iccidRaw), scratch);
        identity->iccidValid = arduinoModemStringHasAnyChar(identity->iccid);
        identity->iccidRawValid = arduinoModemStringHasAnyChar(identity->iccidRaw);
    }

    if (arduinoModemGetSubscriberNumber(identity->phoneNumber,
                                        sizeof(identity->phoneNumber),
                                        &identity->phoneNumberType) == 0)
    {
        identity->phoneNumberValid = arduinoModemStringHasAnyChar(identity->phoneNumber);
    }

    memset(scratch, 0, sizeof(scratch));
    if (appGetSNNumSync(scratch))
    {
        arduinoModemCopyBoundedString(identity->sn, sizeof(identity->sn), scratch);
        identity->snValid = arduinoModemStringHasAnyChar(identity->sn);
    }

    soc_mobile_get_sim_id(&simId, &simAuto);
    (void)simAuto;
    if (simId != 0xFFU)
    {
        identity->simId = simId;
        identity->simIdValid = 1U;
    }

    return 0;
}

int arduinoCoreModemGetSignalQuality(ArduinoCoreModemSignalQuality *signal)
{
    uint8_t csq = 99U;
    int8_t snr = 0;
    int8_t rsrp = 127;
    int8_t rsrq = 127;

    if (signal == NULL)
    {
        return -1;
    }

    memset(signal, 0, sizeof(*signal));
    signal->csq = 99U;
    signal->rsrpRaw = 127;
    signal->rsrqRaw = 127;

    if (appGetSignalQualitySync(&csq, &snr, &rsrp, &rsrq) != CMS_RET_SUCC)
    {
        return -2;
    }

    signal->valid = 1U;
    signal->csq = csq;
    signal->snrDb = snr;
    signal->snrValid = 1U;
    signal->rsrpRaw = rsrp;
    signal->rsrqRaw = rsrq;

    signal->rssiValid = arduinoModemCsqRssiIsValid(csq);
    if (signal->rssiValid != 0U)
    {
        signal->rssiDbm = arduinoModemCsqToRssiDbm(csq);
    }

    signal->rsrpValid = arduinoModemRsrpRawIsValid(rsrp);
    if (signal->rsrpValid != 0U)
    {
        signal->rsrpDbm = arduinoModemRsrpRawToDbm(rsrp);
    }

    signal->rsrqValid = arduinoModemRsrqRawIsValid(rsrq);
    if (signal->rsrqValid != 0U)
    {
        signal->rsrqDb = arduinoModemRsrqRawToDb(rsrq);
    }

    return 0;
}

int arduinoCoreModemGetCellInfo(ArduinoCoreModemCellInfo *cell)
{
    BasicCellListInfo cellList;

    if (cell == NULL)
    {
        return -1;
    }

    memset(cell, 0, sizeof(*cell));
    memset(&cellList, 0, sizeof(cellList));

    if (appGetECBCInfoSync(&cellList) != CMS_RET_SUCC)
    {
        return -2;
    }

    if (cellList.sCellPresent == FALSE)
    {
        return 0;
    }

    cell->valid = 1U;
    cell->mcc = arduinoModemBcd3ToNumber(cellList.sCellInfo.plmn.mcc);
    cell->mnc = arduinoModemBcdMncToNumber(cellList.sCellInfo.plmn.mncWithAddInfo, &cell->mncDigits);
    cell->tac = cellList.sCellInfo.tac;
    cell->cellId = cellList.sCellInfo.cellId;
    cell->earfcn = cellList.sCellInfo.earfcn;
    cell->physicalCellId = cellList.sCellInfo.phyCellId;
    cell->band = cellList.sCellInfo.band;
    cell->snrValid = (cellList.sCellInfo.snrPresent != FALSE) ? 1U : 0U;
    cell->snrDb = cellList.sCellInfo.snr;
    cell->rsrpDbm = cellList.sCellInfo.rsrp;
    cell->rsrqDb = cellList.sCellInfo.rsrq;
    cell->neighborCount = cellList.nCellNum;

    return 0;
}

int arduinoCoreModemGetAPN(uint8_t cid, char *apn, size_t apnSize)
{
    uint8_t scratch[CMI_PS_MAX_APN_LEN + 1U];

    if ((apn == NULL) || (apnSize == 0U))
    {
        return -1;
    }

    apn[0] = '\0';
    if (cid > 15U)
    {
        return -2;
    }

    memset(scratch, 0, sizeof(scratch));
    if (appGetAPNSettingSync(cid, scratch) != CMS_RET_SUCC)
    {
        return -3;
    }

    arduinoModemCopyBoundedString(apn, apnSize, (const char *)scratch);
    return 0;
}

int arduinoCoreModemSetAPN(uint8_t cid, const char *apn, uint8_t pdnType)
{
    SetPsBearerParams params;
    size_t apnLength = 0U;

    if (cid > 15U)
    {
        return -1;
    }

    if (arduinoModemPdnTypeIsValid(pdnType) == 0U)
    {
        return -2;
    }

    if (arduinoModemValidateApn(apn, &apnLength) == 0U)
    {
        return -3;
    }

    memset(&params, 0, sizeof(params));
    params.bearerCtxInfo.cid = cid;
    params.bearerCtxInfo.pdnType = pdnType;
    params.bearerCtxInfo.apnPresentType = CMI_UPDATE_WITH_NEW;
    params.bearerCtxInfo.apnLength = (uint8_t)apnLength;
    if (apnLength > 0U)
    {
        memcpy(params.bearerCtxInfo.apnStr, apn, apnLength);
    }

    return (appSetEpsBeaerParamSync(&params) == CMS_RET_SUCC) ? 0 : -4;
}

int arduinoCoreModemGetTimeStatus(ArduinoCoreModemTimeStatus *status)
{
    time_t nitzSeconds = 0;
    time_t nowSeconds = 0;
    struct tm *localTime = NULL;
    int8_t timezoneQuarterHours = 0;
    utc_timer_value_t *utcNow = NULL;

    if (status == NULL)
    {
        return -1;
    }

    memset(status, 0, sizeof(*status));

    time(&nowSeconds);
    status->epoch = (uint32_t)nowSeconds;

    utcNow = OsaSystemTimeReadUtc();
    if (utcNow != NULL)
    {
        status->milliseconds = (uint16_t)utcNow->UTCms;
    }

    localTime = localtime(&nowSeconds);
    if (localTime != NULL)
    {
        status->year = (uint16_t)(localTime->tm_year + 1900);
        status->month = (uint8_t)(localTime->tm_mon + 1);
        status->day = (uint8_t)localTime->tm_mday;
        status->hour = (uint8_t)localTime->tm_hour;
        status->minute = (uint8_t)localTime->tm_min;
        status->second = (uint8_t)localTime->tm_sec;
    }

    (void)arduinoCoreTimeGetTimezoneQuarterHours(&timezoneQuarterHours);
    status->timezoneQuarterHours = timezoneQuarterHours;

    if ((appGetSystemTimeNitzSecsSync(&nitzSeconds) == CMS_RET_SUCC) &&
        (arduinoModemEpochLooksValid((uint32_t)nitzSeconds) != 0U))
    {
        status->nitzSynced = 1U;
    }

    status->synced = (arduinoModemEpochLooksValid(status->epoch) != 0U) ? 1U : 0U;
    status->valid = status->synced;

    return 0;
}
