

#ifndef _ST7789_NRF52840_H_
#define _ST7789_NRF52840_H_

#include "Arduino.h"
#include "Print.h"
#include "ERGFX.h"
#include <SPI.h>


#define ST7789_TFTWIDTH  76
#define ST7789_TFTHEIGHT 284



#define ST7789_XOFFSET   0x52  
#define ST7789_YOFFSET   0x12  


#define ST7789_CASET   0x2A
#define ST7789_PASET   0x2B
#define ST7789_RAMWR   0x2C


#define ST7789_BLACK   0x0000
#define ST7789_BLUE    0x001F
#define ST7789_RED     0xF800
#define ST7789_GREEN   0x07E0
#define ST7789_CYAN    0x07FF
#define ST7789_MAGENTA 0xF81F
#define ST7789_YELLOW  0xFFE0
#define ST7789_WHITE   0xFFFF
#define ST7789_ORANGE  0xFD20

class ST7789_nRF : public ERGFX {

 public:

  ST7789_nRF(int8_t cs, int8_t dc, int8_t mosi, int8_t sclk, int8_t rst);


  ST7789_nRF(int8_t cs, int8_t dc, int8_t rst = -1);

  void     begin(void),
           setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1),
           pushColor(uint16_t color),
           fillScreen(uint16_t color),
           drawPixel(int16_t x, int16_t y, uint16_t color),
           drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color),
           drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color),
           fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

  uint16_t color565(uint8_t r, uint8_t g, uint8_t b);

 private:
  void     spiwrite(uint8_t c),
           writecommand(uint8_t c),
           writedata(uint8_t d);

  int8_t   _cs, _dc, _rst, _mosi, _sclk;
  boolean  _hwSPI;
};

#endif
