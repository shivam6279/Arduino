#include "SD.h"
#include <SdFat.h>
#include "GSM.h"
#include "settings.h"
#include "http.h"
#include <avr/wdt.h>
#include <EEPROM.h>

SdFat sd;
SdFile datalog;

void CheckOTA() {
  char body_r[40], number[14];
  if(CheckSMS()) { //Check sms for OTA
    GetSMS(number, body_r);
    while(Serial1.available()) Serial1.read();
    
    if(!sd.exists("OtaTemp")) {
      if(!sd.mkdir("OtaTemp")) {
        SendSMS(number, "No SD card detected");
        return false;
      }
    }
    
    if(toupper(body_r[0]) == 'O' &&  toupper(body_r[1]) == 'T' &&toupper(body_r[2]) == 'A' && sd.begin(53)) {
      SendSMS(number, "Downloading new firmware");
      #if SERIAL_OUTPUT
        Serial.println("Updating firmware");
      #endif
      delay(500);
      sd.chdir();
      delay(100);

      if(sd.exists("OtaTemp/TEMP_OTA.HEX")) sd.remove("OtaTemp/TEMP_OTA.HEX");
      if(sd.exists("firmware.BIN")) sd.remove("firmware.BIN");
      delay(500);
      if(DownloadHex()) {
        #if SERIAL_OUTPUT
          Serial.println("\nNew firmware downloaded");
          Serial.println("Converting .hex file to .bin");
        #endif
        if(SDHexToBin()) {
          while(Serial1.available()) Serial1.read();
          #if SERIAL_OUTPUT
            Serial.println("Done\nRestarting and reprogramming");
          #endif
          SendSMS(number, "Download succesful, Restarting and reprogramming");
          EEPROM.write(0x1FF,0xF0);
          wdt_enable(WDTO_500MS);
          wdt_reset();
          delay(600);
        } else {
          #if SERIAL_OUTPUT
            Serial.println("SD card copy error- hex file checksum failed");
          #endif
            SendSMS(number, "OTA failed - No SD card detected");
          return false;
        }
      } else {
        #if SERIAL_OUTPUT
          Serial.println("Download failed");
        #endif
          SendSMS(number, "Firmware download failed");
        return false;
      }
    }
  }
}

bool DownloadHex() {
  int i, j, sd_index = 0;
  unsigned long t;
  char ch;
  uint8_t sd_buffer[512];

  SdFile datalog;

  sd.chdir();
  if(!sd.exists("id.txt")) {
    if(!sd.begin(53)) return false;
  }
  if(!sd.exists("OtaTemp")){
    if(!sd.mkdir("OtaTemp")) return false;
  }
  datalog.open("OtaTemp/temp_ota.hex",  FILE_WRITE);
  delay(5000);
  HttpInit();
  i = OTA_URL.length();
  Serial1.print("AT+QHTTPURL=" + String(i) + ",30\r");
  delay(1000);
  ShowSerialData();
  Serial1.print(OTA_URL + '\r');
  delay(100);
  ShowSerialData();
  Serial1.println("AT+QHTTPGET=30\r");
  delay(700);
  //--------Read httpget response--------
  j = 0;
  do {
    if(Serial1.available()) {
      ch = Serial1.read();
      #if SERIAL_RESPONSE
  	    Serial.print(ch);
  	  #endif
    }
    delay(1);
    j++;
  }while(!isdigit(ch) && j < 10000);
  if(j >= 10000) return false;
  j = 0;
  do {
    if(Serial1.available()) {
      ch = Serial1.read();
      #if SERIAL_RESPONSE
  	    Serial.print(ch);
  	  #endif
    }
    delay(1);
    j++;
  }while(!isAlpha(ch) && j < 10000);
  if(j >= 10000) return false;
  delay(100);
  ShowSerialData();
  if(ch != 'O') return false;
  //--If the response is not 'OK' - return--

  datalog.write(':');
  
  Serial1.println("AT+QHTTPREAD=30\r");

  //-----Read until the actual data------
  t = millis();
  for(i = 0; i < 3 && (millis() - t) < 3000;){
    if(Serial1.read() ==  '>') i++;
  }
  if((millis() - t) > 3000) return false;
  ch = Serial1.read();
  t = millis();
  while(ch != ':' && (millis() - t) < 3000) ch = Serial1.read();
  if((millis() - t) > 3000) return false;
  //--------------------------------------

  i = 0;
  j = 1;
  t = millis();
  while(1) {
    while(Serial1.available()){
      ch = Serial1.read();
      if(ch == 'O') break;
      sd_buffer[sd_index++] = ch;
      if(sd_index >= 512) {
        sd_index = 0;
        datalog.write(sd_buffer, 512);
      }
      i = 0;
      Serial.print(ch);
    }
    if(ch == 'O') break;
    while(Serial1.available() == 0){
      i++;
      delay(1);
      if(i > 10000) break;
    }
    if(i > 10000) break;
  }
  datalog.write(sd_buffer, sd_index);
  datalog.sync();
  datalog.seekCur(-1);
  datalog.close();
  while(Serial1.available()) Serial1.read();
  return true;
}

bool SDHexToBin() {
  unsigned char buff[37], ch, n, t_ch[2];
  uint16_t i, j, temp_checksum;
  long int checksum;
  bool flag = false;

  SdFile datalog, data_temp;

  sd.chdir();
  if(!sd.exists("id.txt")) {
    if(!sd.begin(53)) return false;
  }
  delay(100);
  if(!sd.exists("OtaTemp/TEMP_OTA.HEX")) return false;

  data_temp.open("OtaTemp/TEMP_OTA.HEX", O_READ);
  datalog.open("firmware.bin", O_WRITE | O_CREAT);

  ch = data_temp.read();
  while((ch < 'a' && ch > ':') || ch < '0' || ch > 'f') {
    ch = data_temp.read();
  }
  data_temp.seekCur(-1);
  j = 0;
  while(data_temp.available()) {
    data_temp.read(buff, 9);

    //Get the number of bytes in the line
    for(i = 1, checksum = 0; i < 9; i++) {
      t_ch[0] = buff[i++];
      t_ch[1] = buff[i];
      if((t_ch[0] >= '0' && t_ch[0] <= '9') || (t_ch[0] >= 'a' && t_ch[0] <= 'f') || (t_ch[0] >= 'A' && t_ch[0] <= 'F')) {// Checks if the character read is a hexadecmal character
        CharToInt(t_ch[0]);
        CharToInt(t_ch[1]);

        ch = t_ch[0] * 16 + t_ch[1];
        checksum += ch;
        if(i == 2) n = ch;
      }
    }

    //Read the data
    if(n != 0){
	  data_temp.read(buff, (n*2)+4);
	  for(i = 0; i < (n*2); i++) {
		t_ch[0] = buff[i++];
		t_ch[1] = buff[i];
		if((t_ch[0] >= '0' && t_ch[0] <= '9') || (t_ch[0] >= 'a' && t_ch[0] <= 'f') || (t_ch[0] >= 'A' && t_ch[0] <= 'F')) {
		  CharToInt(t_ch[0]);
		  CharToInt(t_ch[1]);

		  ch = t_ch[0] * 16 + t_ch[1];
		  checksum += ch;
		  datalog.write(ch);
	      if(++j == 500) {
	        j = 0;
	        datalog.sync();
	      }
	    }
	  }
	}
	else{
	  if(buff[7] == '0' && buff[8] == '1'){
	  	flag = true;
	  	break;
	  }
	}
    
    t_ch[0] = buff[(n*2)];
    t_ch[1] = buff[(n*2) + 1];
    if((t_ch[0] >= '0' && t_ch[0] <= '9') || (t_ch[0] >= 'a' && t_ch[0] <= 'f') || (t_ch[0] >= 'A' && t_ch[0] <= 'F')) {
      CharToInt(t_ch[0]);
      CharToInt(t_ch[1]);

      temp_checksum = t_ch[0] * 16 + t_ch[1];
    }
    if((checksum + temp_checksum) % 0x100 != 0) return false;
  }
  datalog.sync();
  datalog.close();
  data_temp.close();
  return flag;
}

bool WriteSD(weatherData w) {
  sd.chdir();
  if(!sd.exists("id.txt")) {
    if(!sd.begin(53)) return false;
  }
  if(!sd.exists("Datalog")) {
    if(!sd.mkdir("Datalog")) return false;
  }
  if(!sd.exists("Datalog/datalog.csv")) {
    datalog.open("Datalog/datalog.csv", FILE_WRITE);
    datalog.println("success, id, ts, t1, t2, h, w, r, p, a, s, cv, bv, sg");
  } else {
    datalog.open("Datalog/datalog.csv", FILE_WRITE);
  }
  datalog.seekEnd();
  datalog.print(w.flag);
  datalog.print(", ");
  datalog.print(ws_id);
  datalog.print(", ");
  datalog.write((w.t.day / 10) % 10 + '0'); 
  datalog.write((w.t.day % 10) + '0');
  datalog.write('/');
  datalog.write((w.t.month / 10) % 10 + '0'); 
  datalog.write((w.t.month % 10) + '0');
  datalog.write('/');
  datalog.write((w.t.year / 1000) % 10 + '0'); 
  datalog.write((w.t.year / 100) % 10 + '0'); 
  datalog.write((w.t.year / 10) % 10 + '0'); 
  datalog.write((w.t.year % 10) + '0');
  datalog.print(" ");
  datalog.write((w.t.hours / 10) % 10 + '0'); 
  datalog.write((w.t.hours % 10) + '0');
  datalog.write(':');
  datalog.write((w.t.minutes / 10) % 10 + '0'); 
  datalog.write((w.t.minutes % 10) + '0');
  datalog.write(':');
  datalog.write((w.t.seconds / 10) % 10 + '0'); 
  datalog.write((w.t.seconds % 10) + '0');
  datalog.print(", ");
  datalog.print(w.temp1);
  datalog.print(", ");
  datalog.print(w.temp2);
  datalog.print(", ");
  datalog.print(w.hum);
  datalog.print(", ");
  datalog.print(w.wind_speed);
  datalog.print(", ");
  datalog.print(w.rain);
  datalog.print(", ");
  datalog.print(w.pressure);
  datalog.print(", ");
  datalog.print(w.amps);
  datalog.print(", ");
  datalog.print(w.solar_radiation);
  datalog.print(", ");
  datalog.print(w.panel_voltage);
  datalog.print(", ");
  datalog.print(w.battery_voltage);
  datalog.print(", ");
  datalog.println(w.signal_strength);
  datalog.close();
  return true;
}

unsigned int check() {
  char ch;
  int lines = 0, p;
  int i;
  sd.chdir();
  if(!sd.exists("id.txt")) {
    if(!sd.begin(53)) return -1;
  }
  if(!sd.exists("Datalog")) {
    if(!sd.mkdir("Datalog")) return -1;
  }
  if(!sd.exists("Datalog/datalog.csv")) {
    return -1;
  }
  datalog.open("Datalog/datalog.csv", FILE_READ);
  datalog.seekEnd();
  do {
    datalog.seekCur(-2);
    while(datalog.peek()!= '\n' && datalog.curPosition() != 0) {
      datalog.seekCur(-1);
    }
    if(datalog.curPosition() == 0) break;
    datalog.seekCur(1);
    ch = datalog.peek();
    Serial.println(ch);
    if(ch == '0') lines++;
  }while(ch == '0');
  datalog.close();
  return lines;
}

bool UploadOldSD() {
  char ch, t[20];
  int p;
  int i;
  uint16_t read_length;

  weatherData w;

  sd.chdir();
  if(!sd.exists("id.txt")) {
    if(!sd.begin(53)) return false;
  }
  if(!sd.exists("Datalog")) {
    if(!sd.mkdir("Datalog")) return false;
  }
  
  if(check() <= 0) return true;
  
  datalog.open("Datalog/datalog.csv", FILE_WRITE);
  datalog.seekEnd();

  do {
    datalog.seekCur(-2);
    while(datalog.peek()!= '\n' && datalog.curPosition() != 0) {
      datalog.seekCur(-1);
    }
    if(datalog.curPosition() == 0) break;
    datalog.seekCur(1);
    ch = datalog.peek();
  }while(ch == '0');
  while(datalog.read()!= '\n');
  
  HttpInit();

  while(datalog.available()) {
    while(datalog.read() != ','); //Read past 'success' flag
    while(datalog.read() != ','); //Read past ws_id
    while(datalog.read() != ','); //Read past timestamp

    //Read temp1
    i = 0;
    do {
      ch = datalog.read();
      if(ch != ' ' && ch != ',') t[i++] = ch;
    }while(ch != ',');
    t[i] = '\0';
    w.temp1 = (String(t)).toFloat();

    //Read temp2
    i = 0;
    do {
      ch = datalog.read();
      if(ch != ' ' && ch != ',') t[i++] = ch;
    }while(ch != ',');
    t[i] = '\0';
    w.temp2 = (String(t)).toFloat();

    //Read humidity
    i = 0;
    do {
      ch = datalog.read();
      if(ch != ' ' && ch != ',') t[i++] = ch;
    }while(ch != ',');
    t[i] = '\0';
    w.hum = (String(t)).toFloat();

    //Read wind speed
    i = 0;
    do {
      ch = datalog.read();
      if(ch != ' ' && ch != ',') t[i++] = ch;
    }while(ch != ',');
    t[i] = '\0';
    w.wind_speed = (String(t)).toInt();

    //Read rain
    i = 0;
    do {
      ch = datalog.read();
      if(ch != ' ' && ch != ',') t[i++] = ch;
    }while(ch != ',');
    t[i] = '\0';
    w.rain = (String(t)).toInt();

    //Read pressure
    i = 0;
    do {
      ch = datalog.read();
      if(ch != ' ' && ch != ',') t[i++] = ch;
    }while(ch != ',');
    t[i] = '\0';
    w.pressure = (String(t)).toInt();

    //Read solar radiation
    i = 0;
    do {
      ch = datalog.read();
      if(ch != ' ' && ch != ',') t[i++] = ch;
    }while(ch != ',');
    t[i] = '\0';
    w.solar_radiation = (String(t)).toFloat();

    //Read current
    i = 0;
    do {
      ch = datalog.read();
      if(ch != ' ' && ch != ',') t[i++] = ch;
    }while(ch != ',');
    t[i] = '\0';
    w.amps = (String(t)).toFloat();

    //Read panel voltage
    i = 0;
    do {
      ch = datalog.read();
      if(ch != ' ' && ch != ',') t[i++] = ch;
    }while(ch != ',');
    t[i] = '\0';
    w.panel_voltage = (String(t)).toFloat();

    //Read battery voltage
    i = 0;
    do {
      ch = datalog.read();
      if(ch != ' ' && ch != ',') t[i++] = ch;
    }while(ch != ',');
    t[i] = '\0';
    w.battery_voltage = (String(t)).toFloat();

    //Read signal strength
    i = 0;
    do {
      ch = datalog.read();
      if(ch != ' ' && ch != ',') t[i++] = ch;
    }while(ch != '\n');
    t[i] = '\0';
    w.signal_strength = String(t).toInt();

    if(!SendWeatherURL(w)) {
      datalog.close();
      return false;
    }
    ShowSerialData();  
    Serial1.println("AT+QHTTPREAD=30\r");
    delay(400);
    read_length = Serial1.available();
    ShowSerialData();
    if(read_length < 70) {
      datalog.close();
      return false;
    }
    
    datalog.seekCur(-2);
    while(datalog.peek()!= '\n') {
      datalog.seekCur(-1);
    }
    datalog.read();
    datalog.write('1');
    while(datalog.read()!= '\n');
  }

  datalog.close();
  return true;
}

void CharToInt(unsigned char &a){
  if((a >= '0' && a <= '9')) {
    a -= '0';
  } 
  else if(a >= 'a' && a <= 'f') {
    a -= 87;
  }
  else if(a >= 'A' && a <= 'F') {
    a -= 55;
  }
}
