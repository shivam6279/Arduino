#ifndef weatherData_h
#define weatherData_h

#include "Arduino.h"

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

class weatherData {
public:
  //Temperature and humidity
  int id;
  float temp1, temp2, hum;
  long int pressure;

  //Solar Radiation
  float solar_radiation;

  //Power
  float panel_voltage, battery_voltage, amps;

  //Tipping Bucket
  int rain;

  //Anemometer
  double wind_speed, davis_wind_speed;
  double wind_direction;
  double wind_stdDiv;

  //Signal Srength
  int signal_strength;

  //Time of data read
  realTime t;

  //Upload success flag
  bool flag;  

//public:
  void Reset(int);
  void CheckIsNan();
  void PrintData();
};

double GetWindDirection();
extern void AddTime(realTime, realTime, realTime&);
extern void SubtractTime(realTime, realTime, realTime&);
extern double ArrayAvg(double[], int);
extern double StdDiv(double[], int);
extern bool Debug();

#endif
