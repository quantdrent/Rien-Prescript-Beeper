# Rien Index Beeper Device

![License: MIT](https://img.shields.io/badge/License-MIT-green)

This works similar to [this project](https://github.com/quantdrent/ESP32-Prescript-Beeper) with many qol.

this runs on web ble instead of wifi html, i recommend to use a SuperMini nRF52840 as the MCU but any esp32 boards should work too, i picked this up since i had it lying around and it has a built in charger and better battery performance than a esp32. If you want to use any other boards you need to tweak the gpio in config.h.

if you are unable to obtain a 2.25 inch tft screen you need to change the library and the object name, you also need to tweak the case in tinkercad just ungroup the white object and change the screen hole size and dont forget to group it again before exporting or ask help to @quantdrent on tiktok or discord

**You need to use a browser that supports web ble**

## Required Materials
* SuperMini nRF52840 [[AliExpress](https://www.aliexpress.com/item/1005006019812115.html)] (a supermini esp32 should work too)
* 2.25 Inch TFT LCD Module 76x284, ST7789 [[AliExpress](https://www.aliexpress.com/item/1005011855033572.html)]
* 2x Touch capacitive switches TTP-223 [[AliExpress](https://www.aliexpress.com/item/32964219843.html)]
* AWG 26 wires

>  Dont forget you need to solder too!

## Required Libraries

- `Adafruit ST7735 and ST7789 Library` (with their dependency)
- `Preinstalled board libraries`

## Instructions
> Look up "how to setup (boardname) Arduino IDE" before proceeding, for nrf check this out https://www.beachyuk.com/blog/connecting-and-testing-promicro-nrf52840-clones
1. Open .ino file on arduino ide
2. Open config.h and tweak the gpio if you are not using the nrf52
3. Upload the sketch
4. wire and assemble everything
5. Open the page linked below the description (make sure your browser supports web bluetooth)
6. Click Connect Beeper
7. Larp

## Images
wip.

The web ble page was forked from Kritzkingvoid Prescript web project

https://kritzkingvoid.github.io/Prescripts/
