#include <Arduino.h>

static bool gWaitOk = false;
static bool gAfterWaitStatusOk = false;
static AIR780EPMModemStatus gAfterWaitStatus;

static void printTextField(const char *name, bool valid, const char *value) {
  Serial.print("+ARDUINO: MODEM_INFO,IDENTITY,");
  Serial.print(name);
  Serial.print(",VALID,");
  Serial.print(valid ? 1 : 0);
  Serial.print(",VALUE,");
  Serial.println(valid ? value : "");
}

static void printIdentity() {
  AIR780EPMModemIdentity identity;
  const bool ok = Modem.getIdentity(identity);

  Serial.print("+ARDUINO: MODEM_INFO,IDENTITY,GET,");
  Serial.println(ok ? 1 : 0);
  printTextField("IMEI", identity.imeiValid, identity.imei);
  printTextField("IMSI", identity.imsiValid, identity.imsi);
  printTextField("ICCID", identity.iccidValid, identity.iccid);
  printTextField("ICCID_RAW", identity.iccidRawValid, identity.iccidRaw);
  printTextField("PHONE_NUMBER", identity.phoneNumberValid, identity.phoneNumber);
  Serial.print("+ARDUINO: MODEM_INFO,IDENTITY,PHONE_TYPE,VALUE,");
  Serial.println(identity.phoneNumberValid ? identity.phoneNumberType : 0);
  printTextField("SN", identity.snValid, identity.sn);

  Serial.print("+ARDUINO: MODEM_INFO,IDENTITY,SIMID,VALID,");
  Serial.print(identity.simIdValid ? 1 : 0);
  Serial.print(",VALUE,");
  Serial.println(identity.simIdValid ? identity.simId : 0);
}

static void printSignal() {
  AIR780EPMSignalQuality signal;
  const bool ok = Modem.getSignalQuality(signal);

  Serial.print("+ARDUINO: MODEM_INFO,SIGNAL,GET,");
  Serial.println(ok ? 1 : 0);
  Serial.print("+ARDUINO: MODEM_INFO,SIGNAL,CSQ,");
  Serial.print(signal.csq);
  Serial.print(",RSSI_VALID,");
  Serial.print(signal.rssiValid ? 1 : 0);
  Serial.print(",RSSI_DBM,");
  Serial.print(signal.rssiDbm);
  Serial.print(",RSRP_VALID,");
  Serial.print(signal.rsrpValid ? 1 : 0);
  Serial.print(",RSRP_RAW,");
  Serial.print(signal.rsrpRaw);
  Serial.print(",RSRP_DBM,");
  Serial.print(signal.rsrpDbm);
  Serial.print(",RSRQ_VALID,");
  Serial.print(signal.rsrqValid ? 1 : 0);
  Serial.print(",RSRQ_RAW,");
  Serial.print(signal.rsrqRaw);
  Serial.print(",RSRQ_DB,");
  Serial.print(signal.rsrqDb);
  Serial.print(",SNR_VALID,");
  Serial.print(signal.snrValid ? 1 : 0);
  Serial.print(",SNR_DB,");
  Serial.println(signal.snrDb);
}

static bool printStatus(const char *stage, AIR780EPMModemStatus &status) {
  const bool ok = Modem.getStatus(status);

  Serial.print("+ARDUINO: MODEM_INFO,STATUS,STAGE,");
  Serial.print(stage);
  Serial.print(",GET,");
  Serial.print(ok ? 1 : 0);
  Serial.print(",PS_READY,");
  Serial.print(status.psReady ? 1 : 0);
  Serial.print(",CEREG,");
  Serial.print(status.ceregState);
  Serial.print(",REGISTERED,");
  Serial.print(status.registered ? 1 : 0);
  Serial.print(",NET_VALID,");
  Serial.print(status.netInfoValid ? 1 : 0);
  Serial.print(",NET_READY,");
  Serial.print(status.networkReady ? 1 : 0);
  Serial.print(",IPV4_CID,");
  Serial.print(status.ipv4Cid);
  Serial.print(",IPV6_CID,");
  Serial.print(status.ipv6Cid);
  Serial.print(",HAS_IPV4,");
  Serial.print(status.hasIPv4 ? 1 : 0);
  Serial.print(",IP,");
  Serial.println(status.localIPv4);

  return ok;
}

static void printApnForCid(const char *stage, const char *role, uint8_t cid) {
  char apn[101];
  const bool ok = Modem.getAPN(apn, sizeof(apn), cid);

  Serial.print("+ARDUINO: MODEM_INFO,APN,STAGE,");
  Serial.print(stage);
  Serial.print(",ROLE,");
  Serial.print(role);
  Serial.print(",CID,");
  Serial.print(cid);
  Serial.print(",GET,");
  Serial.print(ok ? 1 : 0);
  Serial.print(",VALUE,");
  Serial.println(ok ? apn : "");
}

static void printApnProbe(const char *stage, const AIR780EPMModemStatus &status) {
  printApnForCid(stage, "CID0", 0);

  if (status.ipv4Cid != 0xFFU) {
    printApnForCid(stage, "ACTIVE_IPV4", status.ipv4Cid);
  } else {
    Serial.print("+ARDUINO: MODEM_INFO,APN,STAGE,");
    Serial.print(stage);
    Serial.println(",ROLE,ACTIVE_IPV4,CID,255,GET,0,VALUE,");
  }
}

static void printCell() {
  AIR780EPMCellInfo cell;

  Serial.println("+ARDUINO: MODEM_INFO,CELL,QUERY,START");
  const bool ok = Modem.getCellInfo(cell);

  Serial.print("+ARDUINO: MODEM_INFO,CELL,GET,");
  Serial.print(ok ? 1 : 0);
  Serial.print(",VALID,");
  Serial.print(cell.valid ? 1 : 0);
  Serial.print(",MCC,");
  Serial.print(cell.mcc);
  Serial.print(",MNC,");
  Serial.print(cell.mnc);
  Serial.print(",MNC_DIGITS,");
  Serial.print(cell.mncDigits);
  Serial.print(",TAC,");
  Serial.print(cell.tac);
  Serial.print(",CID,");
  Serial.print(cell.cid);
  Serial.print(",EARFCN,");
  Serial.print(cell.earfcn);
  Serial.print(",PCI,");
  Serial.print(cell.physicalCellId);
  Serial.print(",BAND,");
  Serial.print(cell.band);
  Serial.print(",RSRP_DBM,");
  Serial.print(cell.rsrpDbm);
  Serial.print(",RSRQ_DB,");
  Serial.print(cell.rsrqDb);
  Serial.print(",SNR_VALID,");
  Serial.print(cell.snrValid ? 1 : 0);
  Serial.print(",SNR_DB,");
  Serial.print(cell.snrDb);
  Serial.print(",NCELL,");
  Serial.println(cell.neighborCount);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("+ARDUINO: MODEM_INFO,READY");
  Modem.begin();
  printIdentity();
  printSignal();

  AIR780EPMModemStatus earlyStatus;
  printStatus("EARLY", earlyStatus);
  printApnProbe("EARLY", earlyStatus);

  Serial.println("+ARDUINO: MODEM_INFO,NETWORK_WAIT,START,60000");
  const bool waitOk = Modem.waitForNetwork(60000UL);
  Serial.print("+ARDUINO: MODEM_INFO,NETWORK_WAIT,DONE,");
  Serial.println(waitOk ? 1 : 0);

  AIR780EPMModemStatus readyStatus;
  gAfterWaitStatusOk = printStatus("AFTER_WAIT", readyStatus);
  gAfterWaitStatus = readyStatus;
  gWaitOk = waitOk;
  printApnProbe("AFTER_WAIT", readyStatus);

  printCell();
  Serial.println("+ARDUINO: MODEM_INFO,DONE");
}

void loop() {
  delay(10000);
  Serial.print("+ARDUINO: MODEM_INFO,RUNTIME,WAIT_OK,");
  Serial.print(gWaitOk ? 1 : 0);
  Serial.print(",STATUS_OK,");
  Serial.print(gAfterWaitStatusOk ? 1 : 0);
  Serial.print(",REGISTERED,");
  Serial.print(gAfterWaitStatus.registered ? 1 : 0);
  Serial.print(",NET_READY,");
  Serial.print(gAfterWaitStatus.networkReady ? 1 : 0);
  Serial.print(",HAS_IPV4,");
  Serial.print(gAfterWaitStatus.hasIPv4 ? 1 : 0);
  Serial.print(",IP,");
  Serial.println(gAfterWaitStatus.localIPv4);
  printSignal();
}
