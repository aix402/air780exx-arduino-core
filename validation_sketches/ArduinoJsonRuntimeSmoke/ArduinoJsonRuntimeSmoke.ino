#include <Arduino.h>
#include <ArduinoJson.h>

static bool gPass = false;
static size_t gLength = 0;
static char gBuffer[192] = {0};

static void printResult() {
  char line[288];
  snprintf(line,
           sizeof(line),
           "+ARDUINO: ARDUINOJSON_RUNTIME,%s,LEN,%lu,JSON,%s",
           gPass ? "PASS" : "FAIL",
           static_cast<unsigned long>(gLength),
           gBuffer);
  Serial.println(line);
}

void setup() {
  Serial.begin(921600);
  delay(200);

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 7
  JsonDocument doc;
#else
  StaticJsonDocument<192> doc;
#endif

  DeserializationError error = deserializeJson(doc, "{\"ok\":true,\"value\":42,\"name\":\"air780epm\"}");
  if (error) {
    Serial.print(F("+ARDUINO: ARDUINOJSON_RUNTIME,FAIL,PARSE,"));
    Serial.println(error.c_str());
    return;
  }

  bool ok = doc["ok"] | false;
  int value = doc["value"] | 0;
  const char *name = doc["name"] | "";

  doc["value"] = value + 1;
  doc["platform"] = F("arduino");

  gLength = serializeJson(doc, gBuffer, sizeof(gBuffer));
  gPass = ok && value == 42 && strcmp(name, "air780epm") == 0 && gLength > 0 && gLength < sizeof(gBuffer);

  printResult();
}

void loop() {
  delay(1000);
  printResult();
}
