#include "settings.h"

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

//All pin definitions and settings are in "settings.h"

#if HT_MODE == 1
#define DHTPIN 25    
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);
#endif

//BMP180
#if ENABLE_BMP180 == true
BMP180 barometer;
#endif

//SD
bool SD_flag = false;

//Wind speed
volatile int wind_speed_counter;
boolean wind_flag = false, wind_temp;

//Rain guage
volatile int rain_counter;
boolean rain_flag = false, rain_temp;

//Voltage
float R1 = 100000.0;
float R2 = 10000.0;
int mVperAmp = 185; // use 100 for 20A Module and 66 for 30A Module
int ACSoffset = 2500; 

//Timer interrupt
uint8_t timer1_counter = 0;
volatile uint8_t GPS_wait;
volatile boolean four_sec = false, read_flag = false, upload_flag = false;

//Weather data
weatherData w;
wtime current_time;

uint8_t reading_number = 0, number_of_fail_uploads = 0;
boolean startup = true;

OneWire oneWire(METAL_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

void setup(){
  pinMode(RAIN_LED, OUTPUT);
  pinMode(WIND_LED, OUTPUT);
  pinMode(UPLOAD_LED, OUTPUT);
  pinMode(53, OUTPUT);//SD card's CS pin
  pinMode(RAIN_PIN, INPUT);
  pinMode(WIND_PIN , INPUT);
  pinMode(GSM_DTR_PIN, OUTPUT);
  digitalWrite(GSM_DTR_PIN, LOW);
  pinMode(GSM_PWRKEY_PIN, OUTPUT);
  digitalWrite(GSM_PWRKEY_PIN, LOW);

  Serial.begin(9600); 
  Serial1.begin(19200);
  pinMode(19, INPUT);  
  digitalWrite(19, HIGH);
  Serial2.begin(9600);
  pinMode(17, INPUT);  
  digitalWrite(17, HIGH);
  //Serial3.begin(9600);
  
  uint8_t i = 0;
  SD_flag = sd.begin(53);
  delay(100);
  if(!sd.exists("id.txt")){
    #if SERIAL_OUTPUT
    Serial.println("No id file found on sd card, using bakup id");
    #endif
    BACKUP_ID.toCharArray(w.id, 5);
  }
  else{
    datalog.open("id.txt", FILE_READ);
    while(datalog.available()){
      w.id[i++] = datalog.read();
    }
    w.id[i] = '\0';
    datalog.close();
  }

  //Serial.println("OTA WOKRED!!!\n\n");
  
  //Wire.begin();
  dht.begin();
  sensors.begin();
  
  #if enable_BMP180
  barometer = BMP180();
  barometer.SoftReset();
  barometer.Initialize();
  #endif
  
  #if SERIAL_OUTPUT
  Serial.println("Sensor id: " + String(w.id));
  Serial.println("Data upload frequency: " + (String)DATA_UPLOAD_FREQUENCY + " minutes");
  Serial.println("Data read frequency: " + (String)DATA_READ_FREQUENCY + " minutes");
  if(HT_MODE == 0) Serial.println("Using SHT21"); else if(HT_MODE == 1) Serial.println("Using DHT22"); else Serial.println("No HT sensor");
  if(ENABLE_BMP180) Serial.println("BMP180 enabled"); else Serial.println("BMP180 disabled");
  if(ENABLE_GPS) Serial.println("GPS enabled"); else Serial.println("GPS disabled");
  Serial.println();
  #endif

  
  
  //Talk();

  delay(2000);  //Wait for the GSM module to boot up

  /*
  digitalWrite(UPLOAD_LED, HIGH);
  delay(10000);//Wait for the SIM to log on to the network
  DigitalWrite(UPLOAD_LED, LOW);
  */

  //Initialize GSM module
  InitGSM();  
  
  //Interrupt initialization
  InitInterrupt(); //Timer1: 0.25hz, Timer2: 8Khz
  #if SERIAL_OUTPUT == true
  Serial.println("\nSeconds till next upload:");
  #endif
}
 
void loop() {
  int i = 0, j, avg_counter = 0;
  boolean temp_read, temp_upload, temp_network;
  
  rain_counter = 0;
  wind_speed_counter = 0;
  reading_number = 0;

  WeatherDataReset(w);
  TimeDataReset(current_time);
  
  while(1) {
    CheckOTA();
    
    if(four_sec){
      four_sec = false;
      temp_read = read_flag;
      temp_upload = upload_flag;
  
      #if SERIAL_OUTPUT
      if(timer1_counter) Serial.println(((DATA_UPLOAD_FREQUENCY * 15) - timer1_counter + 1) * 4);
      else Serial.println("4");
      #endif

      //SHT
      #if HT_MODE == 0
      w.hum = (w.hum * float(avg_counter) + SHT2x.GetHumidity()) / (float(avg_counter) + 1.0);
      //w.temp1 = (w.temp1 * float(avg_counter) + SHT2x.GetTemperature()) / (float(avg_counter) + 1.0);
      #endif
      //DHT
      #if HT_MODE == 1
      w.hum = (w.hum * float(avg_counter) + dht.readHumidity()) / (float(avg_counter) + 1.0);
      w.temp1 = (w.temp1 * float(avg_counter) + dht.readTemperature()) / (float(avg_counter) + 1.0);
      #endif
      #if ENABLE_BMP180 == true
      w.pressure = (w.pressure * float(avg_counter) + barometer.GetPressure()) / (float(avg_counter) + 1.0);
      //w.temp2 = (w.temp2 * avg_counter + barometer.GetTemperature()) / (float(avg_counter) + 1.0);
      #endif
      //w.solar_radiation = (w.solar_radiation * float(avg_counter) + float(analogRead(solar_radiation_pin))) / (float(avg_counter) + 1.0);
      
      w.battery_voltage = (w.battery_voltage * float(avg_counter) + (analogRead(BATTERY_PIN) * 5.0 / 1024.0 * (R1 + R2) / R2)) / (float(avg_counter) + 1.0);
      
      w.amps = (w.amps * float(avg_counter) + (((analogRead(CURRENT_SENSOR_PIN) / 1023.0 * 5000.0) - ACSoffset) / mVperAmp)) / (float(avg_counter) + 1.0);

      w.voltage = (w.voltage * float(avg_counter) + (analogRead(CHARGE_PIN) * 5.0 / 1024.0 * ((R1 + R2) / R2))) / (float(avg_counter) + 1.0);
      sensors.requestTemperatures();
      w.temp2 = (w.temp2 * avg_counter + sensors.getTempCByIndex(0)) / (float(avg_counter) + 1.0);
      //Serial.println(String(w.voltage) + ", " + String(w.battery_voltage) + ", " + String(w.temp2) + ", " + String(w.amps));
      
      avg_counter++;

      WeatherCheckIsNan(w);
  
      if(temp_read){//Read data
        read_flag = false;
        digitalWrite(UPLOAD_LED, HIGH);
        
        //Read data
        avg_counter = 0;
        
        w.wind_speed = ((wind_speed_counter) / DATA_UPLOAD_FREQUENCY);
        wind_speed_counter = 0;
        
        w.rain = rain_counter;
        rain_counter = 0;
        
        #if enable_GPS == true
        //GetGPS();
        #endif

        #if SERIAL_OUTPUT
        PrintWeatherData(w);
        #endif
        
        reading_number++;
        
        digitalWrite(UPLOAD_LED, LOW);
      }
      if(temp_upload){//Upload data
        digitalWrite(UPLOAD_LED, HIGH);

        GSMModuleWake();
        w.signal_strength = GetSignalStrength();
        #if SERIAL_OUTPUT
        Serial.println("Signal strength: " + String(w.signal_strength));
        #endif
        
        GSMModuleWake();
        if(!SubmitHttpRequest(w, current_time)) {          
          number_of_fail_uploads++;
          reading_number = 0;
          #if SERIAL_OUTPUT
          Serial.println("\nUpload failed - sending sms");
          #endif
          delay(100);
          //upload_sms(w, phone_number);
        }
        else{
          startup = false;
          reading_number = 0;
          number_of_fail_uploads = 0;
          #if SERIAL_OUTPUT
          Serial.println("\nUpload successful");
          #endif
        }
        delay(2000);
        
        #if SERIAL_OUTPUT
        Serial.println("\nWriting to SD card");
        #endif
        WriteSD(w, current_time);
        
        digitalWrite(UPLOAD_LED, LOW);
        upload_flag = false;
        #if SERIAL_OUTPUT
        if(current_time.flag) PrintTime(current_time);
        Serial.println("Seconds till next upload:");
        #endif
      }
    }
  }
}

void CheckOTA(){
  char body_r[40], number[14];
  if(CheckSMS()) { //Check sms for OTA
    GetSMS(number, body_r);
	  if(toupper(body_r[0]) == 'O' &&  toupper(body_r[1]) == 'T' &&toupper(body_r[2]) == 'A' && sd.begin(53)) {
  	  SendSMS(number, "Downloading new firmware");
  	  #if SERIAL_OUTPUT
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
  	    #if SERIAL_OUTPUT
  	    Serial.println("\nNew firmware downloaded");
  	    Serial.println("Converting .hex file to .bin");
  	    #endif
  	    if(SDHexToBin()){
  	      #if SERIAL_OUTPUT
  	      Serial.println("Done\nRestarting and re-programming");
  	      #endif
  	      while(Serial1.available()) Serial1.read();
  	      EEPROM.write(0x1FF,0xF0);
  	      wdt_enable(WDTO_500MS);
  	      wdt_reset();
  	      delay(600);
  	    }
  	    else{
  	      #if SERIAL_OUTPUT
  	      Serial.println("SD card copy error- hex file checksum failed");
  	      #endif
  	    }
  	  }
  	  else{
  	    #if SERIAL_OUTPUT
  	    Serial.println("Download failed");
  	    #endif
  	  }
  	}
  }
}

//Interrupt gets called every 4 seconds
ISR(TIMER1_COMPA_vect){ 
  #if enable_GPS
  if(GPS_wait < 2) GPS_wait++;
  #endif
  timer1_counter++;
  if(timer1_counter == DATA_UPLOAD_FREQUENCY * 15){ 
    timer1_counter = 0;
    upload_flag = true;
  }
  if(timer1_counter % (DATA_READ_FREQUENCY * 15) == 0) read_flag = true;
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
  rain_temp = digitalRead(RAIN_PIN);
  if(rain_temp == 1 && rain_flag == true) {
    digitalWrite(RAIN_LED, HIGH);
    rain_counter++;
    rain_flag = false;
  }
  else if(rain_temp == 0) {
    rain_flag = true;
    digitalWrite(RAIN_LED, LOW);
  }
  wind_temp = digitalRead(WIND_PIN);
  if(wind_temp == 0 && wind_flag == true) {
    wind_speed_counter++;
    wind_flag = false;
    digitalWrite(WIND_LED, HIGH);
  } 
  else if(wind_temp == 1) {
    wind_flag = true;
    digitalWrite(WIND_LED, LOW);
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

