#include <Arduino.h>
#include <LittleFS.h>

namespace {

void reportBool(const char *tag, bool value) {
  Serial.print("+ARDUINO: LITTLEFS,");
  Serial.print(tag);
  Serial.print(",");
  Serial.println(value ? "OK" : "FAIL");
}

String readAll(File &file) {
  String data;

  while (file.available() > 0) {
    int ch = file.read();
    if (ch < 0) {
      break;
    }
    data += (char)ch;
  }

  return data;
}

}  // namespace

void setup() {
  const char *kDirPath = "/lfstest";
  const char *kFilePath = "/lfstest/report.txt";
  const char *kRenamedPath = "/lfstest/report2.txt";
  const char *kPayload = "air780epm-littlefs";
  bool beginOk;
  bool mkdirOk;
  bool existsOriginal;
  bool renameOk;
  bool existsRenamed;
  bool removeOk;
  bool rmdirOk;
  bool rootOpenOk;
  bool rootFoundDir = false;
  size_t totalBytes;
  size_t usedBefore;
  size_t usedAfterWrite;
  size_t usedAfterCleanup;
  String readBack;
  size_t entryCount = 0U;

  Serial.begin(115200);
  delay(1200);

  Serial.println("+ARDUINO: LITTLEFS,BOOT");

  beginOk = LittleFS.begin(true);
  reportBool("BEGIN", beginOk);
  if (!beginOk) {
    Serial.println("+ARDUINO: LITTLEFS,FAIL,BEGIN");
    return;
  }

  totalBytes = LittleFS.totalBytes();
  usedBefore = LittleFS.usedBytes();
  Serial.print("+ARDUINO: LITTLEFS,CAPACITY,TOTAL,");
  Serial.print((unsigned int)totalBytes);
  Serial.print(",USED,");
  Serial.println((unsigned int)usedBefore);

  (void)LittleFS.remove(kRenamedPath);
  (void)LittleFS.remove(kFilePath);
  (void)LittleFS.rmdir(kDirPath);

  mkdirOk = LittleFS.mkdir(kDirPath);
  reportBool("MKDIR", mkdirOk);

  File file = LittleFS.open(kFilePath, FILE_WRITE);
  bool openWriteOk = (bool)file;
  reportBool("OPEN_WRITE", openWriteOk);
  if (openWriteOk) {
    file.print(kPayload);
    file.close();
  }

  existsOriginal = LittleFS.exists(kFilePath);
  reportBool("EXISTS_ORIG", existsOriginal);

  renameOk = LittleFS.rename(kFilePath, kRenamedPath);
  reportBool("RENAME", renameOk);

  existsRenamed = LittleFS.exists(kRenamedPath);
  reportBool("EXISTS_RENAMED", existsRenamed);

  File readFile = LittleFS.open(kRenamedPath, FILE_READ);
  bool openReadOk = (bool)readFile;
  reportBool("OPEN_READ", openReadOk);
  if (openReadOk) {
    readBack = readAll(readFile);
    readFile.close();
  }

  usedAfterWrite = LittleFS.usedBytes();
  Serial.print("+ARDUINO: LITTLEFS,READBACK,");
  Serial.println(readBack.c_str());
  Serial.print("+ARDUINO: LITTLEFS,USED_AFTER_WRITE,");
  Serial.println((unsigned int)usedAfterWrite);

  File root = LittleFS.open("/");
  rootOpenOk = root && root.isDirectory();
  reportBool("OPEN_ROOT", rootOpenOk);
  if (rootOpenOk) {
    File entry = root.openNextFile();
    while (entry) {
      ++entryCount;
      if (entry.isDirectory() && String(entry.name()) == "lfstest") {
        rootFoundDir = true;
      }
      entry = root.openNextFile();
    }
  }

  Serial.print("+ARDUINO: LITTLEFS,ROOT_COUNT,");
  Serial.println((unsigned int)entryCount);
  reportBool("ROOT_FOUND_DIR", rootFoundDir);

  removeOk = LittleFS.remove(kRenamedPath);
  rmdirOk = LittleFS.rmdir(kDirPath);
  usedAfterCleanup = LittleFS.usedBytes();
  reportBool("REMOVE", removeOk);
  reportBool("RMDIR", rmdirOk);

  Serial.print("+ARDUINO: LITTLEFS,USED_AFTER_CLEANUP,");
  Serial.println((unsigned int)usedAfterCleanup);

  if (mkdirOk &&
      openWriteOk &&
      existsOriginal &&
      renameOk &&
      existsRenamed &&
      openReadOk &&
      rootOpenOk &&
      rootFoundDir &&
      readBack == kPayload &&
      removeOk &&
      rmdirOk) {
    Serial.println("+ARDUINO: LITTLEFS,PASS");
  } else {
    Serial.println("+ARDUINO: LITTLEFS,FAIL,RUNTIME");
  }
}

void loop() {
  delay(1000);
}
