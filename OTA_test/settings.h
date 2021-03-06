#ifndef settings_h
#define settings_h

#include "Arduino.h"

//--------------------------------------Pin Config--------------------------------------  

#define RAIN_LED 35
#define WIND_LED 41
#define UPLOAD_LED 12
#define RAIN_PIN 4
#define WIND_PIN A1
#define DHTPIN 25  
#define SOLAR_RADIATION_PIN A3
#define METAL_SENSOR_PIN 26
#define SD_CARD_CS_PIN 53

#define BATTERY_PIN A14
#define CHARGE_PIN A11
#define CURRENT_SENSOR_PIN A15

#define GSM_DTR_PIN 24
#define GSM_PWRKEY_PIN 22

//---------------------------------------Settings----------------------------------------  

extern String URL;
extern String OTA_URL;
extern String TIME_URL;

extern String SD_csv_header;

extern String PHONE_NUMBER;
extern String BACKUP_ID; //Backup id in case there is no sd card

#define WIND_RADIUS 63.7 / 1000.0	//63.7mm
#define WIND_SCALER 2.5

#define HT_MODE 1				        // 0 for SHT21, 1 for DST, 2 for none
       
#define DATA_UPLOAD_FREQUENCY 2	//Minutes -- should be a multiple of the read frequency
#define DATA_READ_FREQUENCY 2  //Minutes
#define BUFFER_SIZE (DATA_UPLOAD_FREQUENCY / DATA_READ_FREQUENCY)

#define ENABLE_GPS false        //True to enable GPS   
#define ENABLE_BMP180 false 	  //True to enable BMP180

extern bool SERIAL_OUTPUT; 		  //True to debug: display values through the serial port
extern bool SERIAL_RESPONSE;	  //True to see GSM module serial response

//--------------------------------------------------------------------------------------

#endif
