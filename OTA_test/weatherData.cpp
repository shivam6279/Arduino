#include "weatherData.h"
#include "SD.h"
#include "GSM.h"

void weatherData::Reset(int ws_id){
  id = ws_id;
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
  Serial.print(F("Temperature 1(C): ")); Serial.println(temp1);
  Serial.print(F("Temperature 2(C): ")); Serial.println(temp2);
  Serial.print(F("Humidity(%RH): ")); Serial.println(hum);
  Serial.print(F("Wind speed (m/s): ")); Serial.println(wind_speed);
  Serial.print(F("Standard deviation: ")); Serial.println(wind_stdDiv);
  Serial.print(F("Rain: ")); Serial.println(rain);
  Serial.print(F("Current(Amps): ")); Serial.println(amps);
  Serial.print(F("Panel Voltage(V): ")); Serial.println(panel_voltage);
  Serial.print(F("Battery Voltage(V): ")); Serial.println(battery_voltage);
  #if enable_BMP180
    Serial.print(F("Pressure: ")); Serial.println(pressure); 
  #endif
  Serial.print(F("Solar radiation(V): ")); Serial.println(float(solar_radiation) * 5.0 / 1023.0); 
  Serial.print(F("Signal Strength: ")); Serial.println(signal_strength); 
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

bool Debug() {
  char ch;
  SdFile datalog;

  while(Serial.available()) {
    ch = Serial.peek();
    //Talk
    if(ch == 'T') {
      if(Serial.available() < 4)
        return false;
      else {
        Serial.read();
        if(Serial.read() == 'a') {
          if(Serial.read() == 'l') {
            if(Serial.read() == 'k') {
              Talk();
              return true;
            }
          }
        }
      }
    }

    //Datalog.txt
    else if(ch == 'd') {
      if(Serial.available() < 11)
        return false;
      else {
        Serial.read();
        if(Serial.read() == 'a') {
          if(Serial.read() == 't') {
            if(Serial.read() == 'a') {
              if(Serial.read() == 'l') {
                if(Serial.read() == 'o') {
                  if(Serial.read() == 'g') {
                    if(Serial.read() == '.') {
                      if(Serial.read() == 'c') {
                        if(Serial.read() == 's') {
                          if(Serial.read() == 'v') {
                            Serial.println();
                            if(!sd.exists("datalog.csv")) {
                              Serial.println("datalog.csv not found");
                              return false;
                            }
                            Serial.println("Displaying datalog.csv");
                            if(!datalog.open("datalog.csv", FILE_READ)) {
                              datalog.close();
                              return false;
                            }
                            while(datalog.available()) {
                              Serial.write(datalog.read());
                            }
                            datalog.close();
                            Serial.println();
                            return true;
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    //id.txt
    else if(ch == 'i') {
      if(Serial.available() < 6)
        return false;
      else {
        Serial.read();
        if(Serial.read() == 'd') {
          if(Serial.read() == '.') {
            if(Serial.read() == 't') {
              if(Serial.read() == 'x') {
                if(Serial.read() == 't') {
                  Serial.println();
                  if(!sd.exists("id.txt")) {
                    Serial.println("id.txt not found");
                    return false;
                  }
                  Serial.println("Displaying id.txt");
                  if(!datalog.open("id.txt", FILE_READ)) {
                    datalog.close();
                    return false;
                  }
                  while(datalog.available()) {
                    Serial.write(datalog.read());
                  }
                  datalog.close();
                  Serial.println();
                  return true;
                }
              }
            }
          }
        }
      }
    }
    else {
      Serial.read();
    }
  }
  
  return false;
}