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

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define I2C_SDA_PIN 3
#define I2C_SCL_PIN 4

#define BUTTON_PASS_PIN  6
#define BUTTON_FAIL_PIN  10

#define DEBOUNCE_DELAY  50

#define BLE_CHUNK_SIZE      20
#define BLE_CHUNK_DELAY_MS  10

#define CUSTOMS_FILE    "/customs.txt"
#define STATS_FILE      "/stats.txt"
#define SETTINGS_FILE   "/settings.txt"

#define DEFAULT_TEXT_SCALE          1
#define DEFAULT_SCRAMBLE_FRAMES     20
#define DEFAULT_SCRAMBLE_DELAY      20
#define DEFAULT_REVEAL_DELAY        20

#endif
