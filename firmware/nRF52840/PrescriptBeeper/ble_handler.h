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
void showPrescriptOnDisplay(const char* text, unsigned long durationMs, bool infinite, bool respond = true);
bool isAuthenticated = false;
char currentSessionPinBuf[8] = {0};
char pendingDisplayText[64] = {0};
extern bool pendingDisplayUpdate;
extern int textScale;
extern int scrambleDurationFrames;
extern int scrambleDelayMs;
extern int revealDelayMs;
extern int timerPosition;
extern float timerScale;
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
  if (bleRequirePin) {
    isAuthenticated = false;
    long pinVal = random(100000, 999999);
    snprintf(currentSessionPinBuf, sizeof(currentSessionPinBuf), "%ld", pinVal);
    snprintf(pendingDisplayText, sizeof(pendingDisplayText), "PIN: %ld", pinVal);
    pendingDisplayUpdate = true;
  } else {
    isAuthenticated = true;
    snprintf(pendingDisplayText, sizeof(pendingDisplayText), "Connected.");
    pendingDisplayUpdate = true;
  }
}

void onDisconnect(uint16_t conn_handle, uint8_t reason) {
  (void) conn_handle;
  (void) reason;
  if (isAuthenticated) {
    snprintf(pendingDisplayText, sizeof(pendingDisplayText), "Disconnected.");
    pendingDisplayUpdate = true;
  } else {
    isDisplaying = false;
    isInfinite = false;
  }
}

void parseCommand(String message) {
  if (bleRequirePin && !isAuthenticated) {
    if (message == "CMD:AUTH_RESEND") {
      long pinVal = random(100000, 999999);
      snprintf(currentSessionPinBuf, sizeof(currentSessionPinBuf), "%ld", pinVal);
      snprintf(pendingDisplayText, sizeof(pendingDisplayText), "PIN: %ld", pinVal);
      pendingDisplayUpdate = true;
      bleuart.print("RES:AUTH_RESENT\n");
      return;
    }
    if (message.startsWith("CMD:AUTH|MSG:")) {
      String pin = message.substring(13);
      pin.trim();
      if (pin == String(currentSessionPinBuf)) {
        isAuthenticated = true;
        bleuart.print("RES:AUTH_OK\n");
        showPrescriptOnDisplay("Connected.", 2000, false);
      } else {
        bleuart.print("RES:AUTH_FAIL\n");
        Bluefruit.disconnect(0);
      }
    } else {
      bleuart.print("RES:AUTH_REQUIRED\n");
    }
    return;
  }

  if (message == "CMD:PASS") {
    simulatePass = true;
  }
  else if (message == "CMD:FAIL") {
    simulateFail = true;
  }
  else if (message == "CMD:NEXT") {
    simulatePass = false;
    simulateFail = false;

    int idx = -1;
    String line = getRandomPrescript(&idx);
    if (line.length() > 0) {
      int dur = 10;
      bool respond = true;
      String text = "";
      if (parsePrescriptLine(line, dur, respond, text)) {
        showPrescriptOnDisplay(text.c_str(), dur * 1000UL, dur == 0, respond);
        if (bleuart.notifyEnabled() && idx >= 0) {
          String evt = "EVT:PRESCRIPT|IDX:" + String(idx) + "\n";
          sendChunked(evt.c_str(), evt.length());
        }
      }
    }
  }
  else if (message.startsWith("CMD:SHOW_IDX:")) {
    int idx = message.substring(13).toInt();
    String line = getPrescriptByIndex(idx);
    if (line.length() > 0) {
      int dur = 10;
      bool respond = true;
      String text = "";
      if (parsePrescriptLine(line, dur, respond, text)) {
        showPrescriptOnDisplay(text.c_str(), dur * 1000UL, dur == 0, respond);
      }
    }
  }
  else if (message.startsWith("CMD:SHOW")) {
    int durIndex = message.indexOf("DUR:");
    int resIndex = message.indexOf("RES:");
    int msgIndex = message.indexOf("MSG:");

    if (durIndex != -1 && msgIndex != -1) {
      String durStr = message.substring(durIndex + 4, (resIndex != -1) ? resIndex - 1 : msgIndex - 1);

      bool respond = true;
      if (resIndex != -1) {
          respond = (message.substring(resIndex + 4, msgIndex - 1) != "0");
      }

      String msgText = message.substring(msgIndex + 4);

      bool inf = (durStr == "INF" || durStr == "-" || durStr == "0");
      unsigned long durMs = inf ? 0 : durStr.toInt() * 1000UL;

      showPrescriptOnDisplay(msgText.c_str(), durMs, inf, respond);
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
    invalidatePrescriptCache();
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
    Bluefruit.Periph.clearBonds();
    textScale = DEFAULT_TEXT_SCALE;
    delay(100);
    NVIC_SystemReset();
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
      int c4 = data.indexOf(',', c3 + 1);
      int c5 = data.indexOf(',', c4 + 1);
      int c6 = data.indexOf(',', c5 + 1);
      int c7 = data.indexOf(',', c6 + 1);
      int c8 = data.indexOf(',', c7 + 1);

      int c9 = data.indexOf(',', c8 + 1);

      if (c1 != -1 && c2 != -1 && c3 != -1) {
        textScale = data.substring(0, c1).toInt();
        scrambleDurationFrames = data.substring(c1 + 1, c2).toInt();
        scrambleDelayMs = data.substring(c2 + 1, c3).toInt();

        if (c4 != -1 && c5 != -1) {
          revealDelayMs = data.substring(c3 + 1, c4).toInt();
          timerPosition = data.substring(c4 + 1, c5).toInt();

          if (c6 != -1) {
            timerScale = data.substring(c5 + 1, c6).toFloat();
            if (c7 != -1) {
              bleRequirePin = data.substring(c6 + 1, c7).toInt() != 0;
              if (c8 != -1) {
                useProportionalFont = data.substring(c7 + 1, c8).toInt() != 0;
                if (c9 != -1) {
                  textColor = (uint16_t)data.substring(c8 + 1, c9).toInt();
                  timerFormatLong = data.substring(c9 + 1).toInt() != 0;
                } else {
                  textColor = (uint16_t)data.substring(c8 + 1).toInt();
                  timerFormatLong = false;
                }
              } else {
                useProportionalFont = data.substring(c7 + 1).toInt() != 0;
                textColor = DEFAULT_TEXT_COLOR;
                timerFormatLong = false;
              }
            } else {
              bleRequirePin = data.substring(c6 + 1).toInt() != 0;
            }
          } else {
            timerScale = data.substring(c5 + 1).toFloat();
          }
        } else {
          revealDelayMs = data.substring(c3 + 1).toInt();
        }

        writeSettings();
      } else {
        textScale = data.toInt();
        writeSettings();
      }
    }
  }
}

String bleBuffer = "";

void handleBLE() {
  while (bleuart.available()) {
    char c = bleuart.read();
    if (c == '\n') {
      bleBuffer.trim();
      if (bleBuffer.length() > 0) {
        parseCommand(bleBuffer);
      }
      bleBuffer = "";
    } else {
      bleBuffer += c;
    }
  }
}

void bleInit() {
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);
  Bluefruit.begin();

  ble_gap_addr_t addr;
  sd_ble_gap_addr_get(&addr);
  addr.addr[0] += 1;
  sd_ble_gap_addr_set(&addr);

  Bluefruit.setTxPower(4);
  Bluefruit.setName("Beeper");

  Bluefruit.Periph.setConnectCallback(onConnect);
  Bluefruit.Periph.setDisconnectCallback(onDisconnect);

  bledfu.begin();

  bledis.setManufacturer("quantdrent");
  bledis.setModel("nRF52840 Super Mini");
  bledis.begin();

  bleuart.setPermission(SECMODE_OPEN, SECMODE_OPEN);
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
