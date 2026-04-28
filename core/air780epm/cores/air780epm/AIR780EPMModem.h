#ifndef AIR780EPM_MODEM_H
#define AIR780EPM_MODEM_H

#include <stddef.h>
#include <stdint.h>

#include "IPAddress.h"
#include "WString.h"

enum AIR780EPMPdnType {
    AIR780EPM_PDN_IPV4 = 1U,
    AIR780EPM_PDN_IPV6 = 2U,
    AIR780EPM_PDN_IPV4V6 = 3U,
    AIR780EPM_PDN_NON_IP = 5U
};

struct AIR780EPMModemStatus {
    bool psReady;
    uint8_t ceregState;
    bool registered;
    bool netInfoValid;
    uint8_t netMgrResult;
    uint8_t netStatus;
    uint8_t ipType;
    bool networkReady;
    uint8_t ipv4Cid;
    uint8_t ipv6Cid;
    bool hasIPv4;
    IPAddress localIPv4;
    uint8_t dnsCount;
    IPAddress dns[2];

    AIR780EPMModemStatus();
};

struct AIR780EPMModemIdentity {
    bool imeiValid;
    bool imsiValid;
    bool iccidValid;
    bool iccidRawValid;
    bool phoneNumberValid;
    bool snValid;
    bool simIdValid;
    char imei[16];
    char imsi[17];
    char iccid[21];
    char iccidRaw[21];
    char phoneNumber[32];
    char sn[33];
    uint8_t phoneNumberType;
    uint8_t simId;

    AIR780EPMModemIdentity();
};

struct AIR780EPMSignalQuality {
    bool valid;
    uint8_t csq;
    bool rssiValid;
    int16_t rssiDbm;
    bool rsrpValid;
    int8_t rsrpRaw;
    int16_t rsrpDbm;
    bool rsrqValid;
    int8_t rsrqRaw;
    int16_t rsrqDb;
    bool snrValid;
    int8_t snrDb;

    AIR780EPMSignalQuality();
};

struct AIR780EPMCellInfo {
    bool valid;
    uint16_t mcc;
    uint16_t mnc;
    uint8_t mncDigits;
    uint16_t tac;
    uint32_t cid;
    uint32_t cellId;
    uint32_t earfcn;
    uint16_t physicalCellId;
    uint8_t band;
    bool snrValid;
    int8_t snrDb;
    int16_t rsrpDbm;
    int16_t rsrqDb;
    uint8_t neighborCount;

    AIR780EPMCellInfo();
};

struct AIR780EPMTimeStatus {
    bool valid;
    bool synced;
    bool nitzSynced;
    uint32_t epoch;
    uint16_t milliseconds;
    int8_t timezoneQuarterHours;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    AIR780EPMTimeStatus();
};

class AIR780EPMModem {
public:
    bool begin();
    bool getStatus(AIR780EPMModemStatus &status) const;
    bool getIdentity(AIR780EPMModemIdentity &identity) const;
    bool getSignalQuality(AIR780EPMSignalQuality &signal) const;
    bool getCellInfo(AIR780EPMCellInfo &cell) const;
    bool getTimeStatus(AIR780EPMTimeStatus &status) const;
    bool getAPN(char *out, size_t outSize, uint8_t cid = 0U) const;
    bool setAPN(const char *apn, uint8_t cid = 0U, uint8_t pdnType = AIR780EPM_PDN_IPV4V6);
    bool getPhoneNumber(char *out, size_t outSize) const;
    bool isRegistered() const;
    bool isNetworkReady() const;
    bool isTimeSynced() const;
    bool isNetworkTimeSynced() const;
    bool waitForNetwork(uint32_t timeoutMs = 60000UL) const;
    bool waitForTimeSync(uint32_t timeoutMs = 5000UL) const;
    bool waitForNetworkTime(uint32_t timeoutMs = 5000UL) const;
    bool activatePDP(uint32_t timeoutMs = 60000UL);
    uint8_t ceregState() const;
    IPAddress localIP() const;
    uint32_t epochTime() const;
    int8_t timeZoneQuarterHours() const;
    int csq() const;
    int rssi() const;
    String imei() const;
    String imsi() const;
    String iccid() const;
    String iccidRaw() const;
    String phoneNumber() const;
    String sn() const;
    String apn(uint8_t cid = 0U) const;
};

extern AIR780EPMModem Modem;

#endif
