#include <nrf_wdt.h>
#include "config.h"
#include "display.h"
#include "storage.h"
#include "ble_handler.h"

unsigned long lastDebounceTimePass = 0;
unsigned long lastDebounceTimeFail = 0;
unsigned long lastDebounceTimePower = 0;
int lastPassState = HIGH;
int lastFailState = HIGH;
int lastPowerState = HIGH;
bool passHandled = false;
bool failHandled = false;
unsigned long powerPressedTime = 0;
bool bleRequirePin = false;

void enterSleep() {
  Serial.println("CLEARING DISPLAY");
  isDisplaying = false;
  currentDisplayText = "";
  displayIdle();
  tft.fillScreen(COLOR_BG);

  Serial.println("STOPPING BLE");
  Bluefruit.Advertising.stop();
  if (Bluefruit.connected()) {
    Bluefruit.disconnect(Bluefruit.connHandle());
  }
  delay(200);

  Serial.println("KILLING VCC");
  digitalWrite(EXT_VCC, LOW);
  delay(100);

  Serial.println("WAITING FOR BUTTON RELEASE");
  while(digitalRead(BUTTON_POWER_PIN) == LOW) {
    delay(10);
  }
  delay(300);

  Serial.println("SLEEP STEP 5: CONFIGURING WAKEUP PIN");
  nrf_gpio_cfg_sense_input(g_ADigitalPinMap[BUTTON_POWER_PIN], NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

  Serial.println("ENTERING SYSTEM OFF");
  Serial.flush();
  delay(100);

  sd_power_system_off();
  while(1) {
    __WFE();
  }
}

unsigned long displayStartTime = 0;
unsigned long displayDurationMs = 0;
bool isDisplaying = false;
bool isInfinite = false;
String currentDisplayText = "";
bool pendingDisplayUpdate = false;

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PASS_PIN, INPUT_PULLUP);
  pinMode(BUTTON_FAIL_PIN, INPUT_PULLUP);
  pinMode(BUTTON_POWER_PIN, INPUT_PULLUP);

  pinMode(EXT_VCC, OUTPUT);
  digitalWrite(EXT_VCC, HIGH);

  InternalFS.begin();
  readSettings();

  displayInit();
  displayIdle();

  bleInit();
  startAdv();

  NRF_WDT->CONFIG = 0x01;
  NRF_WDT->CRV = 32 * 32768;
  NRF_WDT->RREN = 0x01;
  NRF_WDT->TASKS_START = 1;

  randomSeed(analogRead(A0));

  lastPassState = digitalRead(BUTTON_PASS_PIN);
  lastFailState = digitalRead(BUTTON_FAIL_PIN);
  lastPowerState = digitalRead(BUTTON_POWER_PIN);
  passHandled = true;
  failHandled = true;
  lastDebounceTimePass = millis();
  lastDebounceTimeFail = millis();
  lastDebounceTimePower = millis();
}

void loop() {
  NRF_WDT->RR[0] = WDT_RR_RR_Reload;
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

  static String serialBuffer = "";
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      serialBuffer.trim();
      if (serialBuffer.length() > 0) {
        parseCommand(serialBuffer);
      }
      serialBuffer = "";
    } else {
      serialBuffer += c;
    }
  }

  if (!displayScrambling) {
    flushStatsIfNeeded();
  }

  yield();
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

  if (currentDisplayText != "CLEAR." && currentDisplayText != "FAILED." && currentDisplayText != "Connected.") {
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
      if (requiresResponse && currentDisplayText != "CLEAR." && currentDisplayText != "FAILED." && currentDisplayText != "Connected." && !currentDisplayText.startsWith("PIN: ")) {
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

    bool isIdleOrFinished = (!isDisplaying || currentDisplayText == "CLEAR." || currentDisplayText == "FAILED." || currentDisplayText == "Connected." || !requiresResponse);
    if (isIdleOrFinished && !fromWeb) {
      int idx = -1;
      String line = getRandomPrescript(&idx);
      if (line.length() > 0) {
        int dur = 10;
        bool respond = true;
        String text = "";
        if (parsePrescriptLine(line, dur, respond, text)) {
          showPrescriptOnDisplay(text.c_str(), dur * 1000UL, false, respond);
          if (bleuart.notifyEnabled() && idx >= 0) {
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
      if (requiresResponse && currentDisplayText != "CLEAR." && currentDisplayText != "FAILED.") {
        addStats("0,1,0");
        if (bleuart.notifyEnabled() && !fromWeb) bleuart.print("EVT:FAIL\n");
        showPrescriptOnDisplay("FAILED.", 800, false);
      } else if (!requiresResponse && wasTimeout) {
        displayIdle();
      }
    }
  }
  lastFailState = failReading;

  int powerReading = digitalRead(BUTTON_POWER_PIN);
  if (powerReading != lastPowerState) {
    lastDebounceTimePower = millis();
  }

  if ((millis() - lastDebounceTimePower) > DEBOUNCE_DELAY) {
    if (powerReading == LOW) {
      if (powerPressedTime == 0) {
        powerPressedTime = millis();
        Serial.println("PWR BTN: PRESSED");
      } else if (millis() - powerPressedTime > 1000) {
        Serial.println("PWR BTN: SLEEP TRIGGERED");
        enterSleep();
      }
    } else {
      if (powerPressedTime != 0) {
        Serial.println("PWR BTN: RELEASED");
      }
      powerPressedTime = 0;
    }
  }
  lastPowerState = powerReading;
}
