#include "ST7789_nRF52840.h"
#include <SPI.h>

ST7789_nRF::ST7789_nRF(int8_t cs, int8_t dc, int8_t mosi,
                       int8_t sclk, int8_t rst)
  : ERGFX(ST7789_TFTWIDTH, ST7789_TFTHEIGHT) {
  _cs   = cs;
  _dc   = dc;
  _mosi = mosi;
  _sclk = sclk;
  _rst  = rst;
  _hwSPI = false;
}

ST7789_nRF::ST7789_nRF(int8_t cs, int8_t dc, int8_t rst)
  : ERGFX(ST7789_TFTWIDTH, ST7789_TFTHEIGHT) {
  _cs   = cs;
  _dc   = dc;
  _rst  = rst;
  _hwSPI = true;
  _mosi = _sclk = 0;
}

void ST7789_nRF::spiwrite(uint8_t c) {
  if (_hwSPI) {
    SPI.transfer(c);
  } else {
    for (uint8_t bit = 0x80; bit; bit >>= 1) {
      digitalWrite(_mosi, (c & bit) ? HIGH : LOW);
      digitalWrite(_sclk, LOW);
      digitalWrite(_sclk, HIGH);
    }
  }
}

void ST7789_nRF::writecommand(uint8_t c) {
  digitalWrite(_dc, LOW);
  digitalWrite(_cs, LOW);
  spiwrite(c);
  digitalWrite(_cs, HIGH);
}

void ST7789_nRF::writedata(uint8_t d) {
  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  spiwrite(d);
  digitalWrite(_cs, HIGH);
}

void ST7789_nRF::begin(void) {
  pinMode(_dc, OUTPUT);
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);

  if (_rst > 0) {
    pinMode(_rst, OUTPUT);
    digitalWrite(_rst, HIGH);
    delay(5);
    digitalWrite(_rst, LOW);
    delay(20);
    digitalWrite(_rst, HIGH);
    delay(150);
  }

  if (_hwSPI) {
    SPI.begin();
    SPI.beginTransaction(SPISettings(16000000, MSBFIRST, SPI_MODE0));
  } else {
    pinMode(_sclk, OUTPUT);
    pinMode(_mosi, OUTPUT);
    digitalWrite(_sclk, LOW);
    digitalWrite(_mosi, LOW);
  }


  writecommand(0xB2);
  writedata(0x0C); writedata(0x0C); writedata(0x00);
  writedata(0x33); writedata(0x33);

  writecommand(0xB0);
  writedata(0x00); writedata(0xE0);


  writecommand(0x36);
  writedata(0x60);

  writecommand(0x3A);
  writedata(0x05);

  writecommand(0xB7);
  writedata(0x45);

  writecommand(0xBB);
  writedata(0x1D);

  writecommand(0xC0);
  writedata(0x2C);

  writecommand(0xC2);
  writedata(0x01);

  writecommand(0xC3);
  writedata(0x19);

  writecommand(0xC4);
  writedata(0x20);

  writecommand(0xC6);
  writedata(0x0F);

  writecommand(0xD0);
  writedata(0xA4); writedata(0xA1);

  writecommand(0xD6);
  writedata(0xA1);

  writecommand(0xE0);
  writedata(0xD0); writedata(0x10); writedata(0x21); writedata(0x14);
  writedata(0x15); writedata(0x2D); writedata(0x41); writedata(0x44);
  writedata(0x4F); writedata(0x28); writedata(0x0E); writedata(0x0C);
  writedata(0x1D); writedata(0x1F);

  writecommand(0xE1);
  writedata(0xD0); writedata(0x0F); writedata(0x1B); writedata(0x0D);
  writedata(0x0D); writedata(0x26); writedata(0x42); writedata(0x54);
  writedata(0x50); writedata(0x3E); writedata(0x1A); writedata(0x18);
  writedata(0x22); writedata(0x25);

  writecommand(0x11);
  delay(120);

  writecommand(0x29);
  delay(50);
}

void ST7789_nRF::setAddrWindow(uint16_t x0, uint16_t y0,
                                uint16_t x1, uint16_t y1) {
  x0 += ST7789_XOFFSET;
  x1 += ST7789_XOFFSET;
  y0 += ST7789_YOFFSET;
  y1 += ST7789_YOFFSET;

  writecommand(ST7789_CASET);
  writedata(x0 >> 8); writedata(x0 & 0xFF);
  writedata(x1 >> 8); writedata(x1 & 0xFF);

  writecommand(ST7789_PASET);
  writedata(y0 >> 8); writedata(y0 & 0xFF);
  writedata(y1 >> 8); writedata(y1 & 0xFF);

  writecommand(ST7789_RAMWR);
}

void ST7789_nRF::pushColor(uint16_t color) {
  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  spiwrite(color >> 8);
  spiwrite(color);
  digitalWrite(_cs, HIGH);
}

void ST7789_nRF::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;
  setAddrWindow(x, y, x + 1, y + 1);
  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  spiwrite(color >> 8);
  spiwrite(color);
  digitalWrite(_cs, HIGH);
}

void ST7789_nRF::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  if ((x >= _width) || (y >= _height)) return;
  if ((y + h - 1) >= _height) h = _height - y;
  setAddrWindow(x, y, x, y + h - 1);
  uint8_t hi = color >> 8, lo = color;
  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  while (h--) { spiwrite(hi); spiwrite(lo); }
  digitalWrite(_cs, HIGH);
}

void ST7789_nRF::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  if ((x >= _width) || (y >= _height)) return;
  if ((x + w - 1) >= _width) w = _width - x;
  setAddrWindow(x, y, x + w - 1, y);
  uint8_t hi = color >> 8, lo = color;
  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  while (w--) { spiwrite(hi); spiwrite(lo); }
  digitalWrite(_cs, HIGH);
}

void ST7789_nRF::fillScreen(uint16_t color) {
  fillRect(0, 0, _width, _height, color);
}

void ST7789_nRF::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if ((x >= _width) || (y >= _height)) return;
  if ((x + w - 1) >= _width)  w = _width  - x;
  if ((y + h - 1) >= _height) h = _height - y;
  setAddrWindow(x, y, x + w - 1, y + h - 1);
  uint8_t hi = color >> 8, lo = color;
  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  for (int16_t row = h; row > 0; row--)
    for (int16_t col = w; col > 0; col--) { spiwrite(hi); spiwrite(lo); }
  digitalWrite(_cs, HIGH);
}

uint16_t ST7789_nRF::color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
