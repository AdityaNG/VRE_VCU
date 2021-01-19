# VRE_VCU

![Banner](img/schem1.png)

The VCU handles the following tasks : 

1. Listening to throttle pedal, brake pressure, wheel speed, start button, other dash buttons
2. Generating appropriate throttle signal, RTDS, dispaly data, APPS plausibility check, work on shutdown circuit
3. Data logging, wireless live logging, Remote Emergency Shutdown, etc. 

Libraries used :
1. ["Wire.h"](https://www.arduino.cc/en/reference/wire)
2. ["I2Cdev.h"](http://github.com/jrowberg/i2cdevlib)
3. ["./DS1307.h"]()
4. ["./MPU6050.h"]()
5. ["FS.h"](https://github.com/espressif/arduino-esp32/blob/master/libraries/FS/src/FS.h)
6. ["SD.h"](https://github.com/espressif/arduino-esp32/blob/master/libraries/SD/src/SD.h)
7. ["SPI.h"](https://github.com/espressif/arduino-esp32/tree/master/libraries/SPI)

## TODO
1. Data logging with dates
2. Wireless live data logging
3. Brake Pressure, Wheel Speed, Start button
4. RTDS, APPS check, Shutdown line
