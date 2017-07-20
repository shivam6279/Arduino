#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <String.h>
#include <Wire.h>
#include <SPI.h>
#include <SdFat.h>
#include <avr/wdt.h>

#define rain_led 35
#define wind_led 36
#define upload_led 37
#define rain_pin1 48
#define rain_pin2 49
#define wind_pin1 46
#define wind_pin2 47
#define GSM_module_DTR_pin 2
#define solar_radiation_pin A0


SdFat sd;
SdFile data, data_temp;

void setup(){
  pinMode(rain_led, OUTPUT);
  pinMode(wind_led, OUTPUT);
  pinMode(upload_led, OUTPUT);
  pinMode(53, OUTPUT);
  pinMode(rain_pin1, INPUT);
  pinMode(rain_pin2, INPUT);
  pinMode(wind_pin1, INPUT);
  pinMode(wind_pin2, INPUT);
  pinMode(GSM_module_DTR_pin, OUTPUT);
  digitalWrite(GSM_module_DTR_pin, LOW);

  Serial.begin(9600); 
  Serial1.begin(19200);
  pinMode(19, INPUT);  
  digitalWrite(19, HIGH);
  Serial2.begin(9600);
  pinMode(17, INPUT);  
  digitalWrite(17, HIGH);
  Serial3.begin(9600);


  Serial.println("Ota test");
  
  delay(100);
  
  Wire.begin();

  //talk();

  //GSM_module_reset();
  digitalWrite(upload_led, HIGH);
  //delay(10000);//Wait for the SIM to log on to the network
  digitalWrite(upload_led, LOW);
  Serial1.print("AT+CMGF=1\r"); delay(200);
  #if serial_response
  ShowSerialData();
  #endif
  Serial1.print("AT+CNMI=2,2,0,0,0\r"); delay(200); 
  #if serial_response
  ShowSerialData();
  #endif
  Serial1.print("AT+CLIP=1\r"); delay(200);
  ShowSerialData();
  delay(1000); 
  //GSM_module_sleep();
}
 
void loop(){
  if(sd.begin(53)){
    Serial.println("SD card detected");
  }
  else{
    Serial.println("SD card not detected");
    return;
  }
  delay(1000);
  if(sd.exists("TEMP_OTA.HEX")){
    sd.remove("TEMP_OTA.HEX");
    Serial.println("OTA_temp.hex removed");
  }
  if(sd.exists("firmware.BIN")){
    sd.remove("firmware.BIN");
    Serial.println("firmware.bin removed");
  }
  delay(1000);
  downloadBin();
  delay(1000);
  Serial.println("Converting .hex file to .bin");
  SD_copy();
  Serial.println("Done, restarting");
  EEPROM.write(0x1FF,0xF0);
  wdt_enable(WDTO_500MS);
  wdt_reset();
  delay(600);
}

bool downloadBin(){  
  int i;
  char ch, temp_version[10];
  data.open("temp_ota.hex", FILE_WRITE);
  Serial.println("Uploading data");
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
  Serial1.print("AT+QHTTPURL=26,30\r");
  delay(1000);
  ShowSerialData();
  while(Serial1.available()) Serial1.read();
  Serial1.print("http://www.yobi.tech/ota/4\r");
  delay(100);
  ShowSerialData();
  while(Serial1.available()) Serial1.read();
  Serial1.println("AT+QHTTPGET=30\r");
  delay(700);
  do{
    if(Serial1.available()){
      ch = Serial1.read();
      Serial.write(ch);
    }
  }while(!isdigit(ch));
  
  do{
    if(Serial1.available()){
      ch = Serial1.read();
      Serial.write(ch);
    }
  }while(!isAlpha(ch));
  delay(100);
  ShowSerialData();
  if(ch != 'O'){
    Serial.println("\nCould not connect to server");
    return false;
  }
  Serial.println("\nSuccessfully connected to server");
  Serial1.println("AT+QHTTPREAD=30\r");
  for(i = 0; i < 3;){
    if(Serial1.read() ==  '>') i++;
  }
  ch = Serial1.read();
  while(!isDigit(ch) && ch != ':') ch = Serial1.read();
  data.write(ch);
  i = 0;
  while(1){
    while(Serial1.available()){
      ch = Serial1.read();
      if(ch == 'O') break;
      data.write(ch);
      i = 0;
    }
    if(ch == 'O') break;
    while(Serial1.available() == 0){
      i++;
      delay(1);
      if(i > 3000) break;
    }
    if(i > 3000) break;
  }
  data.seekCur(-1);
  data.close();
  while(Serial1.available()) Serial1.read();
  Serial.println("Done");
  return true;
}

bool SD_copy(){
  unsigned char buff[45], ch, n, t_ch[2];
  uint16_t i, j, temp_checksum;
  long int checksum;
  if(!sd.exists("TEMP_OTA.HEX")){
    return 0;
  }
  data_temp.open("TEMP_OTA.HEX", FILE_READ);
  data.open("firmware.bin", O_WRITE | O_CREAT);

  ch = data_temp.read();
  while((ch < 97 && ch > 58) || ch < 48 || ch > 102){
    ch = data_temp.read();
  }
  data_temp.seekCur(-1);
  j = 0;
  while(data_temp.available()){
    data_temp.read(buff, 9);
    for(i = 1, checksum = 0; i < 9; i++){
      t_ch[0] = buff[i++];
      t_ch[1] = buff[i];
      if((t_ch[0] >= 48 && t_ch[0] <= 57) || (t_ch[0] >= 97 && t_ch[0] <= 102) || (t_ch[0] >= 65 && t_ch[0] <= 70)){
        if((t_ch[0] >= 48 && t_ch[0] <= 57)) t_ch[0] -= 48;
        else{
          if(t_ch[0] >= 97 && t_ch[0] <= 102) t_ch[0] -= 87;
          else t_ch[0] -= 55;
        }
        if((t_ch[1] >= 48 && t_ch[1] <= 57)) t_ch[1] -= 48;
        else{
          if(t_ch[1] >= 97 && t_ch[1] <= 102) t_ch[1] -= 87;
          else t_ch[1] -= 55;
        }
        ch = t_ch[0] * 16 + t_ch[1];
        checksum += ch;
        if(i == 2) n = ch;
      }
    }
    data_temp.read(buff, (n*2)+4);
    for(i = 0; i < (n*2); i++){
      t_ch[0] = buff[i++];
      t_ch[1] = buff[i];
      if((t_ch[0] >= 48 && t_ch[0] <= 57) || (t_ch[0] >= 97 && t_ch[0] <= 102) || (t_ch[0] >= 65 && t_ch[0] <= 70)){
        if((t_ch[0] >= 48 && t_ch[0] <= 57)) t_ch[0] -= 48;
        else{
          if(t_ch[0] >= 97 && t_ch[0] <= 102) t_ch[0] -= 87;
          else t_ch[0] -= 55;
        }
        if((t_ch[1] >= 48 && t_ch[1] <= 57)) t_ch[1] -= 48;
        else{
          if(t_ch[1] >= 97 && t_ch[1] <= 102) t_ch[1] -= 87;
          else t_ch[1] -= 55;
        }
        ch = t_ch[0] * 16 + t_ch[1];
        checksum += ch;
        data.write(ch);
        if(++j == 200){
          j = 0;
          data.flush();
        }
      }
    }
    t_ch[0] = buff[(n*2)];
    t_ch[1] = buff[(n*2) + 1];
    if((t_ch[0] >= 48 && t_ch[0] <= 57) || (t_ch[0] >= 97 && t_ch[0] <= 102) || (t_ch[0] >= 65 && t_ch[0] <= 70)){
      if((t_ch[0] >= 48 && t_ch[0] <= 57)){
        t_ch[0] -= 48;
      }
      else{
        if(t_ch[0] >= 97 && t_ch[0] <= 102) t_ch[0] -= 87;
        else t_ch[0] -= 55;
      }
      if((t_ch[1] >= 48 && t_ch[1] <= 57)){
        t_ch[1] -= 48;
      }
      else{
        if(t_ch[1] >= 97 && t_ch[1] <= 102) t_ch[1] -= 87;
        else t_ch[1] -= 55;
      }
      temp_checksum = t_ch[0] * 16 + t_ch[1];
    }
    if((checksum + temp_checksum) % 0x100 != 0) return false;
  }
  data.flush();
  data.close();
  data_temp.close();
  return true;
}

void talk(){
  Serial.println("Talk");
  while(1){
    if(Serial1.available()){ Serial.write(Serial1.read()); }
    if(Serial.available()){ Serial1.write(Serial.read()); }
  }
}

void ShowSerialData(){
  while(Serial1.available() != 0) Serial.write(Serial1.read());
}
