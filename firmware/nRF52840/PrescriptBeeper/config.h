#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

extern int textScale;
extern int scrambleDurationFrames;
extern int scrambleDelayMs;
extern int revealDelayMs;
extern int timerPosition;
extern float timerScale;
extern bool bleRequirePin;
extern bool useProportionalFont;

#define TFT_WIDTH   284
#define TFT_HEIGHT  76
#define TFT_CS      D5
#define TFT_DC      D6
#define TFT_RST     D7
#define TFT_MOSI    D9
#define TFT_SCLK    D8

#define BUTTON_PASS_PIN  D2
#define BUTTON_FAIL_PIN  D3
#define BUTTON_POWER_PIN D4

#define DEBOUNCE_DELAY  50

#define BLE_CHUNK_SIZE      20
#define BLE_CHUNK_DELAY_MS  10

#define JSON_BUF_SIZE   2048
#define CUSTOMS_FILE    "customs.txt"
#define STATS_FILE      "stats.txt"
#define SETTINGS_FILE   "settings.txt"

#define DEFAULT_TEXT_SCALE          1
#define DEFAULT_SCRAMBLE_FRAMES     20
#define DEFAULT_SCRAMBLE_DELAY      20
#define DEFAULT_REVEAL_DELAY        20

#define COLOR_BG    0x0000
#define DEFAULT_TEXT_COLOR 0xBF5F
extern uint16_t textColor;

#endif
