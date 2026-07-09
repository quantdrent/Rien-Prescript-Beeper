

#include "ERGFX.h"
#include "ST7789_nRF52840.h"
#include <SPI.h>



#define TFT_CS    D5
#define TFT_DC    D6
#define TFT_RST   D7
#define TFT_SCLK  D8
#define TFT_MOSI  D9






ST7789_nRF tft = ST7789_nRF(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);










void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=================================");
  Serial.println("ST7789 76x284 Test - nRF52840");
  Serial.println("=================================");

  Serial.println("Calling tft.begin()...");
  tft.begin();
  delay(100);

  Serial.println("Filling screen RED...");
  tft.fillScreen(ST7789_RED);
  delay(1000);


  Serial.println("Drawing test pattern...");


  tft.fillScreen(ST7789_BLACK);
  delay(200);


  int w = 76;
  int h = 284;


  tft.fillRect(0, 0, w, h / 4, ST7789_RED);


  tft.fillRect(0, h / 4, w, h / 4, ST7789_GREEN);


  tft.fillRect(0, h / 2, w, h / 4, ST7789_BLUE);


  tft.fillRect(0, 3 * (h / 4), w, h / 4, ST7789_YELLOW);

  delay(500);


  tft.setCursor(2, 10);
  tft.setTextColor(ST7789_WHITE);
  tft.setTextSize(1);
  tft.println("Hello!");

  tft.setCursor(2, 85);
  tft.setTextColor(ST7789_BLACK);
  tft.setTextSize(1);
  tft.println("76x284");

  tft.setCursor(2, 155);
  tft.setTextColor(ST7789_WHITE);
  tft.setTextSize(1);
  tft.println("ST7789");

  tft.setCursor(2, 225);
  tft.setTextColor(ST7789_RED);
  tft.setTextSize(1);
  tft.println("nRF52840");

  Serial.println("Setup done! Entering color cycle...");
  delay(3000);
}

void loop() {

  Serial.println("RED");
  tft.fillScreen(ST7789_RED);
  delay(2000);

  Serial.println("GREEN");
  tft.fillScreen(ST7789_GREEN);
  delay(2000);

  Serial.println("BLUE");
  tft.fillScreen(ST7789_BLUE);
  delay(2000);

  Serial.println("WHITE");
  tft.fillScreen(ST7789_WHITE);
  delay(2000);

  Serial.println("CYAN");
  tft.fillScreen(ST7789_CYAN);
  delay(2000);

  Serial.println("MAGENTA");
  tft.fillScreen(ST7789_MAGENTA);
  delay(2000);

  Serial.println("YELLOW");
  tft.fillScreen(ST7789_YELLOW);
  delay(2000);

  Serial.println("BLACK");
  tft.fillScreen(ST7789_BLACK);
  delay(2000);
}
