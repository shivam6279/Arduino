#ifndef settings_h
#define settings_h

//June 26, 2018
//Quectel on/off

#include "Arduino.h"

//--------------------------------------Pin Config--------------------------------------  

#define RAIN_LED 35
#define WIND_LED 41
#define UPLOAD_LED 12
#define RAIN_PIN 4
#define WIND_PIN A1
//#define DHTPIN 25  
#define SOLAR_RADIATION_PIN A3
#define METAL_SENSOR_PIN 26

#define BATTERY_PIN A14
#define CHARGE_PIN A11
#define CURRENT_SENSOR_PIN A15

#define GSM_DTR_PIN 24
#define GSM_PWRKEY_PIN 22

//---------------------------------------Settings----------------------------------------  

const String URL = "http://enigmatic-caverns-27645.herokuapp.com/maytheforcebewithyou?";

const String PHONE_NUMBER = "+919220592205";
const String BACKUP_ID = "201"; //Backup id in case there is no sd card

#define HT_MODE 0				        // 0 for SHT21, 1 for DST, 2 for none
       
#define DATA_UPLOAD_FREQUENCY 1	//Minutes -- should be a multiple of the read frequency
#define DATA_READ_FREQUENCY 1	  //Minutes
#define BUFFER_SIZE (DATA_UPLOAD_FREQUENCY / DATA_READ_FREQUENCY)

#define ENABLE_GPS false        //True to enable GPS   
#define ENABLE_BMP180 false 	  //True to enable BMP180

#define SERIAL_OUTPUT true 		  //True to debug: display values through the serial port
#define SERIAL_RESPONSE true	  //True to see GSM module serial response

//--------------------------------------------------------------------------------------

#endif
