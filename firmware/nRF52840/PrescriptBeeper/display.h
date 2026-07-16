#ifndef DISPLAY_H
#define DISPLAY_H

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
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
extern unsigned long displayStartTime;
extern bool useProportionalFont;
extern bool timerFormatLong;

class Custom_ST7789 : public Adafruit_ST7789 {
public:
  Custom_ST7789(int8_t cs, int8_t dc, int8_t rst) : Adafruit_ST7789(cs, dc, rst) {}
  void setCustomOffsets(int8_t col, int8_t row) {
    setColRowStart(col, row);
  }
};

Custom_ST7789 tft = Custom_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas16 spr(TFT_WIDTH, TFT_HEIGHT);

extern int scrambleDurationFrames;
extern int scrambleDelayMs;
extern int revealDelayMs;

static const char SCRAMBLE_CHARS[] = "ABCDEF@HIJ_LM%OPQR^WX#YZa#b+cdefgh*iqrxyz0123456789";
static const int NUM_SCRAMBLE_CHARS = sizeof(SCRAMBLE_CHARS) - 1;

void displayInit() {
  SPI.setPins(-1, TFT_SCLK, TFT_MOSI);
  tft.init(76, 284);
  tft.setRotation(3);
  tft.setCustomOffsets(18, 82);
  tft.invertDisplay(false);
  tft.fillScreen(COLOR_BG);
}

void displayClear() {
  tft.fillScreen(COLOR_BG);
}

static char finalLines[6][128];
static char workLines[6][128];
static int g_lineCount, g_startY, g_charH, g_startX;
static int g_textSz = 1;

static char timerFinalStr[32];
static char timerWorkStr[32];
static int g_timerSz = 2;
static int g_timerY = 0;

#define IS_TIMER_TAG(s, i) (s[i] == '{' && (s[i+1] == 'T' || s[i+1] == 't') && (s[i+2] == 'I' || s[i+2] == 'i') && (s[i+3] == 'M' || s[i+3] == 'm') && (s[i+4] == 'E' || s[i+4] == 'e') && (s[i+5] == 'R' || s[i+5] == 'r') && s[i+6] == '}')

static bool displayScrambling = false;
static unsigned long displayScrambleStartTime = 0;
static unsigned long displayLastUpdate = 0;
static int displayScramblePhase = 0;
static int displayRevealLine = 0;
static int displayRevealChar = 0;
static int displayScrambleFrames = 0;
unsigned long displayFinishedTime = 0;

void setupFont(int sz) {
  if (!useProportionalFont) {
    spr.setFont(NULL);
    int realSz = sz + 1;
    spr.setTextSize(realSz > 0 ? realSz : 1);
    return;
  }

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

void stripTags(const char* src, char* dst) {
    int i = 0, j = 0;
    bool inTag = false;
    while(src[i]) {
        if(src[i] == '{' && src[i+1] == '#') {
            inTag = true;
            i++;
            continue;
        } else if(src[i] == '{' && src[i+1] == '}') {
            i += 2;
            continue;
        } else if(inTag && src[i] == '}') {
            inTag = false;
            i++;
            continue;
        } else if(!inTag && IS_TIMER_TAG(src, i)) {
            int tLen = strlen(timerWorkStr);
            for(int t=0; t<tLen; t++) {
                dst[j++] = timerWorkStr[t];
            }
            i += 7;
            continue;
        }
        if(!inTag) {
            dst[j++] = src[i];
        }
        i++;
    }
    dst[j] = '\0';
}

int getTextWidth(const char* text) {
  char visible[256];
  stripTags(text, visible);
  int16_t x1, y1;
  uint16_t w, h;
  spr.getTextBounds(visible, 0, 0, &x1, &y1, &w, &h);
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

void formatTimeString(char* buf, size_t bufSize, unsigned long t) {
  if (!timerFormatLong) {
    if (t < 60) snprintf(buf, bufSize, "%lus", t);
    else {
      unsigned long h = t / 3600;
      unsigned long m = (t % 3600) / 60;
      unsigned long s = t % 60;
      if (h > 0) {
        if (s > 0) snprintf(buf, bufSize, "%luh %lum %lus", h, m, s);
        else if (m > 0) snprintf(buf, bufSize, "%luh %lum", h, m);
        else snprintf(buf, bufSize, "%luh", h);
      } else {
        if (s > 0) snprintf(buf, bufSize, "%lum %lus", m, s);
        else snprintf(buf, bufSize, "%lum", m);
      }
    }
  } else {
    if (t < 60) snprintf(buf, bufSize, "%lu second%s", t, t != 1 ? "s" : "");
    else {
      unsigned long h = t / 3600;
      unsigned long m = (t % 3600) / 60;
      unsigned long s = t % 60;
      char hStr[32] = ""; char mStr[32] = ""; char sStr[32] = "";
      if (h > 0) snprintf(hStr, sizeof(hStr), "%lu hour%s", h, h != 1 ? "s" : "");
      if (m > 0 || (h > 0 && s > 0)) snprintf(mStr, sizeof(mStr), "%lu minute%s", m, m != 1 ? "s" : "");
      if (s > 0) snprintf(sStr, sizeof(sStr), "%lu second%s", s, s != 1 ? "s" : "");
      
      if (h > 0 && s > 0) snprintf(buf, bufSize, "%s %s %s", hStr, mStr, sStr);
      else if (h > 0 && m > 0) snprintf(buf, bufSize, "%s %s", hStr, mStr);
      else if (h > 0) snprintf(buf, bufSize, "%s", hStr);
      else if (m > 0 && s > 0) snprintf(buf, bufSize, "%s %s", mStr, sStr);
      else if (m > 0) snprintf(buf, bufSize, "%s", mStr);
    }
  }
}

bool checkHasInlineTimer(const char* txt) {
  for (int i = 0; txt[i] != '\0'; i++) {
    if (IS_TIMER_TAG(txt, i)) return true;
  }
  return false;
}

void setupTimerDisplay(const char* targetText, unsigned long durationMs) {
  bool hasInlineTimer = checkHasInlineTimer(targetText);

  if (!hasInlineTimer && timerPosition == 0) {
    timerFinalStr[0] = '\0';
    timerWorkStr[0] = '\0';
    return;
  }
  
  if (strcmp(targetText, "CLEAR.") == 0 || strcmp(targetText, "FAILED.") == 0 || strcmp(targetText, "Connected.") == 0 || strcmp(targetText, "Disconnected.") == 0 || strcmp(targetText, "NO DATA") == 0) {
    timerFinalStr[0] = '\0';
    timerWorkStr[0] = '\0';
    return;
  }

  if (isInfinite || durationMs == 0) {
    strcpy(timerFinalStr, "nil");
  } else {
    unsigned long t = durationMs / 1000;
    formatTimeString(timerFinalStr, sizeof(timerFinalStr), t);
  }
  strcpy(timerWorkStr, timerFinalStr);

  if (hasInlineTimer && timerPosition == 0) return;

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
  if (timerFinalStr[0] == '\0') return;

  bool hasInlineTimer = false;
  for(int i=0; i<g_lineCount; i++) {
    if(checkHasInlineTimer(workLines[i])) { hasInlineTimer = true; break; }
  }

  if (timerPosition == 0 && !hasInlineTimer) return;

  char newTimerStr[128]; // Increased size for long format
  unsigned long t = (remainingMs + 999) / 1000;
  formatTimeString(newTimerStr, sizeof(newTimerStr), t);

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

  setupTimerDisplay(targetText, displayDurationMs);

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

  // setupTimerDisplay was moved above

  displayScrambling = true;
  displayScrambleStartTime = millis();
  displayLastUpdate = 0;
  displayScramblePhase = 0;
  displayRevealLine = 0;
  displayRevealChar = 0;
  displayScrambleFrames = 0;
}

void drawSprite() {
  spr.fillScreen(COLOR_BG);
  uint16_t currentColor = textColor;

  setupFont(g_textSz);
  for (int i = 0; i < g_lineCount; i++) {
    char vis[128];
    stripTags(workLines[i], vis);
    int16_t x1, y1; uint16_t w, h;
    spr.getTextBounds(vis, 0, 0, &x1, &y1, &w, &h);

    int currentX = (TFT_WIDTH - w) / 2 - x1;
    if (currentX < 0) currentX = 0;

    int yPos = g_startY + (i * g_charH);
    spr.setCursor(currentX, yPos);
    
    spr.setTextColor(currentColor);
    
    for (int j = 0; workLines[i][j] != '\0'; j++) {
      char c = workLines[i][j];
      if (c == '{' && workLines[i][j+1] == '#') {
        j += 2;
        char hexBuf[7] = {0};
        int hl = 0;
        while(workLines[i][j] && workLines[i][j] != '}' && hl < 6) {
          hexBuf[hl++] = workLines[i][j++];
        }
        long hexVal = strtol(hexBuf, NULL, 16);
        uint8_t r = (hexVal >> 16) & 0xFF;
        uint8_t g = (hexVal >> 8) & 0xFF;
        uint8_t b = hexVal & 0xFF;
        currentColor = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        spr.setTextColor(currentColor);
        continue;
      } else if (c == '{' && workLines[i][j+1] == '}') {
        j++;
        currentColor = textColor;
        spr.setTextColor(currentColor);
        continue;
      } else if (IS_TIMER_TAG(workLines[i], j)) {
        j += 6;
        spr.print(timerWorkStr);
        continue;
      }
      spr.print(c);
    }
  }

  if (timerPosition != 0 && timerFinalStr[0] != '\0' && strcmp(timerFinalStr, "nil") != 0) {
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

  tft.drawRGBBitmap(0, 0, spr.getBuffer(), spr.width(), spr.height());
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
        bool inTag = false;
        for (int j = 0; j < llen; j++) {
          char c = finalLines[i][j];
          if (c == '{' && finalLines[i][j+1] == '#') {
              inTag = true;
              workLines[i][j] = c;
          } else if (IS_TIMER_TAG(finalLines[i], j)) {
              inTag = true;
              workLines[i][j] = c;
          } else if (c == '{' && finalLines[i][j+1] == '}') {
              workLines[i][j] = '{';
              workLines[i][j+1] = '}';
              j++;
          } else if (inTag && c == '}') {
              inTag = false;
              workLines[i][j] = c;
          } else if (inTag) {
              workLines[i][j] = c;
          } else {
              if (c != ' ' && c != '{' && c != '}')
                workLines[i][j] = SCRAMBLE_CHARS[random(0, NUM_SCRAMBLE_CHARS)];
              else
                workLines[i][j] = c;
          }
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
          if (finalLines[displayRevealLine][displayRevealChar] == '{') {
              if (finalLines[displayRevealLine][displayRevealChar+1] == '#') {
                  while(displayRevealChar < len && finalLines[displayRevealLine][displayRevealChar] != '}') {
                      workLines[displayRevealLine][displayRevealChar] = finalLines[displayRevealLine][displayRevealChar];
                      displayRevealChar++;
                  }
                  if (displayRevealChar < len) {
                      workLines[displayRevealLine][displayRevealChar] = finalLines[displayRevealLine][displayRevealChar];
                  }
              } else if (finalLines[displayRevealLine][displayRevealChar+1] == '}') {
                  workLines[displayRevealLine][displayRevealChar] = '{';
                  displayRevealChar++;
                  workLines[displayRevealLine][displayRevealChar] = '}';
              }
          }
        }

        if (displayRevealChar < len) {
          workLines[displayRevealLine][displayRevealChar] = finalLines[displayRevealLine][displayRevealChar];
          bool inTagReveal = false;
          for (int j = 0; j < len; j++) {
            char c = finalLines[displayRevealLine][j];
            if (c == '{' && finalLines[displayRevealLine][j+1] == '#') {
                inTagReveal = true;
                if (j > displayRevealChar) workLines[displayRevealLine][j] = c;
            } else if (IS_TIMER_TAG(finalLines[displayRevealLine], j)) {
                inTagReveal = true;
                if (j > displayRevealChar) workLines[displayRevealLine][j] = c;
            } else if (c == '{' && finalLines[displayRevealLine][j+1] == '}') {
                if (j > displayRevealChar) {
                    workLines[displayRevealLine][j] = '{';
                    workLines[displayRevealLine][j+1] = '}';
                }
                j++;
            } else if (inTagReveal && c == '}') {
                inTagReveal = false;
                if (j > displayRevealChar) workLines[displayRevealLine][j] = c;
            } else if (inTagReveal) {
                if (j > displayRevealChar) workLines[displayRevealLine][j] = c;
            } else {
                if (j > displayRevealChar) {
                    if (c != ' ' && c != '{' && c != '}')
                      workLines[displayRevealLine][j] = SCRAMBLE_CHARS[random(0, NUM_SCRAMBLE_CHARS)];
                    else
                      workLines[displayRevealLine][j] = c;
                }
            }
          }

          for (int i = displayRevealLine + 1; i < g_lineCount; i++) {
            int llen = strlen(finalLines[i]);
            bool inTag = false;
            for (int j = 0; j < llen; j++) {
              char c = finalLines[i][j];
              if (c == '{' && finalLines[i][j+1] == '#') {
                  inTag = true;
                  workLines[i][j] = c;
              } else if (IS_TIMER_TAG(finalLines[i], j)) {
                  inTag = true;
                  workLines[i][j] = c;
              } else if (c == '{' && finalLines[i][j+1] == '}') {
                  workLines[i][j] = '{';
                  workLines[i][j+1] = '}';
                  j++;
              } else if (inTag && c == '}') {
                  inTag = false;
                  workLines[i][j] = c;
              } else if (inTag) {
                  workLines[i][j] = c;
              } else {
                  if (c != ' ' && c != '{' && c != '}')
                    workLines[i][j] = SCRAMBLE_CHARS[random(0, NUM_SCRAMBLE_CHARS)];
                  else
                    workLines[i][j] = c;
              }
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
        displayStartTime = ms;
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
  spr.fillScreen(COLOR_BG);
  tft.drawRGBBitmap(0, 0, spr.getBuffer(), spr.width(), spr.height());
}

#endif
