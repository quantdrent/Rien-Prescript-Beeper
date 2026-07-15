//this is a joke version go open the older folder if you want to get the prescript thing

#include <Arduino.h>
#include <SPI.h>
#include "ST7789_nRF52840.h"
#include "frames.h"

#define TFT_CS      D5
#define TFT_DC      D6
#define TFT_RST     D7
#define TFT_MOSI    D9
#define TFT_SCLK    D8

ST7789_nRF tft = ST7789_nRF(TFT_CS, TFT_DC, TFT_RST);

uint16_t frameBuffer[BAD_APPLE_WIDTH * BAD_APPLE_HEIGHT];
int currentFrame = 0;

void setup() {
  Serial.begin(115200);
  pinMode(EXT_VCC, OUTPUT);
  digitalWrite(EXT_VCC, HIGH);
  delay(100);

  SPI.setPins(-1, TFT_SCLK, TFT_MOSI);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(ST7789_BLACK);
}

void loop() {
  if (currentFrame >= BAD_APPLE_FRAMES) {
    currentFrame = 0;
  }

  uint32_t start_idx = bad_apple_frame_offsets[currentFrame];
  uint32_t end_idx = bad_apple_frame_offsets[currentFrame + 1];

  uint16_t currentColor = ST7789_BLACK;
  int pixelIdx = 0;

  for (uint32_t i = start_idx; i < end_idx; i++) {
    uint8_t count = bad_apple_data[i];
    
    for (int p = 0; p < count; p++) {
      if (pixelIdx < (BAD_APPLE_WIDTH * BAD_APPLE_HEIGHT)) {
        frameBuffer[pixelIdx++] = currentColor;
      }
    }
    
    if (count != 255) {
      currentColor = (currentColor == ST7789_BLACK) ? 0xBF5F : ST7789_BLACK;
    }
  }

  int x_offset = (284 - BAD_APPLE_WIDTH) / 2;
  int y_offset = (76 - BAD_APPLE_HEIGHT) / 2;
  tft.drawBuffer(x_offset, y_offset, BAD_APPLE_WIDTH, BAD_APPLE_HEIGHT, frameBuffer);

  currentFrame++;
  
  // aprox 10 fps i think
  delay(100);
}
