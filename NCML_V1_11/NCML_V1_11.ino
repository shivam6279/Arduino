#include "settings.h"

#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <String.h>
#include <Wire.h>
//#include <BMP180.h>
#include <DHT.h>

#include <SPI.h>
#include <RTClib.h>

//#include <DallasTemperature.h>
//#include <OneWire.h>

#include "http.h"
#include "weatherData.h"
#include "GSM.h"
#include "SD.h"
#include "general.h"

//All pin definitions and settings are in "settings.h/cpp"

//OneWire oneWire(METAL_SENSOR_PIN);
//DallasTemperature sensors(&oneWire);

void setup() {  
  unsigned int i;
  char str[10];

  SdFile datalog;

  pinSetup();
  
  delay(100);

  #ifdef USE_HEROKU_URL
    if(!sd.exists("id.txt")){
      if(SERIAL_OUTPUT) {
        Serial.println(F("\nNo id file found on sd card"));
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
  #endif
  #ifdef USE_CWIG_URL
    if(!sd.exists("config.txt")){
      if(SERIAL_OUTPUT) {
        Serial.println(F("\nNo config file found on sd card"));
      }
      ws_id = 0;//BACKUP_ID.toInt();
    } else {
      ReadConfig();
    }
  #endif
  
  #if HT_MODE == 0 || ENABLE_BMP180 == true
    Wire.begin();
  #elif HT_MODE == 1
    dht.begin();
  #endif
//  sensors.begin();
  
  #if ENABLE_BMP180
    barometer = BMP180();
    barometer.SoftReset();
    barometer.Initialize(); 
  #endif

  // RTC
  bool RTC_flag = true;
  DateTime now;
  if(!rtc.begin()) {
    RTC_flag = false;
  } else {
    // Read current time from RTC into system
    now = rtc.now();
    current_time.year = now.year();
    current_time.month = now.month();
    current_time.day = now.day();
    current_time.hours = now.hour();
    current_time.minutes = now.minute();
    current_time.seconds = now.second();
    current_time.flag = true;
  }

  //For testing; If not RTC connected, use time of flashing from the computer
  if(!RTC_flag) {
    current_time.year = 1000 * (String(__DATE__)[7] - '0') + 100 * (String(__DATE__)[8] - '0') + 10 * (String(__DATE__)[9] - '0') + (String(__DATE__)[10] - '0');
    
    if(String(__DATE__)[0] == 'J' && String(__DATE__)[1] == 'a' && String(__DATE__)[2] == 'n')
      current_time.month = 1;
    else if(String(__DATE__)[0] == 'F')
      current_time.month = 2;
    else if(String(__DATE__)[0] == 'M' && String(__DATE__)[1] == 'a' && String(__DATE__)[2] == 'r')
      current_time.month = 3;
    else if(String(__DATE__)[0] == 'A')
      current_time.month = 4;
    else if(String(__DATE__)[0] == 'M' && String(__DATE__)[1] == 'a' && String(__DATE__)[2] == 'y')
      current_time.month = 5;
    else if(String(__DATE__)[0] == 'J' && String(__DATE__)[1] == 'u' && String(__DATE__)[2] == 'n')
      current_time.month = 6;
    else if(String(__DATE__)[0] == 'J' && String(__DATE__)[1] == 'u' && String(__DATE__)[2] == 'l')
      current_time.month = 7;
    else if(String(__DATE__)[0] == 'A')
      current_time.month = 8;
    else if(String(__DATE__)[0] == 'S')
      current_time.month = 9;
    else if(String(__DATE__)[0] == 'O')
      current_time.month = 10;
    else if(String(__DATE__)[0] == 'N')
      current_time.month = 11;
    else if(String(__DATE__)[0] == 'D')
      current_time.month = 12;
      
    current_time.day = 10 * (String(__DATE__)[4] - '0') + (String(__DATE__)[5] - '0');
    current_time.seconds =  10 * (String(__TIME__)[6] - '0') + (String(__TIME__)[7] - '0');
    current_time.minutes =  10 * (String(__TIME__)[3] - '0') + (String(__TIME__)[4] - '0');
    current_time.hours =  10 * (String(__TIME__)[0] - '0') + (String(__TIME__)[1] - '0');
  }

  // Print startup data/configs
  if(SERIAL_OUTPUT) {
    Serial.println();
    Serial.println("Last flashed: " + String(__DATE__) + " " + String(__TIME__));
    #ifdef USE_HEROKU_URL
      Serial.print(F("Sensor id: ")); Serial.println(ws_id);
    #endif
    Serial.print(F("Data upload frequency: ")); Serial.print(DATA_UPLOAD_FREQUENCY); Serial.println(F(" minutes"));
    if(HT_MODE == 0) Serial.println(F("Using SHT21")); else if(HT_MODE == 1) Serial.println(F("Using DHT22")); else if(HT_MODE == 2) Serial.println(F("Using SHT15"));else Serial.println(F("No HT sensor"));
    if(ENABLE_BMP180) Serial.println(F("BMP180 enabled")); else Serial.println(F("BMP180 disabled"));
    if(!RTC_flag) Serial.println("Couldn't find RTC");
        
    Serial.println("Current Time (RTC):");
    Serial.print(current_time.day, DEC);    
    Serial.print('/');
    Serial.print(current_time.month, DEC);
    Serial.print('/');
    Serial.println(current_time.year, DEC);
    Serial.print(current_time.hours, DEC);
    Serial.print(':');
    Serial.print(current_time.minutes, DEC);
    Serial.print(':');
    Serial.println(current_time.seconds, DEC);
  }

  digitalWrite(UPLOAD_LED, HIGH);
  delay(2000);  //Wait for the GSM module to boot up
  digitalWrite(UPLOAD_LED, LOW);

  //Initialize GSM module
  InitGSM();

  // Get IMEI number from GSM module
  GetIMEI(imei_str);
  if(SERIAL_OUTPUT) {
    Serial.print(F("Sensor IMEI: ")); Serial.println(imei_str);
  }
  
  delay(6000);

  //Interrupt initialization  
  InitInterrupt();  //Timer1: 0.25hz, Timer2: 8Khz
}

weatherData w_arr[BUFFER_SIZE];
 
void loop() {
  int i, j;
  unsigned long int avg_counter = 0;
  bool temp_read, temp_upload, temp_network, temp_sd;

  uint16_t reading_number = 0;
  uint16_t number_of_fail_uploads = 0;
  uint16_t initial_sd_card_uploads = 0;

  bool door_flag = true;

  int wind_speed_buffer_counter = 0;

  weatherData w;

  realTime temp_time;
  
  rain_counter = 0;
  wind_speed_counter = 0;
  reading_number = 0;

  w.Reset(ws_id);
  for(i = 0; i < BUFFER_SIZE; i++) w_arr[i].Reset(ws_id);
  while(Serial1.available()) Serial1.read();

  #ifdef USE_HEROKU_URL
    //Fetch new id
    if(ws_id == 0) {
      if(!GetID(ws_id)) ws_id = 0;
    }
  #endif

  GetIMEI(imei_str);

  delay(5000);

  // if(GetTime(current_time)) {
  //   // rtc.adjust(DateTime(current_time.year, current_time.month, current_time.day, current_time.hours, current_time.minutes, current_time.seconds));
  //   Serial.println(F("Current Time (Server):"));
  //   current_time.PrintTime();
  // }
  
  if(SERIAL_OUTPUT) Serial.println(F("\nSeconds till next upload:"));

  //Main loop
  while(1) {
    Debug();

    if(four_sec) {
      four_sec = false;
      temp_read = read_flag;
      temp_upload = upload_flag;

      // CheckOTA();
  
      if(SERIAL_OUTPUT) {
        Serial.print("\nSeconds left: ");
        if(timer1_counter) Serial.println(((DATA_UPLOAD_FREQUENCY * 15) - timer1_counter + 1) * 4);
        else Serial.println(F("4"));
      }

      ShowSerialData();
      
      if(door_flag == true) {
        if(digitalRead(DOOR_PIN) == HIGH) {
          door_flag = false;
          Serial.println("Door opened");
//          Serial.println("Door opened, sending SMS");
//          char door_sms_str[100], door_admin_str[20];
//          ("Id: " + String(ws_id) + " - door opened").toCharArray(door_sms_str, 100);
//          ADMIN_PHONE_NUMBER.toCharArray(door_admin_str, 20);
//          SendSMS(door_admin_str, door_sms_str);
//          delay(4000);
//          ShowSerialData();
        }
      } else if(digitalRead(DOOR_PIN) == LOW) {
        door_flag = true;
      }

      ReadData(w, avg_counter);

      if((avg_counter + 1) % 5 == 0) {
        wind_speed_buffer[avg_counter/5] = double(wind_speed_counter) * WIND_RADIUS * 6.2831 / 20.0 * WIND_SCALER;
      }
      wind_speed_counter = 0;

      avg_counter++;

      //Read data
      if(temp_read) { 
        read_flag = false;

        digitalWrite(UPLOAD_LED, HIGH);
        
        //Read data
        //If IMEI number is not saved, read GSM module again    
        if(imei_str[0] == 'N') {
          GSMModuleWake();  
          GetIMEI(imei_str);
        }         
        w.imei = String(imei_str);
        w.wind_speed = ArrayAvg(wind_speed_buffer, avg_counter / 5);
        w.wind_stdDiv = StdDiv(wind_speed_buffer, avg_counter / 5);
        w.wind_speed_min = ArrayMin(wind_speed_buffer, avg_counter / 5);
        w.wind_speed_max = ArrayMax(wind_speed_buffer, avg_counter / 5);

        w.rain = rain_counter;
        if(digitalRead(RAIN_CONNECT_PIN) == HIGH)
          w.rain = -1;
          
        rain_counter = 0;

        realTime rounded_time = current_time;
        rounded_time.minutes = ((current_time.minutes + DATA_UPLOAD_FREQUENCY-1)/DATA_UPLOAD_FREQUENCY) * DATA_UPLOAD_FREQUENCY;
        rounded_time.seconds = 0;
        if(rounded_time.minutes == 60 && current_time.minutes >= (60 - DATA_UPLOAD_FREQUENCY)) {
          rounded_time.hours++;
          rounded_time.HandleTimeOverflow();
        }

        //w.t = current_time;
        w.t = rounded_time;
        rounded_time.PrintTime();
        w.signal_strength = GetSignalStrength();        
        w.CheckIsNan();

        if(SERIAL_OUTPUT) w.PrintData();

        //---------------SD Card Datalog---------------
        temp_sd = true;
        w.flag = 0;

        if(SERIAL_OUTPUT) Serial.println(F("Writing to the SD card"));
          
        if(!WriteSD(w)) {
          if(SERIAL_OUTPUT) Serial.println(F("Write failed"));
          temp_sd = false;
          break;
        }
        if(startup == true) initial_sd_card_uploads++;
        //----------------------------------------------

        w_arr[reading_number] = w;

        reading_number++;
        avg_counter = 0;
                
        digitalWrite(UPLOAD_LED, LOW);
      }
      
      //Upload data
      if(temp_upload) { 
        digitalWrite(UPLOAD_LED, HIGH);
        upload_flag = false;

      #ifdef USE_CWIG_URL
        if(SERIAL_OUTPUT) {
          Serial.print("\nUploading data: " + String(reading_number) + " record");
          if(reading_number > 1) Serial.print("s");
          Serial.println();
        }

        if(UploadWeatherData(w_arr, reading_number, current_time)) {  //Upload successful              
          startup = false;
          
          number_of_fail_uploads = 0;
          
          if(SERIAL_OUTPUT) Serial.println(F("\nUpload successful"));
          reading_number = 0;
        } else {
          number_of_fail_uploads++;

          if(reading_number >= BUFFER_SIZE) {
            for(i = 0; i < BUFFER_SIZE - 1; i++) w_arr[i] = w_arr[i + 1];
            reading_number--;
            w_arr[reading_number].Reset(ws_id);
          }            
          
          if(SERIAL_OUTPUT) Serial.println(F("\nUpload failed\n"));
          
          delay(100);
        }
          
      #else   
        //Fetch new id
        if(ws_id == 0) {
          if(GetID(ws_id)){
            if(!WriteOldID(initial_sd_card_uploads, ws_id)) {
              ws_id = 0;
            } else {
              for(i = 0; i < BUFFER_SIZE; i++) {
                w[i].id = ws_id;
              }
            }
          } else {
            ws_id = 0;
          }
        }
           
        //-----------------------------------No SD Card---------------------------------

        if(!sd.exists("id.txt") || temp_sd == false) {
          if(SERIAL_OUTPUT) Serial.println("\nUploading data");

          if(UploadWeatherData(w, reading_number, current_time)) {  //Upload successful          
            startup = false;
            w[reading_number - 1].t = current_time;
            
            for(i = 0; i < reading_number; i++) {
              w[i].flag = 1;
            }
            
            number_of_fail_uploads = 0;
            
            if(SERIAL_OUTPUT) Serial.println(F("\nUpload successful"));
          } else {
            number_of_fail_uploads++;
            
            for(i = 0; i < reading_number; i++) {
              w[i].flag = 0;
            }

            if(SERIAL_OUTPUT) Serial.println(F("\nUpload failed\n"));

            delay(100);
            //upload_sms(w[reading_number], SERVER_PHONE_NUMBER);
          }
        } else {

          //----------------------------------------------------------------------------

          if(current_time.flag == false && startup == true) {
            if(SERIAL_OUTPUT) Serial.println(F("\nReading time from server"));

            temp_time = current_time;
            if(GetTime(current_time)) {
              if(SERIAL_OUTPUT) Serial.println(F("\nCurrent Time:"));
              current_time.PrintTime();
              SubtractTime(current_time, temp_time, startup_time);
              if(SERIAL_OUTPUT) Serial.println(F("\nStartup Time:"));
              startup_time.PrintTime();
              if(!WriteOldTime(initial_sd_card_uploads, startup_time)) {
                temp_sd = false;
                if(SERIAL_OUTPUT) Serial.println(F("SD Card failure"));
              }
            } else {
              if(SERIAL_OUTPUT) Serial.println(F("Server request failed"));
              number_of_fail_uploads++; 
              reading_number = 0;
            }
          }
          
          if(current_time.flag && temp_sd) {
            delay(1000);
            if(SERIAL_OUTPUT) Serial.println(F("\nUploading data"));
            if(UploadCSV()) {  //Upload successful          
              startup = false;
              w[reading_number - 1].t = current_time;
              
              for(i = 0; i < reading_number; i++)  w[i].flag = 1;
              
              number_of_fail_uploads = 0;
              
              if(SERIAL_OUTPUT) Serial.println(F("\nUpload successful"));
            } else {
              number_of_fail_uploads++;
  
              if(SERIAL_OUTPUT) Serial.println(F("\nUpload failed\n"));
                            
              //upload_sms(w[reading_number], SERVER_PHONE_NUMBER);
              reading_number = 0;
            }
          }
        }
      #endif
        
        #ifdef GSM_PWRKEY_PIN
          if(number_of_fail_uploads % 5 == 0 && number_of_fail_uploads > 0) {
            InitGSM();
          }
        #endif
        
        digitalWrite(UPLOAD_LED, LOW);
        if(SERIAL_OUTPUT) {
          if(current_time.flag) current_time.PrintTime();
          Serial.println(F("Seconds till next upload:"));
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
  if(timer1_counter % (DATA_UPLOAD_FREQUENCY * 15) == 0) read_flag = true;
  four_sec = true;

  //Increment time by 4 seconds and handle overflow
  current_time.seconds += 4;
  current_time.HandleTimeOverflow();

}

//Interrupt gets called every 125 micro seconds - 8kHz
//Increment a counter everytime 
ISR(TIMER2_COMPA_vect) {
  rain_temp = digitalRead(RAIN_PIN);
  //Debounce Pin
  if(rain_temp == 1 && rain_flag == true) {
    digitalWrite(RAIN_LED, HIGH);
    rain_counter++;
    rain_flag = false;
  }
  else if(rain_temp == 0) {
    rain_flag = true;
    digitalWrite(RAIN_LED, LOW);
  }

  #ifdef WIND_PIN
    wind_temp = digitalRead(WIND_PIN);
    //Debounce Pin
    if(wind_temp == 0 && wind_flag == true) {
      wind_speed_counter++;
      wind_flag = false;
      #ifndef DAVIS_WIND_SPEED_PIN
        digitalWrite(WIND_LED, HIGH);
      #endif
    } 
    else if(wind_temp == 1) {
      wind_flag = true;
      #ifndef DAVIS_WIND_SPEED_PIN
        digitalWrite(WIND_LED, LOW);
      #endif
    } 
  #endif

//  #ifdef DAVIS_WIND_SPEED_PIN
//    wind_temp_davis = digitalRead(DAVIS_WIND_SPEED_PIN);
//    //Debounce Pin
//    if(wind_temp_davis == 0 && wind_flag_davis == true) {
//      wind_speed_counter_davis++;
//      wind_flag_davis = false;
//      digitalWrite(WIND_LED, HIGH);
//    } 
//    else if(wind_temp_davis == 1) {
//      wind_flag_davis = true;
//      digitalWrite(WIND_LED, LOW);
//    } 
//  #endif
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
