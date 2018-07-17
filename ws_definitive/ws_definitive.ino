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

//All pin definitions and settings are in "settings.h"

#if HT_MODE == 1
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
bool wind_flag = false, wind_temp;
double wind_speed_buffer[DATA_READ_FREQUENCY * 15 + 1];

//Rain guage
volatile int rain_counter;
bool rain_flag = false, rain_temp;

//Voltage
float R1 = 100000.0;
float R2 = 10000.0;
int mVperAmp = 185; // use 100 for 20A Module and 66 for 30A Module
int ACSoffset = 2500; 

//Timer interrupt
uint8_t timer1_counter = 0;
volatile uint8_t GPS_wait;
volatile bool four_sec = false, read_flag = false, upload_flag = false;

//Weather data

//Times
realTime current_time;
realTime startup_time;

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
  pinMode(GSM_PWRKEY_PIN, OUTPUT);
  digitalWrite(GSM_PWRKEY_PIN, LOW);

  Serial.begin(115200); 
  Serial1.begin(19200);
  pinMode(19, INPUT);  
  digitalWrite(19, HIGH);
  Serial2.begin(9600);
  pinMode(17, INPUT);  
  digitalWrite(17, HIGH);
  //Serial3.begin(9600);

  SD_flag = sd.begin(53);
  delay(100);
  if(!sd.exists("id.txt")){
    #if SERIAL_OUTPUT
      Serial.println("\nNo id file found on sd card, using bakup id");
    #endif
    BACKUP_ID.toCharArray(ws_id, 5);
  } else {
    #if SERIAL_OUTPUT
      Serial.println("\nSD Card detected");
    #endif
    datalog.open("id.txt", FILE_READ);
    while(datalog.available()) {
      ws_id[i++] = datalog.read();
    }
    ws_id[i] = '\0';
    datalog.close();
  }
  
  #if HT_MODE == 0 || ENABLE_BMP180 == true
    Wire.begin();
  #endif
  #if HT_MODE == 1
    dht.begin();
  #endif
  sensors.begin();
  
  #if ENABLE_BMP180
    barometer = BMP180();
    barometer.SoftReset();
    barometer.Initialize(); 
  #endif

  #if SERIAL_OUTPUT
    Serial.println("Sensor id: " + String(ws_id));
    Serial.println("Data upload frequency: " + (String)DATA_UPLOAD_FREQUENCY + " minutes");
    Serial.println("Data read frequency: " + (String)DATA_READ_FREQUENCY + " minutes");
    if(HT_MODE == 0) Serial.println("Using SHT21"); else if(HT_MODE == 1) Serial.println("Using DHT22"); else Serial.println("No HT sensor");
    if(ENABLE_BMP180) Serial.println("BMP180 enabled"); else Serial.println("BMP180 disabled");
    if(ENABLE_GPS) Serial.println("GPS enabled"); else Serial.println("GPS disabled");
    Serial.println();
  #endif
  
  //Talk();

  digitalWrite(UPLOAD_LED, HIGH);
  delay(2000);  //Wait for the GSM module to boot up
  digitalWrite(UPLOAD_LED, LOW);

  //Initialize GSM module
  InitGSM();  
  
  //Interrupt initialization  
  InitInterrupt();  //Timer1: 0.25hz, Timer2: 8Khz
}
 
void loop() {
  int i, j;
  unsigned long int avg_counter = 0;
  boolean temp_read, temp_upload, temp_network;

  uint16_t reading_number = 0;
  uint16_t number_of_fail_uploads = 0;
  uint16_t initial_sd_card_uploads = 0;

  weatherData w[BUFFER_SIZE];

  realTime temp_time;
  
  rain_counter = 0;
  wind_speed_counter = 0;
  reading_number = 0;

  for(i = 0; i < BUFFER_SIZE; i++) {
    w[i].Reset();
  }
  while(Serial1.available()) Serial1.read();

  #if SERIAL_OUTPUT == true
    Serial.println("\nSeconds till next upload:");
  #endif
  while(1) {
    CheckOTA();
    
    if(four_sec) {
      four_sec = false;
      temp_read = read_flag;
      temp_upload = upload_flag;
  
      #if SERIAL_OUTPUT
        if(timer1_counter) Serial.println(((DATA_UPLOAD_FREQUENCY * 15) - timer1_counter + 1) * 4);
        else Serial.println("4");
      #endif

      ReadData(w[reading_number], avg_counter);

      wind_speed_buffer[avg_counter] = double(wind_speed_counter) * WIND_RADIUS * 6.2831 / 4.0 * WIND_SCALER;
      wind_speed_counter = 0;

      avg_counter++;
  
      if(temp_read) { //Read data
        read_flag = false;

        digitalWrite(UPLOAD_LED, HIGH);
        
        //Read data
        
        w[reading_number].wind_speed = ArrayAvg(wind_speed_buffer, avg_counter);
        w[reading_number].wind_stdDiv = StdDiv(wind_speed_buffer, avg_counter);
        
        w[reading_number].rain = rain_counter;
        rain_counter = 0;

        w[reading_number].t = current_time;

        w[reading_number].signal_strength = GetSignalStrength();
        
        #if ENABLE_GPS
          //GetGPS();
        #endif

        //w[reading_number].CheckIsNan();

        #if SERIAL_OUTPUT
          w[reading_number].PrintData();
        #endif
        
        reading_number++;
        avg_counter = 0;
        
        digitalWrite(UPLOAD_LED, LOW);
      }

      if(temp_upload) { //Upload data
        digitalWrite(UPLOAD_LED, HIGH);

        upload_flag = false;
        
        GSMModuleWake();
        
        //-----------------------------------No SD Card---------------------------------
        if(!sd.exists("id.txt")) {
          #if SERIAL_OUTPUT
            Serial.println("\nUploading data");
          #endif

          if(UploadWeatherData(w, reading_number, current_time)) {  //Upload successful          
            startup = false;
            w[reading_number - 1].t = current_time;
            
            for(i = 0; i < reading_number; i++) {
              w[i].flag = 1;
            }
            
            number_of_fail_uploads = 0;
            
            #if SERIAL_OUTPUT
              Serial.println("\nUpload successful");
            #endif
          } else {
            number_of_fail_uploads++;
            
            for(i = 0; i < reading_number; i++) {
              w[i].flag = 0;
            }

            #if SERIAL_OUTPUT
              Serial.println("\nUpload failed\n");
            #endif

            delay(100);
            //upload_sms(w[reading_number], phone_number);
          }
        } else {
        //------------------------------------------------------------------------------
          for(i = 0; i < reading_number; i++) {
            w[i].flag = 0;
          }
          #if SERIAL_OUTPUT
            Serial.println("Writing to the SD card");
          #endif
          for(i = 0; i < reading_number; i++) {
            if(!WriteSD(w[i])) {
              #if SERIAL_OUTPUT
                Serial.println("Write failed");
              #endif
            }
            if(startup == true) 
              initial_sd_card_uploads++;
          }

          if(current_time.flag == false && startup == true) {
            #if SERIAL_OUTPUT
              Serial.println("\nReading time from server");
            #endif

            temp_time = current_time;
            if(GetTime(current_time)) {
              #if SERIAL_OUTPUT
                Serial.println("\nCurrent Time:");
              #endif
              current_time.PrintTime();
              SubtractTime(current_time, temp_time, startup_time);
              #if SERIAL_OUTPUT
                Serial.println("\nStartup Time:");
              #endif
              startup_time.PrintTime();
              if(!WriteOldTime(initial_sd_card_uploads, startup_time)) {
                #if SERIAL_OUTPUT
                  Serial.println("SD Card failure");
                #endif
              }
            } else {
              #if SERIAL_OUTPUT
                Serial.println("Server request failed");
              #endif
              number_of_fail_uploads++; 
            }
          }
          
          if(current_time.flag) {
            #if SERIAL_OUTPUT
              Serial.println("\nUploading data");
            #endif
            if(UploadOldSD()) {  //Upload successful          
              startup = false;
              w[reading_number - 1].t = current_time;
              
              for(i = 0; i < reading_number; i++) {
                w[i].flag = 1;
              }
              
              number_of_fail_uploads = 0;
              
              #if SERIAL_OUTPUT
                Serial.println("\nUpload successful");
              #endif
            } else {
              number_of_fail_uploads++;
  
              #if SERIAL_OUTPUT
                Serial.println("\nUpload failed\n");
              #endif
              
              //upload_sms(w[reading_number], phone_number);
            }
          }
        }

        #ifdef GSM_PWRKEY_PIN
          if(number_of_fail_uploads % 5 == 0 && number_of_fail_uploads > 0) {
            InitGSM();
          }
        #endif
        
        reading_number = 0;
        
        digitalWrite(UPLOAD_LED, LOW);
        #if SERIAL_OUTPUT
          if(current_time.flag) current_time.PrintTime();
          Serial.println("Seconds till next upload:");
        #endif
      }
    }
  }
}

//Interrupt gets called every 4 seconds
ISR(TIMER1_COMPA_vect) { 
  #if enable_GPS
    if(GPS_wait < 2) GPS_wait++;
  #endif
  timer1_counter++;
  if(timer1_counter == DATA_UPLOAD_FREQUENCY * 15) { 
    timer1_counter = 0;
    upload_flag = true;
  }
  if(timer1_counter % (DATA_READ_FREQUENCY * 15) == 0) read_flag = true;
  four_sec = true;

  //Increment time by 4 seconds and handle overflow
  current_time.seconds += 4;
  current_time.HandleTimeOverflow();

}

//Interrupt gets called every 125 micro seconds
ISR(TIMER2_COMPA_vect) {
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

void ReadData(weatherData &w, int c) {
  //SHT
  #if HT_MODE == 0
    w.hum = (w.hum * float(c) + SHT2x.GetHumidity()) / (float(c) + 1.0);
    w.temp1 = (w.temp1 * float(c) + SHT2x.GetTemperature()) / (float(c) + 1.0);
  #endif
  //DHT
  #if HT_MODE == 1
    w.hum = (w.hum * float(c) + dht.readHumidity()) / (float(c) + 1.0);
    w.temp1 = (w.temp1 * float(c) + dht.readTemperature()) / (float(c) + 1.0);
  #endif
  #if ENABLE_BMP180 == true
    w.pressure = (w.pressure * float(c) + barometer.GetPressure()) / (float(c) + 1.0);
    //w.temp2 = (w.temp2 * c + barometer.GetTemperature()) / (float(c) + 1.0);
  #endif
  //w.solar_radiation = (w.solar_radiation * float(c) + (float(analogRead(BATTERY_PIN)) * 5.0 / 1024.0)) / (float(c) + 1.0);
  
  w.battery_voltage = (w.battery_voltage * float(c) + (float(analogRead(BATTERY_PIN)) * 5.0 / 1024.0 * (R1 + R2) / R2)) / (float(c) + 1.0);
  
  w.amps = (w.amps * float(c) + (((float(analogRead(CURRENT_SENSOR_PIN)) / 1023.0 * 5000.0) - ACSoffset) / mVperAmp)) / (float(c) + 1.0);

  w.panel_voltage = (w.panel_voltage * float(c) + (float(analogRead(CHARGE_PIN)) * 5.0 / 1024.0 * ((R1 + R2) / R2))) / (float(c) + 1.0);
  
  sensors.requestTemperatures();
  w.temp2 = (w.temp2 * float(c) + sensors.getTempCByIndex(0)) / (float(c) + 1.0);

  w.CheckIsNan();
  
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

