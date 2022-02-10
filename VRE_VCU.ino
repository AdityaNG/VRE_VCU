#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#include "Wire.h"
#include "./I2Cdev.h"
#include "./DS1307.h"
#include "./MPU6050.h"
#include "./BT_helper.h"
#include "./accel_gyro_helper.h"
#include "./RTC_helper.h"
#include "FS.h"
#include "SD.h"
#include "./logger.h"
#include "SPI.h"
#include "BTS7960.h"
#include "sounddata.h"
#include <Arduino.h>
#include <ESP32Servo.h>

#include "BluetoothSerial.h"

//#include <TinyGPS++.h>
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

#include <CircularBuffer.h>
#define ROLLING_AVG 10
CircularBuffer<float, ROLLING_AVG> current_l_buf;
CircularBuffer<float, ROLLING_AVG> current_r_buf;

float buf_avg(CircularBuffer<float, ROLLING_AVG> &buf);

static const int RXPin = 16, TXPin = 17;
static const uint32_t GPSBaud = 9600;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);

//bool PRINT_LOGS = false;
volatile bool PRINT_LOGS = true;

QueueHandle_t queue;
int queueSize = 1;

// Pin declaration 
const uint8_t EN_L = 33;
const uint8_t EN_R = 25;
const uint8_t L_PWM = 26;
const uint8_t R_PWM = 15;
const uint8_t R_IS = 34;
const uint8_t L_IS = 35;

const uint8_t BUZZER_PIN = 12;

BTS7960 motorController(EN_L, EN_R, L_PWM, R_PWM, L_IS, R_IS);

Servo steering_servo;  // create servo object to control a servo
int full_left = 10;
int full_right = 170;

int pos = 0;    // variable to store the servo position
// Recommended PWM GPIO pins on the ESP32 include 2,4,12-19,21-23,25-27,32-33 
int servoPin = 27;
const int SHUTDOWN_RELAY = 32;
int LED_BUILTIN = 2;

float CURRENT_DRAW_L, CURRENT_DRAW_R, throttle_val, steering_val;

/* TODO : Understand the BTS7960 Motor Driver's current sense values and convert to Amps
 * 0,2000 - Eco    - Curent Limited to 2000
 * 0,3000 - Normal - Curent Limited to 3000
 * 0,4000 - Power  - No Current Limit
*/
float CURRENT_LIMIT = 2000;

int last_servo_pos = (full_left + full_right)/2;

// Logger Params
// Pointers integers with logging data
float *SENSORS[] = {&CURRENT_LIMIT, &axf, &ayf, &azf, &gxf, &gyf, &gzf, &CURRENT_DRAW_L, &CURRENT_DRAW_R, &throttle_val, &steering_val, &RTC_TIME};
String SENSORS_NAMES[] = {"CURRENT_LIMIT","ax", "ay", "az", "gx", "gy", "gz", "CURRENT_DRAW_L", "CURRENT_DRAW_R", "throttle_val", "steering_val", "RTC_TIME"};
int SENSORS_SIZE = 8;

String LOGS_PATH = "/VEGA";
String NEW_LOG_FILE = "tmp.txt";

/////////////////////////////////////////
void log_sensor_names();
void log_current_frame_serial();
void log_current_frame();

void steering(int steering_angle);
void throttle(int th, int brk);
void throttle(int th);


void emergency_stop();
void startup_TS();

void ESP_BT_Commands(void *pvParameters);

void ESP_BT_Commands_OLD(void *pvParameters);
static void smartDelay(unsigned long ms);

static void printInt(unsigned long val, bool valid, int len);

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t);

void test_steering();
/////////////////////////////////////////


void main_loop(void *pvParameters);

void setup() {
  // Serial Communication
  Serial.begin(115200);
  Serial.println("\n\n\nStarting up...\n");

  // Open SHUTDOWN_RELAY
  pinMode(SHUTDOWN_RELAY, OUTPUT);
  emergency_stop();

  // Allow allocation of all timers
  ESP32PWM::allocateTimer(3);

  // Steering servo
  steering_servo.setPeriodHertz(50);    // standard 50 hz servo
  steering_servo.attach(servoPin, 1000, 2000); // attaches the servo on servoPin to the servo object
  steering_servo.write(90);

  // Bluetooth
  ESP_BT.begin("VRE_VCU"); 
  ESP_BT.println("Bluetooth Logging Started"); 

  /*
   * Start connectiing to auxillary devices
   *  - RTC
   *  - MPU6050
   *  - GPS
   *  - SD Card
  */
  Serial.println("Testing device connections...");
  ESP_BT.println("Testing device connections...");

  // Start I2C
  Wire.begin();

  // Connectect to RTC
  rtc.initialize();
  Serial.println(rtc.testConnection() ? "DS1307 connection successful" : "DS1307 connection failed");
  //rtc.setDateTime24(2021, 2, 11, 8, 56, 0); // Use this to set the RTC time

  // Connect to MPU6050
  accelgyro.initialize();
  Serial.print("Connecting to MPU6050");
  int mpu_connect_tries = 0;
  while (!accelgyro.testConnection() && mpu_connect_tries<1) {
    Serial.print(".");
    delay(500);
    mpu_connect_tries++;
  }
  //Serial.println("\t Connected");
  Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

  // Connect to GPS
  // TODO : Test if GPS is online
  Serial.println("Connecting to GPS");
  ss.begin(GPSBaud);

  // Connect to SD Card
  ESP_BT.println("Connecting to SD Card");
  Serial.println("Connecting to SD Card");
  int SD_connect_tries = 0;
  while (!SD.begin()) {
    Serial.print(".");
    delay(100);
    SD_connect_tries++;

    if (SD_connect_tries>=1) {
      Serial.println("Card Mount Failed");
      break;
    } else {
      uint8_t cardType = SD.cardType();

      if(cardType == CARD_NONE){
        Serial.println("No SD card attached");
        ESP_BT.println("No SD card attached");
        SD_available = false;
        break;
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
    
      break;
    }
  }

  // Init done
  play_tone(boot_tone, BUZZER_PIN);

  queue = xQueueCreate( queueSize, sizeof( int ) );

  if(queue == NULL){
    Serial.println("Error creating the queue");
  }
  
  // Now set up two tasks to run independently.
  xTaskCreatePinnedToCore(
    ESP_BT_Commands
    ,  "ESP_BT_Commands"   // A name just for humans
    ,  10240  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    main_loop
    ,  "main_loop"
    ,  10240  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

  
}

void loop()
{
  // Empty. Things are done in Tasks.
}

bool started = false;
int last_log_print = 0, last_sesor_check = 0;
String LAST_FAULT_MESSAGE = "";
void main_loop(void *pvParameters) {
  int last_recieved_command = 0;
  while (true) {
    //Serial.println("Main loop");
    if (millis() - last_log_print >= 3000) {
      //PRINT_LOGS
      if (PRINT_LOGS) {
        //print_time();
        printDateTime(gps.date, gps.time);
        log_current_frame_serial();
      }
      last_log_print = millis();
    }
  
    if (millis() - last_sesor_check >= 1000) {
      get_RTC_TIME();   // Get RTC vals
      get_MPU_values(); // GET MPU6050 vals
  
      // Read Motor Current draw
      current_l_buf.push(motorController.CurrentSenseLeft());
      current_r_buf.push(motorController.CurrentSenseRight());
      CURRENT_DRAW_L = buf_avg(current_l_buf);
      CURRENT_DRAW_R = buf_avg(current_r_buf);
  
      
      last_sesor_check = millis();
    }

   // TODO : Throttle kill

    /*
    int messagesWaiting = uxQueueMessagesWaiting(queue);
    if (messagesWaiting>0) {
      //xQueueReceive(queue, &last_recieved_command, portMAX_DELAY); 
      xQueueReceive(queue, &last_recieved_command, 0); 
    }*/

    /*
    xQueueReceive(queue, &last_recieved_command, 0); 
    
    Serial.println("Delay : " + String(millis() - last_recieved_command ));
    
    if (millis() - last_recieved_command > 1500) {
      throttle(0);
      Serial.println("KILLING THROTTLE");
      //delay(1000);
    }
    */

    //delay(500);
  }
}

void ESP_BT_Commands(void *pvParameters) {
  int last_recieved_command = 0;  
  (void) pvParameters;

  bool ts_stat = false;

  while (true) {
    if (!ESP_BT.available())
      continue;
    char incoming = ESP_BT.read(); //Read what we recevive
    if (incoming!=';')
      continue;
    if (!ESP_BT.available())
      continue;
    
    int b1, th, st;

    b1 = ESP_BT.read();
    th = ESP_BT.read();
    st = ESP_BT.read();    

    bool sd, p, lg, regen, z1, z2, lft, rev;

    sd    = b1>>7 & 1;
    p     = b1>>6 & 1;
    lg    = b1>>5 & 1;
    regen = b1>>4 & 1;
    z1    = b1>>3 & 1;
    z2    = b1>>2 & 1;
    lft   = b1>>1 & 1;
    rev   = b1>>0 & 1;

    th = th * ((!rev)? 1: -1);
    st = st * ((lft)? 1: -1);
    
    if (p!=0 || z1!=0 || z2!=0)
      continue;

    if (ts_stat && sd) {
      //Shutdown 
      emergency_stop(); 
      ts_stat = false;
    }

    if (!ts_stat && !sd) {
      // start
      startup_TS();
      ts_stat = true;
    }

    if (lg) {
      PRINT_LOGS = true;
    } else {
      PRINT_LOGS = false;  
    }

    throttle(th, regen);
    steering(st);
    
    //last_recieved_command = millis();
    //xQueueSend(queue, &last_recieved_command, portMAX_DELAY);
    
  }
}

/////// helpers

float buf_avg(CircularBuffer<float, ROLLING_AVG> &buf) {
  float avg = 0;
  for (int i=0; i<buf.size(); i++) {
    avg += buf[i];
  }  
  avg = avg/buf.size();
  return avg;
}

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
  
  for (int i=0; i<SENSORS_SIZE; i++) {
    appendFile(SD, NEW_LOG_FILE.c_str(), SENSORS_NAMES[i].c_str());
    appendFile(SD, NEW_LOG_FILE.c_str(), ",");
  }
  appendFile(SD, NEW_LOG_FILE.c_str(), "\n");

}


void log_current_frame_serial() {
  
  for (int i=0; i<SENSORS_SIZE; i++) {
    Serial.print(SENSORS_NAMES[i] + ": " + String(*SENSORS[i]) + ", ");
    ESP_BT.print(SENSORS_NAMES[i] + ": " + String(*SENSORS[i]) + ", ");
  }
  Serial.println();
  ESP_BT.println();
}

void log_current_frame() {
  
  for (int i=0; i<SENSORS_SIZE; i++) {
    appendFile(SD, NEW_LOG_FILE.c_str(), String(*SENSORS[i]).c_str());
    appendFile(SD, NEW_LOG_FILE.c_str(), ",");
    //Serial.print(SENSORS_NAMES[i] + ": " + String(*SENSORS[i]) + ", ");
  }
  appendFile(SD, NEW_LOG_FILE.c_str(), "\n");
  //Serial.println();
}

void steering(int steering_angle) {

  // TODO : Plausibility check
  int out = constrain(map(steering_angle,-255,255,full_left, full_right),full_left, full_right);
  int mid = (full_left + full_right)/2;
  int inc = 50;
  inc = min(inc, abs(last_servo_pos-out));
  if (last_servo_pos<out) {
    for (int i=0; i<inc; i++) {
      steering_servo.write(last_servo_pos + i);  
      delay(5);
    }
    last_servo_pos += inc;
  } else if (last_servo_pos>out) {
    for (int i=0; i<inc; i++) {
      steering_servo.write(last_servo_pos - i); 
      delay(5);  
    }
    last_servo_pos -= inc;
  }
}

void throttle(int th, int brk) {
  //return;
  motorController.Enable();  

  if (brk) {
    motorController.Stop();  
    return;
  }
  
  int out = abs(th);
  if ((CURRENT_DRAW_L-CURRENT_LIMIT)>-300 || (CURRENT_DRAW_R-CURRENT_LIMIT)>-300) {
    //th = 0;
    //th = th * 0.1;
    
    //if (th != 0) th = 10;
    Serial.println("Overcurrent");
    //motorController.Disable();
  } else {
    
  }

  
  if (th>0) {
    motorController.TurnLeft(out);
  } else if (th<0) {
    //motorController.Stop();
    motorController.TurnRight(out);
  } else {
    //motorController.Stop();
    //motorController.Disable();
  }
  // TODO : Plausibility check
  //constrain(map(abs(steering_angle),0,100,0,255),0,255)
  
  //Serial.println("TH - " + String(th) + "; PWM - " + String(out));
}

void throttle(int th) {
  throttle(th, false);
}

void emergency_stop() {
  digitalWrite(SHUTDOWN_RELAY, LOW);
  play_tone(stop_tone, BUZZER_PIN);
}


void startup_TS() {
  digitalWrite(SHUTDOWN_RELAY, HIGH);
  play_tone(start_tone, BUZZER_PIN);
}

void ESP_BT_Commands_OLD(void *pvParameters) {
  int last_recieved_command = 0;  
  (void) pvParameters;

  while (true) {
    if (!ESP_BT.available())
      continue;
    char incoming = ESP_BT.read(); //Read what we recevive
    if (incoming!=';')
      continue;
    if (!ESP_BT.available())
      continue;
    
    String command = "";
    
    incoming = ESP_BT.read(); //Read what we recevive
    while (ESP_BT.available() && incoming!=';') //Check if we receive anything from Bluetooth
    {
      command += incoming;
      incoming = ESP_BT.read(); //Read what we recevive
    }
    last_recieved_command = millis();
    xQueueSend(queue, &last_recieved_command, portMAX_DELAY);
    if (command != "") {
      //ESP_BT.println("Got : " + command);
      if (command == "start") {
          ESP_BT.println("TS start");
          Serial.println("TS start");
          startup_TS();
      } else if (command == "stop") {
          ESP_BT.println("TS stop");
          Serial.println("TS stop");
          emergency_stop();
      } else if(command.indexOf("CURRENT_LIMIT")>=0 && command.length()==18) {
        // Format CURRENT_LIMIT 2000;
        CURRENT_LIMIT = (float) command.substring(14,18).toInt();
        log_current_frame_serial();
      } else if (command.indexOf("actuate")>=0 && command.length()==17) {
        // Format >>actuate +050 -000;
        // Format >>actuate +THR -STR;
        throttle_val = command.substring(9,12).toInt() * ((command[8]=='-')? -1 : +1);
        steering_val = command.substring(14,17).toInt() * ((command[13]=='-')? -1 : +1);
        throttle(throttle_val);
        steering(steering_val);
        
        //ESP_BT.println(String(THR) + ":" + String(STR));
        //Serial.println(String(throttle_val) + ":" + String(steering_val));
        //log_current_frame_serial();
      } else if (command == "log_enable") {
        PRINT_LOGS = true;
      } else if (command == "log_disable") {
        PRINT_LOGS = false;
      } else if (command == "accel") {
        // TODO : Async ESP_BT print
        ESP_BT.print("a/g:\t");
          ESP_BT.print(ax); ESP_BT.print("\t");
          ESP_BT.print(ay); ESP_BT.print("\t");
          ESP_BT.print(az); ESP_BT.print("\t");
          ESP_BT.print(gx); ESP_BT.print("\t");
          ESP_BT.print(gy); ESP_BT.print("\t");
          ESP_BT.println(gz);
      } else {
        ESP_BT.println("WARN unknown_command ");
        Serial.println("WARN unknown_command " + command);
      }
    }
  }
}

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

static void printInt(unsigned long val, bool valid, int len)
{
  char sz[32] = "*****************";
  if (valid)
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  Serial.print(sz);
  smartDelay(0);
}

static void printDateTime(TinyGPSDate &d, TinyGPSTime &t)
{
  if (!d.isValid())
  {
    Serial.print(F("********** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d ", d.month(), d.day(), d.year());
    Serial.print(sz);
  }
  
  if (!t.isValid())
  {
    Serial.print(F("******** "));
  }
  else
  {
    char sz[32];
    sprintf(sz, "%02d:%02d:%02d ", t.hour(), t.minute(), t.second());
    Serial.print(sz);
  }

  printInt(d.age(), d.isValid(), 5);
  smartDelay(0);
}

void test_steering() {
  for (pos = full_left; pos <= full_right; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    steering_servo.write(pos);    // tell servo to go to position in variable 'pos'
    delay(5);             // waits 15ms for the servo to reach the position
  }
  for (pos = full_right; pos >= full_left; pos -= 1) { // goes from 180 degrees to 0 degrees
    steering_servo.write(pos);    // tell servo to go to position in variable 'pos'
    delay(5);             // waits 15ms for the servo to reach the position
  }
  for (pos = full_left; pos <= 90; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    steering_servo.write(pos);    // tell servo to go to position in variable 'pos'
    delay(5);             // waits 15ms for the servo to reach the position
  }  
}
