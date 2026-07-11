#ifndef CONFIG_H
#define CONFIG_H

#define TFT_CS    D5
#define TFT_DC    D6
#define TFT_RST   D7
#define TFT_SCLK  D8
#define TFT_MOSI  D9
#define TFT_WIDTH  284
#define TFT_HEIGHT 76

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

// default setttings
#define DEFAULT_TEXT_SCALE          2
#define DEFAULT_SCRAMBLE_FRAMES     30
#define DEFAULT_SCRAMBLE_DELAY      5
#define DEFAULT_REVEAL_DELAY        15

#define COLOR_BG    0x0000
#define COLOR_TEXT  0xBF5F

#endif
