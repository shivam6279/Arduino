#include "SD.h"
#include <SdFat.h>
#include "GSM.h"
#include "settings.h"

bool DownloadHex() {
  int i, j;
  unsigned long t;
  char ch;
  sd.chdir();
  if(!sd.exists("id.txt")) {
    if(!sd.begin(53)) return false;
  }
  if(!sd.exists("OtaTemp")){
    if(!sd.mkdir("OtaTemp")) return false;
  }

  datalog.open("OtaTemp/temp_ota.hex",  FILE_WRITE);
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
  Serial1.print("AT+QHTTPURL=26,30\r");
  delay(1000);
  #if SERIAL_RESPONSE
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif  
  Serial1.print("http://www.yobi.tech/ota/4\r");
  delay(100);
  #if SERIAL_RESPONSE
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif  
  while(Serial1.available()) Serial1.read();
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
  }while(!isdigit(ch) && j < 3000);
  if(j >= 3000) return false;
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
  }while(!isAlpha(ch) && j < 3000);
  if(j >= 3000) return false;
  delay(100);
  #if SERIAL_RESPONSE
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif  
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
  //datalog.write(ch);
  //--------------------------------------

  i = 0;
  j = 1;
  t = millis();
  while(1){
    while(Serial1.available()){
      ch = Serial1.read();
      if(ch == 'O') break;
      datalog.write(ch);
      i = 0;
      Serial.print(ch);
    }
    if(ch == 'O') break;
    while(Serial1.available() == 0){
      i++;
      delay(1);
      if(i > 3000) break;
    }
    if(i > 3000) break;
  }
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

bool WriteSD(weatherData w, wtime t) {
  sd.chdir();
  if(!sd.exists("id.txt")) {
    if(!sd.begin(53)) return false;
  }
  if(!sd.exists("Datalog")) {
    if(!sd.mkdir("Datalog")) return false;
  }

  datalog.open("Datalog/datalog.txt", FILE_WRITE);
  datalog.seekEnd();
  datalog.print('<'); 
  datalog.print("'id':" + String(w.id) + ",");
  datalog.print("'ts':");
  datalog.write((t.day / 10) % 10 + '0'); datalog.write((t.day % 10) + '0');
  datalog.write('/');
  datalog.write((t.month / 10) % 10 + '0'); datalog.write((t.month % 10) + '0');
  datalog.write('/');
  datalog.write((t.year / 1000) % 10 + '0'); datalog.write((t.year / 100) % 10 + '0'); datalog.write((t.year / 10) % 10 + '0'); datalog.write((t.year % 10) + '0');
  datalog.print(" ");
  datalog.write((t.hours / 10) % 10 + '0'); datalog.write((t.hours % 10) + '0');
  datalog.write(':');
  datalog.write((t.minutes / 10) % 10 + '0'); datalog.write((t.minutes % 10) + '0');
  datalog.write(':');
  datalog.write((t.seconds / 10) % 10 + '0'); datalog.write((t.seconds % 10) + '0');
  datalog.print(",'t1':" + String(w.temp1) + ",");
  datalog.print("'t2':" + String(w.temp2) + ",");
  datalog.print("'h':" + String(w.hum) + ",");
  datalog.print("'w':" + String(w.wind_speed) + ",");
  datalog.print("'r':" + String(w.rain) + ",");
  datalog.print("'p':" + String(w.pressure) + ",");
  datalog.print("'s':" + String(w.solar_radiation) + ",");
  datalog.print("'lt':" + String(w.latitude) + ",");
  datalog.print("'ln':" + String(w.longitude) + ",");
  datalog.println("'sg':" + String(w.signal_strength) + ">");
  datalog.close();
  return true;
}

void CharToInt(unsigned char &a){
  if((a >= '0' && a <= '9')) a -= '0';
  else{
    if(a >= 'a' && a <= 'f') a -= 87;
    else a -= 55;
  }
}
