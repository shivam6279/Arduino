#include "http.h"
#include <string.h>
#include "weatherData.h"
#include "GSM.h"
#include "settings.h"
#include "SD.h"

bool HttpInit() {
  int timeout;
  GSMModuleWake();
  SendATCommand("AT+QIFGCNT=0", "OK", 500);
  GSMReadUntil("\n", 50); ShowSerialData();
  SendATCommand("AT+QICSGP=1,\"CMNET\"", "OK", 500);
  GSMReadUntil("\n", 50); ShowSerialData();
  SendATCommand("AT+QIREGAPP", "OK", 500);
  GSMReadUntil("\n", 50); ShowSerialData();
  if(SendATCommand("AT+QIACT", "OK", 30000) == -1) {
    GSMReadUntil("\n", 50); ShowSerialData();
    return false;
  }
  GSMReadUntil("\n", 50); 
  return true;
}

bool UploadWeatherData(weatherData w[], int n, realTime &wt) {
  uint16_t timeout;
  int i;
  if(!HttpInit()) 
    return false;

  if(n < 1) 
    n = 1;

#ifdef USE_HEROKU_URL
  for(i = n - 1; i >= 0; i--) {
    if(!SendWeatherURL(w[i])) 
      return false;
    ShowSerialData();
    delay(100); 
    if(SendATCommand("AT+QHTTPREAD=30", "OK", 5000) == -1) {
      GSMReadUntil("\n", 50); 
      return false;
    }
    GSMReadUntil("\n", 50); 
    ShowSerialData();
  }
  //Read Time
  ReadTime(wt);
  return true;
#endif
#ifdef USE_CWIG_URL
  while(n > 0) {
    if(!SendWeatherURL(w[0])) 
      return false;
    ShowSerialData();
    delay(100); 
    if(SendATCommand("AT+QHTTPREAD=30", "OK", 5000) == -1) {
      GSMReadUntil("\n", 50); 
      return false;
    }
    ShowSerialData();

    for(i = 0; i < BUFFER_SIZE - 1; i++) w[i] = w[i + 1];
    n--;
  }
  //Read Time
//  ReadTimeResponse_NCML(wt);
  return true;
#endif
}

bool SendWeatherURL(weatherData w) {
  uint16_t timeout;
  char ch;
  int str_len;

  #ifdef USE_HEROKU_URL 
    str_len = URL_HEROKU.length(); // URL string length
    str_len += String(w.id).length() + 4;
    str_len += String(w.temp).length() + 4;
    str_len += String(w.hum).length() + 3;
    str_len += String(w.wind_speed).length() + 3;
    str_len += String(w.rain).length() + 3;
    str_len += String(w.pressure).length() + 3;
    str_len += String(w.amps).length() + 3;
    str_len += String(w.panel_voltage).length() + 3;
    str_len += String(w.battery_voltage).length() + 4;
    str_len += String(w.signal_strength).length() + 3;
  
    delay(1000);
    GSMModuleWake();
    
    Serial1.print("AT+QHTTPURL=");
    Serial1.print(str_len);
    Serial1.print(",30\r");
    delay(1000);
    ShowSerialData();   
    
    Serial1.print(URL_HEROKU);   
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
      //Serial1.write((w.t.seconds / 10) % 10 + '0');
      //Serial1.write(w.t.seconds % 10 + '0');
      Serial1.write('0');
      Serial1.write('0');
      Serial1.write('&');
    }    
    Serial1.print("t1=" + String(w.temp) + "&");
    Serial1.print("h=" + String(w.hum) + "&");
    Serial1.print("w=" + String(w.wind_speed) + "&");
    Serial1.print("r=" + String(w.rain) + "&");
    Serial1.print("p=" + String(w.pressure) + "&");
    Serial1.print("s=" + String(w.amps) + "&");
    Serial1.print("v=" + String(w.panel_voltage) + "&");
    Serial1.print("bv=" + String(w.battery_voltage) + "&");
    Serial1.print("sg=" + String(w.signal_strength) + '\r');
    delay(300);
  #endif
  
  #ifdef USE_CWIG_URL
    str_len = URL_CWIG.length() + 4; // URL string length
    str_len += String(w.imei).length() + 1;
    str_len += String(w.temp_max).length() + 1;
    str_len += String(w.temp).length() + 1;
    str_len += String(w.temp_min).length() + 1;
    str_len += String(w.hum_max).length() + 1;
    str_len += String(w.hum).length() + 1;
    str_len += String(w.hum_min).length() + 1;
    str_len += 24;
    str_len += String(w.wind_speed_min).length() + 1;
    str_len += String(w.wind_speed).length() + 1;
    str_len += String(w.wind_speed_max).length() + 1;
    str_len += String(w.rain).length() + 1 + 2 + 2;

    str_len += String(w.wind_direction).length() + 1;
    str_len += String(w.panel_voltage).length() + 1;
    str_len += String(w.battery_voltage).length();
    if(w.t.flag == 1) {
      str_len += 14;
      if(w.t.month >= 10)
        str_len ++;
    } else {
      str_len += 2;
    }
    delay(1000);
    GSMModuleWake();
    
    Serial1.print("AT+QHTTPURL=");
    Serial1.print(str_len);
    Serial1.print(",30\r");
    delay(1000);
    ShowSerialData();   
    
    Serial1.print(URL_CWIG + "\"<"); 
    Serial1.print(w.imei + ";");
    
    if(w.t.flag == 1) {
      Serial1.write((w.t.day / 10) % 10 + '0');
      Serial1.write(w.t.day % 10 + '0');
      Serial1.write('/');
      if(w.t.month >= 10)
        Serial1.write((w.t.month / 10) % 10 + '0');
      Serial1.write(w.t.month % 10 + '0');
      Serial1.write('/');
      Serial1.write((w.t.year / 10) % 10 + '0');
      Serial1.write(w.t.year % 10 + '0');
      Serial1.write(';');
      Serial1.write((w.t.hours / 10) % 10 + '0');
      Serial1.write(w.t.hours % 10 + '0');
      Serial1.write(':');
      Serial1.write((w.t.minutes / 10) % 10 + '0');
      Serial1.write(w.t.minutes % 10 + '0');
      Serial1.write(';');
    } else {
      Serial1.print("0;");      
    }
    Serial1.print(String(w.temp_max) + ";");
    Serial1.print(String(w.temp) + ";");
    Serial1.print(String(w.temp_min) + ";");
    
    Serial1.print(String(w.hum_max) + ";");
    Serial1.print(String(w.hum) + ";");
    Serial1.print(String(w.hum_min) + ";");

    Serial1.print(String(w.rain) + ";");
    Serial1.print("0;");
    Serial1.print("0;");

    Serial1.print("0;0;0;0;0;0;0;0;0;0;0;0;");

    Serial1.print(String(w.wind_speed_max) + ";");
    Serial1.print(String(w.wind_speed) + ";");
    Serial1.print(String(w.wind_speed_min) + ";");

    Serial1.print(String(w.wind_direction) + ";");
    Serial1.print(String(w.panel_voltage) + ";");
    Serial1.print(String(w.battery_voltage) + ">\"\r");
    delay(300);

    Serial.print(URL_CWIG + "\"<"); 
    Serial.print(w.imei + ";");
    
    if(w.t.flag == 1) {
      Serial.write((w.t.year / 1000) % 10 + '0');
      Serial.write((w.t.year / 100) % 10 + '0');
      Serial.write((w.t.year / 10) % 10 + '0');
      Serial.write(w.t.year % 10 + '0');
      Serial.write('/');
      Serial.write((w.t.month / 10) % 10 + '0');
      Serial.write(w.t.month % 10 + '0');
      Serial.write('/');
      Serial.write((w.t.day / 10) % 10 + '0');
      Serial.write(w.t.day % 10 + '0');
      Serial.write(';');
      Serial.write((w.t.hours / 10) % 10 + '0');
      Serial.write(w.t.hours % 10 + '0');
      Serial.write(':');
      Serial.write((w.t.minutes / 10) % 10 + '0');
      Serial.write(w.t.minutes % 10 + '0');
      Serial.write(':');
      //Serial1.write((w.t.seconds / 10) % 10 + '0');
      //Serial1.write(w.t.seconds % 10 + '0');
      Serial.write('0');
      Serial.write('0');
      Serial.write(';');
    } else {
      Serial.print("0;");      
    }
    Serial.print(String(w.temp_max) + ";");
    Serial.print(String(w.temp) + ";");
    Serial.print(String(w.temp_min) + ";");
    
    Serial.print(String(w.hum_max) + ";");
    Serial.print(String(w.hum) + ";");
    Serial.print(String(w.hum_min) + ";");

    Serial.print(String(w.rain) + ";");
    Serial.print("0;");
    Serial.print("0;");

    Serial.print("0;0;0;0;0;0;0;0;0;0;0;0;");

    Serial.print(String(w.wind_speed_max) + ";");
    Serial.print(String(w.wind_speed) + ";");
    Serial.print(String(w.wind_speed_min) + ";");

    Serial.print(String(w.wind_direction) + ";");
    Serial.print(String(w.panel_voltage) + ";");
    Serial.print(String(w.battery_voltage) + ">\"\r");
    delay(300);
  #endif

  ShowSerialData();  
  if(SendATCommand("AT+QHTTPGET=30", "OK", 30000) < 1) {
    GSMReadUntil("\n", 50); ShowSerialData();
    return false;
  }
  GSMReadUntil("\n", 50); ShowSerialData();
  return true;
}

bool ReadTime(realTime &wt){
  int i, t;
  char ch, temp_time[8];
  do {
    do {
      ch = Serial1.read();
      if(SERIAL_RESPONSE) {
        Serial.print(ch);
  	  }
  	}while(ch != 'e');
  	ch = Serial1.read();
  	if(SERIAL_RESPONSE) {
      Serial.print(ch);
  	}
  }while(ch != '(');
  //Year
  for(i = 0, ch = 0;; i++) {
    ch = Serial1.read();
    if(SERIAL_RESPONSE) {
      Serial.print(ch);
    }
    if(ch != ',') 
      temp_time[i] = ch;
    else break;
  }
  for(i = i - 1, t = 1, wt.year = 0; i >= 0; i--, t *= 10){
  	wt.year += (temp_time[i] - '0') * t;
  }
  //Month
  for(i = 0, ch = Serial1.read();; i++) {
    ch = Serial1.read();
    if(SERIAL_RESPONSE) {
      Serial.print(ch);
    }
    if(ch != ',') 
      temp_time[i] = ch;
    else break;
  }
  for(i = i - 1, t = 1, wt.month = 0; i >= 0; i--, t *= 10){
  	wt.month += (temp_time[i] - '0') * t;
  }
  //Day
  for(i = 0, ch = Serial1.read();; i++) {
    ch = Serial1.read();
    if(SERIAL_RESPONSE) {
      Serial.print(ch);
    }
    if(ch != ',') 
      temp_time[i] = ch;
    else break;
  }
  for(i = i - 1, t = 1, wt.day = 0; i >= 0; i--, t *= 10){
  	wt.day += (temp_time[i] - '0') * t;
  }
  //Hour
  for(i = 0, ch = Serial1.read();; i++) {
    ch = Serial1.read();
    if(SERIAL_RESPONSE) {
      Serial.print(ch);
    }
    if(ch != ',') 
      temp_time[i] = ch;
    else break;
  }
  for(i = i - 1, t = 1, wt.hours = 0; i >= 0; i--, t *= 10){
  	wt.hours += (temp_time[i] - '0') * t;
  }
  //Minutes
  for(i = 0, ch = Serial1.read();; i++) {
    ch = Serial1.read();
    if(SERIAL_RESPONSE) {
      Serial.print(ch);
    }
    if(ch != ',') 
      temp_time[i] = ch;
    else break;
  }
  for(i = i - 1, t = 1, wt.minutes = 0; i >= 0; i--, t *= 10){
  	wt.minutes += (temp_time[i] - '0') * t;
  }
  //Seconds
  for(i = 0, ch = Serial1.read();; i++) {
    ch = Serial1.read();
    if(SERIAL_RESPONSE) {
      Serial.print(ch);
    }
    if(ch != ',') 
      temp_time[i] = ch;
    else break;
  }
  for(i = i - 1, t = 1, wt.seconds = 0; i >= 0; i--, t *= 10){
  	wt.seconds += (temp_time[i] - '0') * t;
  }
  ShowSerialData();

  wt.flag = 1;
  return true;
}

bool GetID(int &id) {
  SdFile datalog;
  float lat, lon;

  char str[10];
  int i, str_len;
  char ch;

  int temp_id;
  
  if(!HttpInit()) 
    return false;

  if(SERIAL_OUTPUT)
    Serial.println("Getting new id");

  GSMModuleWake();
  Serial1.print("AT+QHTTPURL=" + String(CHECK_ID_URL.length()) + ",30\r");
  GSMReadUntil("CONNECT", 20000);
  GSMReadUntil("\n", 100);
  Serial1.print(CHECK_ID_URL + '\r');
  GSMReadUntil("OK", 5000);
  GSMReadUntil("\n", 100);

  GSMModuleWake();
  if(SendATCommand("AT+QHTTPGET=30", "OK", 30000) < 1) {
    GSMReadUntil("\n", 100);
    ShowSerialData();
    return false;
  }
  GSMReadUntil("\n", 100);
  ShowSerialData();
  
  GSMModuleWake();
  if(SendATCommand("AT+QHTTPREAD=30", "CONNECT", 1000) == -1) {
    GSMReadUntil("OK", 1000); 
    GSMReadUntil("\n", 50); 
    return false;
  }
  delay(100);
  ch = Serial1.peek();
  while(!isDigit(ch)) {
    ch = Serial1.read();
  }

  for(i = 0; isDigit(ch);) {
    str[i++] = ch;
    ch = Serial1.read();
  }
  str[i] = '\0';
  temp_id = (String(str)).toInt();

  GSMReadUntil("OK", 1000); 
  GSMReadUntil("\n", 50); 

  if(SERIAL_OUTPUT)
    Serial.println("ID: " + String(temp_id));

  if(!GetGSMLoc(lat, lon))
    return false;

  if(SERIAL_OUTPUT)
    Serial.println("Location: " + String(lat) + ", " + String(lon));

  str_len = CREATE_ID_URL.length();
  str_len += String(id).length() + 3;
  str_len += String(DATA_UPLOAD_FREQUENCY).length() + 6;
  str_len += String(lat).length() + 4;
  str_len += String(lon).length() + 4;

  GSMModuleWake();
  Serial1.print("AT+QHTTPURL=" + String(str_len) + ",30\r");
  GSMReadUntil("CONNECT", 20000);
  GSMReadUntil("\n", 100);
  Serial1.print(CREATE_ID_URL);
  Serial1.print("id=" + String(temp_id));  delay(1);
  Serial1.print("&freq=" + String(DATA_UPLOAD_FREQUENCY));  delay(1);
  Serial1.print("&lt=" + String(lat));  delay(1);
  Serial1.print("&ln=" + String(lon) + "\r"); delay(1);
  GSMReadUntil("OK", 5000);
  GSMReadUntil("\n", 100);

  if(SendATCommand("AT+QHTTPGET=30", "OK", 30000) < 1) {
    GSMReadUntil("\n", 100);
    ShowSerialData();
    return false;
  }
  GSMReadUntil("\n", 100);
  ShowSerialData();
  if(SendATCommand("AT+QHTTPREAD=30", "CONNECT", 1000) == -1) {
    GSMReadUntil("OK", 1000); 
    GSMReadUntil("\n", 50); 
    return false;
  }
  delay(50);
  GSMReadUntil("OK", 1000); 
  GSMReadUntil("\n", 50); 
  ShowSerialData();

  GSMModuleWake();
  SendIdSMS(temp_id, SERVER_PHONE_NUMBER);

  id = temp_id;

  sd.begin(SD_CARD_CS_PIN);
  if(sd.exists("id.txt"))
    sd.remove("id.txt");
  
  datalog.open("id.txt",  O_WRITE | O_CREAT);
  datalog.print(id);
  datalog.close();

  return true;
}

bool GetTime(realTime &w) {
  uint8_t i, c;
  realTime t;
  if(!HttpInit()) 
    return false;
  Serial1.print("AT+QHTTPURL=" + String(TIME_URL.length()) + ",30\r");
  delay(1000);
  ShowSerialData();
  Serial1.print(TIME_URL + '\r');
  delay(300);
  ShowSerialData();

  if(SendATCommand("AT+QHTTPGET=30", "OK", 30000) < 1) {
    GSMReadUntil("\n", 500);
    ShowSerialData();
    return false;
  }
  ShowSerialData();

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
    Serial.println("Fail");
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

bool SendIdSMS(int id, String SERVER_PHONE_NUMBER) {
  GSMModuleWake();
  Serial1.print("AT+CMGS=\"" + SERVER_PHONE_NUMBER + "\"\n");
  delay(100);
  Serial1.print("HK9D7 ");
  Serial1.print("NEWID ");
  Serial1.print(id);
  delay(100);
  Serial1.write(26);
  Serial1.write('\n');
  delay(100);
  Serial1.write('\n');
  ShowSerialData();

  return true;
}

void UploadSMS(weatherData w, String SERVER_PHONE_NUMBER) {
  Serial1.print("AT+CMGS=\"" + SERVER_PHONE_NUMBER + "\"\n");
  delay(100);
  Serial1.print("HK9D7 ");
  Serial1.print("'id':" + String(w.id) + ",");
  Serial1.print("'t1':" + String(w.temp) + ",");
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
