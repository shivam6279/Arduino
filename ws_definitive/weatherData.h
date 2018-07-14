#ifndef weatherData_h
#define weatherData_h

#include "Arduino.h"

extern char ws_id[4];

class realTime {
public:
  bool flag;
  uint16_t seconds, minutes, hours, day, month;
  uint16_t year;

  realTime() {
    seconds = 0;
    minutes = 0;
    hours = 0;
    day = 0;
    month = 0;
    year = 0;
    flag = 0;
  }
  void PrintTime();
  void HandleTimeOverflow();
  int DaysInMonth();
  bool CheckLeapYear();
};

struct weatherData {
public:
  float temp1, temp2, hum;
  float solar_radiation;
  float panel_voltage, battery_voltage, amps;
  int rain, wind_speed;
  int signal_strength;
  long int pressure;
  bool flag;
  realTime t;

//public:
  void Reset();
  void CheckIsNan();
  void PrintData();
};

extern void AddTime(realTime, realTime, realTime&);
extern void SubtractTime(realTime, realTime, realTime&);


#endif
