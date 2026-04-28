#ifndef ARDUINO_MODEM_IO_H
#define ARDUINO_MODEM_IO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ArduinoCoreModemStatus_Tag {
    uint8_t psReady;
    uint8_t ceregState;
    uint8_t registered;
    uint8_t netInfoValid;
    uint8_t netMgrResult;
    uint8_t netStatus;
    uint8_t ipType;
    uint8_t networkReady;
    uint8_t ipv4Cid;
    uint8_t ipv6Cid;
    uint8_t hasIPv4;
    uint8_t ipv4[4];
    uint8_t dnsCount;
    uint8_t dns[2][4];
} ArduinoCoreModemStatus;

typedef struct ArduinoCoreModemIdentity_Tag {
    uint8_t imeiValid;
    uint8_t imsiValid;
    uint8_t iccidValid;
    uint8_t iccidRawValid;
    uint8_t phoneNumberValid;
    uint8_t snValid;
    uint8_t simIdValid;
    char imei[16];
    char imsi[17];
    char iccid[21];
    char iccidRaw[21];
    char phoneNumber[32];
    char sn[33];
    uint8_t phoneNumberType;
    uint8_t simId;
} ArduinoCoreModemIdentity;

typedef struct ArduinoCoreModemSignalQuality_Tag {
    uint8_t valid;
    uint8_t csq;
    uint8_t rssiValid;
    int16_t rssiDbm;
    uint8_t rsrpValid;
    int8_t rsrpRaw;
    int16_t rsrpDbm;
    uint8_t rsrqValid;
    int8_t rsrqRaw;
    int16_t rsrqDb;
    uint8_t snrValid;
    int8_t snrDb;
} ArduinoCoreModemSignalQuality;

typedef struct ArduinoCoreModemCellInfo_Tag {
    uint8_t valid;
    uint16_t mcc;
    uint16_t mnc;
    uint8_t mncDigits;
    uint16_t tac;
    uint32_t cellId;
    uint32_t earfcn;
    uint16_t physicalCellId;
    uint8_t band;
    uint8_t snrValid;
    int8_t snrDb;
    int16_t rsrpDbm;
    int16_t rsrqDb;
    uint8_t neighborCount;
} ArduinoCoreModemCellInfo;

typedef struct ArduinoCoreModemTimeStatus_Tag {
    uint8_t valid;
    uint8_t synced;
    uint8_t nitzSynced;
    uint32_t epoch;
    uint16_t milliseconds;
    int8_t timezoneQuarterHours;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} ArduinoCoreModemTimeStatus;

int arduinoCoreModemGetStatus(ArduinoCoreModemStatus *status);
int arduinoCoreModemGetIdentity(ArduinoCoreModemIdentity *identity);
int arduinoCoreModemGetSignalQuality(ArduinoCoreModemSignalQuality *signal);
int arduinoCoreModemGetCellInfo(ArduinoCoreModemCellInfo *cell);
int arduinoCoreModemGetAPN(uint8_t cid, char *apn, size_t apnSize);
int arduinoCoreModemSetAPN(uint8_t cid, const char *apn, uint8_t pdnType);
int arduinoCoreModemGetTimeStatus(ArduinoCoreModemTimeStatus *status);

#ifdef __cplusplus
}
#endif

#endif
