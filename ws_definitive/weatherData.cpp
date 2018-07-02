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
}

void PrintWeatherData(weatherData w) {
  Serial.println("Humidity(%RH): " + String(w.hum));
  Serial.println("Temperature 1(C): " + String(w.temp1));
  Serial.println("Temperature 2(C): " + String(w.temp2));
  Serial.println("Panel Voltage: " + String(w.panel_voltage));
  Serial.println("Battery Voltage: " + String(w.battery_voltage));
  Serial.println("Current (Amps): " + String(w.amps));
  #if enable_BMP180
  Serial.println(", BMP: " + String(w.temp2)); 
  Serial.println("Pressure: " + String(w.pressure) + "atm"); 
  #endif
  Serial.println("Solar radiation(Voltage): " + String(float(w.solar_radiation) * 5.0 / 1023.0)); 
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
  while(t.seconds >= 60) { 
    t.seconds -= 60; 
    t.minutes++;
    if(t.minutes >= 60) { 
      t.minutes = 0; 
      t.hours++;
      if(t.hours >= 24) { 
        t.hours = 0; 
        t.day++;
        if((t.month == 1 || t.month == 3 || t.month == 5 || t.month == 7 || t.month == 8 || t.month == 10 || t.month == 12) && t.day > 31) { 
          t.day = 1; 
          t.month++; 
        }
        else if((t.month == 4 || t.month == 6 || t.month == 9 || t.month == 11) && t.day > 30) { 
          t.day = 1; 
          t.month++; 
        }
        else if(t.month == 2 && t.day > 28) { 
          t.day = 1; 
          t.month++; 
        }
        if(t.month > 12) {  
          t.month = 1; 
          t.year++; 
        } 
      } 
    } 
  }
}
