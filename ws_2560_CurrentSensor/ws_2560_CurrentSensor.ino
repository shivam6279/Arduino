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
volatile int wind_speed_counter;
boolean wind_flag = false, wind_temp;

//Rain guage
volatile int rain_counter;
boolean rain_flag = false, rain_temp;

//Power
const int analogIn = A15;
int mVperAmp = 185; // use 100 for 20A Module and 66 for 30A Module
int ACSoffset = 2500; 
double Voltage = 0;
double Amps = 0;

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

OneWire oneWire(metal_sensor_pin);
DallasTemperature sensors(&oneWire);

void setup(){
  pinMode(rain_led, OUTPUT);
  pinMode(wind_led, OUTPUT);
  pinMode(upload_led, OUTPUT);
  pinMode(53, OUTPUT);//SD card's CS pin
  pinMode(rain_pin, INPUT);
  pinMode(wind_pin , INPUT);
  pinMode(GSM_module_DTR_pin, OUTPUT);
  digitalWrite(GSM_module_DTR_pin, LOW);

  Serial.begin(115200); 
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
    #if serial_output
    Serial.println("No id file found on sd card, using bakup id");
    #endif
    backup_id.toCharArray(w.id, 5);
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
  
  #if serial_output
  Serial.println("Sensor id: " + String(w.id));
  Serial.println("Data upload frequency: " + (String)data_upload_frequency + " minutes");
  Serial.println("Data read frequency: " + (String)data_read_frequency + " minutes");
  if(HT_mode == 0) Serial.println("Using SHT21"); else if(HT_mode == 1) Serial.println("Using DHT22"); else Serial.println("No HT sensor");
  if(enable_BMP180) Serial.println("BMP180 enabled"); else Serial.println("BMP180 disabled");
  if(enable_GPS) Serial.println("GPS enabled"); else Serial.println("GPS disabled");
  Serial.println();
  #endif

  //Talk();

  //GSMModuleReset();
  digitalWrite(upload_led, HIGH);
  delay(10000);//Wait for the SIM to log on to the network
  digitalWrite(upload_led, LOW);

  //Initialize GSM module
  InitGSM();  
  
  //Interrupt initialization
  noInterrupts();//stop interrupts
  TCCR1A = 0; TCCR1B = 0; TCNT1  = 0; OCR1A = 62500; TCCR1B |= (1 << WGM12); TCCR1B |= 5;
  TCCR1B &= 253; TIMSK1 |= (1 << OCIE1A); TCCR2A = 0; TCCR2B = 0; TCNT2  = 0;
  OCR2A = 249; TCCR2A |= (1 << WGM21); TCCR2B |= (1 << CS21); TIMSK2 |= (1 << OCIE2A);
  #if serial_output == true
  Serial.println("\nSeconds till next upload:");
  #endif
  
  interrupts();//Timer1: 0.25hz, Timer2: 8Khz
}
 
void loop(){
  int i = 0, j, avg_counter = 0;
  boolean temp_read, temp_upload, temp_network;
  
  rain_counter = 0;
  wind_speed_counter = 0;
  reading_number = 0;

  WeatherDataReset(w);
  TimeDataReset(current_time);
  
  while(1){
    CheckOTA();
    
    if(four_sec){
      four_sec = false;
      temp_read = read_flag;
      temp_upload = upload_flag;
  
      #if serial_output
      if(timer1_counter) Serial.println(((data_upload_frequency * 15) - timer1_counter + 1) * 4);
      else Serial.println("4");
      #endif
        
      #if HT_mode == 0
      w.hum = (w.hum * float(avg_counter) + SHT2x.GetHumidity()) / (float(avg_counter) + 1.0);
      //w.temp1 = (w.temp1 * float(avg_counter) + SHT2x.GetTemperature()) / (float(avg_counter) + 1.0);
      #endif
      #if HT_mode == 1
      w.hum = (w.hum * float(avg_counter) + dht.readHumidity()) / (float(avg_counter) + 1.0);
      w.temp1 = (w.temp1 * float(avg_counter) + dht.readTemperature()) / (float(avg_counter) + 1.0);
      #endif
      #if enable_BMP180 == true
      w.pressure = (w.pressure * float(avg_counter) + barometer.GetPressure()) / (float(avg_counter) + 1.0);
      //w.temp2 = (w.temp2 * avg_counter + barometer.GetTemperature()) / (float(avg_counter) + 1.0);
      #endif
      //w.solar_radiation = (w.solar_radiation * float(avg_counter) + float(analogRead(solar_radiation_pin))) / (float(avg_counter) + 1.0);
      
      bvoltageValue = analogRead(batteryInput);
      bvout = (bvoltageValue * 5.0) / 1024.0; // see text
      bvin = bvout / (R2/(R1+R2)); 
      if (bvin<0.09) {
        bvin=0.0;//statement to quash undesired reading !
      } 
      w.battery_voltage = (w.battery_voltage * float(avg_counter) + bvin) / (float(avg_counter) + 1.0);
      
      Voltage = (analogRead(analogIn) / 1023.0) * 5000; // Gets you mV
      w.amps = (w.amps * float(avg_counter) + ((Voltage - ACSoffset) / mVperAmp)) / (float(avg_counter) + 1.0);
      
      voltageValue = analogRead(voltageInput);
      vout = (voltageValue * 5.0) / 1024.0; // see text
      vin = vout / (R2/(R1+R2)); 
      if (vin<0.09) {
        vin=0.0;//statement to quash undesired reading !
      } 
      w.voltage = (w.voltage * float(avg_counter) + vin) / (float(avg_counter) + 1.0);
      sensors.requestTemperatures();
      w.temp2 = (w.temp2 * avg_counter + sensors.getTempCByIndex(0)) / (float(avg_counter) + 1.0);
      //Serial.println(String(w.voltage) + ", " + String(w.battery_voltage) + ", " + String(w.temp2) + ", " + String(w.amps));
      
      avg_counter++;

      WeatherCheckIsNan(w);
  
      if(temp_read){//Read data
        read_flag = false;
        digitalWrite(upload_led, HIGH);
        
        //Read data
        avg_counter = 0;
        
        w.wind_speed = ((wind_speed_counter) / data_upload_frequency);
        wind_speed_counter = 0;
        
        w.rain = rain_counter;
        rain_counter = 0;
        
        #if enable_GPS == true
        //GetGPS();
        #endif

        #if serial_output
        PrintWeatherData(w);
        #endif
        
        reading_number++;
        
        digitalWrite(upload_led, LOW);
      }
      if(temp_upload){//Upload data
        digitalWrite(upload_led, HIGH);

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
          Serial.println("\nUpload failed - sending sms");
          #endif
          delay(100);
          //upload_sms(w, phone_number);
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
        
        #if serial_output
        Serial.println("\nWriting to SD card");
        #endif
        WriteSD(w, current_time);
        
        digitalWrite(upload_led, LOW);
        upload_flag = false;
        #if serial_output
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
  	}
  }
}

//Interrupt gets called every 4 seconds
ISR(TIMER1_COMPA_vect){ 
  #if enable_GPS
  if(GPS_wait < 2) GPS_wait++;
  #endif
  timer1_counter++;
  if(timer1_counter == data_upload_frequency * 15){ 
    timer1_counter = 0;
    upload_flag = true;
  }
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
  rain_temp = digitalRead(rain_pin);
  if(rain_temp == 1 && rain_flag == true) {
    digitalWrite(rain_led, HIGH);
    rain_counter++;
    rain_flag = false;
  }
  else if(rain_temp == 0) {
    rain_flag = true;
    digitalWrite(rain_led, LOW);
  }
  wind_temp = digitalRead(wind_pin);
  if(wind_temp == 0 && wind_flag == true) {
    wind_speed_counter++;
    wind_flag = false;
    digitalWrite(wind_led, HIGH);
  } 
  else if(wind_temp == 1) {
    wind_flag = true;
    digitalWrite(wind_led, LOW);
  } 
}
