/*****************************************************
 * ST7789 Driver for ER-TFTM2.25-1 (76x284 TFT)
 * Ported from EastRising ESP32 example to nRF52840
 * 
 * Adapted to inherit from Adafruit_GFX for robust
 * font and graphics support.
 *****************************************************/

#ifndef _ST7789_NRF52840_H_
#define _ST7789_NRF52840_H_

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <SPI.h>

// Display dimensions for ER-TFTM2.25-1
#define ST7789_TFTWIDTH  76
#define ST7789_TFTHEIGHT 284

// Memory offset for this specific panel inside the ST7789 controller RAM
// The 76x284 window sits at offset (0x52, 0x12) inside the 240x320 framebuffer
#define ST7789_XOFFSET   0x52  // 82 decimal
#define ST7789_YOFFSET   0x12  // 18 decimal

// ST7789 Commands
#define ST7789_CASET   0x2A
#define ST7789_PASET   0x2B
#define ST7789_RAMWR   0x2C

// Color definitions (RGB565)
#define ST7789_BLACK   0x0000
#define ST7789_BLUE    0x001F
#define ST7789_RED     0xF800
#define ST7789_GREEN   0x07E0
#define ST7789_CYAN    0x07FF
#define ST7789_MAGENTA 0xF81F
#define ST7789_YELLOW  0xFFE0
#define ST7789_WHITE   0xFFFF
#define ST7789_ORANGE  0xFD20

class ST7789_nRF : public Adafruit_GFX {

 public:
  // Hardware SPI constructor
  ST7789_nRF(int8_t cs, int8_t dc, int8_t rst = -1);

  void     begin(void),
           setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1),
           pushColor(uint16_t color),
           fillScreen(uint16_t color),
           drawBuffer(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *buffer),
           drawPixel(int16_t x, int16_t y, uint16_t color) override,
           drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) override,
           drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) override,
           fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) override;

  uint16_t color565(uint8_t r, uint8_t g, uint8_t b);
  void setRotation(uint8_t r) override;

 private:
  void     spiwrite(uint8_t c),
           writecommand(uint8_t c),
           writedata(uint8_t d);

  int8_t   _cs, _dc, _rst;
  uint8_t  _xstart = ST7789_XOFFSET;
  uint8_t  _ystart = ST7789_YOFFSET;
};

#endif
