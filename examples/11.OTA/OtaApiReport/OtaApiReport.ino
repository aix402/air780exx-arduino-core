static const char *kOtaUrl = "";

static AIR780EPMOTAClass::State g_lastState = AIR780EPMOTAClass::OTA_STATE_IDLE;

static const char *otaStateName(AIR780EPMOTAClass::State state)
{
    switch (state) {
        case AIR780EPMOTAClass::OTA_STATE_IDLE: return "IDLE";
        case AIR780EPMOTAClass::OTA_STATE_STARTING: return "STARTING";
        case AIR780EPMOTAClass::OTA_STATE_DOWNLOADING: return "DOWNLOADING";
        case AIR780EPMOTAClass::OTA_STATE_VERIFYING: return "VERIFYING";
        case AIR780EPMOTAClass::OTA_STATE_STAGED: return "STAGED";
        case AIR780EPMOTAClass::OTA_STATE_APPLYING: return "APPLYING";
        case AIR780EPMOTAClass::OTA_STATE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

static void printState(const char *tag)
{
    const AIR780EPMOTAClass::State state = AIR780EPMOTA.state();
    Serial.printf("+ARDUINO: OTA_API_REPORT,%s,STATE,%s,ERR,%ld,DOWN,%lu,TOTAL,%lu,STAGED,%d\r\n",
                  tag,
                  otaStateName(state),
                  static_cast<long>(AIR780EPMOTA.lastError()),
                  static_cast<unsigned long>(AIR780EPMOTA.downloadedBytes()),
                  static_cast<unsigned long>(AIR780EPMOTA.totalBytes()),
                  AIR780EPMOTA.isStaged() ? 1 : 0);
}

void setup()
{
    Serial.begin(921600);
    delay(100);
    Serial.println("+ARDUINO: OTA_API_REPORT,READY");

    printState("INITIAL");
    if (kOtaUrl[0] == '\0') {
        Serial.println("+ARDUINO: OTA_API_REPORT,SKIP,NO_URL");
        return;
    }

    if (!Modem.waitForNetwork(60000UL) || !Modem.activatePDP(60000UL)) {
        Serial.println("+ARDUINO: OTA_API_REPORT,SKIP,NO_NETWORK");
        printState("NO_NETWORK");
        return;
    }

    AIR780EPMOTAConfig config;
    config.insecure = true;

    Serial.printf("+ARDUINO: OTA_API_REPORT,BEGIN,%d\r\n",
                  AIR780EPMOTA.begin(kOtaUrl, config) ? 1 : 0);
    printState("BEGIN_RESULT");
}

void loop()
{
    const AIR780EPMOTAClass::State state = AIR780EPMOTA.poll();
    if (state != g_lastState) {
        g_lastState = state;
        printState("TRANSITION");
        if (state == AIR780EPMOTAClass::OTA_STATE_STAGED) {
            Serial.println("+ARDUINO: OTA_API_REPORT,STAGED,WAIT_APPLY");
        }
    }

    delay(250);
}
