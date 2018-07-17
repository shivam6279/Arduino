#include "http.h"
#include <string.h>
#include "weatherData.h"
#include "GSM.h"
#include "settings.h"

bool SendHttpGet() {
  uint16_t timeout;
  char ch;

  ShowSerialData();
  Serial1.println("AT+QHTTPGET=20\r");
  delay(700);
  timeout = 0;
  do {
    if(Serial1.available()) {
      ch = Serial1.read();
      #if SERIAL_RESPONSE
        Serial.print(ch);
      #endif
    }
    timeout++;
    delay(1);
  }while(!isdigit(ch) && timeout < 20000);
  if(timeout >= 20000) return false;

  do {
    if(Serial1.available()) {
      ch = Serial1.read();
      #if SERIAL_RESPONSE
        Serial.print(ch);
      #endif
    }
    timeout++;
    delay(1);
  }while(!isAlpha(ch) && timeout < 20000);
  if(timeout >= 20000) return false;

  delay(100);
  ShowSerialData();
  delay(100);
  if(ch != 'O') return false;  
  return true;
}

bool HttpInit() {
  int timeout;
  GSMModuleWake();
  Serial1.println("AT+QIFGCNT=0\r");
  delay(100);
  ShowSerialData();
  Serial1.println("AT+QICSGP=1,\"CMNET\"\r");
  delay(100);
  ShowSerialData();
  Serial1.println("AT+QIREGAPP\r");
  delay(100);
  ShowSerialData();
  Serial1.println("AT+QIACT\r");
  for(timeout = 0; Serial1.available() < 2 && timeout < 100; timeout++) delay(100);
  delay(1000);
  ShowSerialData();
  if(timeout > 100) return false;
  return true;
}

bool UploadWeatherData(weatherData w[], uint8_t n, realTime &wt) {
  uint16_t read_length;
  uint16_t timeout;
  int i;
  if(!HttpInit()) return false;

  if(n < 1) n = 1;
  for(i = n - 1; i >= 0; i--) {
    if(!SendWeatherURL(w[i])) return false;
    ShowSerialData();  
    Serial1.println("AT+QHTTPREAD=30\r");
    delay(400);
    read_length = Serial1.available();
    if(i != 0) ShowSerialData();
  }
  //Read Time
  if(read_length > 70) {
    ReadTime(wt);
    return true;
  } else {
    return false;
  }
}

bool SendWeatherURL(weatherData w) {
  uint16_t timeout;
  char ch;
  int str_len;
  str_len = URL.length(); // URL string length
  str_len += String(w.id).length() + 4;
  str_len += String(w.temp1).length() + 4;
  str_len += String(w.temp2).length() + 4;
  str_len += String(w.hum).length() + 3;
  str_len += String(w.wind_speed).length() + 3;
  str_len += String(w.rain).length() + 3;
  str_len += String(w.pressure).length() + 3;
  str_len += String(w.amps).length() + 3;
  str_len += String(w.panel_voltage).length() + 3;
  str_len += String(w.battery_voltage).length() + 4;
  str_len += String(w.signal_strength).length() + 3;

  delay(1000);
  Serial1.print("AT+QHTTPURL=");
  Serial1.print(str_len);
  Serial1.print(",30\r");
  delay(1000);
  ShowSerialData();   
  
  Serial1.print(URL);   
  //Serial1.print(String(row_number + 1) + "=<"); 
  Serial1.print("id=" + String(w.id) + "&");
  if(w.t.flag == 1) {
    Serial1.print("ts=");
    Serial1.write((w.t.year / 1000) % 10 + '0');
    Serial1.write((w.t.year / 100) % 10 + '0');
    Serial1.write((w.t.year / 10) % 10 + '0');
    Serial1.write(w.t.year % 10 + '0');
    Serial1.write('-');
    Serial1.write((w.t.month / 10) % 10 + '0');
    Serial1.write(w.t.month % 10 + '0');
    Serial1.write('-');
    Serial1.write((w.t.day / 10) % 10 + '0');
    Serial1.write(w.t.day % 10 + '0');
    Serial1.write('T');
    Serial1.write((w.t.hours / 10) % 10 + '0');
    Serial1.write(w.t.hours % 10 + '0');
    Serial1.write(':');
    Serial1.write((w.t.minutes / 10) % 10 + '0');
    Serial1.write(w.t.minutes % 10 + '0');
    Serial1.write(':');
    Serial1.write((w.t.seconds / 10) % 10 + '0');
    Serial1.write(w.t.seconds % 10 + '0');
    Serial1.write('&');
  }    
  Serial1.print("t1=" + String(w.temp1) + "&");
  Serial1.print("t2=" + String(w.temp2) + "&");
  Serial1.print("h=" + String(w.hum) + "&");
  Serial1.print("w=" + String(w.wind_speed) + "&");
  Serial1.print("r=" + String(w.rain) + "&");
  Serial1.print("p=" + String(w.pressure) + "&");
  Serial1.print("s=" + String(w.amps) + "&");
  Serial1.print("v=" + String(w.panel_voltage) + "&");
  Serial1.print("bv=" + String(w.battery_voltage) + "&");
  Serial1.print("sg=" + String(w.signal_strength) + '\r');
  delay(300);
  
  ShowSerialData();  
  return SendHttpGet();
}

bool ReadTime(realTime &wt){
  int i, t;
  char ch, temp_time[8];
  do {
    do {
      ch = Serial1.read();
      #if SERIAL_RESPONSE
  	  Serial.print(ch);
  	  #endif
  	}while(ch != 'e');
  	ch = Serial1.read();
  	#if SERIAL_RESPONSE
	  Serial.print(ch);
  	#endif
  }while(ch != '(');
  //Year
  for(i = 0, ch = 0;; i++) {
    ch = Serial1.read();
    #if SERIAL_RESPONSE
    Serial.print(ch);
    #endif
    if(ch != ',') temp_time[i] = ch;
    else break;
  }
  for(i = i - 1, t = 1, wt.year = 0; i >= 0; i--, t *= 10){
  	wt.year += (temp_time[i] - '0') * t;
  }
  //Month
  for(i = 0, ch = Serial1.read();; i++) {
    ch = Serial1.read();
    #if SERIAL_RESPONSE
      Serial.print(ch);
    #endif
    if(ch != ',') temp_time[i] = ch;
    else break;
  }
  for(i = i - 1, t = 1, wt.month = 0; i >= 0; i--, t *= 10){
  	wt.month += (temp_time[i] - '0') * t;
  }
  //Day
  for(i = 0, ch = Serial1.read();; i++) {
    ch = Serial1.read();
    #if SERIAL_RESPONSE
      Serial.print(ch);
    #endif
    if(ch != ',') temp_time[i] = ch;
    else break;
  }
  for(i = i - 1, t = 1, wt.day = 0; i >= 0; i--, t *= 10){
  	wt.day += (temp_time[i] - '0') * t;
  }
  //Hour
  for(i = 0, ch = Serial1.read();; i++) {
    ch = Serial1.read();
    #if SERIAL_RESPONSE
      Serial.print(ch);
    #endif
    if(ch != ',') temp_time[i] = ch;
    else break;
  }
  for(i = i - 1, t = 1, wt.hours = 0; i >= 0; i--, t *= 10){
  	wt.hours += (temp_time[i] - '0') * t;
  }
  //Minutes
  for(i = 0, ch = Serial1.read();; i++) {
    ch = Serial1.read();
    #if SERIAL_RESPONSE
      Serial.print(ch);
    #endif
    if(ch != ',') temp_time[i] = ch;
    else break;
  }
  for(i = i - 1, t = 1, wt.minutes = 0; i >= 0; i--, t *= 10){
  	wt.minutes += (temp_time[i] - '0') * t;
  }
  //Seconds
  for(i = 0, ch = Serial1.read();; i++) {
    ch = Serial1.read();
    #if SERIAL_RESPONSE
      Serial.print(ch);
    #endif
    if(ch != ',') temp_time[i] = ch;
    else break;
  }
  for(i = i - 1, t = 1, wt.seconds = 0; i >= 0; i--, t *= 10){
  	wt.seconds += (temp_time[i] - '0') * t;
  }
  ShowSerialData();

  wt.flag = 1;
  return true;
}

bool GetTime(realTime &w) {
  uint8_t i, c;
  realTime t;
  if(!HttpInit()) return false;
  Serial1.print("AT+QHTTPURL=" + String(TIME_URL.length()) + ",30\r");
  delay(100);
  Serial1.print(TIME_URL + '\r');
  delay(500);
  ShowSerialData();
  if(!SendHttpGet()) return false;
  delay(200);
  ShowSerialData();
  Serial1.println("AT+QHTTPREAD=30\r");
  delay(800);
  for(i = 0, c = 0; c < 2 && i < 200; i++) {
    if(Serial1.read() == '\n')
      c++;
    delay(10);
  }
  if(i > 200 || Serial1.available() < 19) {
    ShowSerialData();
    return false;
  }
  t.year = (Serial1.read() - 48) * 1000; 
  t.year += (Serial1.read() - 48) * 100; 
  t.year += (Serial1.read() - 48) * 10; 
  t.year += (Serial1.read() - 48);
  Serial1.read();
  t.month = (Serial1.read() - 48) * 10; 
  t.month += (Serial1.read() - 48);
  Serial1.read();
  t.day = (Serial1.read() - 48) * 10; 
  t.day += (Serial1.read() - 48);
  Serial1.read();
  t.hours = (Serial1.read() - 48) * 10; 
  t.hours += (Serial1.read() - 48);
  Serial1.read();
  t.minutes = (Serial1.read() - 48) * 10; 
  t.minutes += (Serial1.read() - 48);
  Serial1.read();
  t.seconds = (Serial1.read() - 48) * 10; 
  t.seconds += (Serial1.read() - 48);
  ShowSerialData();
  t.flag = 1;
  w = t;
  return true;
}

void UploadSMS(weatherData w, String phone_number) {
  Serial1.print("AT+CMGS=\"" + phone_number + "\"\n");
  delay(100);
  Serial1.print("HK9D7 ");
  Serial1.print("'id':" + String(w.id) + ",");
  Serial1.print("'t1':" + String(w.temp1) + ",");
  Serial1.print("'t2':" + String(w.temp2) + ",");
  Serial1.print("'h':" + String(w.hum) + ",");
  Serial1.print("'w':" + String(w.wind_speed) + ",");
  Serial1.print("'r':" + String(w.rain) + ",");
  Serial1.print("'p':" + String(w.pressure) + ",");
  Serial1.print("'s':" + String(w.amps) + ",");
  Serial1.print("'sg':" + String(w.signal_strength));
  delay(100);
  Serial1.write(26);
  Serial1.write('\n');
  delay(100);
  Serial1.write('\n');
  ShowSerialData();
}
