#ifndef settings_h
#define settings_h

#include "Arduino.h"

/*
Board versions:
3 - With current sensor, without transistor for quectel reset
4 - With current sensor, with transistor

4 - NCML
*/
#define BOARD_VERSION 5

//--------------------------------------Pin Config--------------------------------------  
#if BOARD_VERSION == 3
  #define RAIN_LED 35
  #define WIND_LED 41
  #define UPLOAD_LED 12
  #define RAIN_PIN 4
  #define WIND_PIN A7
  #define DHTPIN 25  
  #define SOLAR_RADIATION_PIN A0
  #define METAL_SENSOR_PIN 27
  #define SD_CARD_CS_PIN 53
  
  #define BATTERY_PIN A14
  #define CHARGE_PIN A11
  #define CURRENT_SENSOR_PIN A15

  #define GSM_DTR_PIN 24

#elif BOARD_VERSION == 4
  #define RAIN_LED 35
  #define WIND_LED 41
  #define UPLOAD_LED 12
  #define RAIN_PIN 4
  #define WIND_PIN 26
  #define DAVIS_WIND_DIRECTION_PIN A1
  #define DHTPIN 25  
  #define SOLAR_RADIATION_PIN A4
  #define METAL_SENSOR_PIN 27
  #define SD_CARD_CS_PIN 53
  
  #define BATTERY_PIN A14
  #define CHARGE_PIN A11
  #define CURRENT_SENSOR_PIN A15
  #define LeafSensor1_PIN A3
  #define ThermistorPin_PIN A12
  #define REF_3V3_PIN A0
  #define UVOUT_PIN A13
  
  #define GSM_DTR_PIN 24
  #define GSM_PWRKEY_PIN 22
  #define Addr 0x44
  #define Addr2 0x45

#elif BOARD_VERSION == 5
  #define RAIN_LED 35
  #define WIND_LED 41
  #define UPLOAD_LED 12
  #define RAIN_PIN 5
  #define WIND_PIN 26
  #define DAVIS_WIND_DIRECTION_PIN A1
  #define DHTPIN 25  
  #define SOLAR_RADIATION_PIN A4
  #define METAL_SENSOR_PIN 27
  #define SD_CARD_CS_PIN 53

  #define RAIN_CONNECT_PIN 4
  #define DOOR_PIN 6
  
  #define BATTERY_PIN A14
  #define CHARGE_PIN A11
  #define CURRENT_SENSOR_PIN A15
  #define LeafSensor1_PIN A13
  #define ThermistorPin_PIN A10
  #define REF_3V3_PIN A0
  #define UVOUT_PIN A3

  #define SHT15_DATA_PIN 43
  #define SHT15_CLK_PIN 42
  
  #define GSM_DTR_PIN 24
  #define GSM_PWRKEY_PIN 22
  #define Addr 0x44
  #define Addr2 0x45

  #define DAVIS_WIND_SPEED_PIN 26
  #define DAVIS_WIND_DIRECTION_PIN A1
  #define DAVIS_WIND_DIRECTION_OFFSET 0.0
  #define REF_3V3 A0
  #define UVOUT A13
#endif  

//---------------------------------------Settings----------------------------------------  

extern String URL_HEROKU, URL_CWIG;
extern String OTA_URL;
extern String TIME_URL;
extern String CHECK_ID_URL;
extern String CREATE_ID_URL;

extern String SD_csv_header;

extern String SERVER_PHONE_NUMBER;
extern String ADMIN_PHONE_NUMBER;
extern String BACKUP_ID; //Backup id in case there is no sd card

#define USE_CWIG_URL

#define WIND_RADIUS 63.7 / 1000.0	//63.7mm
#define WIND_SCALER 1.0

#define HT_MODE 0			        // 0 for SHT21, 1 for DST, 2 for SHT15, 3 for none
       
#define DATA_UPLOAD_FREQUENCY 60	//Minutes -- should be a multiple of the read frequency
#define DATA_READ_FREQUENCY 60  //Minutes
//#define BUFFER_SIZE (DATA_UPLOAD_FREQUENCY / DATA_READ_FREQUENCY)
#define BUFFER_SIZE 24

#define ENABLE_BMP180 false 	  //True to enable BMP180

extern bool SERIAL_OUTPUT; 		  //True to debug: display values through the serial port
extern bool SERIAL_RESPONSE;	  //True to see GSM module serial response

//--------------------------------------------------------------------------------------

#endif
