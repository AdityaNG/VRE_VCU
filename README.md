# VRE_VCU

![Board](img/board.jpg)

The VCU handles the following tasks : 

1. Listening to throttle pedal, brake pressure, wheel speed, start button, other dash buttons
2. Generating appropriate throttle signal, RTDS, dispaly data, APPS plausibility check, work on shutdown circuit
3. Data logging, wireless live logging, Remote Emergency Shutdown, etc. 

## Libraries used :

- ["ESP32CAN.h"](https://www.arduino.cc/reference/en/libraries/can/)
- ["Wire.h"](https://www.arduino.cc/en/reference/wire)
- ["I2Cdev.h"](http://github.com/jrowberg/i2cdevlib)
- ["./DS1307.h"](https://github.com/jrowberg/i2cdevlib/tree/master/Arduino/DS1307)
- ["./MPU6050.h"](https://github.com/jrowberg/i2cdevlib/tree/master/Arduino/MPU6050)
- ["FS.h"](https://github.com/espressif/arduino-esp32/blob/master/libraries/FS/src/FS.h)
- ["SD.h"](https://github.com/espressif/arduino-esp32/blob/master/libraries/SD/src/SD.h)
- ["SPI.h"](https://github.com/espressif/arduino-esp32/tree/master/libraries/SPI)

Refer to [AdityaNG/VRE_CAN](https://github.com/AdityaNG/VRE_CAN) for CAN Data logger implementation.

![Banner](img/schem1.png)

## TODO
1. Data logging with dates
2. Wireless live data logging
3. Brake Pressure, Wheel Speed, Start button
4. RTDS, APPS check, Shutdown line
