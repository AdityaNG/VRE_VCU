#include "I2Cdev.h"
#include "./DS1307.h"
#include "./BT_helper.h"

float RTC_TIME = 0;

DS1307 rtc;
uint16_t year;
uint8_t month, day, hours, minutes, seconds;
void get_RTC_TIME() {
  rtc.getDateTime24(&year, &month, &day, &hours, &minutes, &seconds);

  //RTC_TIME = seconds + 60*minutes + 3600*hours + 86400*(day+total_days_till_month(month-1));
  RTC_TIME = seconds + 60*minutes + 3600*hours + 86400*day;

  /*
  String TIME = "";
  TIME+= year+"-";
  if (month < 10) TIME+="0";
  TIME+= month+"-";
  if (day < 10) TIME+="0";
  TIME+= day+" ";
  if (hours < 10) TIME+="0";
  TIME+= hours+":";
  if (minutes < 10) TIME+="0";
  TIME+= minutes+":";
  if (seconds < 10) TIME+="0";
  TIME+= seconds;
  */
}

void print_time() {
  //Serial.print("rtc:\t");
  Serial.print(year); Serial.print("-");
  if (month < 10) Serial.print("0");
  Serial.print(month); Serial.print("-");
  if (day < 10) Serial.print("0");
  Serial.print(day); Serial.print(" ");
  if (hours < 10) Serial.print("0");
  Serial.print(hours); Serial.print(":");
  if (minutes < 10) Serial.print("0");
  Serial.print(minutes); Serial.print(":");
  if (seconds < 10) Serial.print("0");
  Serial.print(seconds);
  Serial.print(" | ");


  ESP_BT.print(year); ESP_BT.print("-");
  if (month < 10) ESP_BT.print("0");
  ESP_BT.print(month); ESP_BT.print("-");
  if (day < 10) ESP_BT.print("0");
  ESP_BT.print(day); ESP_BT.print(" ");
  if (hours < 10) ESP_BT.print("0");
  ESP_BT.print(hours); ESP_BT.print(":");
  if (minutes < 10) ESP_BT.print("0");
  ESP_BT.print(minutes); ESP_BT.print(":");
  if (seconds < 10) ESP_BT.print("0");
  ESP_BT.print(seconds);
  ESP_BT.print(" | ");

  
}

int days_in_month(int month) {
  /* Constant number of month declarations */
    const int MONTHS[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return MONTHS[month - 1];
}

int total_days_till_month(int month) {
  /* Constant number of month declarations */
    if (month==0)
      return 0;
    return days_in_month(month) + total_days_till_month(month-1);
}
