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
int ws_id;

//Times
realTime current_time;
realTime startup_time;

bool startup = true;

OneWire oneWire(METAL_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

void setup() {  
  unsigned int i;
  char str[10];

  SdFile datalog;

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
  Serial1.begin(115200);
  pinMode(19, INPUT);  
  digitalWrite(19, HIGH);
  Serial2.begin(9600);
  pinMode(17, INPUT);  
  digitalWrite(17, HIGH);
  //Serial3.begin(9600);

  SD_flag = sd.begin(SD_CARD_CS_PIN);
  delay(100);
  if(!sd.exists("id.txt")){
    if(SERIAL_OUTPUT) {
      Serial.println("\nNo id file found on sd card");
    }
    ws_id = 0;//BACKUP_ID.toInt();
  } else {
    datalog.open("id.txt", FILE_READ);
    while(datalog.available()) {
      str[i++] = datalog.read();
    }
    str[i] = '\0';
    ws_id = String(str).toInt();
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

  if(SERIAL_OUTPUT) {
    Serial.println("Sensor id: " + String(ws_id));
    Serial.println("Data upload frequency: " + (String)DATA_UPLOAD_FREQUENCY + " minutes");
    Serial.println("Data read frequency: " + (String)DATA_READ_FREQUENCY + " minutes");
    if(HT_MODE == 0) Serial.println("Using SHT21"); else if(HT_MODE == 1) Serial.println("Using DHT22"); else Serial.println("No HT sensor");
    if(ENABLE_BMP180) Serial.println("BMP180 enabled"); else Serial.println("BMP180 disabled");
    if(ENABLE_GPS) Serial.println("GPS enabled"); else Serial.println("GPS disabled");
    Serial.println();
  }

  digitalWrite(UPLOAD_LED, HIGH);
  delay(2000);  //Wait for the GSM module to boot up
  digitalWrite(UPLOAD_LED, LOW);

  //Initialize GSM module
  InitGSM();
  
  delay(6000);
  
  //Talk();
  
  //Interrupt initialization  
  InitInterrupt();  //Timer1: 0.25hz, Timer2: 8Khz
}
 
void loop() {
  int i, j;
  unsigned long int avg_counter = 0;
  bool temp_read, temp_upload, temp_network, temp_sd;

  uint16_t reading_number = 0;
  uint16_t number_of_fail_uploads = 0;
  uint16_t initial_sd_card_uploads = 0;

  weatherData w[BUFFER_SIZE];

  realTime temp_time;
  
  rain_counter = 0;
  wind_speed_counter = 0;
  reading_number = 0;

  for(i = 0; i < BUFFER_SIZE; i++) {
    w[i].Reset(ws_id);
  }
  while(Serial1.available()) Serial1.read();

  if(ws_id == 0) {
    if(GetID(ws_id)) {
      Serial.println("True");
    } else {
      ws_id = 0;
    }
  }
  
  if(SERIAL_OUTPUT) {
    Serial.println("\nSeconds till next upload:");
  }
  
  while(1) {
    Debug();

    if(ws_id == 0) {
      if(GetID(ws_id)) {
        Serial.println("True");
      } else {
        ws_id = 0;
      }
    }
    if(four_sec) {
      four_sec = false;
      temp_read = read_flag;
      temp_upload = upload_flag;

      CheckOTA();
  
      if(SERIAL_OUTPUT) {
        if(timer1_counter) Serial.println(((DATA_UPLOAD_FREQUENCY * 15) - timer1_counter + 1) * 4);
        else Serial.println("4");
      }

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

        if(SERIAL_OUTPUT) {
          w[reading_number].PrintData();
        }
        
        reading_number++;
        avg_counter = 0;
        
        digitalWrite(UPLOAD_LED, LOW);
      }

      if(temp_upload) { //Upload data
        digitalWrite(UPLOAD_LED, HIGH);

        upload_flag = false;

        //-------------------------------SD Card Datalog--------------------------------

        temp_sd = true;
        if(sd.exists("id.txt")) {
          for(i = 0; i < reading_number; i++) {
            w[i].flag = 0;
          }
          if(SERIAL_OUTPUT) {
            Serial.println("\nWriting to the SD card");
          }
          for(i = 0; i < reading_number; i++) {
            if(!WriteSD(w[i])) {
              if(SERIAL_OUTPUT) {
                Serial.println("Write failed");
              }
              temp_sd = false;
              break;
            }
            if(startup == true) 
              initial_sd_card_uploads++;
          }
        }
        
        //-----------------------------------No SD Card---------------------------------

        if(!sd.exists("id.txt") || temp_sd == false) {
          if(SERIAL_OUTPUT) {
            Serial.println("\nUploading data");
          }

          if(UploadWeatherData(w, reading_number, current_time)) {  //Upload successful          
            startup = false;
            w[reading_number - 1].t = current_time;
            
            for(i = 0; i < reading_number; i++) {
              w[i].flag = 1;
            }
            
            number_of_fail_uploads = 0;
            
            if(SERIAL_OUTPUT) {
              Serial.println("\nUpload successful");
            }
          } else {
            number_of_fail_uploads++;
            
            for(i = 0; i < reading_number; i++) {
              w[i].flag = 0;
            }

            if(SERIAL_OUTPUT) {
              Serial.println("\nUpload failed\n");
            }

            delay(100);
            //upload_sms(w[reading_number], phone_number);
          }
        } else {

          //----------------------------------------------------------------------------

          if(current_time.flag == false && startup == true) {
            if(SERIAL_OUTPUT) {
              Serial.println("\nReading time from server");
            }

            temp_time = current_time;
            if(GetTime(current_time)) {
              if(SERIAL_OUTPUT) {
                Serial.println("\nCurrent Time:");
              }
              current_time.PrintTime();
              SubtractTime(current_time, temp_time, startup_time);
              if(SERIAL_OUTPUT) {
                Serial.println("\nStartup Time:");
              }
              startup_time.PrintTime();
              if(!WriteOldTime(initial_sd_card_uploads, startup_time)) {
                temp_sd = false;
                if(SERIAL_OUTPUT) {
                  Serial.println("SD Card failure");
                }
              }
            } else {
              if(SERIAL_OUTPUT) {
                Serial.println("Server request failed");
              }
              number_of_fail_uploads++; 
            }
          }
          
          if(current_time.flag && temp_sd) {
            delay(1000);
            if(SERIAL_OUTPUT) {
              Serial.println("\nUploading data");
            }
            if(UploadCSV()) {  //Upload successful          
              startup = false;
              w[reading_number - 1].t = current_time;
              
              for(i = 0; i < reading_number; i++) {
                w[i].flag = 1;
              }
              
              number_of_fail_uploads = 0;
              
              if(SERIAL_OUTPUT) {
                Serial.println("\nUpload successful");
              }
            } else {
              number_of_fail_uploads++;
  
              if(SERIAL_OUTPUT) {
                Serial.println("\nUpload failed\n");
              }
              
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
        if(SERIAL_OUTPUT) {
          if(current_time.flag) current_time.PrintTime();
          Serial.println("Seconds till next upload:");
        }
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

