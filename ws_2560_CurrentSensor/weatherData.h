#ifndef weatherData_h
#define weatherData_h

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

void WeatherDataReset(weatherData &w){
  w.temp1 = 0.0;
  w.temp2 = 0.0;
  w.hum = 0.0;
  w.solar_radiation = 0.0;
  w.latitude = 0.0;
  w.longitude = 0.0;
  w.voltage = 0.0;
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
  Serial.println("Voltage: " + String(w.voltage));
  Serial.println("Battery Voltage: " + String(w.battery_voltage));
  Serial.println("Current (Amps): " + String(w.amps));
  #if enable_BMP180
  Serial.println(", BMP: " + String(w.temp2)); 
  Serial.println("Pressure: " + String(w.pressure) + "atm"); 
  #endif
  Serial.println("Solar radiation(Voltage): " + String(float(w.solar_radiation) * 5.0 / 1023.0)); 
  Serial.println();
}

void TimeDataReset(wtime &wt){
  wt.flag = 0;
  wt.year  = 0;
  wt.month = 0;
  wt.day = 0;
  wt.hours = 0;
  wt.minutes = 0;
  wt.seconds = 0;
}

void PrintTime(wtime wt){
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
  if(isnan(w.voltage)) w.voltage = 0;
  if(isnan(w.battery_voltage)) w.battery_voltage = 0;
  if(isnan(w.amps)) w.amps = 0;
  if(isnan(w.pressure)) w.pressure = 0;
}

#endif
