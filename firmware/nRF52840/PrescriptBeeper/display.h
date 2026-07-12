#ifndef DISPLAY_H
#define DISPLAY_H

#include <SPI.h>
#include <Adafruit_GFX.h>
#include "ST7789_nRF52840.h"
#include "config.h"

#include "PixelifySans_Regular9pt7b.h"
#include "PixelifySans_Regular12pt7b.h"
#include "PixelifySans_Regular20pt7b.h"

void drawSprite();

extern int timerPosition;
extern float timerScale;
extern int textScale;
extern bool isInfinite;
extern unsigned long displayDurationMs;
extern bool useProportionalFont;

ST7789_nRF tft(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 spr(TFT_WIDTH, TFT_HEIGHT);

extern int scrambleDurationFrames;
extern int scrambleDelayMs;
extern int revealDelayMs;

static const char SCRAMBLE_CHARS[] = "ABCDEF@HIJ_LM%OPQR^WX#YZa#b+cdefgh*iqrxyz0123456789";
static const int NUM_SCRAMBLE_CHARS = sizeof(SCRAMBLE_CHARS) - 1;

void displayInit() {
  SPI.setPins(-1, TFT_SCLK, TFT_MOSI);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ST7789_BLACK);
}

void displayClear() {
  tft.fillScreen(ST7789_BLACK);
}

static char finalLines[6][128];
static char workLines[6][128];
static int g_lineCount, g_startY, g_charH, g_startX;
static int g_textSz = 1;

static char timerFinalStr[32];
static char timerWorkStr[32];
static int g_timerY = 0;
static int g_timerSz = 1;

static bool displayScrambling = false;
static unsigned long displayScrambleStartTime = 0;
static unsigned long displayLastUpdate = 0;
static int displayScramblePhase = 0;
static int displayRevealLine = 0;
static int displayRevealChar = 0;
static int displayScrambleFrames = 0;
unsigned long displayFinishedTime = 0;

void setupFont(int sz) {
  if (sz <= 0) {
    spr.setFont(NULL);
    spr.setTextSize(1);
  } else if (sz == 1) {
    spr.setFont(&PixelifySans_Regular9pt7b);
    spr.setTextSize(1);
  } else if (sz <= 3) {
    spr.setFont(&PixelifySans_Regular12pt7b);
    spr.setTextSize(1);
  } else {
    spr.setFont(&PixelifySans_Regular20pt7b);
    spr.setTextSize(sz - 2);
  }
}

int getFontHeight() {
  int16_t x1, y1;
  uint16_t w, h;
  spr.getTextBounds("Ay|", 0, 0, &x1, &y1, &w, &h);
  return h;
}

int getTextWidth(const char* text) {
  int16_t x1, y1;
  uint16_t w, h;
  spr.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return w;
}

int buildLayout(const char* text, char linesBuf[][128], int* lineCount, int* outStartY, int* outCharH) {
  int sz = textScale;

  int maxW = TFT_WIDTH * 0.90;
  int maxH = TFT_HEIGHT * 0.90;

  while (sz >= 0) {
    setupFont(sz);
    int charH = getFontHeight();

    int lines = 1;
    char lineStr[128];
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

        char testBuf[128];
        if (lineLen == 0) snprintf(testBuf, sizeof(testBuf), "%s", wordBuf);
        else snprintf(testBuf, sizeof(testBuf), "%s %s", lineStr, wordBuf);

        if (getTextWidth(testBuf) > maxW && lineLen > 0) {
          lines++;
          memset(lineStr, 0, sizeof(lineStr));
          lineLen = 0;
          p++;
          continue;
        }
      }

      lineStr[lineLen] = *p;
      lineStr[lineLen + 1] = '\0';

      if (getTextWidth(lineStr) > maxW && lineLen > 0) {

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
  if (sz < 0) sz = 0;

  setupFont(sz);
  int charH = getFontHeight();

  for (int i = 0; i < 6; i++) memset(linesBuf[i], 0, 128);

  int li = 0;
  int charIdx = 0;
  const char* p = text;

  while (*p && li < 6) {
    if (*p == ' ') {
      const char* q = p + 1;
      int wl = 0;
      char wordBuf[32];
      memset(wordBuf, 0, sizeof(wordBuf));
      while (*q && *q != ' ' && wl < 31) { wordBuf[wl++] = *q++; }

      char testBuf[128];
      if (charIdx == 0) snprintf(testBuf, sizeof(testBuf), "%s", wordBuf);
      else snprintf(testBuf, sizeof(testBuf), "%s %s", linesBuf[li], wordBuf);

      if (getTextWidth(testBuf) > maxW && charIdx > 0) {
        li++;
        charIdx = 0;
        p++;
        if (li >= 6) break;
        continue;
      }
    }

    linesBuf[li][charIdx] = *p;
    linesBuf[li][charIdx + 1] = '\0';

    if (getTextWidth(linesBuf[li]) > maxW && charIdx > 0) {
      linesBuf[li][charIdx] = '\0';
      li++;
      charIdx = 0;
      if (li >= 6) break;
      linesBuf[li][charIdx++] = *p;
    } else {
      charIdx++;
    }
    p++;
  }

  *lineCount = li + 1;
  *outCharH = charH;
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

  g_timerSz = (int)(textScale * timerScale);
  if (g_timerSz < 1) g_timerSz = 1;

  setupFont(g_timerSz);
  int16_t x1, y1;
  uint16_t w, h;
  spr.getTextBounds(timerFinalStr, 0, 0, &x1, &y1, &w, &h);

  while (g_timerSz > 1 && w > (TFT_WIDTH - 10)) {
      g_timerSz--;
      setupFont(g_timerSz);
      spr.getTextBounds(timerFinalStr, 0, 0, &x1, &y1, &w, &h);
  }

  if (timerPosition >= 1 && timerPosition <= 3) g_timerY = 5 - y1;
  else g_timerY = TFT_HEIGHT - 5 - h - y1;
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
        drawSprite();
    }
  }
}

void beginScramble(const char* targetText) {
  char finalStr[128];
  snprintf(finalStr, sizeof(finalStr), "_%s_", targetText);

  char tempLines[6][128];
  int tempCount;
  g_textSz = buildLayout(finalStr, tempLines, &tempCount, &g_startY, &g_charH);

  g_lineCount = (tempCount > 6) ? 6 : tempCount;

  for (int i = 0; i < 6; i++) {
    memset(finalLines[i], 0, 128);
    memset(workLines[i], 0, 128);
  }

  for (int i = 0; i < g_lineCount; i++) {
    strncpy(finalLines[i], tempLines[i], 127);
  }

  setupFont(g_textSz);
  int16_t x1, y1; uint16_t w, h;
  spr.getTextBounds("Ay|", 0, 0, &x1, &y1, &w, &h);
  int blockH = g_lineCount * h;
  g_startY = (TFT_HEIGHT - blockH) / 2 - y1;

  // g_startX is no longer needed since each line calculates its own center

  setupTimerDisplay(targetText, displayDurationMs);

  displayScrambling = true;
  displayScrambleStartTime = millis();
  displayLastUpdate = 0;
  displayScramblePhase = 0;
  displayRevealLine = 0;
  displayRevealChar = 0;
  displayScrambleFrames = 0;
}

void drawSprite() {
  spr.fillScreen(ST7789_BLACK);
  spr.setTextColor(ST7789_WHITE);

  setupFont(g_textSz);
  for (int i = 0; i < g_lineCount; i++) {
    int16_t x1, y1; uint16_t w, h;
    spr.getTextBounds(workLines[i], 0, 0, &x1, &y1, &w, &h);

    int currentX = (TFT_WIDTH - w) / 2 - x1;
    if (currentX < 0) currentX = 0;

    int yPos = g_startY + (i * g_charH);
    spr.setCursor(currentX, yPos);
    spr.print(workLines[i]);
  }

  if (timerFinalStr[0] != '\0') {
    setupFont(g_timerSz);
    int16_t x1, y1; uint16_t w, h;
    spr.getTextBounds(timerWorkStr, 0, 0, &x1, &y1, &w, &h);

    int currentTimerX = 0;
    if (timerPosition == 1 || timerPosition == 4) currentTimerX = 5 - x1;
    else if (timerPosition == 2 || timerPosition == 5) currentTimerX = (TFT_WIDTH - w) / 2 - x1;
    else if (timerPosition == 3 || timerPosition == 6) currentTimerX = TFT_WIDTH - w - 5 - x1;
    if (currentTimerX < 0) currentTimerX = 0;

    spr.setCursor(currentTimerX, g_timerY);
    spr.print(timerWorkStr);
  }

  tft.drawBuffer(0, 0, spr.width(), spr.height(), spr.getBuffer());
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

      drawSprite();

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
          drawSprite();
          displayRevealChar++;
        } else {
          displayRevealLine++;
          displayRevealChar = 0;
        }
      } else {
        displayScrambling = false;
        displayFinishedTime = ms;
        drawSprite();
      }
    }
  }
}

void displayIdle() {
  if (!displayScrambling && timerFinalStr[0] == '\0' && g_lineCount == 0) return;
  memset(timerFinalStr, 0, sizeof(timerFinalStr));
  memset(timerWorkStr, 0, sizeof(timerWorkStr));
  g_lineCount = 0;
  spr.fillScreen(ST7789_BLACK);
  tft.drawBuffer(0, 0, spr.width(), spr.height(), spr.getBuffer());
}

#endif
