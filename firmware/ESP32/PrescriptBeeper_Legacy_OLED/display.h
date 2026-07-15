#ifndef DISPLAY_H
#define DISPLAY_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "config.h"

extern int timerPosition;
extern float timerScale;
extern int textScale;
extern bool isInfinite;
extern unsigned long displayDurationMs;
extern bool useProportionalFont;

Adafruit_SH1106G oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

extern int scrambleDurationFrames;
extern int scrambleDelayMs;
extern int revealDelayMs;

static const char SCRAMBLE_CHARS[] = "ABCDEF@HIJ_LM%OPQR^WX#YZa#b+cdefgh*iqrxyz0123456789";
static const int NUM_SCRAMBLE_CHARS = sizeof(SCRAMBLE_CHARS) - 1;

static char finalLines[6][64];
static char workLines[6][64];
static int g_lineCount, g_startY, g_charH;
static int g_textSz = 1;

static char timerFinalStr[32];
static char timerWorkStr[32];
static int g_timerY = 0;

static bool displayScrambling = false;
static unsigned long displayScrambleStartTime = 0;
static unsigned long displayLastUpdate = 0;
static int displayScramblePhase = 0;
static int displayRevealLine = 0;
static int displayRevealChar = 0;
static int displayScrambleFrames = 0;
unsigned long displayFinishedTime = 0;

void drawOledBuffer();

void displayInit() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  if (!oled.begin(0x3C, true)) {
    Serial.println("OLED FAILED");
  }
  oled.setRotation(2);
  oled.clearDisplay();
  oled.display();
}

void displayClear() {
  oled.clearDisplay();
  oled.display();
}

const uint8_t PROP_WIDTHS[96] = {
  3, 1, 3, 5, 5, 5, 5, 3, 3, 3, 5, 5, 3, 5, 2, 5,
  5, 3, 5, 5, 5, 5, 5, 5, 5, 5, 1, 2, 4, 5, 4, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 3, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 4, 5, 4, 5, 5,
  3, 5, 5, 5, 5, 5, 4, 5, 5, 3, 4, 4, 3, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 3, 1, 3, 5, 5
};

const uint8_t PROP_OFFSETS[96] = {
  0, 2, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 2, 0, 3, 0,
  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 0, 1, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0,
  1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0
};

int getTextWidth(const char* text, int sz) {
  if (!useProportionalFont) {
    return strlen(text) * 6 * sz;
  }
  int w = 0;
  while (*text) {
    unsigned char c = *text++;
    if (c >= 32 && c <= 127) {
      w += (PROP_WIDTHS[c - 32] + 1) * sz;
    } else {
      w += 6 * sz;
    }
  }
  return w;
}

int buildLayout(const char* text, char linesBuf[][64], int* lineCount, int* outStartY, int* outCharH) {
  int sz = textScale > 0 ? textScale : 1;
  int maxW = SCREEN_WIDTH - 4;
  int maxH = SCREEN_HEIGHT;

  while (sz >= 1) {
    int charH = 8 * sz;

    int lines = 1;
    char lineStr[64];
    memset(lineStr, 0, sizeof(lineStr));
    int lineLen = 0;
    const char* p = text;

    while (*p) {
      if (*p == ' ') {
        const char* q = p + 1;
        int wl = 0;
        char wordBuf[32];
        memset(wordBuf, 0, sizeof(wordBuf));
        while (*q && *q != ' ' && wl < 31) { wordBuf[wl++] = *q++; }

        char testBuf[64];
        if (lineLen == 0) snprintf(testBuf, sizeof(testBuf), "%s", wordBuf);
        else snprintf(testBuf, sizeof(testBuf), "%s %s", lineStr, wordBuf);

        if (getTextWidth(testBuf, sz) > maxW && lineLen > 0) {
          lines++;
          memset(lineStr, 0, sizeof(lineStr));
          lineLen = 0;
          p++;
          continue;
        }
      }

      lineStr[lineLen] = *p;
      lineStr[lineLen + 1] = '\0';

      if (getTextWidth(lineStr, sz) > maxW && lineLen > 0) {
        lines++;
        lineStr[0] = *p;
        lineStr[1] = '\0';
        lineLen = 1;
      } else {
        lineLen++;
      }
      p++;
    }

    if (lines * charH <= maxH) break;
    sz--;
  }

  if (sz < 1) sz = 1;

  int charH = 8 * sz;
  *outCharH = charH;
  for (int i = 0; i < 6; i++) memset(linesBuf[i], 0, 64);
  int li = 0, charIdx = 0;
  const char* p = text;

  while (*p && li < 6) {
    if (*p == ' ') {
      const char* q = p + 1;
      int wl = 0;
      char wordBuf[32];
      memset(wordBuf, 0, sizeof(wordBuf));
      while (*q && *q != ' ' && wl < 31) { wordBuf[wl++] = *q++; }

      char testBuf[64];
      if (charIdx == 0) snprintf(testBuf, sizeof(testBuf), "%s", wordBuf);
      else snprintf(testBuf, sizeof(testBuf), "%s %s", linesBuf[li], wordBuf);

      if (getTextWidth(testBuf, sz) > maxW && charIdx > 0) {
        li++; charIdx = 0; p++;
        if (li >= 6) break;
        continue;
      }
    }
    linesBuf[li][charIdx] = *p;
    linesBuf[li][charIdx + 1] = '\0';
    if (getTextWidth(linesBuf[li], sz) > maxW && charIdx > 0) {
      linesBuf[li][charIdx] = '\0';
      li++; charIdx = 0;
      if (li >= 6) break;
      linesBuf[li][charIdx++] = *p;
    } else charIdx++;
    p++;
  }
  *lineCount = li + 1;
  return sz;
}

void setupTimerDisplay(const char* targetText, unsigned long durationMs) {
  if (timerPosition == 0 || durationMs == 0 || isInfinite) {
    timerFinalStr[0] = '\0';
    timerWorkStr[0] = '\0';
    return;
  }

  if (strcmp(targetText, "CLEAR.") == 0 || strcmp(targetText, "FAILED.") == 0 || strcmp(targetText, "Connected.") == 0 || strcmp(targetText, "NO DATA") == 0) {
    timerFinalStr[0] = '\0';
    timerWorkStr[0] = '\0';
    return;
  }

  unsigned long t = durationMs / 1000;
  if (t < 60) snprintf(timerFinalStr, sizeof(timerFinalStr), "%lus", t);
  else {
    unsigned long h = t / 3600;
    unsigned long m = (t % 3600) / 60;
    unsigned long s = t % 60;
    if (h > 0) {
      if (s > 0) snprintf(timerFinalStr, sizeof(timerFinalStr), "%luh %lum %lus", h, m, s);
      else if (m > 0) snprintf(timerFinalStr, sizeof(timerFinalStr), "%luh %lum", h, m);
      else snprintf(timerFinalStr, sizeof(timerFinalStr), "%luh", h);
    } else {
      if (s > 0) snprintf(timerFinalStr, sizeof(timerFinalStr), "%lum %lus", m, s);
      else snprintf(timerFinalStr, sizeof(timerFinalStr), "%lum", m);
    }
  }
  strcpy(timerWorkStr, timerFinalStr);

  if (timerPosition >= 1 && timerPosition <= 3) g_timerY = 1;
  else g_timerY = SCREEN_HEIGHT - 2 - 8;
}

void updateTimerDisplay(unsigned long remainingMs) {
  if (timerPosition == 0 || isInfinite || timerFinalStr[0] == '\0') return;

  char newTimerStr[32];
  unsigned long t = (remainingMs + 999) / 1000;
  if (t < 60) snprintf(newTimerStr, sizeof(newTimerStr), "%lus", t);
  else {
    unsigned long h = t / 3600;
    unsigned long m = (t % 3600) / 60;
    unsigned long s = t % 60;
    if (h > 0) {
      if (s > 0) snprintf(newTimerStr, sizeof(newTimerStr), "%luh %lum %lus", h, m, s);
      else if (m > 0) snprintf(newTimerStr, sizeof(newTimerStr), "%luh %lum", h, m);
      else snprintf(newTimerStr, sizeof(newTimerStr), "%luh", h);
    } else {
      if (s > 0) snprintf(newTimerStr, sizeof(newTimerStr), "%lum %lus", m, s);
      else snprintf(newTimerStr, sizeof(newTimerStr), "%lum", m);
    }
  }

  if (strcmp(timerFinalStr, newTimerStr) != 0) {
    strcpy(timerFinalStr, newTimerStr);
    if (!displayScrambling) {
        strcpy(timerWorkStr, newTimerStr);
        drawOledBuffer();
    }
  }
}

void beginScramble(const char* targetText) {
  char finalStr[128];
  snprintf(finalStr, sizeof(finalStr), "_%s_", targetText);

  char tempLines[6][64];
  int tempCount;

  g_textSz = buildLayout(finalStr, tempLines, &tempCount, &g_startY, &g_charH);
  g_lineCount = (tempCount > 6) ? 6 : tempCount;

  for (int i = 0; i < 6; i++) {
    memset(finalLines[i], 0, 64);
    memset(workLines[i], 0, 64);
  }

  for (int i = 0; i < g_lineCount; i++) {
    strncpy(finalLines[i], tempLines[i], 63);
  }

  int blockH = g_lineCount * g_charH;
  g_startY = (SCREEN_HEIGHT - blockH) / 2;
  if (g_startY < 0) g_startY = 0;

  setupTimerDisplay(targetText, displayDurationMs);

  displayScrambling = true;
  displayScrambleStartTime = millis();
  displayLastUpdate = 0;
  displayScramblePhase = 0;
  displayRevealLine = 0;
  displayRevealChar = 0;
  displayScrambleFrames = 0;
}

void drawOledBuffer() {
  oled.clearDisplay();
  oled.setTextColor(SH110X_WHITE);
  oled.setFont(NULL);

  for (int i = 0; i < g_lineCount; i++) {
    int w = getTextWidth(workLines[i], g_textSz);
    int currentX = (SCREEN_WIDTH - w) / 2;
    if (currentX < 0) currentX = 0;

    int yPos = g_startY + (i * g_charH);

    if (!useProportionalFont) {
      oled.setTextSize(g_textSz);
      oled.setCursor(currentX, yPos);
      oled.print(workLines[i]);
    } else {
      int cx = currentX;
      for (int j = 0; j < strlen(workLines[i]); j++) {
        unsigned char c = workLines[i][j];
        if (c >= 32 && c <= 127) {
          int offset = PROP_OFFSETS[c - 32];
          oled.drawChar(cx - offset, yPos, c, SH110X_WHITE, SH110X_WHITE, g_textSz);
          cx += (PROP_WIDTHS[c - 32] + 1) * g_textSz;
        } else {
          oled.drawChar(cx, yPos, c, SH110X_WHITE, SH110X_BLACK, g_textSz);
          cx += 6 * g_textSz;
        }
      }
    }
  }

  if (timerFinalStr[0] != '\0') {
    int w = getTextWidth(timerWorkStr, 1);
    int currentTimerX = 0;
    if (timerPosition == 1 || timerPosition == 4) currentTimerX = 2;
    else if (timerPosition == 2 || timerPosition == 5) currentTimerX = (SCREEN_WIDTH - w) / 2;
    else if (timerPosition == 3 || timerPosition == 6) currentTimerX = SCREEN_WIDTH - w - 2;
    if (currentTimerX < 0) currentTimerX = 0;

    if (!useProportionalFont) {
      oled.setTextSize(1);
      oled.setCursor(currentTimerX, g_timerY);
      oled.print(timerWorkStr);
    } else {
      int cx = currentTimerX;
      for (int j = 0; j < strlen(timerWorkStr); j++) {
        unsigned char c = timerWorkStr[j];
        if (c >= 32 && c <= 127) {
          int offset = PROP_OFFSETS[c - 32];
          oled.drawChar(cx - offset, g_timerY, c, SH110X_WHITE, SH110X_WHITE, 1);
          cx += (PROP_WIDTHS[c - 32] + 1);
        } else {
          oled.drawChar(cx, g_timerY, c, SH110X_WHITE, SH110X_BLACK, 1);
          cx += 6;
        }
      }
    }
  }

  oled.display();
}

void handleDisplayScramble() {
  if (!displayScrambling) return;

  unsigned long ms = millis();

  if (displayScramblePhase == 0) {
    if (ms - displayLastUpdate >= scrambleDelayMs) {
      displayLastUpdate = ms;
      displayScrambleFrames++;

      for (int i = 0; i < g_lineCount; i++) {
        int llen = strlen(finalLines[i]);
        for (int j = 0; j < llen; j++) {
          if (finalLines[i][j] != ' ')
            workLines[i][j] = SCRAMBLE_CHARS[random(0, NUM_SCRAMBLE_CHARS)];
          else
            workLines[i][j] = ' ';
        }
      }

      if (timerFinalStr[0] != '\0') {
        int tlen = strlen(timerFinalStr);
        for (int j = 0; j < tlen; j++) {
          if (timerFinalStr[j] != ' ')
            timerWorkStr[j] = SCRAMBLE_CHARS[random(0, NUM_SCRAMBLE_CHARS)];
          else
            timerWorkStr[j] = ' ';
        }
        timerWorkStr[tlen] = '\0';
      }

      drawOledBuffer();

      if (displayScrambleFrames >= scrambleDurationFrames) {
        displayScramblePhase = 1;
        displayLastUpdate = ms;

        if (timerFinalStr[0] != '\0') {
            strcpy(timerWorkStr, timerFinalStr);
        }
      }
    }
  }
  else if (displayScramblePhase == 1) {
    if (ms - displayLastUpdate >= revealDelayMs) {
      displayLastUpdate = ms;

      if (displayRevealLine < g_lineCount) {
        int len = strlen(finalLines[displayRevealLine]);
        if (displayRevealChar < len) {
          workLines[displayRevealLine][displayRevealChar] = finalLines[displayRevealLine][displayRevealChar];
          for (int j = displayRevealChar + 1; j < len; j++) {
            if (finalLines[displayRevealLine][j] != ' ')
              workLines[displayRevealLine][j] = SCRAMBLE_CHARS[random(0, NUM_SCRAMBLE_CHARS)];
            else
              workLines[displayRevealLine][j] = ' ';
          }

          for (int i = displayRevealLine + 1; i < g_lineCount; i++) {
            int llen = strlen(finalLines[i]);
            for (int j = 0; j < llen; j++) {
              if (finalLines[i][j] != ' ')
                workLines[i][j] = SCRAMBLE_CHARS[random(0, NUM_SCRAMBLE_CHARS)];
              else
                workLines[i][j] = ' ';
            }
          }
          drawOledBuffer();
          displayRevealChar++;
        } else {
          displayRevealLine++;
          displayRevealChar = 0;
        }
      } else {
        displayScrambling = false;
        displayFinishedTime = ms;
        drawOledBuffer();
      }
    }
  }
}

void displayIdle() {
  if (!displayScrambling && timerFinalStr[0] == '\0' && g_lineCount == 0) return;
  memset(timerFinalStr, 0, sizeof(timerFinalStr));
  memset(timerWorkStr, 0, sizeof(timerWorkStr));
  g_lineCount = 0;
  displayClear();
}

#endif
