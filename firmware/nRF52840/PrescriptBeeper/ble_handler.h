#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

#include <bluefruit.h>
#include "config.h"

BLEDfu  bledfu;
BLEDis  bledis;
BLEUart bleuart;

void saveCustomRule(String rule);
void sendCustomRules();
void sendStats();
void addStats(String diffData);
void readSettings();
void writeSettings();
void sendSettings();
void showPrescriptOnDisplay(const char* text, unsigned long durationMs, bool infinite);
extern int textScale;
extern bool isDisplaying;
extern bool isInfinite;
extern unsigned long displayStartTime;
extern unsigned long displayDurationMs;
extern String currentDisplayText;

extern bool simulatePass;
extern bool simulateFail;

bool bleNotifyReady() {
  return bleuart.notifyEnabled();
}

void sendChunked(const char* data, int len) {
  for (int i = 0; i < len; i += BLE_CHUNK_SIZE) {
    int chunkLen = min(BLE_CHUNK_SIZE, len - i);
    bleuart.write((const uint8_t*)(data + i), chunkLen);
    delay(BLE_CHUNK_DELAY_MS);
  }
}

void onConnect(uint16_t conn_handle) {
  (void) conn_handle;
  showPrescriptOnDisplay("Connected.", 2000, false);
}

void onDisconnect(uint16_t conn_handle, uint8_t reason) {
  (void) conn_handle;
  (void) reason;
  isDisplaying = false;
  isInfinite = false;
}

void parseCommand(String message) {
  if (message == "CMD:PASS") {
    simulatePass = true;
  }
  else if (message == "CMD:FAIL") {
    simulateFail = true;
  }
  else if (message == "CMD:NEXT") {
    if (!isDisplaying) {
      String line = getRandomPrescript();
      if (line.length() > 0) {
        int dur = 10;
        String text = "";
        if (parsePrescriptLine(line, dur, text)) {
          showPrescriptOnDisplay(text.c_str(), dur * 1000UL, false);
          if (bleuart.notifyEnabled()) {
            String evt = "EVT:PRESCRIPT|DUR:" + String(dur) + "|MSG:" + text + "\n";
            sendChunked(evt.c_str(), evt.length());
          }
        }
      }
    }
  }
  else if (message.startsWith("CMD:SHOW")) {
    int durIndex = message.indexOf("DUR:");
    int msgIndex = message.indexOf("MSG:");

    if (durIndex != -1 && msgIndex != -1) {
      String durStr = message.substring(durIndex + 4, msgIndex - 1);
      String msgText = message.substring(msgIndex + 4);

      bool inf = (durStr == "INF");
      unsigned long durMs = inf ? 0 : durStr.toInt() * 1000UL;

      showPrescriptOnDisplay(msgText.c_str(), durMs, inf);
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
    InternalFS.remove(SETTINGS_FILE);
    textScale = DEFAULT_TEXT_SCALE;
    sendStats();
  }
  else if (message == "CMD:GET_SETTINGS") {
    sendSettings();
  }
  else if (message.startsWith("CMD:SET_SETTINGS|MSG:")) {
    int msgIndex = message.indexOf("MSG:");
    if (msgIndex != -1) {
      String data = message.substring(msgIndex + 4);
      int c1 = data.indexOf(',');
      int c2 = data.indexOf(',', c1 + 1);
      int c3 = data.indexOf(',', c2 + 1);
      if (c1 != -1 && c2 != -1 && c3 != -1) {
        textScale = data.substring(0, c1).toInt();
        scrambleDurationFrames = data.substring(c1 + 1, c2).toInt();
        scrambleDelayMs = data.substring(c2 + 1, c3).toInt();
        revealDelayMs = data.substring(c3 + 1).toInt();
        writeSettings();
      } else {
        textScale = data.toInt();
        writeSettings();
      }
    }
  }
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

void bleInit() {
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
}

void startAdv() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(bleuart);
  Bluefruit.ScanResponse.addName();

  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);
  Bluefruit.Advertising.setFastTimeout(30);
  Bluefruit.Advertising.start(0);
}

#endif
