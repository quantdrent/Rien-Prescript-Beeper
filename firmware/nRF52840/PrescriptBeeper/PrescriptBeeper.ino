#include <nrf_wdt.h>
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

unsigned long displayStartTime = 0;
unsigned long displayDurationMs = 0;
bool isDisplaying = false;
bool isInfinite = false;
String currentDisplayText = "";

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PASS_PIN, INPUT_PULLUP);
  pinMode(BUTTON_FAIL_PIN, INPUT_PULLUP);

  InternalFS.begin();
  displayInit();
  displayIdle();

  bleInit();
  startAdv();

  NRF_WDT->CONFIG = 0x01;
  NRF_WDT->CRV = 32 * 32768;
  NRF_WDT->RREN = 0x01;
  NRF_WDT->TASKS_START = 1;

  readSettings();
  randomSeed(analogRead(A0));
}

void loop() {
  NRF_WDT->RR[0] = WDT_RR_RR_Reload;
  handleButtons();
  handleBLE();
  updateScramble();
  handleDisplayTimer();
  yield();
}

bool simulatePass = false;
bool simulateFail = false;
bool timerTimeoutFail = false;

void showPrescriptOnDisplay(const char* text, unsigned long durationMs, bool infinite) {
  currentDisplayText = String(text);
  isDisplaying = true;
  isInfinite = infinite;
  displayDurationMs = durationMs;

  beginScramble(text);

  displayStartTime = millis();
}

void handleDisplayTimer() {
  if (isDisplaying && !isInfinite && !displayScrambling) {
    if (millis() - displayFinishedTime >= displayDurationMs) {
      if (currentDisplayText != "CLEAR." && currentDisplayText != "FAILED.") {
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

    if (!isDisplaying && !fromWeb) {
      String line = getRandomPrescript();
      if (line.length() > 0) {
        int dur = 10;
        String text = "";
        if (parsePrescriptLine(line, dur, text)) {
          showPrescriptOnDisplay(text.c_str(), dur * 1000UL, false);
        }
      } else {
        displayStatus("NO DATA");
        delay(1000);
        displayIdle();
      }
    } else {
      if (currentDisplayText != "CLEAR." && currentDisplayText != "FAILED.") {
        if (bleuart.notifyEnabled() && !fromWeb) bleuart.print("EVT:PASS\n");
        showPrescriptOnDisplay("CLEAR.", 800, false);
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
      if (currentDisplayText != "CLEAR." && currentDisplayText != "FAILED.") {
        if (bleuart.notifyEnabled() && !fromWeb) bleuart.print("EVT:FAIL\n");
        showPrescriptOnDisplay("FAILED.", 800, false);
      }
    }
  }
  lastFailState = failReading;
}
