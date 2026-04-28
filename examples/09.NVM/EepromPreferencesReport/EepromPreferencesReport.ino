#include <Arduino.h>
#include <EEPROM.h>
#include <Preferences.h>

namespace {

constexpr size_t kEepromSize = 64U;
constexpr int kEepromCounterAddress = 0;

void reportBool(const char *tag, bool value) {
  Serial.print("+ARDUINO: NVM,");
  Serial.print(tag);
  Serial.print(",");
  Serial.println(value ? "OK" : "FAIL");
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1200);

  bool eepromBeginOk = EEPROM.begin(kEepromSize);
  uint8_t previousEepromCount = eepromBeginOk ? EEPROM.read(kEepromCounterAddress) : 0U;
  uint8_t nextEepromCount = (uint8_t)(previousEepromCount + 1U);
  bool eepromCommitOk = false;

  if (eepromBeginOk) {
    EEPROM.write(kEepromCounterAddress, nextEepromCount);
    eepromCommitOk = EEPROM.commit();
  }

  Preferences prefs;
  bool prefsBeginOk = prefs.begin("nvmtest", false);
  uint32_t previousPrefsCount = prefsBeginOk ? prefs.getUInt("count", 0U) : 0U;
  uint32_t nextPrefsCount = previousPrefsCount + 1U;
  size_t putCountSize = prefsBeginOk ? prefs.putUInt("count", nextPrefsCount) : 0U;
  size_t putStringSize = prefsBeginOk ? prefs.putString("label", "air780epm") : 0U;
  String label = prefsBeginOk ? prefs.getString("label", "") : String("");
  prefs.end();

  Serial.println("+ARDUINO: NVM,BOOT");
  reportBool("EEPROM_BEGIN", eepromBeginOk);
  reportBool("EEPROM_COMMIT", eepromCommitOk);
  Serial.print("+ARDUINO: NVM,EEPROM,PREV,");
  Serial.print((unsigned int)previousEepromCount);
  Serial.print(",NEXT,");
  Serial.println((unsigned int)nextEepromCount);

  reportBool("PREFS_BEGIN", prefsBeginOk);
  Serial.print("+ARDUINO: NVM,PREFS,PREV,");
  Serial.print(previousPrefsCount);
  Serial.print(",NEXT,");
  Serial.print(nextPrefsCount);
  Serial.print(",PUT_COUNT_SIZE,");
  Serial.print((unsigned int)putCountSize);
  Serial.print(",PUT_LABEL_SIZE,");
  Serial.print((unsigned int)putStringSize);
  Serial.print(",LABEL,");
  Serial.println(label.c_str());

  Serial.println("+ARDUINO: NVM,READY");
}

void loop() {
  delay(1000);
}
