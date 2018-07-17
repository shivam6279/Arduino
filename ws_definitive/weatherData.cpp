#include "weatherData.h"

char ws_id[4];

void weatherData::Reset(){
  temp1 = 0.0;
  temp2 = 0.0;
  hum = 0.0;
  solar_radiation = 0.0;
  panel_voltage = 0.0;
  battery_voltage = 0.0;
  amps = 0.0;
  rain = 0;
  wind_speed = 0;
  signal_strength = 0;
  pressure = 0;
  flag = 0;
}

void weatherData::CheckIsNan() {
  if(isnan(hum)) 
    hum = 0.0;
  if(isnan(temp1)) 
    temp1 = 0.0;
  if(isnan(temp2)) 
    temp2 = 0.0;
  if(isnan(solar_radiation)) 
    solar_radiation = 0.0;
  if(isnan(panel_voltage)) 
    panel_voltage = 0.0;
  if(isnan(battery_voltage)) 
    battery_voltage = 0.0;
  if(isnan(amps)) 
    amps = 0.0;
  if(isnan(wind_speed)) 
    wind_speed = 0.0;
  if(isnan(wind_stdDiv)) 
    wind_stdDiv = 0.0;
  if(isnan(pressure)) 
    pressure = 0;
  if(isnan(signal_strength)) 
    signal_strength = 0.0;
}

void weatherData::PrintData() {
  Serial.println("Temperature 1(C): " + String(temp1));
  Serial.println("Temperature 2(C): " + String(temp2));
  Serial.println("Humidity(%RH): " + String(hum));
  Serial.println("Wind: " + String(wind_speed));
  Serial.println("Standard deviation: " + String(wind_stdDiv));
  Serial.println("Rain: " + String(rain));
  Serial.println("Current (Amps): " + String(amps));
  Serial.println("Panel Voltage(V): " + String(panel_voltage));
  Serial.println("Battery Voltage(V): " + String(battery_voltage));
  #if enable_BMP180
    Serial.println("Pressure: " + String(pressure) + "atm"); 
  #endif
  Serial.println("Solar radiation(V): " + String(float(solar_radiation) * 5.0 / 1023.0)); 
  Serial.println("Signal Strength: " + String(signal_strength)); 
}

void realTime::PrintTime() {
	Serial.print(day / 10); 
  Serial.print(day % 10);
	Serial.print('/');
	Serial.print(month / 10); 
  Serial.print(month % 10); 
	Serial.print('/');
	Serial.println(year);
	Serial.print(hours / 10); 
  Serial.print(hours % 10);
	Serial.print(':');
	Serial.print(minutes / 10); 
  Serial.print(minutes % 10);
	Serial.print(':');
	Serial.print(seconds / 10); 
  Serial.println(seconds % 10);
}

void realTime::HandleTimeOverflow() {
  minutes += seconds / 60;
  seconds = seconds % 60;

  hours += minutes / 60;
  minutes = minutes % 60;

  day += hours / 24;
  hours = hours % 24;

  if(month && flag) {
    while(day > DaysInMonth()) { 
      day -= DaysInMonth();
      month++; 
      if(month > 12) {
        month = 1;
        year++;
      }
    }
  }
}

int realTime::DaysInMonth() {
  if(month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) 
    return 31;
  else if(month == 4 || month == 6 || month == 9 || month == 11) 
    return 30;
  else if(month == 2) {
    if(CheckLeapYear()) 
      return 29;
    else
      return 28;
  }
  return 0;
}

bool realTime::CheckLeapYear() {
    if (year % 400 == 0)
        return true;

    if (year % 100 == 0)
        return false;

    if (year % 4 == 0)
        return true;

    return false;
}

void AddTime(realTime a, realTime b, realTime &c) {
  c.seconds = a.seconds + b.seconds;
  c.minutes = a.minutes + b.minutes;
  c.hours = a.hours + b.hours;
  
  c.day = a.day + b.day;
  c.month = a.month + b.month;
  c.year = a.year + b.year;

  c.flag = a.flag | b.flag;

  c.HandleTimeOverflow();
}

//a is a calendar date, b is the duration between c and a
void SubtractTime(realTime a, realTime b, realTime &c) {
  a.HandleTimeOverflow();

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
            a.month = 12;
            a.day += a.DaysInMonth();
            a.year--;
            a.month = 12;
          } else {
            a.month--;
            a.day += a.DaysInMonth();
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
          a.month = 12;
          a.day += a.DaysInMonth();
          a.year--;
          a.month = 12;
        } else {
          a.month--;
          a.day += a.DaysInMonth();
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
        a.month = 12;
        a.day += a.DaysInMonth();
        a.year--;
        a.month = 12;
      } else {
        a.month--;
        a.day += a.DaysInMonth();
      }
    } 
    a.day--;
    a.hours += 24;
  } 
  c.hours = a.hours - b.hours;

  while(a.day <= b.day) {
    if(a.month == 1) {
      a.month = 12;
      a.day += a.DaysInMonth();
      a.year--;
      a.month = 12;
    } else {
      a.month--;
      a.day += a.DaysInMonth();
    }
  } 
  c.day = a.day - b.day;

  c.month = a.month;
  c.year = a.year;
}

double ArrayAvg(double a[], int n) {
  double avg;
  int i;

  if(n < 0) 
    return 0.0;

  for(i = 0, avg = 0.0; i < n; i++) {
    avg += a[i];
  }
  avg /= double(n);

  return avg;
}

double StdDiv(double a[], int n) {
  double avg, r;
  int i;

  if(n < 0) 
    return 0.0;

  avg = ArrayAvg(a, n);

  for(i = 0, r = 0.0; i < n; i++) {
    r += (a[i] - avg) * (a[i] - avg);
  }
  r = sqrt(r / double(n));
  return r;
}
