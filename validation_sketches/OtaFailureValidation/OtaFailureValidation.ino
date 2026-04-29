static const char *kDownloadFailUrl = "http://ota-smoke.invalid/notfound.sota";
static const char *kVerifyFailUrl = "http://example.com/";

static bool g_started = false;
static bool g_startedFailProbe = false;
static bool g_startedVerifyProbe = false;
static bool g_reported = false;
static unsigned long g_stateSince = 0UL;

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

static void printSnapshot(const char *tag)
{
    Serial.printf("+ARDUINO: OTA_FAIL,%s,STATE,%s,ERR,%ld,DOWN,%lu,TOTAL,%lu\r\n",
                  tag,
                  otaStateName(AIR780EPMOTA.state()),
                  static_cast<long>(AIR780EPMOTA.lastError()),
                  static_cast<unsigned long>(AIR780EPMOTA.downloadedBytes()),
                  static_cast<unsigned long>(AIR780EPMOTA.totalBytes()));
}

void setup()
{
    AIR780EPMOTAConfig authMismatch;

    Serial.begin(921600);
    delay(100);
    Serial.println("+ARDUINO: OTA_FAIL,READY");

    Serial.printf("+ARDUINO: OTA_FAIL,EMPTY_URL,%d,ERR,%ld\r\n",
                  AIR780EPMOTA.begin("", AIR780EPMOTAConfig()) ? 1 : 0,
                  static_cast<long>(AIR780EPMOTA.lastError()));

    Serial.printf("+ARDUINO: OTA_FAIL,BAD_SCHEME,%d,ERR,%ld\r\n",
                  AIR780EPMOTA.begin("ftp://example.com/update.sota", AIR780EPMOTAConfig()) ? 1 : 0,
                  static_cast<long>(AIR780EPMOTA.lastError()));

    authMismatch.user = "user-only";
    Serial.printf("+ARDUINO: OTA_FAIL,AUTH_MISMATCH,%d,ERR,%ld\r\n",
                  AIR780EPMOTA.begin("http://example.com/update.sota", authMismatch) ? 1 : 0,
                  static_cast<long>(AIR780EPMOTA.lastError()));

    Serial.printf("+ARDUINO: OTA_FAIL,NO_NETWORK_GUARD,%d,ERR,%ld\r\n",
                  AIR780EPMOTA.begin(kDownloadFailUrl, AIR780EPMOTAConfig()) ? 1 : 0,
                  static_cast<long>(AIR780EPMOTA.lastError()));

    if (Modem.waitForNetwork(60000UL) && Modem.activatePDP(60000UL)) {
        AIR780EPMOTAConfig config;
        g_started = AIR780EPMOTA.begin(kDownloadFailUrl, config);
        g_stateSince = millis();
        Serial.printf("+ARDUINO: OTA_FAIL,BEGIN_INVALID_HOST,%d,ERR,%ld\r\n",
                      g_started ? 1 : 0,
                      static_cast<long>(AIR780EPMOTA.lastError()));
        if (g_started) {
            Serial.printf("+ARDUINO: OTA_FAIL,BEGIN_AGAIN,%d,ERR,%ld\r\n",
                          AIR780EPMOTA.begin(kDownloadFailUrl, config) ? 1 : 0,
                          static_cast<long>(AIR780EPMOTA.lastError()));
            Serial.printf("+ARDUINO: OTA_FAIL,CLEAR_WHILE_RUNNING,%d,ERR,%ld\r\n",
                          AIR780EPMOTA.clear() ? 1 : 0,
                          static_cast<long>(AIR780EPMOTA.lastError()));
        }
    }
    else {
        Serial.println("+ARDUINO: OTA_FAIL,SKIP,NO_NETWORK");
    }
}

void loop()
{
    const AIR780EPMOTAClass::State state = AIR780EPMOTA.poll();

    if (!g_started) {
        delay(1000);
        return;
    }

    if (!g_startedFailProbe) {
        if ((state == AIR780EPMOTAClass::OTA_STATE_ERROR) ||
            ((millis() - g_stateSince) > 45000UL)) {
            printSnapshot("DOWNLOAD_FAIL_DONE");
            Serial.printf("+ARDUINO: OTA_FAIL,CLEAR_AFTER_ERROR,%d,ERR,%ld\r\n",
                          AIR780EPMOTA.clear() ? 1 : 0,
                          static_cast<long>(AIR780EPMOTA.lastError()));

            AIR780EPMOTAConfig config;
            g_startedFailProbe = AIR780EPMOTA.begin(kVerifyFailUrl, config);
            g_stateSince = millis();
            Serial.printf("+ARDUINO: OTA_FAIL,BEGIN_VERIFY_PROBE,%d,ERR,%ld\r\n",
                          g_startedFailProbe ? 1 : 0,
                          static_cast<long>(AIR780EPMOTA.lastError()));
        }
        delay(250);
        return;
    }

    if (!g_startedVerifyProbe) {
        g_startedVerifyProbe = true;
    }

    if (!g_reported &&
        ((state == AIR780EPMOTAClass::OTA_STATE_ERROR) ||
         (state == AIR780EPMOTAClass::OTA_STATE_STAGED) ||
         ((millis() - g_stateSince) > 60000UL))) {
        printSnapshot("VERIFY_PROBE_DONE");
        g_reported = true;
    }

    delay(250);
}
