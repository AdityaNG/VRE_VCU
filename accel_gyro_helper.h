#include "I2Cdev.h"
#include "./MPU6050.h"

float axf, ayf, azf;
float gxf, gyf, gzf;

MPU6050 accelgyro(0x69); // <-- use for AD0 high

int16_t ax, ay, az;
int16_t gx, gy, gz;

void get_MPU_values() {
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);  
  // 1788 -512  15148 -263  212 -183 - Offsets
  //ax-=-1788; ay-=-512; az-=15148; gx-=-263; gy-=212; gz-=-183;
  axf=ax; ayf=ay; azf=az; gxf=gx; gyf=gy; gzf=gz;
  /*
  Serial.print("a/g:\t");
        Serial.print(ax); Serial.print("\t");
        Serial.print(ay); Serial.print("\t");
        Serial.print(az); Serial.print("\t");
        Serial.print(gx); Serial.print("\t");
        Serial.print(gy); Serial.print("\t");
        Serial.println(gz);
  */

        
}
