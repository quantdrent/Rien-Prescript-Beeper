#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "config.h"

#define SERVICE_UUID           "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
volatile bool deviceConnected = false;
volatile bool oldDeviceConnected = false;

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
  return deviceConnected;
}

void sendChunked(const char* data, int len) {
  if (!deviceConnected) return;
  for (int i = 0; i < len; i += BLE_CHUNK_SIZE) {
    if (!deviceConnected) break;
    int chunkLen = min(BLE_CHUNK_SIZE, len - i);
    pTxCharacteristic->setValue((uint8_t*)(data + i), chunkLen);
    pTxCharacteristic->notify();
    delay(BLE_CHUNK_DELAY_MS);
  }
}

void onClientConnect() {
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

void onClientDisconnect() {
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
      String res = "RES:AUTH_RESENT\n";
      sendChunked(res.c_str(), res.length());
      return;
    }
    if (message.startsWith("CMD:AUTH|MSG:")) {
      String pin = message.substring(13);
      pin.trim();
      if (pin == String(currentSessionPinBuf)) {
        isAuthenticated = true;
        String res = "RES:AUTH_OK\n";
        sendChunked(res.c_str(), res.length());
        showPrescriptOnDisplay("Connected.", 2000, false);
      } else {
        String res = "RES:AUTH_FAIL\n";
        sendChunked(res.c_str(), res.length());
        pServer->disconnect(pServer->getConnId());
      }
    } else {
      String res = "RES:AUTH_REQUIRED\n";
      sendChunked(res.c_str(), res.length());
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
        if (bleNotifyReady() && idx >= 0) {
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
    LittleFS.remove(CUSTOMS_FILE);
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
    LittleFS.remove(CUSTOMS_FILE);
    LittleFS.remove(STATS_FILE);
    LittleFS.remove(SETTINGS_FILE);
    textScale = DEFAULT_TEXT_SCALE;
    delay(100);
    ESP.restart();
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

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      onClientConnect();
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      onClientDisconnect();
    }
};

String incomingBleBuffer = "";
bool hasNewBleData = false;
portMUX_TYPE bleMux = portMUX_INITIALIZER_UNLOCKED;


class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {
        portENTER_CRITICAL(&bleMux);
        incomingBleBuffer += rxValue.c_str();
        hasNewBleData = true;
        portEXIT_CRITICAL(&bleMux);
      }
    }
};

void bleInit() {
  BLEDevice::init("Beeper");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                       BLECharacteristic::PROPERTY_WRITE
                     );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
}

void startAdv() {
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
}

void handleBLE() {
  if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;
  }
  if (!deviceConnected && oldDeviceConnected) {
      delay(500);
      pServer->startAdvertising();
      oldDeviceConnected = deviceConnected;
  }

  if (hasNewBleData) {
    String localCopy = "";
    portENTER_CRITICAL(&bleMux);
    localCopy = incomingBleBuffer;
    incomingBleBuffer = "";
    hasNewBleData = false;
    portEXIT_CRITICAL(&bleMux);

    while (localCopy.length() > 0) {
      int nl = localCopy.indexOf('\n');
      if (nl != -1) {
        String msg = localCopy.substring(0, nl);
        localCopy = localCopy.substring(nl + 1);
        msg.trim();
        if (msg.length() > 0) {
          parseCommand(msg);
        }
      } else {
        // No newline found yet, put it back
        portENTER_CRITICAL(&bleMux);
        incomingBleBuffer = localCopy + incomingBleBuffer;
        if (incomingBleBuffer.length() > 0) hasNewBleData = true;
        portEXIT_CRITICAL(&bleMux);
        break;
      }
    }
  }
}

#endif
