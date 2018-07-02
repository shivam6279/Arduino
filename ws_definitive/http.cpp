#include "http.h"
#include <string.h>
#include "weatherData.h"
#include "GSM.h"
#include "settings.h"

bool SubmitHttpRequest(weatherData w[], uint8_t n, real_time &wt) {
  uint8_t read_length;
  uint16_t timeout;
  int str_len, i;
  char t[4];
  
  #if SERIAL_OUTPUT
  Serial.println("Uploading data");
  #endif
  while(Serial1.available()) Serial1.read();
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
  delay(2000);
  ShowSerialData();
  while(Serial1.available()) Serial1.read();

  for(i = n - 1; i >= 0; i--) {
    //String length of HTTP request
    str_len = URL.length(); // URL string length
    str_len += String(ws_id).length() + 4;
    str_len += String(w[i].temp1).length() + 4;
    str_len += String(w[i].temp2).length() + 4;
    str_len += String(w[i].hum).length() + 3;
    str_len += String(w[i].wind_speed).length() + 3;
    str_len += String(w[i].rain).length() + 3;
    str_len += String(w[i].pressure).length() + 3;
    str_len += String(w[i].amps).length() + 3;
    str_len += String(w[i].panel_voltage).length() + 3;
    str_len += String(w[i].battery_voltage).length() + 4;
    str_len += String(w[i].signal_strength).length() + 3;
  
    t[0] = ((str_len / 100) % 10) + 48;
    t[1] = ((str_len / 10) % 10) + 48;
    t[2] = (str_len % 10) + 48;
    t[3] = '\0';
    Serial1.print("AT+QHTTPURL=");
    Serial1.print(t);
    Serial1.print(",30\r");
    delay(1000);
    ShowSerialData();   
    
    Serial1.print(URL);   
    //Serial1.print(String(row_number + 1) + "=<"); 
    Serial1.print("id=" + String(ws_id) + "&");
    Serial1.print("t1=" + String(w[i].temp1) + "&");
    Serial1.print("t2=" + String(w[i].temp2) + "&");
    Serial1.print("h=" + String(w[i].hum) + "&");
    Serial1.print("w=" + String(w[i].wind_speed) + "&");
    Serial1.print("r=" + String(w[i].rain) + "&");
    Serial1.print("p=" + String(w[i].pressure) + "&");
    Serial1.print("s=" + String(w[i].amps) + "&");
    Serial1.print("v=" + String(w[i].panel_voltage) + "&");
    Serial1.print("bv=" + String(w[i].battery_voltage) + "&");
    Serial1.print("sg=" + String(w[i].signal_strength) + '\r');
    delay(300);
    
    ShowSerialData();  
    Serial1.println("AT+QHTTPGET=30\r");
    delay(100);
    ShowSerialData();  
    for(timeout = 0; Serial1.available() < 3 && timeout < 200; timeout++) delay(100);
    if(timeout > 200) return 0;
    
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
