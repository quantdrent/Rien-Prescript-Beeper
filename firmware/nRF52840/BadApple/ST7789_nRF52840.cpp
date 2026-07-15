#include "ST7789_nRF52840.h"

// Hardware SPI constructor
ST7789_nRF::ST7789_nRF(int8_t cs, int8_t dc, int8_t rst)
  : Adafruit_GFX(ST7789_TFTWIDTH, ST7789_TFTHEIGHT) {
  _cs   = cs;
  _dc   = dc;
  _rst  = rst;
}

// ===== Low-level SPI =====

inline void ST7789_nRF::spiwrite(uint8_t c) {
  SPI.transfer(c);
}

void ST7789_nRF::writecommand(uint8_t c) {
  digitalWrite(_dc, LOW);   // Command mode
  digitalWrite(_cs, LOW);   // Select
  spiwrite(c);
  digitalWrite(_cs, HIGH);  // Deselect
}

void ST7789_nRF::writedata(uint8_t d) {
  digitalWrite(_dc, HIGH);  // Data mode
  digitalWrite(_cs, LOW);   // Select
  spiwrite(d);
  digitalWrite(_cs, HIGH);  // Deselect
}

// ===== Initialization =====
// This is the EXACT init sequence from the manufacturer's
// ER-TFTM2.25-1 driver. Do not change these register values.

void ST7789_nRF::begin(void) {
  // Configure pins
  pinMode(_dc, OUTPUT);
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);  // Deselect

  // Hardware reset
  if (_rst > 0) {
    pinMode(_rst, OUTPUT);
    digitalWrite(_rst, HIGH);
    delay(5);
    digitalWrite(_rst, LOW);
    delay(20);
    digitalWrite(_rst, HIGH);
    delay(150);
  }

  // Set up SPI
  SPI.begin();
  
  // We start a transaction for the init sequence
  SPI.beginTransaction(SPISettings(32000000, MSBFIRST, SPI_MODE0));

  // ===== Manufacturer init sequence =====

  // Porch control
  writecommand(0xB2);
  writedata(0x0C); writedata(0x0C); writedata(0x00);
  writedata(0x33); writedata(0x33);

  // RAM control
  writecommand(0xB0);
  writedata(0x00); writedata(0xE0);

  // MADCTL - Memory Access Control
  writecommand(0x36);
  writedata(0x00);

  // Color mode - 16bit/pixel (RGB565)
  writecommand(0x3A);
  writedata(0x05);

  // Gate control
  writecommand(0xB7);
  writedata(0x45);

  // VCOMS setting
  writecommand(0xBB);
  writedata(0x1D);

  // LCM control
  writecommand(0xC0);
  writedata(0x2C);

  // VDV and VRH command enable
  writecommand(0xC2);
  writedata(0x01);

  // VRH set
  writecommand(0xC3);
  writedata(0x19);

  // VDV set
  writecommand(0xC4);
  writedata(0x20);

  // Frame rate control
  writecommand(0xC6);
  writedata(0x0F);

  // Power control
  writecommand(0xD0);
  writedata(0xA4); writedata(0xA1);

  // Undocumented but required
  writecommand(0xD6);
  writedata(0xA1);

  // Positive gamma correction
  writecommand(0xE0);
  writedata(0xD0); writedata(0x10); writedata(0x21);
  writedata(0x14); writedata(0x15); writedata(0x2D);
  writedata(0x41); writedata(0x44); writedata(0x4F);
  writedata(0x28); writedata(0x0E); writedata(0x0C);
  writedata(0x1D); writedata(0x1F);

  // Negative gamma correction
  writecommand(0xE1);
  writedata(0xD0); writedata(0x0F); writedata(0x1B);
  writedata(0x0D); writedata(0x0D); writedata(0x26);
  writedata(0x42); writedata(0x54); writedata(0x50);
  writedata(0x3E); writedata(0x1A); writedata(0x18);
  writedata(0x22); writedata(0x25);

  // Exit sleep
  writecommand(0x11);
  delay(120);

  // Display ON
  writecommand(0x29);
  delay(50);
  
  SPI.endTransaction();
}

// ===== Address Window =====
// Critical: This applies the panel-specific offsets (0x52, 0x12)
// Without these offsets you'll be writing to the wrong area of
// the ST7789's internal 240x320 framebuffer -> garbage on screen.

void ST7789_nRF::setAddrWindow(uint16_t x0, uint16_t y0,
                                uint16_t x1, uint16_t y1) {
  x0 += _xstart;
  x1 += _xstart;
  y0 += _ystart;
  y1 += _ystart;

  writecommand(ST7789_CASET);  // Column address set
  writedata(x0 >> 8);
  writedata(x0 & 0xFF);
  writedata(x1 >> 8);
  writedata(x1 & 0xFF);

  writecommand(ST7789_PASET);  // Row address set
  writedata(y0 >> 8);
  writedata(y0 & 0xFF);
  writedata(y1 >> 8);
  writedata(y1 & 0xFF);

  writecommand(ST7789_RAMWR);  // Write to RAM
}

// ===== Drawing Functions =====

void ST7789_nRF::pushColor(uint16_t color) {
  SPI.beginTransaction(SPISettings(32000000, MSBFIRST, SPI_MODE0));
  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  spiwrite(color >> 8);
  spiwrite(color);
  digitalWrite(_cs, HIGH);
  SPI.endTransaction();
}

void ST7789_nRF::drawPixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) return;

  SPI.beginTransaction(SPISettings(32000000, MSBFIRST, SPI_MODE0));
  setAddrWindow(x, y, x, y);

  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  spiwrite(color >> 8);
  spiwrite(color);
  digitalWrite(_cs, HIGH);
  SPI.endTransaction();
}

void ST7789_nRF::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
  if ((x < 0) || (x >= _width) || (y >= _height)) return;
  if (y < 0) { h += y; y = 0; }
  if ((y + h) > _height) h = _height - y;
  if (h <= 0) return;

  SPI.beginTransaction(SPISettings(32000000, MSBFIRST, SPI_MODE0));
  setAddrWindow(x, y, x, y + h - 1);

  uint8_t hi = color >> 8, lo = color;

  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  while (h--) {
    spiwrite(hi);
    spiwrite(lo);
  }
  digitalWrite(_cs, HIGH);
  SPI.endTransaction();
}

void ST7789_nRF::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
  if ((y < 0) || (y >= _height) || (x >= _width)) return;
  if (x < 0) { w += x; x = 0; }
  if ((x + w) > _width) w = _width - x;
  if (w <= 0) return;

  SPI.beginTransaction(SPISettings(32000000, MSBFIRST, SPI_MODE0));
  setAddrWindow(x, y, x + w - 1, y);

  uint8_t hi = color >> 8, lo = color;

  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  while (w--) {
    spiwrite(hi);
    spiwrite(lo);
  }
  digitalWrite(_cs, HIGH);
  SPI.endTransaction();
}

void ST7789_nRF::fillScreen(uint16_t color) {
  fillRect(0, 0, _width, _height, color);
}

void ST7789_nRF::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  if ((x >= _width) || (y >= _height)) return;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if ((x + w) > _width)  w = _width  - x;
  if ((y + h) > _height) h = _height - y;
  if (w <= 0 || h <= 0) return;

  SPI.beginTransaction(SPISettings(32000000, MSBFIRST, SPI_MODE0));
  setAddrWindow(x, y, x + w - 1, y + h - 1);

  uint8_t hi = color >> 8, lo = color;

  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  
  uint32_t pixels = (uint32_t)w * h;
  while (pixels--) {
    spiwrite(hi);
    spiwrite(lo);
  }
  
  digitalWrite(_cs, HIGH);
  SPI.endTransaction();
}

uint16_t ST7789_nRF::color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void ST7789_nRF::drawBuffer(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *buffer) {
  if ((x >= _width) || (y >= _height)) return;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if ((x + w) > _width)  w = _width  - x;
  if ((y + h) > _height) h = _height - y;
  if (w <= 0 || h <= 0) return;

  SPI.beginTransaction(SPISettings(32000000, MSBFIRST, SPI_MODE0));
  setAddrWindow(x, y, x + w - 1, y + h - 1);

  digitalWrite(_dc, HIGH);
  digitalWrite(_cs, LOW);
  
  uint32_t pixels = (uint32_t)w * h;
  
  // In-place byte swap for ST7789 Big-Endian requirement
  for (uint32_t i = 0; i < pixels; i++) {
    uint16_t c = buffer[i];
    buffer[i] = (c >> 8) | (c << 8);
  }
  
  // Fast block transfer using EasyDMA
  SPI.transfer((uint8_t*)buffer, pixels * 2);
  
  // Swap back to Little-Endian (in case the canvas is reused)
  for (uint32_t i = 0; i < pixels; i++) {
    uint16_t c = buffer[i];
    buffer[i] = (c >> 8) | (c << 8);
  }
  
  digitalWrite(_cs, HIGH);
  SPI.endTransaction();
}

void ST7789_nRF::setRotation(uint8_t m) {
  rotation = m % 4;
  SPI.beginTransaction(SPISettings(32000000, MSBFIRST, SPI_MODE0));
  writecommand(0x36); // MADCTL
  switch (rotation) {
    case 0:
      writedata(0x00);
      _width  = ST7789_TFTWIDTH;
      _height = ST7789_TFTHEIGHT;
      _xstart = ST7789_XOFFSET; // 82
      _ystart = ST7789_YOFFSET; // 18
      break;
    case 1:
      writedata(0x60); // MV | MX
      _width  = ST7789_TFTHEIGHT;
      _height = ST7789_TFTWIDTH;
      _xstart = ST7789_YOFFSET; // 18
      _ystart = ST7789_XOFFSET; // 82
      break;
    case 2:
      writedata(0xC0); // MY | MX
      _width  = ST7789_TFTWIDTH;
      _height = ST7789_TFTHEIGHT;
      _xstart = 240 - ST7789_TFTWIDTH - ST7789_XOFFSET; // 240-76-82 = 82
      _ystart = 320 - ST7789_TFTHEIGHT - ST7789_YOFFSET; // 320-284-18 = 18
      break;
    case 3:
      writedata(0xA0); // MY | MV
      _width  = ST7789_TFTHEIGHT;
      _height = ST7789_TFTWIDTH;
      _xstart = ST7789_YOFFSET; // 18
      _ystart = ST7789_XOFFSET; // 82
      break;
  }
  SPI.endTransaction();
}
