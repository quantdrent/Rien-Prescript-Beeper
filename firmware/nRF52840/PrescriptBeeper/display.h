#ifndef DISPLAY_H
#define DISPLAY_H

extern int timerPosition;
extern float timerScale;
extern int textScale;
extern bool isInfinite;
extern unsigned long displayDurationMs;

#include "ST7789_nRF52840.h"
#include "config.h"

#include <SPI.h>

ST7789_nRF tft = ST7789_nRF(TFT_CS, TFT_DC, TFT_RST);

extern int textScale;
extern int scrambleDurationFrames;
extern int scrambleDelayMs;
extern int revealDelayMs;

static const char SCRAMBLE_CHARS[] = "ABCDEF@HIJ_LM%OPQR^WX#YZa#b+cdefgh*iqrxyz0123456789";
static const int NUM_SCRAMBLE_CHARS = sizeof(SCRAMBLE_CHARS) - 1;

void displayInit() {
  SPI.setPins(-1, TFT_SCLK, TFT_MOSI);
  tft.begin();
  tft.fillScreen(COLOR_BG);
  tft.setTextWrap(false);
}

void displayClear() {
  tft.fillScreen(COLOR_BG);
}

int buildLayout(const char* text, char linesBuf[][128], int* lineCount, int* outStartY, int* outCharW, int* outCharH) {
  int sz = textScale;
  while (sz >= 1) {
    int charW = sz * 6;
    int charH = sz * 8;
    int cpl = TFT_WIDTH / charW;
    if (cpl < 1) cpl = 1;
    int lines = 1, col = 0;
    const char* p = text;
    while (*p) {
      if (*p == ' ') {
        const char* q = p + 1;
        int wl = 0;
        while (*q && *q != ' ') { wl++; q++; }
        if (col + 1 + wl > cpl && col > 0) { lines++; col = 0; p++; continue; }
      }
      col++;
      if (col > cpl) { lines++; col = 1; }
      p++;
    }
    if (lines * charH <= TFT_HEIGHT) break;
    sz--;
  }
  if (sz < 1) sz = 1;

  int charW = sz * 6;
  int charH = sz * 8;
  int cpl = TFT_WIDTH / charW;
  if (cpl < 1) cpl = 1;

  for (int i = 0; i < 10; i++) memset(linesBuf[i], 0, 128);

  int col = 0, li = 0;
  const char* p = text;
  while (*p && li < 10) {
    if (*p == ' ') {
      const char* q = p + 1;
      int wl = 0;
      while (*q && *q != ' ') { wl++; q++; }
      if (col + 1 + wl > cpl && col > 0) { li++; col = 0; p++; continue; }
    }
    if (col >= cpl) { li++; col = 0; }
    if (li < 10) { linesBuf[li][col] = *p; col++; }
    p++;
  }
  *lineCount = li + 1;
  *outStartY = (TFT_HEIGHT - (*lineCount) * charH) / 2;
  if (*outStartY < 0) *outStartY = 0;
  *outCharW = charW;
  *outCharH = charH;
  return sz;
}

static char finalLines[6][32];
static char screenLines[6][32];
static char workLines[6][32];
static int g_lineX[6];
static int g_lineCount, g_startY, g_charW, g_charH, g_sz;

static char timerFinalStr[32];
static char timerWorkStr[32];
static char timerScreenStr[32];
static int g_timerX = 0, g_timerY = 0;
static int g_timerSz = 1;
static int g_timerCharW = 0, g_timerCharH = 0;

static bool displayScrambling = false;
static unsigned long displayScrambleStartTime = 0;
static unsigned long displayLastUpdate = 0;
static int displayScramblePhase = 0;
static int displayRevealLine = 0;
static int displayRevealChar = 0;
static int displayScrambleFrames = 0;

void drawLinesDiff() {
  for (int i = 0; i < g_lineCount; i++) {
    int len = strlen(workLines[i]);
    for (int j = 0; j < len; j++) {
      if (screenLines[i][j] != workLines[i][j]) {
        tft.drawChar(g_lineX[i] + j * g_charW, g_startY + i * g_charH, workLines[i][j], COLOR_TEXT, COLOR_BG, g_sz);
        screenLines[i][j] = workLines[i][j];
      }
    }
  }
}

void drawTimerDiff() {
  if (timerFinalStr[0] == '\0') return;
  
  int len = strlen(timerWorkStr);
  int screenLen = strlen(timerScreenStr);
  
  for (int j = len; j < screenLen; j++) {
    tft.drawChar(g_timerX + j * g_timerCharW, g_timerY, ' ', COLOR_TEXT, COLOR_BG, g_timerSz);
    timerScreenStr[j] = '\0';
  }
  
  for (int j = 0; j < len; j++) {
    if (timerScreenStr[j] != timerWorkStr[j]) {
      tft.drawChar(g_timerX + j * g_timerCharW, g_timerY, timerWorkStr[j], COLOR_TEXT, COLOR_BG, g_timerSz);
      timerScreenStr[j] = timerWorkStr[j];
    }
  }
}

void updateTimerDisplay(unsigned long remainingMs) {
  if (timerPosition == 0 || isInfinite || timerFinalStr[0] == '\0') return;
  
  char newTimerStr[32];
  unsigned long t = (remainingMs + 999) / 1000;
  if (t < 60) {
    snprintf(newTimerStr, sizeof(newTimerStr), "%lus", t);
  } else {
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
      strcpy(timerWorkStr, timerFinalStr);
      drawTimerDiff();
    }
  }
}

void setupTimerDisplay(const char* targetText, unsigned long durationMs) {
  if (timerPosition == 0 || durationMs == 0 || isInfinite) {
    timerFinalStr[0] = '\0';
    return;
  }
  
  if (strcmp(targetText, "CLEAR.") == 0 || strcmp(targetText, "FAILED.") == 0 || strcmp(targetText, "Connected.") == 0 || strcmp(targetText, "NO DATA") == 0) {
    timerFinalStr[0] = '\0';
    return;
  }
  
  unsigned long t = durationMs / 1000;
  if (t < 60) {
    snprintf(timerFinalStr, sizeof(timerFinalStr), "%lus", t);
  } else {
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
  
  g_timerSz = (int)(textScale * timerScale);
  if (g_timerSz < 1) g_timerSz = 1;
  g_timerCharW = g_timerSz * 6;
  g_timerCharH = g_timerSz * 8;
  
  int len = strlen(timerFinalStr);
  int w = len * g_timerCharW;
  
  if (timerPosition == 1 || timerPosition == 4) g_timerX = 5; 
  else if (timerPosition == 2 || timerPosition == 5) g_timerX = (TFT_WIDTH - w) / 2; 
  else if (timerPosition == 3 || timerPosition == 6) g_timerX = TFT_WIDTH - w - 5; 
  
  if (timerPosition >= 1 && timerPosition <= 3) g_timerY = 5; 
  else g_timerY = TFT_HEIGHT - g_timerCharH - 5; 
  
  memset(timerScreenStr, 0, sizeof(timerScreenStr));
  memset(timerWorkStr, 0, sizeof(timerWorkStr));
}

unsigned long displayFinishedTime = 0;

void beginScramble(const char* targetText) {
  char finalStr[128];
  snprintf(finalStr, sizeof(finalStr), "_%s_", targetText);

  char tempLines[10][128];
  int tempCount;
  g_sz = buildLayout(finalStr, tempLines, &tempCount, &g_startY, &g_charW, &g_charH);
  
  g_lineCount = (tempCount > 6) ? 6 : tempCount;
  
  for (int i = 0; i < 6; i++) {
    memset(finalLines[i], 0, 32);
    memset(screenLines[i], 0, 32);
    memset(workLines[i], 0, 32);
  }

  for (int i = 0; i < g_lineCount; i++) {
    strncpy(finalLines[i], tempLines[i], 31);
    int llen = strlen(finalLines[i]);
    g_lineX[i] = (TFT_WIDTH - llen * g_charW) / 2;
    if (g_lineX[i] < 0) g_lineX[i] = 0;
  }

  tft.fillScreen(COLOR_BG);

  setupTimerDisplay(targetText, displayDurationMs);

  displayScrambling = true;
  displayScrambleStartTime = millis();
  displayLastUpdate = 0;
  displayScramblePhase = 0;
  displayRevealLine = 0;
  displayRevealChar = 0;
  displayScrambleFrames = 0;
}

void updateScramble() {
  if (!displayScrambling) return;
  
  unsigned long now = millis();
  
  if (displayScramblePhase == 0) {
    if (now - displayLastUpdate < scrambleDelayMs) return;
    displayLastUpdate = now;
    
    if (displayScrambleFrames == 0) {
      for (int li = 0; li < g_lineCount; li++) {
        int llen = strlen(finalLines[li]);
        for (int ci = 0; ci < llen; ci++) {
          workLines[li][ci] = SCRAMBLE_CHARS[random(NUM_SCRAMBLE_CHARS)];
        }
        workLines[li][llen] = '\0';
      }
      if (timerFinalStr[0] != '\0') {
        int tlen = strlen(timerFinalStr);
        for (int ci = 0; ci < tlen; ci++) {
          timerWorkStr[ci] = SCRAMBLE_CHARS[random(NUM_SCRAMBLE_CHARS)];
        }
        timerWorkStr[tlen] = '\0';
      }
    } else {
      for (int c = 0; c < 8; c++) {
        int li = random(g_lineCount);
        int llen = strlen(finalLines[li]);
        if (llen > 0) {
          int ci = random(llen);
          workLines[li][ci] = SCRAMBLE_CHARS[random(NUM_SCRAMBLE_CHARS)];
        }
      }
      if (timerFinalStr[0] != '\0') {
        int tlen = strlen(timerFinalStr);
        if (tlen > 0) {
          int ci = random(tlen);
          timerWorkStr[ci] = SCRAMBLE_CHARS[random(NUM_SCRAMBLE_CHARS)];
        }
      }
    }
    drawLinesDiff();
    drawTimerDiff();
    
    displayScrambleFrames++;
    if (displayScrambleFrames >= scrambleDurationFrames) {
      displayScramblePhase = 1;
    }
  } 
  else if (displayScramblePhase == 1) {
    if (now - displayLastUpdate < revealDelayMs) return;
    displayLastUpdate = now;
    
    for (int li = 0; li < g_lineCount; li++) {
      int llen = strlen(finalLines[li]);
      for (int ci = 0; ci < llen; ci++) {
        if (li < displayRevealLine || (li == displayRevealLine && ci <= displayRevealChar)) {
          workLines[li][ci] = finalLines[li][ci];
        }
      }
      workLines[li][llen] = '\0';
    }
    
    if (timerFinalStr[0] != '\0') {
      int tlen = strlen(timerFinalStr);
      for (int ci = 0; ci < tlen; ci++) {
        if (displayRevealLine > 0 || (displayRevealLine == 0 && ci <= displayRevealChar)) {
          timerWorkStr[ci] = timerFinalStr[ci];
        } else {
          timerWorkStr[ci] = SCRAMBLE_CHARS[random(NUM_SCRAMBLE_CHARS)];
        }
      }
      timerWorkStr[tlen] = '\0';
    }
    
    for (int c = 0; c < 8; c++) {
      int li = random(g_lineCount);
      int llen = strlen(finalLines[li]);
      if (llen > 0) {
        int ci = random(llen);
        if (li > displayRevealLine || (li == displayRevealLine && ci > displayRevealChar)) {
          workLines[li][ci] = SCRAMBLE_CHARS[random(NUM_SCRAMBLE_CHARS)];
        }
      }
    }
    
    drawLinesDiff();
    drawTimerDiff();
    
    displayRevealChar++;
    if (displayRevealChar >= (int)strlen(finalLines[displayRevealLine])) {
      displayRevealChar = 0;
      displayRevealLine++;
      if (displayRevealLine >= g_lineCount) {
        displayScrambling = false;
        displayFinishedTime = millis();
        
        for (int li = 0; li < g_lineCount; li++) {
          strcpy(workLines[li], finalLines[li]);
        }
        drawLinesDiff();
        
        if (timerFinalStr[0] != '\0') {
          strcpy(timerWorkStr, timerFinalStr);
          drawTimerDiff();
        }
      }
    }
  }
}

void displayPrescript(const char* text) {
  beginScramble(text);
}

void displayStatus(const char* text) {
  displayClear();
  int sz = 2, charW = sz * 6, charH = sz * 8;
  int x = (TFT_WIDTH - (int)strlen(text) * charW) / 2;
  int y = (TFT_HEIGHT - charH) / 2;
  if (x < 0) x = 0; if (y < 0) y = 0;
  tft.setTextSize(sz);
  tft.setTextColor(COLOR_TEXT);
  tft.setCursor(x, y);
  tft.print(text);
}

void displayIdle() {
  displayClear();
}

#endif
