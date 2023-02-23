## Components
### Parts list
#### Microcontroller
* Arduino Nano 33 IoT

#### Shields
* DFRobot LCD Keypad Shield

#### Breakout Boards
##### RTC
* [Adafruit DS3231 Precision RTC Breakout](https://learn.adafruit.com/adafruit-ds3231-precision-rtc-breakout)

##### Level Shifters
* SparkFun Logic Level Converter - Bi-Directional

##### Terminals
* SparkFun TRRS 3.5mm Jack Breakout
* Micro USB 2.0 Breakout Board

#### Structure
* Prototyping HAT für Raspberry Pi

#### Power Supply
* Official Raspberry Pi 12.5W Micro USB Power Supply (+5.1V DC)

### Further information
Find manufacturer data-sheets in [Datasheets](./Datasheets/).

##### Arduino Nano 33 IoT
* [GitHub - ostaquet/Arduino-Nano-33-IoT-Ultimate-Guide: Arduino Nano 33 IoT - Ultimate guide](https://github.com/ostaquet/Arduino-Nano-33-IoT-Ultimate-Guide)

##### DFRobot Arduino LCD Keypad Shield
* [Arduino_LCD_KeyPad_Shield__SKU__DFR0009_ (Version 1.0)](https://web.archive.org/web/20210610030650/https://wiki.dfrobot.com/Arduino_LCD_KeyPad_Shield__SKU__DFR0009_)
  * [LCDKeypad Shield Schematics V1.0](https://web.archive.org/web/20210610030650/https://www.dfrobot.com/image/data/DFR0009/LCDKeypad%20Shield%20V1.0%20SCH.pdf)
  * [Shield diagram](https://web.archive.org/web/20210610030650/http://www.shieldlist.org/dfrobot/lcd)

## Known issues
### Unstable USB (data-) connectivity
The Nano 33 IoT's AREF-pin is connected to the VUSB-pin (5V). This seems to interfere with the microcontroller's capability to maintain a stable (uninterrupted) USB data-connectivity.

As a workaround temporarily remove the connection of AREF to VUSB. (Note: While that connection is removed, some of the LCD Keypad Shield's button will stop working.)
