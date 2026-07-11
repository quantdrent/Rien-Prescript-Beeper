# Rien Index Beeper Device

![License: MIT](https://img.shields.io/badge/License-MIT-green)

This works similar to [this project](https://github.com/quantdrent/ESP32-Prescript-Beeper) with many qol.
> If you want to use a esp32 then follow that guide instead (you will be able to use the same website for sending stuff but smaller device)

this runs on web ble instead of the old wifi html version, i used a nrf52840 because i had these lying around and maybe someday ill remake this to support a esp32 c3/s3
if you are unable to obtain a 2.25 inch tft screen you need to change the library and the object name, you also need to tweak the case in tinkercad or ask help to @quantdrent on tiktok and discord

**You need to use a browser that supports web ble**
(Chrome, Edge , Opera, etc)

https://quantdrent.github.io/Rien-Prescript-Beeper

## Required Materials
* SuperMini nRF52840 [[AliExpress](https://www.aliexpress.com/item/1005006019812115.html)] (a supermini esp32 should work too)
* 2.25 Inch TFT LCD Module 76x284, ST7789 [[AliExpress](https://www.aliexpress.com/item/1005011855033572.html)]
* 2x Touch capacitive switches TTP-223 [[AliExpress](https://www.aliexpress.com/item/32964219843.html)]
* Wires (use ur prefered wire type, i used awg 24)

>  Dont forget you need to solder too!
You also need access to a 3d printer to print the case check models folder.

## Instructions
> Look up "how to setup (boardname) Arduino IDE" before proceeding, for nrf52840 check this out https://www.beachyuk.com/blog/connecting-and-testing-promicro-nrf52840-clones
> For nr52840 dont forget to solder boost (two pads) if ur batt is more than 500
1. Solder Everything based on your wiring or you can follow the schematics in images folder if you are using a nrf52840 
2. Open the firmware folder, open the nrf52840 and open the .ino
3. Open config.h and tweak the gpio if you are not using the nrf52840
4. Upload the sketch
5. Open [this page](https://quantdrent.github.io/Rien-Prescript-Beeper/)
6. Click pair and find your device
7. click prescrits on the top middle and you can add or remove your saved prescripts
8. click the button on the middle of the screen to send a random prescripts
9. you can also get a random prescripts by pressing the buttons on your physical device
10. you can factory reset by pressing the red button on bottom right

## Images
wip.

The web ble page was forked from Kritzkingvoid Prescript web project

https://kritzkingvoid.github.io/Prescripts/ 
