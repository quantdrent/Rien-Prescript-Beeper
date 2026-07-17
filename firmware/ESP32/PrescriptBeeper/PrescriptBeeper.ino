//this is still experimental i havent have the chance to test this thuroughly if you decided to use this version please let me know
#include <Arduino.h>
#include "config.h"
#include "display.h"
#include "storage.h"
#include "ble_handler.h"

unsigned long lastDebounceTimePass = 0;
unsigned long lastDebounceTimeFail = 0;
int lastPassState = HIGH;
int lastFailState = HIGH;
bool passHandled = false;
bool failHandled = false;

bool pendingDisplayUpdate = false;
bool isDisplaying = false;
bool isInfinite = false;
unsigned long displayStartTime = 0;
unsigned long displayDurationMs = 0;
String currentDisplayText = "";

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PASS_PIN, INPUT_PULLUP);
  pinMode(BUTTON_FAIL_PIN, INPUT_PULLUP);

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
  } else {
    Serial.println("LittleFS Mounted");
  }

  readSettings();

  displayInit();
  displayIdle();

  bleInit();
  startAdv();

  randomSeed(esp_random());

  lastPassState = digitalRead(BUTTON_PASS_PIN);
  lastFailState = digitalRead(BUTTON_FAIL_PIN);
  passHandled = true;
  failHandled = true;
  lastDebounceTimePass = millis();
  lastDebounceTimeFail = millis();
}

bool simulatePass = false;
bool simulateFail = false;
bool timerTimeoutFail = false;
bool hasShownPrescript = false;
bool requiresResponse = true;

void showPrescriptOnDisplay(const char* text, unsigned long durationMs, bool infinite, bool respond) {
  currentDisplayText = String(text);
  isDisplaying = true;
  isInfinite = infinite;
  displayDurationMs = durationMs;
  requiresResponse = respond;

  if (currentDisplayText != "CLEAR." && currentDisplayText != "FAILED." && currentDisplayText != "Connected." && currentDisplayText != "Disconnected.") {
    if (!currentDisplayText.startsWith("PIN: ")) {
      addStats("0,0,1");
    }
  }

  beginScramble(text);

  displayStartTime = millis();
  hasShownPrescript = true;
}

void handleDisplayTimer() {
  if (!hasShownPrescript) return;
  if (isDisplaying && !isInfinite && !displayScrambling) {
    unsigned long elapsed = millis() - displayFinishedTime;
    if (elapsed < displayDurationMs) {
      updateTimerDisplay(displayDurationMs - elapsed);
    } else {
      if (requiresResponse && currentDisplayText != "CLEAR." && currentDisplayText != "FAILED." && currentDisplayText != "Connected." && currentDisplayText != "Disconnected." && !currentDisplayText.startsWith("PIN: ")) {
        timerTimeoutFail = true;
      } else {
        isDisplaying = false;
        currentDisplayText = "";
        displayIdle();
      }
    }
  }
}

void handleButtons() {
  int passReading = digitalRead(BUTTON_PASS_PIN);
  if (passReading != lastPassState) {
    lastDebounceTimePass = millis();
    if (passReading == HIGH) passHandled = false;
  }
  bool passTriggered = (!passHandled && passReading == LOW && (millis() - lastDebounceTimePass) > DEBOUNCE_DELAY);

  if (passTriggered || simulatePass) {
    bool fromWeb = simulatePass;
    simulatePass = false;
    if (passTriggered) passHandled = true;

    bool isIdleOrFinished = (!isDisplaying || currentDisplayText == "CLEAR." || currentDisplayText == "FAILED." || currentDisplayText == "Connected." || currentDisplayText == "Disconnected." || !requiresResponse);
    if (isIdleOrFinished && !fromWeb) {
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
      } else {
        showPrescriptOnDisplay("NO DATA", 1000, false, false);
      }
    } else {
      if (requiresResponse && currentDisplayText != "CLEAR." && currentDisplayText != "FAILED.") {
        addStats("1,0,0");
        if (bleNotifyReady() && !fromWeb) {
          String res = "EVT:PASS\n";
          sendChunked(res.c_str(), res.length());
        }
        showPrescriptOnDisplay("CLEAR.", 800, false, false);
      }
    }
  }
  lastPassState = passReading;

  int failReading = digitalRead(BUTTON_FAIL_PIN);
  if (failReading != lastFailState) {
    lastDebounceTimeFail = millis();
    if (failReading == HIGH) failHandled = false;
  }
  bool failTriggered = (!failHandled && failReading == LOW && (millis() - lastDebounceTimeFail) > DEBOUNCE_DELAY);

  if (failTriggered || simulateFail || timerTimeoutFail) {
    bool fromWeb = simulateFail;
    simulateFail = false;
    bool wasTimeout = timerTimeoutFail;
    timerTimeoutFail = false;

    if (failTriggered) failHandled = true;

    if (isDisplaying || fromWeb || wasTimeout) {
      if (requiresResponse && currentDisplayText != "CLEAR." && currentDisplayText != "FAILED.") {
        addStats("0,1,0");
        if (bleNotifyReady() && !fromWeb) {
          String res = "EVT:FAIL\n";
          sendChunked(res.c_str(), res.length());
        }
        showPrescriptOnDisplay("FAILED.", 800, false, false);
      } else if (!requiresResponse && wasTimeout) {
        displayIdle();
      }
    }
  }
  lastFailState = failReading;
}

void loop() {
  handleButtons();
  handleBLE();
  handleDisplayScramble();
  handleDisplayTimer();

  if (pendingDisplayUpdate) {
    pendingDisplayUpdate = false;
    if (String(pendingDisplayText).startsWith("PIN: ")) {
      showPrescriptOnDisplay(pendingDisplayText, 30000, false, false);
    } else {
      showPrescriptOnDisplay(pendingDisplayText, 2000, false, true);
    }
  }

  if (!displayScrambling) {
    flushStatsIfNeeded();
  }

  yield();
}
