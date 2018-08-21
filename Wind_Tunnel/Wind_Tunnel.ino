#include "settings.h"

#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <String.h>
#include <Wire.h>
//#include <BMP180.h>
#include <DHT.h>
#include <SHT2x.h>
#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#include "http.h"
#include "weatherData.h"
#include "GSM.h"
#include "SD.h"

//All pin definitions and settings are in "settings.h/cpp"

//Times in seconds
#define DATALOG_TIME 20
#define STARTUP_WAIT 10
#define OFF_TIME 2
#define ZERO_SPEED_TIME 3

#define RELAY_HIGH_PIN 25
#define RELAY_LOW_PIN A3
#define RELAY_MID_PIN 26

const float radius = 63.7 / 1000.0;
const float scaler = 2.5 * 1.165;

//Wind speed
volatile int wind_speed_counter;
double wind_speed;
bool wind_flag = false, wind_temp;

//Wind speed
volatile int rain_speed_counter;
double rain_speed;
bool rain_flag = false, rain_temp;

//Timer interrupt
uint8_t timer1_counter = 0;
volatile bool four_sec = false, read_flag = false, upload_flag = false;

//Weather data
int ws_id = 2001;

bool startup = true;

OneWire oneWire(METAL_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

void setup() {  
  unsigned int i;

  pinMode(RAIN_LED, OUTPUT);
  pinMode(WIND_LED, OUTPUT);
  pinMode(UPLOAD_LED, OUTPUT);
  pinMode(SD_CARD_CS_PIN, OUTPUT);
  pinMode(RAIN_PIN, INPUT);
  pinMode(WIND_PIN , INPUT);
  pinMode(GSM_DTR_PIN, OUTPUT);
  digitalWrite(GSM_DTR_PIN, LOW);
  #ifdef GSM_PWRKEY_PIN
    pinMode(GSM_PWRKEY_PIN, OUTPUT);
    digitalWrite(GSM_PWRKEY_PIN, LOW);
  #endif

  pinMode(RELAY_LOW_PIN, OUTPUT);
  digitalWrite(RELAY_LOW_PIN, LOW);
  pinMode(RELAY_MID_PIN , OUTPUT);
  digitalWrite(RELAY_MID_PIN, LOW);
  pinMode(RELAY_HIGH_PIN, OUTPUT);
  digitalWrite(RELAY_HIGH_PIN, LOW);

  Serial.begin(115200); 
  Serial1.begin(19200);
  pinMode(19, INPUT);  
  digitalWrite(19, HIGH);
  Serial2.begin(9600);
  pinMode(17, INPUT);  
  digitalWrite(17, HIGH);
  //Serial3.begin(9600);

  Serial.println("Wind Tunnel");
  Serial.println("ID: " + String(ws_id));

  digitalWrite(UPLOAD_LED, HIGH);
  delay(2000);  //Wait for the GSM module to boot up
  digitalWrite(UPLOAD_LED, LOW);

  delay(6000);
  //Talk();
  //Initialize GSM module
  InitGSM();  

  //Interrupt initialization  
  InitInterrupt();  //Timer1: 0.25hz, Timer2: 8Khz
}
 
void loop() {
  int i, j;
  bool temp_read, temp_upload, temp_network;
  double wind_speed_low, wind_speed_mid, wind_speed_high;
  double rain_speed_low, rain_speed_mid, rain_speed_high;

  unsigned long timeout, temp_time = 0, cutoff_time;

  weatherData w;
  
  wind_speed_counter = 0;
  
  while(Serial1.available()) Serial1.read();
  
  while(1) {
    Serial.println("Waiting for the anemometer to stop");
    while(1) {
      wind_speed_counter = 0;
      for(timeout = millis(); wind_speed_counter == 0 && (millis() - timeout) < (ZERO_SPEED_TIME * 1000);) {
        delay(100);
      }
      cutoff_time = millis() - temp_time;
      
      //Serial.println(wind_speed_counter);
      if((millis() - timeout) >= (ZERO_SPEED_TIME * 1000))
        break;
    }
    
    Serial.println("Low");
    digitalWrite(RELAY_LOW_PIN, HIGH);
    delay(3 * STARTUP_WAIT * 1000);
    wind_speed_counter = 0;
    rain_speed_counter = 0;
    delay(DATALOG_TIME * 1000);
    wind_speed_low = wind_speed_counter * radius * 6.2831 / float(DATALOG_TIME) * scaler;
    rain_speed_low = rain_speed_counter / float(DATALOG_TIME);

    digitalWrite(RELAY_LOW_PIN, LOW);
    Serial.println("Off");
    delay(OFF_TIME * 1000);

    Serial.println("Mid");
    digitalWrite(RELAY_MID_PIN, HIGH);
    delay(STARTUP_WAIT * 1000);
    wind_speed_counter = 0;
    rain_speed_counter = 0;
    delay(DATALOG_TIME * 1000);
    wind_speed_mid = wind_speed_counter * radius * 6.2831 / float(DATALOG_TIME) * scaler;
    rain_speed_mid = rain_speed_counter / float(DATALOG_TIME);

    digitalWrite(RELAY_MID_PIN, LOW);
    Serial.println("Off");
    delay(OFF_TIME * 1000);

    Serial.println("High");
    digitalWrite(RELAY_HIGH_PIN, HIGH);
    delay(STARTUP_WAIT * 1000);
    wind_speed_counter = 0;
    rain_speed_counter = 0;
    delay(DATALOG_TIME * 1000);
    wind_speed_high = wind_speed_counter * radius * 6.2831 / float(DATALOG_TIME) * scaler;
    rain_speed_high = rain_speed_counter / float(DATALOG_TIME);

    digitalWrite(RELAY_HIGH_PIN, LOW);
    temp_time = millis();
    
    Serial.println("Off");
    delay(OFF_TIME * 1000);

    digitalWrite(UPLOAD_LED, HIGH);

    Serial.println("Low Speed\tMid Speed\tHighSpeed\tCutoff Time");
    Serial.println(String(wind_speed_low) + "\t\t" + String(wind_speed_mid) + "\t\t" + String(wind_speed_high) + "\t\t" + String(float(cutoff_time) / 1000.0));
    Serial.println(String(rain_speed_low) + "\t\t" + String(rain_speed_mid) + "\t\t" + String(rain_speed_high));

    w.Reset(ws_id);
    w.t.flag = 0;
    w.temp1 = wind_speed_low;
    w.temp2 = wind_speed_mid;
    w.hum = wind_speed_high;
    if(startup == false) {
      w.wind_speed = float(cutoff_time) / 1000.0;
    }

    w.panel_voltage = rain_speed_low;
    w.battery_voltage = rain_speed_mid;
    w.amps = rain_speed_high;

    HttpInit();
    SendWeatherURL(w);
    ShowSerialData();  
    GSMModuleWake();
    if(SendATCommand("AT+QHTTPREAD=30", "OK", 1000) == -1) {
      GSMReadUntil("\n", 50); 
      return false;
    }
    GSMReadUntil("\n", 100); 

    digitalWrite(UPLOAD_LED, LOW);

    startup = false;
  }
}

//Interrupt gets called every 4 seconds
ISR(TIMER1_COMPA_vect) { 
  timer1_counter++;
  if(timer1_counter == DATA_UPLOAD_FREQUENCY * 15) { 
    timer1_counter = 0;
    upload_flag = true;
  }
  if(timer1_counter % (DATA_READ_FREQUENCY * 15) == 0) read_flag = true;
  four_sec = true;
}

//Interrupt gets called every 125 micro seconds
ISR(TIMER2_COMPA_vect) {
  //wind_temp = digitalRead(WIND_PIN);
  wind_temp = (PINF >> 1) & 0x01;
  if(wind_temp == 0 && wind_flag == true) {
    wind_speed_counter++;
    wind_flag = false;
    //digitalWrite(WIND_LED, HIGH);
    PORTG = PORTG | 0b00000001;
  } 
  else if(wind_temp == 1) {
    wind_flag = true;
    //digitalWrite(WIND_LED, LOW);
    PORTG = PORTG & 0b11111110;
  } 

  //rain_temp = digitalRead(RAIN_PIN);
  rain_temp = (PING >> 5) & 0x01;
  if(rain_temp == 0 && rain_flag == true) {
    rain_speed_counter++;
    rain_flag = false;
    //digitalWrite(RAIN_LED, HIGH);
    PORTC = PORTC | 0b00000100;
  } 
  else if(rain_temp == 1) {
    rain_flag = true;
    //digitalWrite(RAIN_LED, LOW);
    PORTC = PORTC & 0b11111011;
  } 
}

void InitInterrupt() {
  noInterrupts();//stop interrupts
  TCCR1A = 0; 
  TCCR1B = 0; 
  TCNT1  = 0; 
  OCR1A = 62500; 
  TCCR1B |= (1 << WGM12); 
  TCCR1B |= 5;
  TCCR1B &= 253; 
  TIMSK1 |= (1 << OCIE1A); 
  TCCR2A = 0; 
  TCCR2B = 0; 
  TCNT2  = 0;
  OCR2A = 249; 
  TCCR2A |= (1 << WGM21); 
  TCCR2B |= (1 << CS21); 
  TIMSK2 |= (1 << OCIE2A);
  interrupts();
}

