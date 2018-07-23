#include "SD.h"
#include <SdFat.h>
#include "GSM.h"
#include "settings.h"
#include "http.h"
#include "weatherData.h"
#include <avr/wdt.h>
#include <EEPROM.h>

SdFat sd;
SdFile datalog, data_temp;

bool CheckOTA() {
  char body_r[40], number[14];

  if(!sd.exists("id.txt")) {
  	if(!sd.begin(SD_CARD_CS_PIN)) {
			return false;
		}
	}
  #if SERIAL_OUTPUT
    Serial.println("Updating firmware");
  #endif
  delay(500);
  sd.chdir();
  delay(100);

  if(sd.exists("TEMP_OTA.HEX")) sd.remove("TEMP_OTA.HEX");
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
      EEPROM.write(0x1FF,0xF0);
      wdt_enable(WDTO_500MS);
      wdt_reset();
      delay(600);
    } else {
      #if SERIAL_OUTPUT
        Serial.println("SD card copy error- hex file checksum failed");
      #endif
      return false;
    }
  } else {
    #if SERIAL_OUTPUT
      Serial.println("Firmware download failed");
    #endif
    return false;
  }
}

bool DownloadHex() {
  int i, j, sd_index = 0;
  unsigned long t;
  char ch;
  uint8_t sd_buffer[512];
	
  sd.chdir();
  if(!sd.exists("id.txt")) {
    if(!sd.begin(SD_CARD_CS_PIN)) return false;
  }
  datalog.open("temp_ota.hex",  FILE_WRITE);
  delay(5000);
  HttpInit();
  i = OTA_URL.length();
  Serial1.print("AT+QHTTPURL=" + String(i) + ",30\r");
  delay(1000);
  ShowSerialData();
  Serial1.print(OTA_URL + '\r');
  delay(100);
  ShowSerialData();
  
  if(!SendHttpGet())
    return false;

  delay(500);

  datalog.write(':');
  
  Serial1.println("AT+QHTTPREAD=30\r");

  //--------Read until the actual data---------
  t = millis();
  for(i = 0; i < 3 && (millis() - t) < 3000;){
    if(Serial1.read() ==  '>') i++;
  }
  if((millis() - t) > 3000) return false;
  ch = Serial1.read();
  t = millis();
  while(ch != ':' && (millis() - t) < 3000) 
    ch = Serial1.read();
  if((millis() - t) > 3000) 
    return false;
  //-------------------------------------------

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
      if(i > 30000) break;
    }
    if(i > 30000) break;
  }
  datalog.write(sd_buffer, sd_index);
  datalog.sync();
  datalog.seekCur(-1);
  datalog.close();
  while(Serial1.available()) Serial1.read();

  if((millis() - t) < 15000)
    return false;
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
    if(!sd.begin(SD_CARD_CS_PIN)) return false;
  }
  delay(100);
  if(!sd.exists("TEMP_OTA.HEX")) return false;

  data_temp.open("TEMP_OTA.HEX", O_READ);

  if(data_temp.fileSize() < 50000) {
    data_temp.close();
    return false;
  }
  
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
    if(n != 0) {
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
	bool flag;
	char ch, str[100];
	int i;

  SD_csv_header.toCharArray(str, 100);
  Serial.println(str);
	
  if(!sd.exists("id.txt")) {
    if(!sd.begin(SD_CARD_CS_PIN)) return false;
  }
  delay(50);
  if(!sd.exists("datalog.csv")) {
    delay(50);
    if(!datalog.open("datalog.csv", FILE_WRITE)) {
      datalog.close();
      #if SERIAL_OUTPUT
        Serial.println("Could not create CSV file");
      #endif
      return false;
    }
    delay(50);
    datalog.println(SD_csv_header);
  } else {
    if(!datalog.open("datalog.csv", FILE_WRITE)) {
      datalog.close();
      #if SERIAL_OUTPUT
        Serial.println("Could not open CSV file");
      #endif
      return false;
    }
  }
	datalog.seekSet(0);
	
	i = 0;
  flag = true;
  Serial.println(SD_csv_header.length());
	for(i = 0, flag = true, ch = datalog.read(); ch != '\n' && i < 100; i++){
    if(ch == '\n' && str[i] == '\0')
      break;
		if((ch != str[i]) || i > (SD_csv_header.length() + 1)) {
      Serial.println(i);
      Serial.println(ch);
      Serial.println(str[i]);
			flag = false;
			break;
		}
    ch = datalog.read();
	};
  
	if(flag == false || i >= 100) {
    Serial.println("false");
		datalog.close();
		delay(50);
		sd.remove("datalog.csv");
		delay(50);
		if(!datalog.open("datalog.csv", FILE_WRITE)) {
      datalog.close();
      #if SERIAL_OUTPUT
        Serial.println("Could not create CSV file");
      #endif
      return false;
    }
    delay(50);
    datalog.println(SD_csv_header);
	}	
	
  datalog.seekEnd();
  datalog.print(w.flag);
  datalog.print(',');
  datalog.print(w.t.flag);
  datalog.print(',');
  datalog.print(w.id);
  datalog.print(',');
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
  datalog.print('T');
  datalog.write((w.t.hours / 10) % 10 + '0'); 
  datalog.write((w.t.hours % 10) + '0');
  datalog.write(':');
  datalog.write((w.t.minutes / 10) % 10 + '0'); 
  datalog.write((w.t.minutes % 10) + '0');
  datalog.write(':');
  datalog.write((w.t.seconds / 10) % 10 + '0'); 
  datalog.write((w.t.seconds % 10) + '0');
  datalog.print(',');
  datalog.print(w.temp1);
  datalog.print(',');
  datalog.print(w.temp2);
  datalog.print(',');
  datalog.print(w.hum);
  datalog.print(',');
  datalog.print(w.wind_speed);
  datalog.print(',');
  datalog.print(w.rain);
  datalog.print(',');
  datalog.print(w.pressure);
  datalog.print(',');
  datalog.print(w.amps);
  datalog.print(',');
  datalog.print(w.solar_radiation);
  datalog.print(',');
  datalog.print(w.panel_voltage);
  datalog.print(',');
  datalog.print(w.battery_voltage);
  datalog.print(',');
  datalog.println(w.signal_strength);
	
  if(!datalog.close())
    return false;
	
  return true;
}

bool UploadOldSD() {
  char ch, t[20];
  int p;
  int i;
  uint16_t read_length, timeout;

  weatherData w;

  sd.chdir();
  if(!sd.exists("id.txt")) {
    if(!sd.begin(SD_CARD_CS_PIN)) return false;
  }
  i = GetPreviousFailedUploads();
  #if SERIAL_OUTPUT
    Serial.println("Number of missed uploads: " + String(i));
  #endif
  if(i == 0) return true;
  
  datalog.open("datalog.csv", FILE_WRITE);
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
    while(datalog.read() != ','); //Read past success flag
		
		t[0] = datalog.read();
		t[1] = '\0';
    w.t.flag = bool((String(t)).toInt());
		
    i = 0;
    do {
      ch = datalog.read();
      if(ch != ' ' && ch != ',') t[i++] = ch;
    }while(ch != ',');
    t[i] = '\0';
    w.id = (String(t)).toInt();
    
    //Read Day
    t[0] = datalog.read();
    t[1] = datalog.read();
    t[2] = '\0';
    w.t.day = (String(t)).toInt();
		datalog.seekCur(1);

    //Read Month
    t[0] = datalog.read();
    t[1] = datalog.read();
    t[2] = '\0';
    w.t.month = (String(t)).toInt();
		datalog.seekCur(1);

    //Read Year
    t[0] = datalog.read();
    t[1] = datalog.read();
    t[2] = datalog.read();
    t[3] = datalog.read();
    t[4] = '\0';
    w.t.year = (String(t)).toInt();
    datalog.seekCur(1);

    //Read Hour
    t[0] = datalog.read();
    t[1] = datalog.read();
    t[2] = '\0';
    w.t.hours = (String(t)).toInt();
    datalog.seekCur(1);

    //Read Minute
    t[0] = datalog.read();
    t[1] = datalog.read();
    t[2] = '\0';
    w.t.minutes = (String(t)).toInt();
    datalog.seekCur(1);

    //Read Day
    t[0] = datalog.read();
    t[1] = datalog.read();
    t[2] = '\0';
    w.t.seconds = (String(t)).toInt();

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
      if(ch != ' ' && ch != '\n') t[i++] = ch;
    }while(ch != '\n');
    t[i] = '\0';
    w.signal_strength = String(t).toInt();

    if(w.t.flag == 1) {
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
    }
    
    datalog.seekCur(-2);
    while(datalog.peek()!= '\n') {
      datalog.seekCur(-1);
    }
    datalog.seekCur(1);
    datalog.write('1');
		
		delay(1000);
		
    for(timeout = 0; datalog.read()!= '\n' && datalog.available() && timeout < 2000; timeout++) 
			delay(1);
		if(timeout > 2000) {
			datalog.close();
			return false;
		}
  }

  datalog.close();
  return true;
}

unsigned int GetPreviousFailedUploads() {
  long int timeout;
  char ch;
  int lines = 0, p;
  int i;
	
  if(!sd.exists("id.txt")) {
    if(!sd.begin(SD_CARD_CS_PIN)) return -1;
  }
  if(!sd.exists("datalog.csv")) {
    return -1;
  }
  datalog.open("datalog.csv", FILE_READ);
	
  datalog.seekEnd();
  timeout = 0;
  do {
    datalog.seekCur(-2);
    for(; datalog.peek()!= '\n' && datalog.curPosition() != 0 && timeout < 2000; timeout++) {
      datalog.seekCur(-1);
      delay(1);
    }
    if(datalog.curPosition() == 0) 
      break;
    datalog.seekCur(1);
    ch = datalog.peek();
    if(ch == '0') 
      lines++;
    if(timeout++ > 2000) {
      datalog.close();
      return -1;
    }
    delay(1);
  }while(ch == '0');
  datalog.close();
  return lines;
}

bool WriteOldTime(int n, realTime t) {
  long int timeout;
  int lines, i;
  char str[20], ch;
  realTime temp;

  if(!sd.exists("id.txt")) {
    if(!sd.begin(SD_CARD_CS_PIN)) {
      #if SERIAL_OUTPUT
        Serial.println("No SD Card detected");
      #endif
      return false;
    }
  }
  if(!sd.exists("datalog.csv")) {
    #if SERIAL_OUTPUT
      Serial.println("No CSV file detected");
    #endif
    return false;
  }
  if(GetPreviousFailedUploads() < n) 
    return false;

  delay(500);
  datalog.open("datalog.csv", FILE_WRITE);
  datalog.seekEnd();
  lines = 0;
  timeout = 0;
  do {
    datalog.seekCur(-2);
    for(; datalog.peek()!= '\n' && datalog.curPosition() != 0 && timeout < 4000; timeout++) {
      datalog.seekCur(-1);
      delay(1);
    }
    if(datalog.curPosition() == 0) 
      break;
    datalog.seekCur(1);
    ch = datalog.peek();
    if(ch == '0') 
      lines++;
    if(timeout++ > 4000) {
      datalog.close();
      return false;
    }
    delay(1);
  }while(ch == '0' && lines < n);
  if(lines != n) {
    datalog.close();
    return false;
  }

  for(; lines > 0; lines--) {
    temp.seconds = 0;
    temp.minutes = 0;
    temp.hours = 0;
    temp.day = 0;
    temp.month = 0;
    temp.year = 0;
    temp.flag = 1;

    //Read past success flag
    for(timeout = 0; datalog.read() != ',' && timeout < 1000; timeout++)
      delay(1);
    if(timeout > 1000) {
      datalog.close();
      return false;
    }
		
    datalog.write('1');
		
    //Read past Id flag
    for(timeout = 0; datalog.read() != '/' && timeout < 1000; timeout++)
			delay(1);
		
    if(timeout > 1000) {
      datalog.close();
      return false;
    }
		
    datalog.seekCur(-3);
		
    str[0] = datalog.read();
    str[1] = datalog.read();
    str[2] = '\0';
    temp.day = String(str).toInt();

    for(timeout = 0; datalog.read() != 'T' && timeout < 2000; timeout++) 
      delay(1);
    if(timeout > 2000) {
      datalog.close();
      return false;
    }

    str[0] = datalog.read();
    str[1] = datalog.read();
    str[2] = '\0';
    temp.hours = String(str).toInt();
    datalog.seekCur(1);
		
    str[0] = datalog.read();  
    str[1] = datalog.read();
    str[2] = '\0';
    temp.minutes = String(str).toInt();
    datalog.seekCur(1);

    str[0] = datalog.read();
    str[1] = datalog.read();
    str[2] = '\0';
    temp.seconds = String(str).toInt();

    AddTime(t, temp, temp);

    datalog.seekCur(-2);
    for(timeout = 0; datalog.peek() != ',' && timeout < 2000; timeout++) {
      datalog.seekCur(-1);
      delay(1);
    }
    if(timeout > 2000) {
      datalog.close();
      return false;
    }
    datalog.seekCur(2);

    datalog.write((temp.day / 10) % 10 + '0'); 
    datalog.write((temp.day % 10) + '0');
    datalog.seekCur(1);
    datalog.write((temp.month / 10) % 10 + '0'); 
    datalog.write((temp.month % 10) + '0');
    datalog.seekCur(1);
    datalog.write((temp.year / 1000) % 10 + '0'); 
    datalog.write((temp.year / 100) % 10 + '0'); 
    datalog.write((temp.year / 10) % 10 + '0'); 
    datalog.write((temp.year % 10) + '0');
    datalog.seekCur(1);
    datalog.write((temp.hours / 10) % 10 + '0'); 
    datalog.write((temp.hours % 10) + '0');
    datalog.seekCur(1);
    datalog.write((temp.minutes / 10) % 10 + '0'); 
    datalog.write((temp.minutes % 10) + '0');
    datalog.seekCur(1);
    datalog.write((temp.seconds / 10) % 10 + '0'); 
    datalog.write((temp.seconds % 10) + '0');

    for(timeout = 0; datalog.read() != '\n' && datalog.available() && timeout < 2000; timeout++);
      delay(1);
    if(timeout > 2000) {
      datalog.close();
      return false;
    }
  }
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
