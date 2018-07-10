#ifndef weatherData_h
#define weatherData_h

#include "Arduino.h"

extern char ws_id[4];

struct real_time {
  bool flag = 0;
  uint16_t seconds = 0, minutes = 0, hours = 0, day = 0, month = 0;
  uint16_t year = 0;
};

struct weatherData {
  float temp1, temp2, hum, solar_radiation;
  float latitude, longitude, panel_voltage, battery_voltage, amps;
  int rain, wind_speed;
  int signal_strength;
  long int pressure;
  bool flag;

  real_time t;
};

extern void WeatherDataReset(weatherData&);
extern void WeatherCheckIsNan(weatherData&);
extern void PrintWeatherData(weatherData);
extern void TimeDataReset(real_time&);
extern void PrintTime(real_time);
extern void HandleTimeOverflow(real_time&);
extern void AddTime(real_time, real_time, real_time&);
extern void SubtractTime(real_time, real_time, real_time&);
extern int DaysInMonth(int, int);
extern bool CheckLeapYear(int);

#endif
