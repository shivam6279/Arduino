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

bool UploadWeatherData(weatherData w[], uint8_t n, real_time &wt) {
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
  char t[4], ch;
  int str_len;
  str_len = URL.length(); // URL string length
  str_len += String(ws_id).length() + 4;
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

  t[0] = ((str_len / 100) % 10) + 48;
  t[1] = ((str_len / 10) % 10) + 48;
  t[2] = (str_len % 10) + 48;
  t[3] = '\0';
  delay(1000);
  Serial1.print("AT+QHTTPURL=");
  Serial1.print(t);
  Serial1.print(",30\r");
  delay(1000);
  ShowSerialData();   
  
  Serial1.print(URL);   
  //Serial1.print(String(row_number + 1) + "=<"); 
  Serial1.print("id=" + String(ws_id) + "&");
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

bool ReadTime(real_time &wt){
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
  #if SERIAL_RESPONSE
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif

  wt.flag = 1;
  return true;
}

bool GetTime(real_time &w) {
  uint8_t i;
  HttpInit();
  Serial1.print("AT+QHTTPURL=24,30\r");
  delay(100);
  Serial1.print("http://www.yobi.tech/IST\r");
  delay(500);
  ShowSerialData();
  if(!SendHttpGet()) return false;
  ShowSerialData();
  for(i = 0; Serial1.available() < 3 && i < 200; i++) delay(100);
  ShowSerialData();
  Serial1.println("AT+QHTTPREAD=30\r");
  delay(600);
  for(i = 0; Serial1.read() != '\n' && i < 200; i++) delay(100);
  for(i = 0; Serial1.read() != '\n' && i < 200; i++) delay(100);
  w.year = (Serial1.read() - 48) * 1000; w.year += (Serial1.read() - 48) * 100; w.year += (Serial1.read() - 48) * 10; w.year += (Serial1.read() - 48);
  Serial1.read();
  w.month = (Serial1.read() - 48) * 10; w.month += (Serial1.read() - 48);
  Serial1.read();
  w.day = (Serial1.read() - 48) * 10; w.day += (Serial1.read() - 48);
  Serial1.read();
  w.hours = (Serial1.read() - 48) * 10; w.hours += (Serial1.read() - 48);
  Serial1.read();
  w.minutes = (Serial1.read() - 48) * 10; w.minutes += (Serial1.read() - 48);
  Serial1.read();
  w.seconds = (Serial1.read() - 48) * 10; w.seconds += (Serial1.read() - 48);
  ShowSerialData();
  return true;
}

void UploadSMS(weatherData w, String phone_number) {
  Serial1.print("AT+CMGS=\"" + phone_number + "\"\n");
  delay(100);
  Serial1.print("HK9D7 ");
  Serial1.print("'id':" + String(ws_id) + ",");
  Serial1.print("'t1':" + String(w.temp1) + ",");
  Serial1.print("'t2':" + String(w.temp2) + ",");
  Serial1.print("'h':" + String(w.hum) + ",");
  Serial1.print("'w':" + String(w.wind_speed) + ",");
  Serial1.print("'r':" + String(w.rain) + ",");
  Serial1.print("'p':" + String(w.pressure) + ",");
  Serial1.print("'s':" + String(w.amps) + ",");
  Serial1.print("'lt':" + String(w.latitude) + ",");
  Serial1.print("'ln':" + String(w.longitude) + ",");
  Serial1.print("'sg':" + String(w.signal_strength));
  delay(100);
  Serial1.write(26);
  Serial1.write('\n');
  delay(100);
  Serial1.write('\n');
  ShowSerialData();
}