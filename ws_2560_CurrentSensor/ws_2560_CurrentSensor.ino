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
weatherData w[BUFFER_SIZE];
real_time current_time;

bool startup = true;

OneWire oneWire(METAL_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

void setup() {  
  unsigned int i;

  pinMode(RAIN_LED, OUTPUT);
  pinMode(WIND_LED, OUTPUT);
  pinMode(UPLOAD_LED, OUTPUT);
  pinMode(53, OUTPUT);  //SD card's CS pin
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
      Serial.println("No id file found on sd card, using bakup id");
    #endif
    BACKUP_ID.toCharArray(ws_id, 5);
  } else {
    datalog.open("id.txt", FILE_READ);
    while(datalog.available()) {
      ws_id[i++] = datalog.read();
    }
    ws_id[i] = '\0';
    datalog.close();
  }
  
  Wire.begin();
  #if HT_MODE == 1
    dht.begin();
  #endif
  sensors.begin();
  
  #if enable_BMP180
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
  //Timer1: 0.25hz, Timer2: 8Khz
  InitInterrupt(); 
  #if SERIAL_OUTPUT == true
    Serial.println("\nSeconds till next upload:");
  #endif
}
 
void loop() {
  int i, j;
  unsigned long int avg_counter = 0;
  boolean temp_read, temp_upload, temp_network;

  uint8_t reading_number = 0, number_of_fail_uploads = 0;
  
  rain_counter = 0;
  wind_speed_counter = 0;
  reading_number = 0;

  for(i = 0; i < BUFFER_SIZE; i++) {
    WeatherDataReset(w[i]);
  }
  TimeDataReset(current_time);
  while(Serial1.available()) Serial1.read();
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

      //SHT
      #if HT_MODE == 0
        w[reading_number].hum = (w[reading_number].hum * float(avg_counter) + SHT2x.GetHumidity()) / (float(avg_counter) + 1.0);
        //w.temp1 = (w.temp1 * float(avg_counter) + SHT2x.GetTemperature()) / (float(avg_counter) + 1.0);
      #endif
      //DHT
      #if HT_MODE == 1
        w[reading_number].hum = (w[reading_number].hum * float(avg_counter) + dht.readHumidity()) / (float(avg_counter) + 1.0);
        w[reading_number].temp1 = (w[reading_number].temp1 * float(avg_counter) + dht.readTemperature()) / (float(avg_counter) + 1.0);
      #endif
      #if ENABLE_BMP180 == true
        w[reading_number].pressure = (w[reading_number].pressure * float(avg_counter) + barometer.GetPressure()) / (float(avg_counter) + 1.0);
        //w.temp2 = (w.temp2 * avg_counter + barometer.GetTemperature()) / (float(avg_counter) + 1.0);
      #endif
      //w.solar_radiation = (w.solar_radiation * float(avg_counter) + (float(analogRead(BATTERY_PIN)) * 5.0 / 1024.0)) / (float(avg_counter) + 1.0);
      
      w[reading_number].battery_voltage = (w[reading_number].battery_voltage * float(avg_counter) + (float(analogRead(BATTERY_PIN)) * 5.0 / 1024.0 * (R1 + R2) / R2)) / (float(avg_counter) + 1.0);
      
      w[reading_number].amps = (w[reading_number].amps * float(avg_counter) + (((float(analogRead(CURRENT_SENSOR_PIN)) / 1023.0 * 5000.0) - ACSoffset) / mVperAmp)) / (float(avg_counter) + 1.0);

      w[reading_number].panel_voltage = (w[reading_number].panel_voltage * float(avg_counter) + (float(analogRead(CHARGE_PIN)) * 5.0 / 1024.0 * ((R1 + R2) / R2))) / (float(avg_counter) + 1.0);
      
      sensors.requestTemperatures();
      w[reading_number].temp2 = (w[reading_number].temp2 * avg_counter + sensors.getTempCByIndex(0)) / (float(avg_counter) + 1.0);
      //Serial.println(String(w.voltage) + ", " + String(w.battery_voltage) + ", " + String(w.temp2) + ", " + String(w.amps));
      
      avg_counter++;

      WeatherCheckIsNan(w[reading_number]);
  
      if(temp_read) { //Read data
        read_flag = false;
        digitalWrite(UPLOAD_LED, HIGH);
        
        //Read data
        avg_counter = 0;
        
        w[reading_number].wind_speed = ((wind_speed_counter) / DATA_UPLOAD_FREQUENCY);
        wind_speed_counter = 0;
        
        w[reading_number].rain = rain_counter;
        rain_counter = 0;

        w[reading_number].t = current_time;

        w[reading_number].signal_strength = GetSignalStrength();
        
        #if enable_GPS
          //GetGPS();
        #endif

        #if SERIAL_OUTPUT
          PrintWeatherData(w[reading_number]);
        #endif

        #if SERIAL_OUTPUT
          Serial.println("\nWriting to SD card");
        #endif
        WriteSD(w[reading_number]);
        
        reading_number++;
        
        digitalWrite(UPLOAD_LED, LOW);
      }
      if(temp_upload) { //Upload data
        digitalWrite(UPLOAD_LED, HIGH);
        
        GSMModuleWake();
        if(SubmitHttpRequest(w, reading_number, current_time)) {  //Upload successful          
          startup = false;
          number_of_fail_uploads = 0;
          #if SERIAL_OUTPUT
            Serial.println("\nUpload successful");
          #endif
        } else {
          number_of_fail_uploads++;
          #if SERIAL_OUTPUT
            Serial.println("\nUpload failed");
          #endif
          if(number_of_fail_uploads % 5 == 0 && number_of_fail_uploads > 0) {
            GSMModuleRestart();
          }
          delay(100);
          //upload_sms(w[reading_number], phone_number);
        }
        reading_number = 0;
        delay(1000);
        
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
  HandleTimeOverflow(current_time);
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

