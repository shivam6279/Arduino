#include "http.h"
#include <string.h>
#include "weatherData.h"
#include "GSM.h"
#include "settings.h"

bool SubmitHttpRequest(weatherData w, wtime &wt) {
  uint8_t j;
  uint16_t i;
  int str_len;
  char t[4];
  
  #if SERIAL_OUTPUT
  Serial.println("Uploading data");
  #endif
  while(Serial1.available()) Serial1.read();
  Serial1.println("AT+QIFGCNT=0\r");
  delay(100);
  #if SERIAL_RESPONSE
  ShowSerialData();
  #endif
  Serial1.println("AT+QICSGP=1,\"CMNET\"\r");
  delay(100);
  #if SERIAL_RESPONSE
  ShowSerialData();
  #endif
  Serial1.println("AT+QIREGAPP\r");
  delay(100);
  #if SERIAL_RESPONSE
  ShowSerialData();
  #endif
  Serial1.println("AT+QIACT\r");
  delay(2000);
  #if SERIAL_RESPONSE
  ShowSerialData();
  #endif
  while(Serial1.available()) Serial1.read();

  //String length of HTTP request
  str_len = 66; // URL string length
  str_len += String(w.id).length() + 4;
  str_len += String(w.temp1).length() + 4;
  str_len += String(w.temp2).length() + 4;
  str_len += String(w.hum).length() + 3;
  str_len += String(w.wind_speed).length() + 3;
  str_len += String(w.rain).length() + 3;
  str_len += String(w.pressure).length() + 3;
  str_len += String(w.amps).length() + 3;
  str_len += String(w.voltage).length() + 3;
  str_len += String(w.battery_voltage).length() + 4;
  str_len += String(w.signal_strength).length() + 3;

  t[0] = ((str_len / 100) % 10) + 48;
  t[1] = ((str_len / 10) % 10) + 48;
  t[2] = (str_len % 10) + 48;
  t[3] = '\0';
  Serial1.print("AT+QHTTPURL=");
  Serial1.print(t);
  Serial1.print(",30\r");
  delay(1000);
  #if SERIAL_RESPONSE
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif    
  
  Serial1.print("http://enigmatic-caverns-27645.herokuapp.com/maytheforcebewithyou?");   
  //Serial1.print(String(row_number + 1) + "=<"); 
  Serial1.print("id=" + String(w.id) + "&");
  Serial1.print("t1=" + String(w.temp1) + "&");
  Serial1.print("t2=" + String(w.temp2) + "&");
  Serial1.print("h=" + String(w.hum) + "&");
  Serial1.print("w=" + String(w.wind_speed) + "&");
  Serial1.print("r=" + String(w.rain) + "&");
  Serial1.print("p=" + String(w.pressure) + "&");
  Serial1.print("s=" + String(w.amps) + "&");
  Serial1.print("v=" + String(w.voltage) + "&");
  Serial1.print("bv=" + String(w.battery_voltage) + "&");
  Serial1.print("sg=" + String(w.signal_strength) + '\r');
  delay(300);
  
  #if SERIAL_RESPONSE
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif   
  Serial1.println("AT+QHTTPGET=30\r");
  delay(100);
  #if SERIAL_RESPONSE
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif   
  for(i = 0; Serial1.available() < 3 && i < 200; i++) delay(100);
  //delay(1000);
  #if SERIAL_RESPONSE
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif   
  Serial1.println("AT+QHTTPREAD=30\r");
  delay(400);
  j = Serial1.available();

  //Read Time
  if(j > 70 && i < 200) {
  	ReadTime(wt);
  	return true;
  }
  else {
  	#if SERIAL_RESPONSE
  	ShowSerialData();
  	#else
  	while(Serial1.available()) Serial1.read();
  	#endif
  	return false;
  }
}

bool ReadTime(wtime &wt){
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

bool GetTime(wtime &w) {
  uint8_t i;
  while(Serial1.available()) Serial1.read();
  Serial1.print("AT+QHTTPURL=24,30\r");
  delay(100);
  Serial1.print("http://www.yobi.tech/IST\r");
  delay(500);
  while(Serial1.available()) Serial1.read();
  Serial1.println("AT+QHTTPGET=30\r");
  delay(100);
  while(Serial1.available()) Serial1.read();
  for(i = 0; Serial1.available() < 3 && i < 200; i++) delay(100);
  while(Serial1.available()) Serial1.read();
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
  while(Serial1.available()) Serial1.read();
  #if SERIAL_OUTPUT
  Serial.println("Date");
  Serial.write((w.day / 10) % 10 + 48); Serial.write((w.day % 10) + 48);
  Serial.write('/');
  Serial.write((w.month / 10) % 10 + 48); Serial.write((w.month % 10) + 48);
  Serial.write('/');
  Serial.write((w.year / 1000) % 10 + 48); Serial.write((w.year / 100) % 10 + 48); Serial.write((w.year / 10) % 10 + 48); Serial.write((w.year % 10) + 48);
  Serial.println("\nTime");
  Serial.write((w.hours / 10) % 10 + 48); Serial.write((w.hours % 10) + 48);
  Serial.write(':');
  Serial.write((w.minutes / 10) % 10 + 48); Serial.write((w.minutes % 10) + 48);
  Serial.write(':');
  Serial.write((w.seconds / 10) % 10 + 48); Serial.write((w.seconds % 10) + 48);
  Serial.println();
  #endif
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
  Serial1.print("'lt':" + String(w.latitude) + ",");
  Serial1.print("'ln':" + String(w.longitude) + ",");
  Serial1.print("'sg':" + String(w.signal_strength));
  delay(100);
  Serial1.write(26);
  Serial1.write('\n');
  delay(100);
  Serial1.write('\n');
  #if SERIAL_RESPONSE
  ShowSerialData();
  #endif
}
