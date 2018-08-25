#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <String.h>
#include <Wire.h>
//#include <BMP180.h>
#include <DHT.h>
#include <SHT2x.h>
#include <SPI.h>
#include <avr/wdt.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#include "http.h"
#include "weatherData.h"
#include "GSM.h"
#include "SD.h"
#include "settings.h"

//All pin definitions and settings are in "settings.h"

#if HT_mode == 1
#define DHTPIN 25    
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);
#endif

//BMP180
#if enable_BMP180 == true
BMP180 barometer;
#endif

//SD
bool SD_flag = false;

//Wind speed
unsigned long wind_speed_counter;
boolean wind_flag = false, wind_temp;

//Rain guage
volatile int rain1_counter, rain2_counter, rain3_counter, rain4_counter, rain5_counter, rain6_counter, rain7_counter, rain8_counter;
boolean rain1_flag = true, rain1_temp;
boolean rain2_flag = true, rain2_temp;
boolean rain3_flag = true, rain3_temp;
boolean rain4_flag = true, rain4_temp;
boolean rain5_flag = true, rain5_temp;
boolean rain6_flag = true, rain6_temp;
boolean rain7_flag = true, rain7_temp;
boolean rain8_flag = true, rain8_temp;

//Power
const int analogIn = A15;
int mVperAmp = 185; // use 100 for 20A Module and 66 for 30A Module
int ACSoffset = 2500; 
double Voltage = 0;
double Amps = 0;

//
unsigned long StartTime   = millis();
unsigned long CurrentTime = millis();
unsigned long ElapsedTime = millis();

//Voltage
float vout = 0.0;
float vin = 0.0;
float R1 = 100000.0; // resistance of R1 (100K) -see text!
float R2 = 10000.0; // resistance of R2 (10K) - see text!
int voltageValue = 0;

//Battery Voltage
float bvout = 0.0;
float bvin = 0.0;
int bvoltageValue = 0;

//Timer interrupt
uint8_t timer1_counter = 0;
volatile uint8_t GPS_wait;
volatile boolean four_sec = false, read_flag = false, upload_flag = false;

//Weather data
weatherData w;
wtime current_time;

uint8_t reading_number = 0, number_of_fail_uploads = 0;
boolean startup = true;


void setup(){
  pinMode(rain_led, OUTPUT);
  pinMode(wind_led, OUTPUT);
  pinMode(upload_led, OUTPUT);
  pinMode(53, OUTPUT);//SD card's CS pin
  pinMode(rain1_pin, INPUT);
  pinMode(rain2_pin, INPUT);
  pinMode(rain3_pin, INPUT);
  pinMode(rain4_pin, INPUT);
  pinMode(rain5_pin, INPUT);
  pinMode(rain6_pin, INPUT);
  pinMode(rain7_pin, INPUT);
  pinMode(rain8_pin, INPUT);
  pinMode(wind_pin , INPUT);
  pinMode(GSM_module_DTR_pin, OUTPUT);
  digitalWrite(GSM_module_DTR_pin, LOW);

  digitalWrite(rain1_pin, HIGH);
  digitalWrite(rain2_pin, HIGH);
  digitalWrite(rain3_pin, HIGH);
  digitalWrite(rain4_pin, HIGH);
  digitalWrite(rain5_pin, HIGH);
  digitalWrite(rain6_pin, HIGH);
  digitalWrite(rain7_pin, HIGH);
  digitalWrite(rain8_pin, HIGH);

  Serial.begin(115200); 
  Serial1.begin(19200);
  pinMode(19, INPUT);  
  digitalWrite(19, HIGH);
  Serial2.begin(9600);
  pinMode(17, INPUT);  
  digitalWrite(17, HIGH);
  //Serial3.begin(9600);
  
  uint8_t i = 0;
  backup_id.toCharArray(w.id, 5);
  
  //Wire.begin();
  dht.begin();
  
  #if enable_BMP180
  barometer = BMP180();
  barometer.SoftReset();
  barometer.Initialize();
  #endif
  
  #if serial_output
  Serial.println("Sensor id: " + String(w.id));
  Serial.println("Data upload frequency: " + (String)data_upload_frequency + " minutes");
  Serial.println("Data read frequency: " + (String)data_read_frequency + " minutes");
  if(HT_mode == 0) Serial.println("Using SHT21"); else if(HT_mode == 1) Serial.println("Using9 DHT22"); else Serial.println("No HT sensor");
  if(enable_BMP180) Serial.println("BMP180 enabled"); else Serial.println("BMP180 disabled");
  if(enable_GPS) Serial.println("GPS enabled"); else Serial.println("GPS disabled");
  Serial.println();
  #endif

  //Talk();

  //GSMModuleReset();
  digitalWrite(upload_led, HIGH);
  delay(5000);//Wait for the SIM to log on to the network
  digitalWrite(upload_led, LOW);

  //Initialize GSM module
  InitGSM();  
  
  //Interrupt initialization
  noInterrupts();//stop interrupts
  TCCR1A = 0; TCCR1B = 0; TCNT1  = 0; OCR1A = 62500; TCCR1B |= (1 << WGM12); TCCR1B |= 5;
  TCCR1B &= 253; TIMSK1 |= (1 << OCIE1A); TCCR2A = 0; TCCR2B = 0; TCNT2  = 0;
  OCR2A = 249; TCCR2A |= (1 << WGM21); TCCR2B |= (1 << CS21); TIMSK2 |= (1 << OCIE2A);
  
  interrupts();//Timer1: 0.25hz, Timer2: 8Khz
}
 
void loop(){
  int i = 0, j, avg_counter = 0;
  boolean temp_read, temp_upload, temp_network;
  
  rain1_counter = 0;
  rain2_counter = 0;
  rain3_counter = 0;
  rain4_counter = 0;
  rain5_counter = 0;
  rain6_counter = 0;
  rain7_counter = 0;
  rain8_counter = 0;
  wind_speed_counter = 0;
  reading_number = 0;

  WeatherDataReset(w);
  TimeDataReset(current_time);  
  
  while(1){
    
    if(four_sec){
      four_sec = false;
  
      #if serial_output
        Serial.print(String(rain1_counter) + '\t');
        Serial.print(String(rain2_counter) + '\t');
        Serial.print(String(rain3_counter) + '\t');
        Serial.print(String(rain4_counter) + '\t');
        Serial.print(String(rain5_counter) + '\t');
        Serial.print(String(rain6_counter) + '\t');
        Serial.print(String(rain7_counter) + '\t');
        Serial.println(String(rain8_counter) + '\t');
      #endif

      WeatherCheckIsNan(w);
      
      if(digitalRead(wind_pin) == 0 && temp_upload){//Upload data
        temp_upload = false;
        for(i = 0; i < 300; i++) {
          if(digitalRead(wind_pin) != 0) {
            temp_upload == true;
            break;
          }
          delay(10);
        }
        if(temp_upload != true) {
          digitalWrite(upload_led, HIGH);
          w.temp1           = rain1_counter;
          w.temp2           = rain2_counter;
          w.hum             = rain3_counter;
          w.rain            = rain4_counter;
          w.wind_speed      = rain5_counter;
          w.solar_radiation = rain6_counter;
          w.voltage         = rain7_counter;
          w.battery_voltage = rain8_counter;
  
          rain1_counter = 0;
          rain2_counter = 0;
          rain3_counter = 0;
          rain4_counter = 0;
          rain5_counter = 0;
          rain6_counter = 0;
          rain7_counter = 0;
          rain8_counter = 0;
  
          GSMModuleWake();
          w.signal_strength = GetSignalStrength();
          #if serial_output
            Serial.println("Signal strength: " + String(w.signal_strength));
          #endif
          
          GSMModuleWake();
          if(!SubmitHttpRequest(w, current_time)) {          
            number_of_fail_uploads++;
            reading_number = 0;
            #if serial_output
              Serial.println("\nUpload failed");
            #endif
          }
          else{
            startup = false;
            reading_number = 0;
            number_of_fail_uploads = 0;
            #if serial_output
              Serial.println("\nUpload successful");
            #endif
          }
          delay(2000);
  
          temp_upload = 1;
          
          digitalWrite(upload_led, LOW);

          while(digitalRead(wind_pin) == 0) {
            delay(10);
          }
        }
      }
    }
  }
}

void CheckOTA(){
//  char body_r[40], number[14];
//  if(CheckSMS()) { //Check sms for OTA
//    GetSMS(number, body_r);
//    if(toupper(body_r[0]) == 'O' &&  toupper(body_r[1]) == 'T' &&toupper(body_r[2]) == 'A' && sd.begin(53)) {
//      SendSMS(number, "Downloading new firmware");
      #if serial_output
      Serial.println("Updating firmware");
      #endif
      delay(500);
      sd.chdir();
      delay(100);
      if(!sd.exists("OtaTemp")){
        if(!sd.mkdir("OtaTemp")) return false;
      }

      if(sd.exists("OtaTemp/TEMP_OTA.HEX")) sd.remove("OtaTemp/TEMP_OTA.HEX");
      if(sd.exists("firmware.BIN")) sd.remove("firmware.BIN");
      delay(500);
      if(DownloadHex()){
        #if serial_output
        Serial.println("\nNew firmware downloaded");
        Serial.println("Converting .hex file to .bin");
        #endif
        if(SDHexToBin()){
          #if serial_output
          Serial.println("Done\nRestarting and re-programming");
          #endif
          while(Serial1.available()) Serial1.read();
          EEPROM.write(0x1FF,0xF0);
          wdt_enable(WDTO_500MS);
          wdt_reset();
          delay(600);
        }
        else{
          #if serial_output
          Serial.println("SD card copy error- hex file checksum failed");
          #endif
        }
      }
      else{
        #if serial_output
        Serial.println("Download failed");
        #endif
      }
//    }
//  }
}

//Interrupt gets called every 4 seconds
ISR(TIMER1_COMPA_vect){ 
  #if enable_GPS
  if(GPS_wait < 2) GPS_wait++;
  #endif
  timer1_counter++;
  if(timer1_counter % (data_read_frequency * 15) == 0) read_flag = true;
  four_sec = true;

  //Increment time by 4 seconds
  //And handle overflow
  current_time.seconds += 4;
  if(current_time.seconds >= 60) { current_time.seconds = 0; current_time.minutes++;
    if(current_time.minutes >= 60) { current_time.minutes = 0; current_time.hours++;
      if(current_time.hours >= 24) { current_time.hours = 0; current_time.day++;
        if((current_time.month == 1 || current_time.month == 3 || current_time.month == 5 || current_time.month == 7 || current_time.month == 8 || current_time.month == 10 || current_time.month == 12) && current_time.day > 31) { current_time.day = 1; current_time.month++; }
        else if((current_time.month == 4 || current_time.month == 6 || current_time.month == 9 || current_time.month == 11) && current_time.day > 30){ current_time.day = 1; current_time.month++; }
        else if(current_time.month == 2 && current_time.day > 28) { current_time.day = 1; current_time.month++; }
        if(current_time.month > 12) {  current_time.month = 1; current_time.year++; } 
      } 
    } 
  }
}

//Interrupt gets called every 125 micro seconds
ISR(TIMER2_COMPA_vect){
    
  rain1_temp = digitalRead(rain1_pin);
  if (rain1_temp == 1 && rain1_flag == true) {
    rain1_counter++;
    rain1_flag = false;
  }
  else if (rain1_temp == 0) {
    rain1_flag = true;
  }

  rain2_temp = digitalRead(rain1_pin);
  if (rain2_temp == 1 && rain2_flag == true) {
    rain2_counter++;
    rain2_flag = false;
  }
  else if (rain2_temp == 0) {
    rain2_flag = true;
  }

  rain3_temp = digitalRead(rain3_pin);
  if (rain3_temp == 1 && rain3_flag == true) {
    rain3_counter++;
    rain3_flag = false;
  }
  else if (rain3_temp == 0) {
    rain3_flag = true;
  }

  rain4_temp = digitalRead(rain4_pin);
  if (rain4_temp == 1 && rain4_flag == true) {
    rain4_counter++;
    rain4_flag = false;
  }
  else if (rain4_temp == 0) {
    rain4_flag = true;
  }

  rain5_temp = digitalRead(rain5_pin);
  if (rain5_temp == 1 && rain5_flag == true) {
    rain5_counter++;
    rain5_flag = false;
  }
  else if (rain5_temp == 0) {
    rain5_flag = true;
  }

  rain6_temp = digitalRead(rain6_pin);
  if (rain6_temp == 1 && rain6_flag == true) {
    rain6_counter++;
    rain6_flag = false;
  }
  else if (rain6_temp == 0) {
    rain6_flag = true;
  }

  rain7_temp = digitalRead(rain7_pin);
  if (rain7_temp == 1 && rain7_flag == true) {
    rain7_counter++;
    rain7_flag = false;
  }
  else if (rain7_temp == 0) {
    rain7_flag = true;
  }

  rain8_temp = digitalRead(rain8_pin);
  if (rain8_temp == 1 && rain8_flag == true) {
    rain8_counter++;
    rain8_flag = false;
  }
  else if (rain8_temp == 0) {
    rain8_flag = true;
  } 
}
