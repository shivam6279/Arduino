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

bool UploadWeatherData(weatherData w[], uint8_t n, realTime &wt) {
  uint16_t timeout;
  int i;
  if(!HttpInit()) 
    return false;

  if(n < 1) 
    n = 1;
  
  for(i = n - 1; i >= 0; i--) {
    if(!SendWeatherURL(w[i])) 
      return false;
    ShowSerialData();  
    if(SendATCommand("AT+QHTTPREAD=30", "CONNECT", 1000) < 1) {
      GSMReadUntil("\n", 100); 
      GSMReadUntil("\n", 50); 
      return false;
    }
    
    if(i != 0) {
      GSMReadUntil("\n", 100); 
      GSMReadUntil("\n", 50); 
      ShowSerialData();
    }
  }
  //Read Time
  delay(100);
  ReadTime(wt);
  return true;
}

bool SendWeatherURL(weatherData w) {
  uint16_t timeout;
  char ch;
  int str_len;
  str_len = URL.length(); // URL string length
  str_len += String(w.id).length() + 4;

  str_len += String(w.temp1).length() + 4;
  str_len += String(w.temp1_min).length() + 7;
  str_len += String(w.temp1_max).length() + 7;

  str_len += String(w.temp2).length() + 4;
  str_len += String(w.temp2_min).length() + 7;
  str_len += String(w.temp2_max).length() + 7;

  str_len += String(w.hum).length() + 3;
  str_len += String(w.hum_min).length() + 6;
  str_len += String(w.hum_max).length() + 6;

  str_len += String(w.wind_speed).length() + 3;
  str_len += String(w.wind_max).length() + 6;

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
  Serial1.print("mint1=" + String(w.temp1_min) + "&");
  Serial1.print("maxt1=" + String(w.temp1_min) + "&");
  Serial1.print("t2=" + String(w.temp2) + "&");
  Serial1.print("mint2=" + String(w.temp2_min) + "&");
  Serial1.print("maxt2=" + String(w.temp2_max) + "&");
  Serial1.print("h=" + String(w.hum) + "&");
  Serial1.print("minh=" + String(w.hum_min) + "&");
  Serial1.print("maxh=" + String(w.hum_max) + "&");
  Serial1.print("w=" + String(w.wind_speed) + "&");
  Serial1.print("maxw" + String(w.wind_max) + "&");
  Serial1.print("r=" + String(w.rain) + "&");
  Serial1.print("p=" + String(w.pressure) + "&");
  Serial1.print("s=" + String(w.amps) + "&");
  Serial1.print("v=" + String(w.panel_voltage) + "&");
  Serial1.print("bv=" + String(w.battery_voltage) + "&");
  Serial1.print("sg=" + String(w.signal_strength) + '\r');
  delay(300);
  
  ShowSerialData();  
  if(SendATCommand("AT+QHTTPGET=30", "OK", 30000) < 1) {
    GSMReadUntil("\n", 50); ShowSerialData();  
    return false;
  }
  GSMReadUntil("\n", 50); ShowSerialData();  
  return true;
}

bool SendTestPacket(int id, realTime &wt) {
  weatherData w;
  w.id = id;
  w.temp1 = 100.0;
  w.temp2 = 100.0;
  w.hum = 100.0;
  w.rain = 100.0;
  w.pressure = 100.0;
  w.amps = 100.0;
  w.panel_voltage = 100.0;
  w.battery_voltage = 100.0;
  w.signal_strength = 100.0;
  w.t.flag = 0;
  return UploadWeatherData(&w, 1, wt);
}

bool ReadTime(realTime &wt){
  int i, t;
  char ch, temp_time[8];
  if(!GSMReadUntil(".datetime(", 1000))
    return false;
    
  delay(200);
  //Year
  for(i = 0;;) {
    ch = Serial1.read();
    if(SERIAL_RESPONSE) {
      Serial.write(ch);
    }
    if(isDigit(ch)) 
      temp_time[i++] = ch;
    else if(ch == ',')
      break;
  }
  temp_time[i] = '\0';
  wt.year = String(temp_time).toInt();
  
  //Month
  for(i = 0;;) {
    ch = Serial1.read();
    if(SERIAL_RESPONSE) {
      Serial.print(ch);
    }
    if(isDigit(ch)) 
      temp_time[i++] = ch;
    else if(ch == ',')
      break;
  }
  temp_time[i] = '\0';
  wt.month = String(temp_time).toInt();
  
  //Day
  for(i = 0;;) {
    ch = Serial1.read();
    if(SERIAL_RESPONSE) {
      Serial.print(ch);
    }
    if(isDigit(ch)) 
      temp_time[i++] = ch;
    else if(ch == ',')
      break;
  }
  temp_time[i] = '\0';
  wt.day = String(temp_time).toInt();
  
  //Hour
  for(i = 0;;) {
    ch = Serial1.read();
    if(SERIAL_RESPONSE) {
      Serial.print(ch);
    }
    if(isDigit(ch)) 
      temp_time[i++] = ch;
    else if(ch == ',')
      break;
  }
  temp_time[i] = '\0';
  wt.hours = String(temp_time).toInt();
  
  //Minutes
  for(i = 0;;) {
    ch = Serial1.read();
    if(SERIAL_RESPONSE) {
      Serial.print(ch);
    }
    if(isDigit(ch)) 
      temp_time[i++] = ch;
    else if(ch == ',')
      break;
  }
  temp_time[i] = '\0';
  wt.minutes = String(temp_time).toInt();
  
  //Seconds
  for(i = 0;;) {
    ch = Serial1.read();
    if(SERIAL_RESPONSE) {
      Serial.print(ch);
    }
    if(isDigit(ch)) 
      temp_time[i++] = ch;
    else if(ch == ',')
      break;
  }
  temp_time[i] = '\0';
  wt.seconds = String(temp_time).toInt();

  ShowSerialData();
  Serial.println();

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

  if(SD_connected == false) {
    if(!sd.begin(SD_CARD_CS_PIN)) {
      return false;
    } else {
      SD_connected = true;  
    }    
  }
  
  if(sd.exists("id.txt"))
    sd.remove("id.txt");
  
  if(datalog.open("id.txt",  O_WRITE | O_CREAT)) {
    datalog.print(id);
    datalog.close();
  }

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
