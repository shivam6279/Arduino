#ifndef weatherData_h
#define weatherData_h

#include "Arduino.h"

struct weatherData {
  char id[4];
  float temp1, temp2, hum, solar_radiation;
  float latitude, longitude, voltage, battery_voltage, amps;
  int rain, wind_speed;
  int signal_strength;
  long int pressure;
};

struct wtime {
  bool flag;
  uint8_t seconds = 0, minutes = 0, hours = 0, day = 0, month = 0;
  uint16_t year = 0;
};

void WeatherDataReset(weatherData &w);
void TimeDataReset(wtime &wt);
void PrintWeatherData(weatherData w);
void PrintTime(wtime wt);
void WeatherCheckIsNan(weatherData &w);

#endif
