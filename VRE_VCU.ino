#include "Wire.h"
#include "I2Cdev.h"
#include "./DS1307.h"
#include "./MPU6050.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <Arduino.h>
#include <ESP32CAN.h>
#include <CAN_config.h>

#include "BluetoothSerial.h"

BluetoothSerial ESP_BT; 

/*
34 - th1 - brown o
35 - th2- orange
15

33 - pwm - brown - throttle
25 - pwm - red   - steering
26 -     - orange
*/

// Pin declaration 
const int ST_EN=25, ST1=15, ST2=26, TH1=14, TH2=27, TH_EN=33;
//const int TH_EN=25, TH1=15, TH2=26, ST1=34, ST2=35, ST_EN=33;
const int freq = 5000;
const int TH_Channel = 0, ST_Channel = 2;
const int resolution = 8;

int LED_BUILTIN = 2;     // 

// setting PWM properties
//const int freq = 5000;
//const int ledChannel = 0;
//const int brakeChannel = 2;
//const int resolution = 8;

//const int brake_max = 60;
//const int throttle_limit = 75;

// CAN Data
float current, volt, volt_12, SOC, DOD;
float resistance, summed_volt, avg_temp, health, low_volt_id, high_volt_id;
float amp_hrs, CCL, DCL;
float pack_id, internal_volt, resistance_cell, open_volt; 

// Logger Params
float WHEEL_SPEED = 0;
float BATTERY_TEMP = 30;
float BATTERY_SOC = 70;
float BATTERY_VOLTAGE = 90;
float CURRENT_DRAW = 2;
float RTC_TIME = 0;
float THROTTLE_VALUE = 0;

// Pins where analogRead is performed
//int DIRECT_SENSORS[] = {BUTTON, APPS, APPS2, BPS};
//String DIRECT_SENSORS_NAMES[] = {"BUTTON", "APPS1", "APPS2", "BPS"};
//int DIRECT_SENSORS_SIZE = 4;

// Pointers integers with more sensor data
float *SENSORS[] = {&current, &volt, &volt_12, &SOC, &DOD, &resistance, &summed_volt, &avg_temp, &health, &low_volt_id, &high_volt_id, &amp_hrs, &CCL, &DCL, &pack_id, &internal_volt, &resistance_cell, &open_volt, &WHEEL_SPEED, &BATTERY_TEMP, &BATTERY_SOC, &BATTERY_VOLTAGE, &CURRENT_DRAW, &THROTTLE_VALUE, &RTC_TIME};
String SENSORS_NAMES[] = {"current", "volt", "volt_12", "SOC", "DOD", "resistance", "summed_volt", "avg_temp", "health", "low_volt_id", "high_volt_id", "amp_hrs", "CCL", "DCL", "pack_id", "internal_volt", "resistance_cell", "open_volt", "WHEEL_SPEED", "BATTERY_TEMP", "BATTERY_SOC", "BATTERY_VOLTAGE", "CURRENT_DRAW", "THROTTLE_VALUE", "RTC_TIME"};
int SENSORS_SIZE = 25;


bool SD_available = false;
String LOGS_PATH = "/VEGA";
String NEW_LOG_FILE = "tmp.txt";
void log_sensor_names() {
  if (!SD_available)
    return;

  // TODO : Use timestamp in place of random number
  NEW_LOG_FILE = LOGS_PATH + "/logs_" + String(random(10000, 99999)) + ".txt";
  
  
  if (!dirExists(SD, LOGS_PATH.c_str())) {
    createDir(SD, LOGS_PATH.c_str());
    //writeFile(SD, NEW_LOG_FILE.c_str(), "Testing");
  }

  listDir(SD, LOGS_PATH.c_str(), 1); 
  
/*  for (int i=0; i<DIRECT_SENSORS_SIZE; i++) {
    appendFile(SD, NEW_LOG_FILE.c_str(), DIRECT_SENSORS_NAMES[i].c_str());
    appendFile(SD, NEW_LOG_FILE.c_str(), ",");
  }
*/
  for (int i=0; i<SENSORS_SIZE; i++) {
    appendFile(SD, NEW_LOG_FILE.c_str(), SENSORS_NAMES[i].c_str());
    appendFile(SD, NEW_LOG_FILE.c_str(), ",");
  }
  appendFile(SD, NEW_LOG_FILE.c_str(), "\n");

}


void log_current_frame_serial() {
  /*
  for (int i=0; i<DIRECT_SENSORS_SIZE; i++) {
    Serial.print(DIRECT_SENSORS_NAMES[i] + ": " + String(map(analogRead(DIRECT_SENSORS[i]), 0, 4095, 0, 255)) + ", ");
    ESP_BT.print(DIRECT_SENSORS_NAMES[i] + ": " + String(map(analogRead(DIRECT_SENSORS[i]), 0, 4095, 0, 255)) + ", ");
  }
  */

  for (int i=0; i<SENSORS_SIZE; i++) {
    Serial.print(SENSORS_NAMES[i] + ": " + String(*SENSORS[i]) + ", ");
    ESP_BT.print(SENSORS_NAMES[i] + ": " + String(*SENSORS[i]) + ", ");
  }
  Serial.println();
  ESP_BT.println();
}

void log_current_frame() {
  /*
  for (int i=0; i<DIRECT_SENSORS_SIZE; i++) {
    appendFile(SD, NEW_LOG_FILE.c_str(), String(map(analogRead(DIRECT_SENSORS[i]), 0, 4095, 0, 255)).c_str());
    appendFile(SD, NEW_LOG_FILE.c_str(), ",");
    //Serial.print(DIRECT_SENSORS_NAMES[i] + ": " + String(map(analogRead(DIRECT_SENSORS[i]), 0, 4095, 0, 255)) + ", ");
  }
  */

  for (int i=0; i<SENSORS_SIZE; i++) {
    appendFile(SD, NEW_LOG_FILE.c_str(), String(*SENSORS[i]).c_str());
    appendFile(SD, NEW_LOG_FILE.c_str(), ",");
    //Serial.print(SENSORS_NAMES[i] + ": " + String(*SENSORS[i]) + ", ");
  }
  appendFile(SD, NEW_LOG_FILE.c_str(), "\n");
  //Serial.println();
}

// Files
bool dirExists(fs::FS &fs, const char * dirname) {
  if (!SD_available) return false;
  //Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open("/");
    if(!root){
        Serial.println("Failed to open directory");
        return false;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return false;
    }

    File file = root.openNextFile();
    while(file){
        //if(file.isDirectory(dirname)) {
          if (file.name()==dirname)
            return true;
        //}
        file = root.openNextFile();
    }

    return false;
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
  if (!SD_available) return;
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.print (file.name());
            time_t t= file.getLastWrite();
            struct tm * tmstruct = localtime(&t);
            Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct->tm_year)+1900,( tmstruct->tm_mon)+1, tmstruct->tm_mday,tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.print(file.size());
            time_t t= file.getLastWrite();
            struct tm * tmstruct = localtime(&t);
            Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n",(tmstruct->tm_year)+1900,( tmstruct->tm_mon)+1, tmstruct->tm_mday,tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
  if (!SD_available) return;
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
  if (!SD_available) return;
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
  if (!SD_available) return;
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  if (!SD_available) return;
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  if (!SD_available) return;
    //Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        //Serial.println("Message appended");
    } else {
        //Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}
// Files END

// Gyro
MPU6050 accelgyro(0x69); // <-- use for AD0 high

int16_t ax, ay, az;
int16_t gx, gy, gz;

void get_MPU_values() {
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);  
  // 1788 -512  15148 -263  212 -183 - Offsets
  ax-=-1788; ay-=-512; az-=15148; gx-=-263; gy-=212; gz-=-183;
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
// Gyro end


// RTC Timekeeper
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

int total_days_till_month(int month) {
  /* Constant number of month declarations */
    if (month==0)
      return 0;
    return days_in_month(month) + total_days_till_month(month-1);
}

int days_in_month(int month) {
  /* Constant number of month declarations */
    const int MONTHS[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return MONTHS[month - 1];
}
// RTC Timekeeper end


void steering(int steering_angle) {
  if (steering_angle>0) {
    digitalWrite(ST1, HIGH);
    digitalWrite(ST2, LOW);
  } else if (steering_angle<0) {
    digitalWrite(ST1, LOW);
    digitalWrite(ST2, HIGH);
  } else {
    digitalWrite(ST1, LOW);
    digitalWrite(ST2, LOW);
  }
  // TODO : Plausibility check
  //constrain(map(abs(steering_angle),0,100,0,255),0,255)
  //analogWrite(ST_EN, );
  int out = constrain(map(abs(steering_angle),0,240,0,255),0,255);
  ledcWrite(ST_Channel, out);
  Serial.println("steering_angle - " + String(steering_angle) + "; PWM - " + String(out));
}

void throttle(int th, int brk) {
  if (th>0) {
    digitalWrite(TH1, HIGH);
    digitalWrite(TH2, LOW);
  } else if (th<0) {
    digitalWrite(TH1, LOW);
    digitalWrite(TH2, HIGH);
  } else {
    digitalWrite(TH1, LOW);
    digitalWrite(TH2, LOW);
  }
  // TODO : Plausibility check
  //constrain(map(abs(steering_angle),0,100,0,255),0,255)
  //analogWrite(ST_EN, );
  int out = constrain(map(abs(th),0,240,0,255),0,255);
  ledcWrite(TH_Channel, out);
  Serial.println("TH - " + String(th) + "; PWM - " + String(out));
}


void throttle_bk(int th, bool brake) {
  if (brake) {
    digitalWrite(TH1, HIGH);
    digitalWrite(TH2, HIGH);
  } else {
    if (th>0) {
      digitalWrite(TH1, HIGH);
      digitalWrite(TH2, LOW);
    } else if (th<0) {
      digitalWrite(TH1, LOW);
      digitalWrite(TH2, HIGH);
    } else {
      digitalWrite(TH1, LOW);
      digitalWrite(TH2, LOW);
    }
  }
  // TODO : Plausibility check
  //constrain(map(abs(steering_angle),0,100,0,255),0,255)
  //int out = constrain(map(abs(th),0,240,0,255),0,160);
  int out = constrain(map(abs(th),0,240,0,255),0,255);
  ledcWrite(TH_Channel, out);
  Serial.println("TH - " + String(th) + "; PWM - " + String(out));
}

void throttle(int th) {
  throttle(th, false);
}

const int SHUTDOWN_RELAY = 13;
void emergency_stop() {
  digitalWrite(SHUTDOWN_RELAY, LOW);
}

void startup_TS() {
  digitalWrite(SHUTDOWN_RELAY, HIGH);
}

void ESP_BT_Commands() {
  int byt;
  if (ESP_BT.available()) //Check if we receive anything from Bluetooth
  {
    char incoming = ESP_BT.read(); //Read what we recevive
    //Serial.print("Received:"); Serial.println(incoming);
    //ESP_BT.print("Received:"); ESP_BT.println(incoming);
    switch (incoming) {
      case 's':
        Serial.println("Remote shutdown triggered");
        ESP_BT.println("Remote shutdown triggered");
        emergency_stop();
        break;  
      case 'g':
        Serial.println("Remote startup triggered");
        ESP_BT.println("Remote startup triggered");
        emergency_stop();
        break;
      case 'f':
        while (!ESP_BT.available()) {}
        byt = ESP_BT.read();
        Serial.println("f : " + String(byt));
        throttle(byt);
        break;
      case 'b':
        while (!ESP_BT.available()) {}
        byt = ESP_BT.read();
        Serial.println("b : " + String(byt));
        throttle(-byt);
        break;
      case 'l':
        while (!ESP_BT.available()) {}
        byt = ESP_BT.read();
        Serial.println("l : " + String(byt));
        steering(byt);
        break;
      case 'r':
        while (!ESP_BT.available()) {}
        byt = ESP_BT.read();
        Serial.println("r : " + String(byt));
        steering(-byt);
        break;
    }
  }
}

void setup() {
  // initialize serial communication
  // (38400 chosen because it works as well at 8MHz as it does at 16MHz, but
  // it's really up to you depending on your project)
  Serial.begin(115200);
  Serial.println("\n\n\nStarting up...\n");

  ESP_BT.begin("VRE_VCU"); 

  ESP_BT.println("Bluetooth Logging Started"); 
  
  
  ledcSetup(TH_Channel, freq, resolution);
  ledcAttachPin(TH_EN, TH_Channel);
  
  ledcSetup(ST_Channel, freq, resolution);
  ledcAttachPin(ST_EN, ST_Channel);
  pinMode(TH_EN, OUTPUT);
  pinMode(TH1, OUTPUT);
  pinMode(TH2, OUTPUT);
  pinMode(ST_EN, OUTPUT);
  pinMode(ST1, OUTPUT);
  pinMode(ST2, OUTPUT);

  //pinMode(THROTTLE_OUT, OUTPUT);
  //pinMode(LED_BUILTIN, OUTPUT);
  //pinMode(BRAKE_LIGHTS, OUTPUT);


  //pinMode(LED_FAULT, OUTPUT);
  //digitalWrite(LED_FAULT, LOW);
  //pinMode(BUZZER, OUTPUT);
  //digitalWrite(BUZZER, LOW);
  //pinMode(BUTTON, INPUT_PULLUP);
  //digitalWrite(BUTTON, HIGH);
  
  //pinMode(APPS, INPUT);
  //digitalWrite(APPS, HIGH);
  //pinMode(BPS, INPUT);
  //digitalWrite(BPS, HIGH);

  //ledcSetup(ledChannel, freq, resolution);
  //ledcSetup(brakeChannel, freq, resolution);
  
  // attach the channel to the GPIO to be controlled
  //ledcAttachPin(THROTTLE_OUT, ledChannel);
  //ledcAttachPin(LED_BUILTIN, ledChannel);
  //ledcAttachPin(BRAKE_LIGHTS, brakeChannel);

  pinMode(SHUTDOWN_RELAY, OUTPUT);
  startup_TS();

  Serial.println("Testing device connections...");
  ESP_BT.println("Testing device connections...");
  
  Wire.begin();
  
  rtc.initialize();
  Serial.println(rtc.testConnection() ? "DS1307 connection successful" : "DS1307 connection failed");
  //rtc.setDateTime24(2021, 2, 11, 8, 56, 0); // Use this to set the RTC time
  //setDateTime24(uint16_t year, uint8_t month, uint8_t day, uint8_t hours, uint8_t minutes, uint8_t seconds);

  accelgyro.initialize();
  Serial.print("Connecting to MPU6050");
  // TODO : Change infinite loop to only attempt for some n times
  int mpu_connect_tries = 0;
  while (!accelgyro.testConnection() && mpu_connect_tries<5) {
    Serial.print(".");
    delay(100);
    mpu_connect_tries++;
  }
  Serial.println("\t Connected");
  Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

  ESP_BT.println("Connecting to SD Card");

  Serial.println("Connect to Card");
  int SD_connect_tries = 0;
  while (!SD.begin()) {
    Serial.print(".");
    delay(100);
    SD_connect_tries++;

    if (SD_connect_tries>=3) {
      Serial.println("Card Mount Failed");
      return;
    }
  }

  /*
  if(!SD.begin()){
      Serial.println("Card Mount Failed");
      ESP_BT.println("Card Mount Failed");
      SD_available = false;
      return;
  }
  */
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    ESP_BT.println("No SD card attached");
    SD_available = false;
    return;
  }

  Serial.print("SD Card Type: ");
  ESP_BT.print("");
  if(cardType == CARD_MMC){
    SD_available = true;
    Serial.println("MMC");
    ESP_BT.println("MMC");
  } else if(cardType == CARD_SD){
    SD_available = true;
    Serial.println("SDSC");
    ESP_BT.println("SDSC");
  } else if(cardType == CARD_SDHC){
    SD_available = true;
    Serial.println("SDHC");
    ESP_BT.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
    ESP_BT.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  ESP_BT.printf("SD Card Size: %lluMB\n", cardSize);

  log_sensor_names();

  readFile(SD, NEW_LOG_FILE.c_str());
}

bool started = false;
int last_log_print = 0;
String LAST_FAULT_MESSAGE = "";
void loop() {
  
  //delay(1000);
  log_current_frame();
  
  if (millis() - last_log_print >= 300) {
    //print_time();
    //log_current_frame_serial();
    last_log_print = millis();
  }


  //get_RTC_TIME();
  //print_time();

  get_MPU_values();

  //CAN_loop();

  ESP_BT_Commands();

  /*
  
  int brakes = analogRead(BPS);
  brakes = map(brakes, 0, 4095, 0, 255);
  ledcWrite(brakeChannel, brakes);

  int apps1 = analogRead(APPS);
  //apps1 = map(apps1, 0, 4095, 0, 255);

  int apps2 = analogRead(APPS2);
  //apps2 = map(apps1, 0, 4095, 0, 255);

  bool plausibility = false;

  if (abs(apps1-apps2)<409) plausibility = true;

  plausibility = true; brakes = 0;
  
  THROTTLE_VALUE = 0;

  if (plausibility) {
    THROTTLE_VALUE = map((apps1 + apps2)/2, 0, 4095, 0, 255);
  }
  
  if (brakes>brake_max && THROTTLE_VALUE>throttle_limit) {
    THROTTLE_VALUE = throttle_limit;
    digitalWrite(LED_FAULT, HIGH);
  } else {
    digitalWrite(LED_FAULT, LOW);
  }

  


  */
  //started = true;
  if (started) {
    //Serial.print(apps1);
    //Serial.print(" - ");
    //Serial.print(apps2);
    //Serial.print(" - ");
    //Serial.println(THROTTLE_VALUE);
    //ledcWrite(ledChannel, abs(THROTTLE_VALUE));
    //digitalWrite();
  } else {
    
  }
}
