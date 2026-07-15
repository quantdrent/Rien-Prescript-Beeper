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

#define TFT_WIDTH 284
#define TFT_HEIGHT 76

#define TFT_MOSI  0
#define TFT_SCLK  1
#define TFT_CS    2
#define TFT_DC    3
#define TFT_RST   4

#define BUTTON_PASS_PIN  5
#define BUTTON_FAIL_PIN  6
#define BUTTON_POWER_PIN 7

#define DEBOUNCE_DELAY  50

#define BLE_CHUNK_SIZE      20
#define BLE_CHUNK_DELAY_MS  10

#define JSON_BUF_SIZE   2048
#define CUSTOMS_FILE    "/customs.txt"
#define STATS_FILE      "/stats.txt"
#define SETTINGS_FILE   "/settings.txt"

#define DEFAULT_TEXT_SCALE          1
#define DEFAULT_SCRAMBLE_FRAMES     20
#define DEFAULT_SCRAMBLE_DELAY      20
#define DEFAULT_REVEAL_DELAY        20

#define COLOR_BG    0x0000
#define DEFAULT_TEXT_COLOR 0xBF5F
extern uint16_t textColor;

#endif
