# Rien Index Beeper Device

![License: MIT](https://img.shields.io/badge/License-MIT-green)

This works similar to [this project](https://github.com/quantdrent/ESP32-Prescript-Beeper) with many qol.
> If you want to use a esp32 then follow that guide instead (you will be able to use the same website for sending stuff but smaller device)

this runs on web ble instead of the old wifi html version, i used a nrf52840 because i had these lying around, CHECK [this project](https://github.com/quantdrent/ESP32-Prescript-Beeper) FOR ESP32 PORT (note this uses a 1.3 Inch oled screen)

if you are unable to obtain a 2.25 inch tft screen you need to change the library and the object name, you also need to tweak the case in tinkercad or ask help to @quantdrent on tiktok and discord

**You need to use a browser that supports web ble**
(Chrome, Edge , Opera, etc)

https://quantdrent.github.io/Rien-Prescript-Beeper

## Required Materials
* SuperMini nRF52840 [[AliExpress](https://www.aliexpress.com/item/1005006019812115.html)]
* 2.25 Inch TFT LCD Module 76x284, ST7789 [[AliExpress](https://www.aliexpress.com/item/1005011855033572.html)]
* 2x Touch capacitive switches TTP-223 [[AliExpress](https://www.aliexpress.com/item/32964219843.html)]
* Wires (use ur prefered wire type, i used awg 24)

>  Dont forget you need to solder too!
You also need access to a 3d printer to print the case check models folder.

## Instructions
> check this out https://www.beachyuk.com/blog/connecting-and-testing-promicro-nrf52840-clones to setup
> dont forget to solder boost (two pads) if ur batt is more than 500
1. Download the firmware in released and pick your microcontroller or click code then Download Zip to get offline version of the site aswell as the firmwares
2. Open the firmware folder, open the nrf52840 and open the .ino
3. Solder Everything based on your .config 
5. Upload the sketch
6. Open [this page](https://quantdrent.github.io/Rien-Prescript-Beeper/) or the html
7. Click pair and find your device
8. click prescrits on the top middle and you can add or remove your saved prescripts aswell as export or import (follow the template by exporting first)
9. click the button on the middle of the screen to send a random prescripts
10. you can also get a random prescripts by pressing the buttons on your physical device
11. you can factory reset by pressing the red button on bottom right

## Images
wip.

The web ble page was forked from Kritzkingvoid Prescript web project

https://kritzkingvoid.github.io/Prescripts/ 
