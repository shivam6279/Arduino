#ifndef GENERAL_h
#define GENERAL_h

#include "settings.h"
#include "weatherData.h"

#if HT_MODE == 1
  #define DHTTYPE DHT22 
  extern DHT dht;
#elif HT_MODE == 2
  extern SHT1x sht15;
#endif

//BMP180
#if ENABLE_BMP180 == true
  extern BMP180 barometer;
#endif

//SD
extern bool SD_flag;

//Wind speed
extern volatile int wind_speed_counter;
extern bool wind_flag, wind_temp;
extern double wind_speed_buffer[DATA_READ_FREQUENCY * 3 + 1];

//Davis wind speed
extern volatile int wind_speed_counter_davis;
extern bool wind_flag_davis, wind_temp_davis;
extern int VaneValue;
extern int Direction;
extern int CalDirection;


//SHT31.h Temperature
extern int temp;
extern float cTemp;
extern float fTemp;
extern float humidity;

//Rain guage
extern volatile int rain_counter;
extern bool rain_flag, rain_temp;

//Voltage
extern float TR1;
extern float TR2;
extern int mVperAmp;
extern int ACSoffset; 

//Timer interrupt
extern uint8_t timer1_counter;
extern volatile uint8_t GPS_wait;
extern volatile bool four_sec, read_flag, upload_flag;

//Weather data
extern int ws_id;
extern char imei_str[20];

//Times
extern realTime current_time;
extern realTime startup_time;

extern RTC_DS3231 rtc;

extern bool startup;

extern void ReadData(weatherData &w, int);
extern bool SHT15_ReadTempHum(float *, float *);
extern void pinSetup();
extern void PrintInitial(RTC_DS3231 rtc);

#endif
