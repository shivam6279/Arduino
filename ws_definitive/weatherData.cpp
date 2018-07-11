#include "weatherData.h"

char ws_id[4];

void WeatherDataReset(weatherData &w){
  w.temp1 = 0.0;
  w.temp2 = 0.0;
  w.hum = 0.0;
  w.solar_radiation = 0.0;
  w.latitude = 0.0;
  w.longitude = 0.0;
  w.panel_voltage = 0.0;
  w.battery_voltage = 0.0;
  w.amps = 0.0;
  w.rain = 0;
  w.wind_speed = 0;
  w.signal_strength = 0;
  w.pressure = 0;
  w.flag = 0;
}

void PrintWeatherData(weatherData w) {
  Serial.println("Temperature 1(C): " + String(w.temp1));
  Serial.println("Temperature 2(C): " + String(w.temp2));
  Serial.println("Humidity(%RH): " + String(w.hum));
  Serial.println("Wind: " + String(w.wind_speed));
  Serial.println("Rain: " + String(w.rain));
  Serial.println("Current (Amps): " + String(w.amps));
  Serial.println("Panel Voltage(V): " + String(w.panel_voltage));
  Serial.println("Battery Voltage(V): " + String(w.battery_voltage));
  #if enable_BMP180
    Serial.println("Pressure: " + String(w.pressure) + "atm"); 
  #endif
  Serial.println("Solar radiation(V): " + String(float(w.solar_radiation) * 5.0 / 1023.0)); 
  Serial.println("Signal Strength: " + String(w.signal_strength)); 
}

void TimeDataReset(real_time &wt){
  wt.flag = 0;
  wt.year  = 0;
  wt.month = 0;
  wt.day = 0;
  wt.hours = 0;
  wt.minutes = 0;
  wt.seconds = 0;
}

void PrintTime(real_time wt) {
	Serial.println("\nDate and time:");
	Serial.print(wt.day / 10); Serial.print(wt.day % 10);
	Serial.print('/');
	Serial.print(wt.month / 10); Serial.print(wt.month % 10); 
	Serial.print('/');
	Serial.println(wt.year);
	Serial.print(wt.hours / 10); Serial.print(wt.hours % 10);
	Serial.print(':');
	Serial.print(wt.minutes / 10); Serial.print(wt.minutes % 10);
	Serial.print(':');
	Serial.print(wt.seconds / 10); Serial.println(wt.seconds % 10);
}

void WeatherCheckIsNan(weatherData &w) {
  if(isnan(w.hum)) w.hum = 0;
  if(isnan(w.temp1)) w.temp1 = 0;
  if(isnan(w.temp2)) w.temp2 = 0;
  if(isnan(w.solar_radiation)) w.solar_radiation = 0;
  if(isnan(w.panel_voltage)) w.panel_voltage = 0;
  if(isnan(w.battery_voltage)) w.battery_voltage = 0;
  if(isnan(w.amps)) w.amps = 0;
  if(isnan(w.pressure)) w.pressure = 0;
}

void HandleTimeOverflow(real_time &t) {
  t.minutes += t.seconds / 60;
  t.seconds = t.seconds % 60;

  t.hours += t.minutes / 60;
  t.minutes = t.minutes % 60;

  t.day += t.hours / 24;
  t.hours = t.hours % 24;

  if(t.month && t.flag) {
    while(t.day > DaysInMonth(t.month, t.year)) { 
      t.day -= DaysInMonth(t.month, t.year);
      t.month++; 
      if(t.month > 12) {
        t.month = 1;
        t.year++;
      }
    }
  }
}

void AddTime(real_time a, real_time b, real_time &c) {
  c.seconds = a.seconds + b.seconds;
  c.minutes = a.minutes + b.minutes;
  c.hours = a.hours + b.hours;
  
  c.day = a.day + b.day;
  c.month = a.month + b.month;
  c.year = a.year + b.year;

  HandleTimeOverflow(c);
}

//a is a calendar date, b is the duration between c and a
void SubtractTime(real_time a, real_time b, real_time &c) {
  HandleTimeOverflow(a);

  c.seconds = 0;
  c.minutes = 0;
  c.hours = 0;
  
  c.day = 0;
  c.month = 0;
  c.hours = 0;

  c.flag = a.flag;

  int temp;

  if(a.seconds < b.seconds) {
    if(a.minutes == 0) {
      if(a.hours == 0) {
        if(a.day == 1) {
          if(a.month == 1) {
            a.day += DaysInMonth(a.month - 1, a.year);
            a.year--;
            a.month = 12;
          } else {
            a.day += DaysInMonth(a.month - 1, a.year);
            a.month--;
          }
        } 
        a.day--;
        a.hours += 24;
      } 
      a.hours--;
      a.minutes += 60;
    }
    a.minutes--;
    a.seconds += 60;
  } 
  c.seconds = a.seconds - b.seconds;

  if(a.minutes < b.minutes) {
    if(a.hours == 0) {
      if(a.day == 1) {
        if(a.month == 1) {
          a.day += DaysInMonth(a.month - 1, a.year);
          a.year--;
          a.month = 12;
        } else {
          a.day += DaysInMonth(a.month - 1, a.year);
          a.month--;
        }
      } 
      a.day--;
      a.hours += 24;
    } 
    a.hours--;
    a.minutes += 60;
  }
  c.minutes = a.minutes - b.minutes;

  if(a.hours < b.hours) {
    if(a.day == 1) {
      if(a.month == 1) {
        a.day += DaysInMonth(a.month - 1, a.year);
        a.year--;
        a.month = 12;
      } else {
        a.day += DaysInMonth(a.month - 1, a.year);
        a.month--;
      }
    } 
    a.day--;
    a.hours += 24;
  } 
  c.hours = a.hours - b.hours;

  while(a.day <= b.day) {
    if(a.month == 1) {
      a.day += DaysInMonth(a.month - 1, a.year);
      a.year--;
      a.month = 12;
    } else {
      a.day += DaysInMonth(a.month - 1, a.year);
      a.month--;
    }
  } 
  c.day = a.day - b.day;

  c.month = a.month;
  c.year = a.year;
}

int DaysInMonth(int month, int year) {
  if(month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) 
    return 31;
  else if(month == 4 || month == 6 || month == 9 || month == 11) 
    return 30;
  else if(month == 2) {
    if(CheckLeapYear(year)) 
      return 29;
    else
      return 28;
  }
}

bool CheckLeapYear(int year) {
    if (year % 400 == 0)
        return true;

    if (year % 100 == 0)
        return false;

    if (year % 4 == 0)
        return true;

    return false;
}
