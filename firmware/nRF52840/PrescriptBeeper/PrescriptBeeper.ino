#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <nrf_wdt.h>

BLEDfu  bledfu;
BLEDis  bledis;
BLEUart bleuart;

using namespace Adafruit_LittleFS_Namespace;

const int BUTTON_PASS_PIN = 2;
const int BUTTON_FAIL_PIN = 3;

const unsigned long DEBOUNCE_DELAY = 50;
const int BLE_CHUNK_SIZE = 20;
const int BLE_CHUNK_DELAY_MS = 10;
const int JSON_BUF_SIZE = 2048;
const char* CUSTOMS_FILE = "customs.txt";
const char* STATS_FILE = "stats.txt";

unsigned long lastDebounceTimePass = 0;
unsigned long lastDebounceTimeFail = 0;
int lastPassState = HIGH;
int lastFailState = HIGH;
bool passHandled = false;
bool failHandled = false;

unsigned long displayStartTime = 0;
unsigned long displayDurationMs = 0;
bool isDisplaying = false;
bool isInfinite = false;

struct Stats {
  int achieved;
  int failed;
  int total;
};

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PASS_PIN, INPUT_PULLUP);
  pinMode(BUTTON_FAIL_PIN, INPUT_PULLUP);

  time_t timeout = millis();
  while (!Serial) {
    if ((millis() - timeout) > 5000) break;
    delay(10);
  }

  Serial.println("Prescript Beeper nRF52840");

  InternalFS.begin();

  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName("PrescriptBeeper");

  Bluefruit.Periph.setConnectCallback(onConnect);
  Bluefruit.Periph.setDisconnectCallback(onDisconnect);

  bledfu.begin();

  bledis.setManufacturer("quantdrent");
  bledis.setModel("nRF52840 Super Mini");
  bledis.begin();

  bleuart.begin();

  startAdv();

  NRF_WDT->CONFIG = 0x01;
  NRF_WDT->CRV = 5 * 32768;
  NRF_WDT->RREN = 0x01;
  NRF_WDT->TASKS_START = 1;

  Serial.println("Searching for web ble");
}

void startAdv(void) {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

void onConnect(uint16_t conn_handle) {
  (void) conn_handle;
  Serial.println("BLE connected.");
}

void onDisconnect(uint16_t conn_handle, uint8_t reason) {
  (void) conn_handle;
  (void) reason;
  isDisplaying = false;
  isInfinite = false;
  Serial.println("BLE disconnected.");
}

void loop() {
  NRF_WDT->RR[0] = WDT_RR_RR_Reload;

  handleButtons();
  handleBLE();
  handleDisplayTimer();
  yield();
}

void handleDisplayTimer() {
  if (isDisplaying && !isInfinite) {
    if (millis() - displayStartTime >= displayDurationMs) {
      isDisplaying = false;
      Serial.println("\n[DISPLAY CLEARED - Time expired]");
    }
  }
}

void handleButtons() {
  int passReading = digitalRead(BUTTON_PASS_PIN);
  if (passReading != lastPassState) {
    lastDebounceTimePass = millis();
    if (passReading == HIGH) passHandled = false;
  }
  if (!passHandled && (millis() - lastDebounceTimePass) > DEBOUNCE_DELAY) {
    if (passReading == LOW && bleuart.notifyEnabled()) {
      Serial.println("PASSED (Button)");
      bleuart.print("EVT:PASS\n");
      passHandled = true;
    }
  }
  lastPassState = passReading;

  int failReading = digitalRead(BUTTON_FAIL_PIN);
  if (failReading != lastFailState) {
    lastDebounceTimeFail = millis();
    if (failReading == HIGH) failHandled = false;
  }
  if (!failHandled && (millis() - lastDebounceTimeFail) > DEBOUNCE_DELAY) {
    if (failReading == LOW && bleuart.notifyEnabled()) {
      Serial.println("FAILED (Button)");
      bleuart.print("EVT:FAIL\n");
      failHandled = true;
    }
  }
  lastFailState = failReading;
}

void handleBLE() {
  if (bleuart.available()) {
    String message = bleuart.readStringUntil('\n');
    message.trim();
    if (message.length() > 0) {
      parseCommand(message);
    }
  }
}

void sendChunked(const char* data, int len) {
  for (int i = 0; i < len; i += BLE_CHUNK_SIZE) {
    int chunkLen = min(BLE_CHUNK_SIZE, len - i);
    bleuart.write((const uint8_t*)(data + i), chunkLen);
    delay(BLE_CHUNK_DELAY_MS);
  }
}

void parseCommand(String message) {
  if (message.startsWith("CMD:SHOW")) {
    int durIndex = message.indexOf("DUR:");
    int msgIndex = message.indexOf("MSG:");

    if (durIndex != -1 && msgIndex != -1) {
      String durStr = message.substring(durIndex + 4, msgIndex - 1);
      String msgText = message.substring(msgIndex + 4);

      if (durStr == "INF") {
        isInfinite = true;
      } else {
        isInfinite = false;
        displayDurationMs = durStr.toInt() * 1000UL;
      }

      displayStartTime = millis();
      isDisplaying = true;

      Serial.println(msgText);
      Serial.println(isInfinite ? "INF" : String(displayDurationMs / 1000));
    }
  }
  else if (message.startsWith("CMD:ADD_CUSTOM")) {
    int msgIndex = message.indexOf("MSG:");
    if (msgIndex != -1) {
      String customRule = message.substring(msgIndex + 4);
      saveCustomRule(customRule);
    }
  }
  else if (message == "CMD:GET_CUSTOMS") {
    sendCustomRules();
  }
  else if (message == "CMD:CLEAR_CUSTOMS") {
    InternalFS.remove(CUSTOMS_FILE);
    Serial.println("Cleared custom prescripts.");
  }
  else if (message == "CMD:GET_STATS") {
    sendStats();
  }
  else if (message.startsWith("CMD:ADD_STATS|MSG:")) {
    int splitIdx = message.indexOf("|MSG:");
    if (splitIdx != -1) {
      String statsData = message.substring(splitIdx + 5);
      addStats(statsData);
    }
  }
  else if (message == "CMD:FACTORY_RESET") {
    InternalFS.remove(CUSTOMS_FILE);
    InternalFS.remove(STATS_FILE);
    Serial.println("Factory reset complete.");
    sendStats();
  }
}

void saveCustomRule(String rule) {
  File customsFile(CUSTOMS_FILE, FILE_O_WRITE, InternalFS);
  if (customsFile) {
    customsFile.seek(customsFile.size());
    customsFile.println(rule);
    customsFile.close();
    Serial.println("Saved new custom rule.");
  } else {
    Serial.println("Failed to open customs file for writing.");
  }
}

void sendCustomRules() {
  if (!bleuart.notifyEnabled()) return;

  File customsFile(CUSTOMS_FILE, FILE_O_READ, InternalFS);
  if (!customsFile) {
    const char* empty = "RES:CUSTOMS|MSG:[]\n";
    sendChunked(empty, strlen(empty));
    return;
  }

  char buf[JSON_BUF_SIZE];
  int pos = 0;
  const char* prefix = "RES:CUSTOMS|MSG:[";
  int prefixLen = strlen(prefix);
  memcpy(buf, prefix, prefixLen);
  pos = prefixLen;

  bool first = true;

  while (customsFile.available()) {
    String line = customsFile.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    line.replace("\"", "\\\"");

    int needed = (first ? 0 : 1) + 1 + line.length() + 1;
    if (pos + needed + 3 >= JSON_BUF_SIZE) break;

    if (!first) buf[pos++] = ',';
    buf[pos++] = '"';
    memcpy(buf + pos, line.c_str(), line.length());
    pos += line.length();
    buf[pos++] = '"';
    first = false;
  }

  buf[pos++] = ']';
  buf[pos++] = '\n';
  buf[pos] = '\0';

  customsFile.close();

  sendChunked(buf, pos);
}

Stats parseCSV(const String& csv) {
  Stats s = {0, 0, 0};
  int c1 = csv.indexOf(',');
  int c2 = csv.lastIndexOf(',');
  if (c1 != -1 && c2 != -1 && c1 != c2) {
    s.achieved = csv.substring(0, c1).toInt();
    s.failed = csv.substring(c1 + 1, c2).toInt();
    s.total = csv.substring(c2 + 1).toInt();
  }
  return s;
}

Stats readStatsFromFile() {
  Stats s = {0, 0, 0};
  File f(STATS_FILE, FILE_O_READ, InternalFS);
  if (f) {
    String content = f.readString();
    f.close();
    content.trim();
    if (content.length() > 0) {
      s = parseCSV(content);
    }
  }
  return s;
}

void writeStatsToFile(Stats s) {
  InternalFS.remove(STATS_FILE);
  File f(STATS_FILE, FILE_O_WRITE, InternalFS);
  if (f) {
    f.println(String(s.achieved) + "," + String(s.failed) + "," + String(s.total));
    f.close();
  }
}

void sendStats() {
  if (!bleuart.notifyEnabled()) return;
  Stats s = readStatsFromFile();
  String msg = "RES:STATS|MSG:" + String(s.achieved) + "," + String(s.failed) + "," + String(s.total) + "\n";
  sendChunked(msg.c_str(), msg.length());
}

void addStats(String diffData) {
  Stats diff = parseCSV(diffData);

  if (diff.achieved > 0) Serial.println("PASSED");
  if (diff.failed > 0) Serial.println("FAILED");

  Stats curr = readStatsFromFile();
  curr.achieved += diff.achieved;
  curr.failed += diff.failed;
  curr.total += diff.total;

  writeStatsToFile(curr);
  sendStats();
}
