#include "SD.h"
#include <SdFat.h>
#include "GSM.h"
#include "settings.h"
#include "http.h"
#include "weatherData.h"
#include <avr/wdt.h>
#include <EEPROM.h>

SdFat sd;

bool CheckOTA() {
  char body_r[40], number[14];
  if(CheckOtaSMS(number)) {
    if(!sd.exists("id.txt")) {
      if(!sd.begin(SD_CARD_CS_PIN)) {
        SendSMS(number, "No SD card detected");
        return false;
      }
    }
    SendSMS(number, "Downloading new firmware");
    if(SERIAL_OUTPUT) {
      Serial.println(F("Updating firmware"));
    }
    delay(500);

    if(sd.exists("TEMP_OTA.HEX")) 
      sd.remove("TEMP_OTA.HEX");
      
    if(sd.exists("firmware.BIN")) 
      sd.remove("firmware.BIN");
    delay(500);
    if(DownloadHex()) {
      delay(5000);
      if(SERIAL_OUTPUT) {
        Serial.println(F("\nNew firmware downloaded"));
        Serial.println(F("Converting .hex file to .bin"));
      }
      if(SDHexToBin()) {
        while(Serial1.available()) Serial1.read();
        if(SERIAL_OUTPUT) {
          Serial.println(F("Done\nRestarting and reprogramming"));
        }
        SendSMS(number, "Download succesful, Restarting and reprogramming");
        EEPROM.write(0x1FF,0xF0);
        wdt_enable(WDTO_500MS);
        wdt_reset();
        delay(600);
      } else {
        if(SERIAL_OUTPUT) {
          Serial.println(F("SD card copy error- hex file checksum failed"));
        }
          SendSMS(number, "OTA failed - No SD card detected");
        return false;
      }
    } else {
      if(SERIAL_OUTPUT) {
        Serial.println(F("Firmware download failed"));
      }
      SendSMS(number, "Firmware download failed");
      return false;
    }
  }
}

bool DownloadHex() {
  int i, j, sd_index = 0;
  unsigned long t, timeout;
  char ch;
  uint8_t sd_buffer[600];

  SdFile datalog;

  Serial1.begin(19200);
  delay(500);
  GSMModuleWake();
  GSMModuleWake();

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
  GSMReadUntil("OK", 5000);
  delay(100);
  if(SendATCommand("AT+QHTTPGET=30", "OK", 30000) < 1) {
    GSMReadUntil("\n", 100);
    ShowSerialData();
    datalog.close();
    return false;
  }
  GSMReadUntil("\n", 100);
  ShowSerialData();

  datalog.write(':');
  GSMModuleWake();
  Serial1.println("AT+QHTTPREAD=500\r");

  //-----Read until the actual data------
  for(i = 0, t = millis(); i < 3 && (millis() - t) < 3000;){
    if(Serial1.read() ==  '>') i++;
  }
  if((millis() - t) >= 3000) {
    datalog.close();
    return false;
  }
  
  for(t = millis(); Serial1.available() < 1 && (millis() - t) < 3000;);
  if((millis() - t) >= 3000) {
    datalog.close();
    return false;
  }
  
  ch = Serial1.read();
  t = millis();
  while(ch != ':' && (millis() - t) < 3000) 
    ch = Serial1.read();
  if((millis() - t) >= 3000) {
    datalog.close();
    return false;
  }
  //--------------------------------------
  
  j = 1;
  t = millis();
  while((millis() - t) < 500000) {
    while(Serial1.available()){
      ch = Serial1.read();
      if(ch == 'O') 
        break;
      sd_buffer[sd_index++] = ch;
      
      //Serial.write(ch);
      
      if(sd_index >= 256) {
        datalog.write(sd_buffer, sd_index);
        sd_index = 0;
      }
    }
    if(ch == 'O') break;    
    for(timeout = millis(); Serial1.available() == 0 && (millis() - timeout) < 15000;);
    if((millis() - timeout) >= 15000) 
      break;
  }
  datalog.write(sd_buffer, sd_index);
  delay(1000);
  datalog.close();
  while(Serial1.available()) Serial1.read();

  if((millis() - t) >= 150000)
    return false;
  
  return true;
}

bool SDHexToBin() {
  unsigned char buff[37];
  uint16_t i, j;
  uint16_t checksum, temp_checksum;
  unsigned long timeout;

  uint8_t ch;
  uint8_t byte_count, p_byte_count, record_type;
  uint32_t address, offset_address = 0, p_address;
  
  bool flag = false, start = true;

  SdFile datalog, data_temp;
  
  if(!sd.exists("id.txt")) {
    if(!sd.begin(SD_CARD_CS_PIN)) {
      return false;
    }
  }
  delay(100);
  if(!sd.exists("temp_ota.hex"))
    return false;

  if(!data_temp.open("temp_ota.hex", O_READ)) {
    data_temp.close();
    return false;
  }
  if(!datalog.open("firmware.bin", O_WRITE | O_CREAT)) {
    datalog.close();
    return false;
  }

  data_temp.seekSet(0);
  datalog.seekSet(0);
  
  j = 0;
  while(data_temp.available() > 8) {
    for(timeout = millis(); data_temp.read() != ':' && (millis() - timeout) < 5000;);
    if((millis() - timeout) >= 5000)
      break;
    
    data_temp.read(buff, 8);

    byte_count = CharToInt(buff[0]) << 4 | CharToInt(buff[1]);
    address = offset_address + CharToInt(buff[2]) << 12 | CharToInt(buff[3]) << 8 | CharToInt(buff[4]) << 4 | CharToInt(buff[5]);
    record_type = CharToInt(buff[6]) << 4 | CharToInt(buff[7]);

    checksum = CharToInt(buff[0]) << 4 | CharToInt(buff[1]);
    checksum += CharToInt(buff[2]) << 4 | CharToInt(buff[3]);
    checksum += CharToInt(buff[4]) << 4 | CharToInt(buff[5]);
    checksum += CharToInt(buff[6]) << 4 | CharToInt(buff[7]);
    
    //Serial.print(String(byte_count) + ", " + String(address) + ", " + String(record_type) + ": ");
    
    //Data
    if(record_type == 0) {
      
      if(address != 0 && start == false) {
        if(address != (p_address + p_byte_count))
          break;
      }

      data_temp.read(buff, (byte_count * 2));
      for(i = 0; i < (byte_count * 2); i += 2) {
        ch = CharToInt(buff[i]) << 4 | CharToInt(buff[i + 1]);
        checksum += ch;
        datalog.write(ch);

        //Serial.print(ch, HEX);

        if(++j == 500) {
          j = 0;
          datalog.sync();
        }
      }
    } 
    //End line
    else if(record_type == 1) {
      data_temp.read(buff, 2);
      if(buff[0] == 'F' && buff[1] == 'F')
        flag = true;
      break;
    }    
    //Segment address
    else if(record_type == 2) {
      if(byte_count != 2)
        break;
      data_temp.read(buff, (byte_count * 2));
      offset_address += CharToInt(buff[0]) << 12 | CharToInt(buff[1]) << 8 | CharToInt(buff[2]) << 4 | CharToInt(buff[3]);
      checksum += CharToInt(buff[0]) << 4 | CharToInt(buff[1]);
      checksum += CharToInt(buff[2]) << 4 | CharToInt(buff[3]);
    } 
    //Start segment address
    else if(record_type == 3) {
      if(byte_count != 4)
        break;
      data_temp.read(buff, (byte_count * 2));
      checksum += CharToInt(buff[0]) << 4 | CharToInt(buff[1]);
      checksum += CharToInt(buff[2]) << 4 | CharToInt(buff[3]);
      checksum += CharToInt(buff[4]) << 4 | CharToInt(buff[5]);
      checksum += CharToInt(buff[6]) << 4 | CharToInt(buff[7]);
    } else {
      break;
    }
    
    data_temp.read(buff, 2);
    temp_checksum = CharToInt(buff[0]) << 4 | CharToInt(buff[1]);
    
    /*Serial.print(", ");
    Serial.print(checksum, HEX);
    Serial.print(", ");
    Serial.println(temp_checksum, HEX);*/
    if((checksum + temp_checksum) % 0x100 != 0) 
      break;

    start = false;

    p_byte_count = byte_count;
    p_address = address;
  }
  datalog.sync();
  datalog.close();
  data_temp.close();
  return flag;
}

bool WriteSD(weatherData w) {
  #ifdef USE_HEROKU_URL 
    WriteSD_old(w);
  #endif
  #ifdef USE_CWIG_URL
    WriteSD_NCML(w);
  #endif
}

bool WriteSD_old(weatherData w) {
	bool flag = false;
	char ch, str[100];
	int i;

  SdFile datalog;

  SD_csv_header.toCharArray(str, 100);
	
  if(!sd.exists("datalog.csv")) {
    delay(50);
    if(!datalog.open("datalog.csv", FILE_WRITE)) {
      if(!sd.begin(SD_CARD_CS_PIN)) {
        datalog.close();
        if(SERIAL_OUTPUT)
          Serial.println(F("Could not create CSV file"));
        
        return false;
      }
      if(!datalog.open("datalog.csv", FILE_WRITE)) {
        datalog.close();
        if(SERIAL_OUTPUT)
          Serial.println(F("Could not create CSV file"));
        
        return false;
      }
    }
    delay(50);
    datalog.println(SD_csv_header);
    flag = true;
  } else {
    if(!datalog.open("datalog.csv", FILE_WRITE)) {
      datalog.close();
      if(SERIAL_OUTPUT)
        Serial.println(F("Could not open CSV file"));
      
      return false;
    }
  }

  if(flag == false) {
    datalog.seekSet(0);
  	i = 0;
    flag = true;
  	for(i = 0, flag = true, ch = datalog.read(); ch != '\n' && i < 100; i++){
      if((!isAlpha(ch) && ch != ',') && i >= SD_csv_header.length())
        break;
  		if((ch != str[i]) || i > SD_csv_header.length()) {
  			flag = false;
  			break;
  		}
      ch = datalog.read();
  	};
  }
  
	if(flag == false || i >= 100) {
		datalog.close();
		delay(50);
		sd.remove("datalog.csv");
		delay(50);
		if(!datalog.open("datalog.csv", FILE_WRITE)) {
      datalog.close();
      if(SERIAL_OUTPUT) 
        Serial.println(F("Could not create CSV file"));
      
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
  datalog.write(((w.id / 10000) % 10) + '0');
  datalog.write(((w.id / 1000) % 10) + '0');
  datalog.write(((w.id / 100) % 10) + '0');
  datalog.write(((w.id / 10) % 10) + '0');
  datalog.write((w.id % 10) + '0');
  datalog.print(',');
  datalog.write((w.t.year / 1000) % 10 + '0'); 
  datalog.write((w.t.year / 100) % 10 + '0'); 
  datalog.write((w.t.year / 10) % 10 + '0'); 
  datalog.write((w.t.year % 10) + '0'); 
  datalog.write('/'); 
  datalog.write((w.t.month / 10) % 10 + '0'); 
  datalog.write((w.t.month % 10) + '0');
  datalog.write('/');
  datalog.write((w.t.day / 10) % 10 + '0'); 
  datalog.write((w.t.day % 10) + '0');
  datalog.print(' ');
  datalog.write((w.t.hours / 10) % 10 + '0'); 
  datalog.write((w.t.hours % 10) + '0');
  datalog.write(':');
  datalog.write((w.t.minutes / 10) % 10 + '0'); 
  datalog.write((w.t.minutes % 10) + '0');
  datalog.write(':');
  datalog.write((w.t.seconds / 10) % 10 + '0'); 
  datalog.write((w.t.seconds % 10) + '0');
  datalog.print(',');
  datalog.print(w.temp);
  datalog.print(',');
  datalog.print(w.hum);
  datalog.print(',');
  datalog.print(w.wind_speed);
  datalog.print(',');
  datalog.print(w.wind_direction);
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
  datalog.print(w.leaf_wetness);
  datalog.print(',');
  datalog.print(w.ThermistorPin);
  datalog.print(',');
  datalog.print(w.uv_sensor);
  datalog.print(',');
  datalog.println(w.signal_strength);
  
  if(!datalog.close())
    return false;
	
  return true;
}

bool WriteFailedData(weatherData w) {
  bool flag = false;
  char ch, str[100];
  int i;

  SdFile datalog;
  
  if(!sd.exists("failed.txt")) {
    delay(50);
    if(!datalog.open("failed.txt", FILE_WRITE)) {
      if(!sd.begin(SD_CARD_CS_PIN)) {
        datalog.close();
        if(SERIAL_OUTPUT)
          Serial.println(F("Could not create Failed Data file"));
        
        return false;
      }
      if(!datalog.open("failed.txt", FILE_WRITE)) {
        datalog.close();
        if(SERIAL_OUTPUT)
          Serial.println(F("Could not create Failed Data file"));
        
        return false;
      }
    }
    delay(50);
  } else {
    if(!datalog.open("failed.txt", FILE_WRITE)) {
      datalog.close();
      if(SERIAL_OUTPUT)
        Serial.println(F("Could not open Failed Data file"));
      
      return false;
    }
  }

  datalog.seekEnd();

  datalog.print("<" + w.imei + ";");
  if(w.t.day >= 10)
    datalog.write((w.t.day / 10) % 10 + '0');
  datalog.write(w.t.day % 10 + '0');
  datalog.write('/');
  if(w.t.month >= 10)
    datalog.write((w.t.month / 10) % 10 + '0');
  datalog.write(w.t.month % 10 + '0');
  datalog.write('/');
  datalog.write((w.t.year / 10) % 10 + '0');
  datalog.write(w.t.year % 10 + '0');
  datalog.write(';');
  datalog.write((w.t.hours / 10) % 10 + '0');
  datalog.write(w.t.hours % 10 + '0');
  datalog.write(':');
  datalog.write((w.t.minutes / 10) % 10 + '0');
  datalog.write(w.t.minutes % 10 + '0');
  datalog.write(';');
  datalog.print(String(w.temp_max) + ";");
  datalog.print(String(w.temp) + ";");
  datalog.print(String(w.temp_min) + ";");
  
  datalog.print(String(w.hum_max) + ";");
  datalog.print(String(w.hum) + ";");
  datalog.print(String(w.hum_min) + ";");

  datalog.print(String(w.rain) + ";");
  datalog.print("0;");
  datalog.print("0;");

  datalog.print("0;0;0;0;0;0;0;0;0;0;0;0;");

  datalog.print(String(w.wind_speed_max) + ";");
  datalog.print(String(w.wind_speed) + ";");
  datalog.print(String(w.wind_speed_min) + ";");

  datalog.print(String(w.wind_direction) + ";");
  datalog.print(String(w.panel_voltage) + ";");
  datalog.println(String(w.battery_voltage) + ">");
  delay(300);

  if(!datalog.close())
    return false;
  
  return true;
}

bool UploadFailedData() {
  bool flag = false;
  char ch;
  String str;
  int i;

  SdFile datalog;

  Serial.println();
  
  if(!sd.exists("failed.txt")) {
    if(SERIAL_OUTPUT) Serial.println(F("No failed data file"));
    return false;
  } else {
    if(!datalog.open("failed.txt", FILE_READ)) {
      datalog.close();
      if(SERIAL_OUTPUT) Serial.println(F("Could not open failed Data file"));
      
      return false;
    }
  }

  datalog.seekSet(0);

  if(SERIAL_OUTPUT) Serial.println(F("Uploading past failed records"));
  
  while(datalog.available()) {
    str = "";
    do {
      ch = datalog.read();
      if(ch >= 32 && ch <= 126) {
        str += ch;
      }
    }while(ch != '\n' && datalog.available());
    
    Serial1.print("AT+QHTTPURL=");
    Serial1.print(URL_CWIG.length() + 2 + str.length());
    Serial1.print(",30\r");
    delay(1000);
    ShowSerialData();
      
    Serial1.print(URL_CWIG + "\""); 
    Serial1.print(str);
    Serial1.print("\"\r");

    Serial.print(URL_CWIG + "\""); 
    Serial.print(str);
    Serial.println("\"");

    delay(300);

    ShowSerialData();  
    if(SendATCommand("AT+QHTTPGET=30", "OK", 30000) < 1) {
      GSMReadUntil("\n", 50); ShowSerialData();
      return false;
    }
    GSMReadUntil("\n", 50); ShowSerialData();
      
    ShowSerialData();
    delay(100); 
    if(SendATCommand("AT+QHTTPREAD=30", "OK", 5000) == -1) {
      GSMReadUntil("\n", 50); 
      return false;
    }
    ShowSerialData();
  }

  if(!datalog.close())
    return false;
    
   sd.remove("failed.txt");
}

bool WriteSD_NCML(weatherData w) {
  bool flag = false;
  char ch, str[100];
  int i;

  SdFile datalog;

  char filename[11];

  filename[0] = 'Y';
  filename[1] = (w.t.year / 1000) % 10 + '0';
  filename[2] = (w.t.year / 100) % 10 + '0';
  filename[3] = 'M';
  filename[4] = (w.t.month / 10) % 10 + '0';
  filename[5] = w.t.month % 10 + '0';
  filename[6] = '.';
  filename[7] = 't';
  filename[8] = 'x';
  filename[9] = 't';
  filename[10] = '\0';
  
  if(!sd.exists(filename)) {
    delay(50);
    if(!datalog.open(filename, FILE_WRITE)) {
      if(!sd.begin(SD_CARD_CS_PIN)) {
        datalog.close();
        if(SERIAL_OUTPUT)
          Serial.println(F("Could not create CSV file"));
        
        return false;
      }
      if(!datalog.open(filename, FILE_WRITE)) {
        datalog.close();
        if(SERIAL_OUTPUT)
          Serial.println(F("Could not create CSV file"));
        
        return false;
      }
    }
    delay(50);
    flag = true;
  } else {
    if(!datalog.open(filename, FILE_WRITE)) {
      datalog.close();
      if(SERIAL_OUTPUT)
        Serial.println(F("Could not open CSV file"));
      
      return false;
    }
  }
  
  datalog.seekEnd();
  
  datalog.print("<" + w.imei + ";");
  if(w.t.day >= 10)
    datalog.write((w.t.day / 10) % 10 + '0');
  datalog.write(w.t.day % 10 + '0');
  datalog.write('/');
  if(w.t.month >= 10)
    datalog.write((w.t.month / 10) % 10 + '0');
  datalog.write(w.t.month % 10 + '0');
  datalog.write('/');
  datalog.write((w.t.year / 10) % 10 + '0');
  datalog.write(w.t.year % 10 + '0');
  datalog.write(';');
  datalog.write((w.t.hours / 10) % 10 + '0');
  datalog.write(w.t.hours % 10 + '0');
  datalog.write(':');
  datalog.write((w.t.minutes / 10) % 10 + '0');
  datalog.write(w.t.minutes % 10 + '0');
  datalog.write(';');
  datalog.print(String(w.temp_max) + ";");
  datalog.print(String(w.temp) + ";");
  datalog.print(String(w.temp_min) + ";");
  
  datalog.print(String(w.hum_max) + ";");
  datalog.print(String(w.hum) + ";");
  datalog.print(String(w.hum_min) + ";");

  datalog.print(String(w.rain) + ";");
  datalog.print("0;");
  datalog.print("0;");

  datalog.print("0;0;0;0;0;0;0;0;0;0;0;0;");

  datalog.print(String(w.wind_speed_max) + ";");
  datalog.print(String(w.wind_speed) + ";");
  datalog.print(String(w.wind_speed_min) + ";");

  datalog.print(String(w.wind_direction) + ";");
  datalog.print(String(w.panel_voltage) + ";");
  datalog.println(String(w.battery_voltage) + ">");
  delay(300);
  
  if(!datalog.close())
    return false;
  
  return true;
}

bool ReadConfig() {
  bool flag = false;
  char ch, str[100], svr[100];
  int i, j;

  SdFile datalog;
  
  if(datalog.open("config.txt", FILE_READ)) {
    while(datalog.available()) {
      i = 0;
      ch = datalog.read();
      while(ch != '\n'){
        str[i++] = ch;
        ch = datalog.read(); 
      }
      str[i] = '\0';
      if(tolower(str[0]) == 's' && tolower(str[1]) == 'v' && tolower(str[2]) == 'r') {
        for(j = 4; j < (i - 2); j++) {
          svr[j - 4] = str[j];
        }
        svr[j - 4] = '\0';
        URL_CWIG = String(svr) + "/?var=";
      }
    }
  } 
  datalog.close();
  return true;
}

bool UploadCSV() {
  char ch, sd_ch;
  long int n, lines;
  int stage, count;
  int line_count;
  long int timeout;
  long int p1, p2, bytes;
  bool flag, t_response;

  SdFile datalog;

  if(!sd.exists("id.txt")) {
    if(!sd.begin(SD_CARD_CS_PIN)) 
      return false;
  }
  p1 = GetPreviousFailedUploads(n);
  Serial.println(n);
  if(n < 0) 
    return false;
    
  if(SERIAL_OUTPUT) {
    Serial.print(F("Number of missed uploads: "));
    Serial.println(n);  
  }
  if(n == 0)
    return true;
  
  if(!HttpInit())
    return false;

  if(!datalog.open("datalog.csv", FILE_WRITE)) {
    datalog.close();
    return false;
  }
  
  datalog.seekSet(p1);

  for(lines = 0; lines < n && datalog.available();) {

    p1 = datalog.curPosition();
    for(line_count = 0; line_count < 200 && datalog.available() && (lines + line_count) < n; line_count++) {
      while(datalog.read() != '\n');
    }
    p2 = datalog.curPosition();
    bytes = p2 - p1;
    datalog.seekSet(p1);

    GSMModuleWake();
    delay(200);
    ShowSerialData();
    if(SERIAL_RESPONSE) {
      Serial.println();
    }
    SendATCommand("AT+QIDNSIP=1", "OK", 1000);
    GSMReadUntil("\n", 50); ShowSerialData();
    SendATCommand("AT+QICLOSE", "OK", 1000);
    GSMReadUntil("\n", 50); ShowSerialData();
    if(SendATCommand("AT+QIOPEN=\"TCP\",\"www.yobi.tech\",\"80\"", "CONNECT OK", 20000) < 1) { //enigmatic-caverns-27645.herokuapp.com
      GSMReadUntil("\n", 50); ShowSerialData();
      return false;
    }
    GSMReadUntil("\n", 50); ShowSerialData();
    while(Serial1.availableForWrite() < 50);
    GSMModuleWake();

    t_response = SERIAL_RESPONSE;
    SERIAL_RESPONSE = 0;
    
    SendATCommand("AT+QISEND", ">", 500);
    GSMReadUntil("\n", 50); ShowSerialData();
    
    //--------------------------------------------POST Message---------------------------------------------------
    
    Serial1.print(F("POST /bulk-upload HTTP/1.1\r\n")); //enigmatic-caverns-27645.herokuapp.com
    Serial1.print(F("Host: www.yobi.tech\r\n"));                                                                    GSMReadUntil("\n", 1000);
    Serial1.print(F("Accept: */*\r\n"));                                                                            GSMReadUntil("\n", 1000);
    Serial1.print(F("Accept-Language: en-US,en;q=0.5\r\n"));                                                        GSMReadUntil("\n", 1000);
    Serial1.print(F("Accept-Encoding: gzip, deflate\r\n"));                                                         GSMReadUntil("\n", 1000);
    Serial1.print(F("Connection: keep-alive\r\n"));                                                                 GSMReadUntil("\n", 1000); 
    Serial1.print(F("Pragma: no-cache\r\n"));                                                                       GSMReadUntil("\n", 1000);
    Serial1.print(F("Cache-Control: no-cache\r\n"));                                                                GSMReadUntil("\n", 1000); 
    Serial1.print(F("User-Agent: Quectel\r\n"));                                                                    GSMReadUntil("\n", 1000); 
    Serial1.print(F("Content-Type: multipart/form-data; boundary=---------------------------196272297923078\r\n")); GSMReadUntil("\n", 1000); 
    Serial1.print(F("Content-Length: "));
    Serial1.print(String(207 + bytes + SD_csv_header.length()) + "\r\n");                                           GSMReadUntil("\n", 1000); 
    Serial1.print(F("\r\n"));                                                                                       GSMReadUntil("\n", 1000);
    Serial1.print(F("-----------------------------196272297923078\r\n"));                                           GSMReadUntil("\n", 1000); 
    Serial1.print(F("Content-Disposition: form-data; name=\"file\"; filename=\"datalog.csv\"\r\n"));                GSMReadUntil("\n", 1000); 
    Serial1.print(F("Content-Type: application/octet-stream\r\n"));                                                 GSMReadUntil("\n", 1000); 
    Serial1.print(F("\r\n"));                                                                                       GSMReadUntil("\n", 1000); 
    Serial1.println(SD_csv_header);                                                                                 GSMReadUntil("\n", 1000); 
    
    Serial1.write(0x1A);
    
    GSMReadUntil("OK", 10000);
    ShowSerialData();

    SendATCommand("AT+QISEND", ">", 1000);
    ShowSerialData();
    
    //Send CSV file data
    for(count = 0, line_count = 0; line_count < 200 && datalog.available();) {
      do {
        sd_ch = datalog.read();
        count++;
        Serial1.print(sd_ch);
        while(Serial1.available() == 0);
        ShowSerialData();
      }while(sd_ch != '\n' && datalog.available());
      ShowSerialData();

      lines++;
      line_count++;

      if(count >= 1200) { 
        count = 0;
        ShowSerialData();
        Serial1.write(0x1A);
        
        GSMReadUntil("OK", 10000);
        
        ShowSerialData();
        
        SendATCommand("AT+QISEND", ">", 1000);
        ShowSerialData();
      }
      if(lines == n) 
        break;
    }
    
    delay(500);
    ShowSerialData();
    Serial1.print("\r\n");
    while(Serial1.availableForWrite() < 50); ShowSerialData();
    Serial1.print(F("-----------------------------196272297923078--\r"));
    
    ShowSerialData();
    Serial1.write(0x1A);

    GSMReadUntil("OK", 10000);

    //--------Read POST response-------

    if(!GSMReadUntil("200 OK", 30000)) {
      GSMReadUntil("\n", 50); ShowSerialData();
      datalog.close();
      return false;
    }

    if(!GSMReadUntil("\"success\"", 30000)) {
      GSMReadUntil("\n", 50); ShowSerialData();
      datalog.close();
      return false;
    }
    GSMReadUntil("\n", 500);
    ShowSerialData();

    if(SERIAL_OUTPUT) {
      Serial.print(F("Succesfully uploaded "));
      Serial.print(line_count);
      Serial.println(F(" records"));  
    }

    datalog.seekSet(p1);
    for(; line_count > 0; line_count--) {
      datalog.write('1');
      while(datalog.read() != '\n');
    }
    datalog.seekSet(p2);
    
    SERIAL_RESPONSE = t_response;
  }

  datalog.close();
  return true;
}

long int GetPreviousFailedUploads(long int &n) {
  unsigned long timeout;
  char ch1, ch2;
  long int lines = 0, p;
  int i;
  n = 0;

  SdFile datalog;
	
  if(!sd.exists("id.txt")) {
    if(!sd.begin(SD_CARD_CS_PIN)) {
      return -1;
    }
  }
  if(!sd.exists("datalog.csv")) {
    return -1;
  }
  delay(200);
  if(!datalog.open("datalog.csv", FILE_READ)) {
    datalog.close();
    return -1;
  }
	delay(200);
  datalog.seekEnd();
  timeout = millis();
  do {
    datalog.seekCur(-6);
    while(datalog.peek()!= '\n' && datalog.curPosition() != 0 && (millis() - timeout) < 200000) {
      datalog.seekCur(-1);
    }
    if(datalog.curPosition() == 0) 
      break;
    datalog.seekCur(1);
    ch1 = datalog.read();
    datalog.seekCur(1);
    ch2 = datalog.read();
    if(ch1 == '0' && ch2 == '1') 
      lines++;
    if((millis() - timeout) > 200000) {
      datalog.close();
      return -1;
    }
  }while(ch1 == '0' && ch2 == '1');
  for(timeout = millis(); datalog.read() != '\n' && datalog.available() && (millis() - timeout) < 1000;);
  if((millis() - timeout) >= 1000) {
    datalog.close();
    return -1;
  }
  p = datalog.curPosition();
  datalog.close();
  
  n = lines;
  return p;
}

bool WriteOldTime(int n, realTime t) {
  unsigned long timeout;
  long int temp_n;
  int lines, i;
  char str[20], ch;
  realTime temp;

  SdFile datalog;

  if(!sd.exists("id.txt")) {
    if(!sd.begin(SD_CARD_CS_PIN)) {
      if(SERIAL_OUTPUT) {
        Serial.println(F("No SD Card detected"));
      }
      return false;
    }
  }
  if(!sd.exists("datalog.csv")) {
    if(SERIAL_OUTPUT) {
      Serial.println(F("No CSV file detected"));
    }
    return false;
  }

  delay(500);
  datalog.open("datalog.csv", FILE_WRITE);
  datalog.seekEnd();
  lines = 0;
  do {
    datalog.seekCur(-2);
    for(timeout = millis(); datalog.peek()!= '\n' && datalog.curPosition() != 0 && (millis() - timeout) < 4000;) {
      datalog.seekCur(-1);
    }
    if(datalog.curPosition() == 0) 
      break;
    datalog.seekCur(1);
    ch = datalog.peek();
    if(ch == '0') 
      lines++;
    if((millis() - timeout) > 4000) {
      datalog.close();
      return false;
    }
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
    for(timeout = millis(); datalog.read() != ',' && (millis() - timeout) < 1000;);

    if((millis() - timeout) > 1000) {
      datalog.close();
      return false;
    }
		
    datalog.write('1');
		
    //Read past Id flag
    for(timeout = millis(); datalog.read() != ',' && (millis() - timeout) < 1000;);
    if((millis() - timeout) > 1000) {
      datalog.close();
      return false;
    }
    for(timeout = millis(); datalog.read() != ',' && (millis() - timeout) < 1000;);		
    if((millis() - timeout) > 1000) {
      datalog.close();
      return false;
    }

    str[0] = datalog.read();
    str[1] = datalog.read();
    str[2] = datalog.read();
    str[3] = datalog.read();
    str[4] = '\0';
    temp.year = String(str).toInt();
    datalog.seekCur(1);

    str[0] = datalog.read();
    str[1] = datalog.read();
    str[2] = '\0';
    temp.month = String(str).toInt();
    datalog.seekCur(1);
   
    str[0] = datalog.read();
    str[1] = datalog.read();
    str[2] = '\0';
    temp.day = String(str).toInt();
    datalog.seekCur(1);

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

    datalog.seekCur(-19);

    datalog.write((temp.year / 1000) % 10 + '0'); 
    datalog.write((temp.year / 100) % 10 + '0'); 
    datalog.write((temp.year / 10) % 10 + '0'); 
    datalog.write((temp.year % 10) + '0');
    datalog.seekCur(1);
    datalog.write((temp.month / 10) % 10 + '0'); 
    datalog.write((temp.month % 10) + '0');
    datalog.seekCur(1);
    datalog.write((temp.day / 10) % 10 + '0'); 
    datalog.write((temp.day % 10) + '0');
    datalog.seekCur(1);
    datalog.write((temp.hours / 10) % 10 + '0'); 
    datalog.write((temp.hours % 10) + '0');
    datalog.seekCur(1);
    datalog.write((temp.minutes / 10) % 10 + '0'); 
    datalog.write((temp.minutes % 10) + '0');
    datalog.seekCur(1);
//    datalog.write((temp.seconds / 10) % 10 + '0'); 
//    datalog.write((temp.seconds % 10) + '0');
     datalog.write('0'); 
    datalog.write('0');

    for(timeout = 0; datalog.read() != '\n' && datalog.available() && timeout < 2000; timeout++);
      delay(1);
    if(timeout > 2000) {
      datalog.close();
      return false;
    }
  }
  datalog.close();
  return true;
}

bool WriteOldID(int n, int id) {
  unsigned long timeout;
  long int temp_n;
  int lines, i;
  char str[20], ch;
  realTime temp;

  SdFile datalog;

  if(!sd.exists("id.txt")) {
    if(!sd.begin(SD_CARD_CS_PIN)) {
      if(SERIAL_OUTPUT) {
        Serial.println(F("No SD Card detected"));
      }
      return false;
    }
  }
  if(!sd.exists("datalog.csv")) {
    if(SERIAL_OUTPUT) {
      Serial.println(F("No CSV file detected"));
    }
    return false;
  }
  GetPreviousFailedUploads(temp_n);
  if(temp_n < n) 
    return false;

  delay(500);
  datalog.open("datalog.csv", FILE_WRITE);
  datalog.seekEnd();
  lines = 0;
  do {
    datalog.seekCur(-2);
    for(timeout = millis(); datalog.peek()!= '\n' && datalog.curPosition() != 0 && (millis() - timeout) < 4000;) {
      datalog.seekCur(-1);
    }
    if(datalog.curPosition() == 0) 
      break;
    datalog.seekCur(1);
    ch = datalog.peek();
    if(ch == '0') 
      lines++;
    if((millis() - timeout) > 4000) {
      datalog.close();
      return false;
    }
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
    for(timeout = millis(); datalog.read() != ',' && (millis() - timeout) < 1000;);

    if((millis() - timeout) > 1000) {
      datalog.close();
      return false;
    }
    //Read past time flag
    for(timeout = millis(); datalog.read() != ',' && (millis() - timeout) < 1000;);

    if((millis() - timeout) > 1000) {
      datalog.close();
      return false;
    }
    
    datalog.write(((id / 10000) % 10) + '0');
    datalog.write(((id / 1000) % 10) + '0');
    datalog.write(((id / 100) % 10) + '0');
    datalog.write(((id / 10) % 10) + '0');
    datalog.write((id % 10) + '0');

    for(timeout = 0; datalog.read() != '\n' && datalog.available() && timeout < 2000; timeout++);
      delay(1);
    if(timeout > 2000) {
      datalog.close();
      return false;
    }
  }
  datalog.close();
  return true;
}

uint8_t CharToInt(unsigned char &a){
  uint8_t r = 0;
  if((a >= '0' && a <= '9')) {
    r = a - '0';
  } 
  else if(a >= 'a' && a <= 'f') {
    r = a -  87;
  }
  else if(a >= 'A' && a <= 'F') {
    r = a - 55;
  }
  return r;
}
