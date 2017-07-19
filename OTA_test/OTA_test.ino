#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <String.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
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

File data, data_temp;

void setup(){
  pinMode(rain_led, OUTPUT);
  pinMode(wind_led, OUTPUT);
  pinMode(upload_led, OUTPUT);
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
  delay(10000);//Wait for the SIM to log on to the network
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
  if(SD.begin()){
    Serial.println("SD card detected");
  }
  else{
    Serial.println("SD card not detected");
    return;
  }
  delay(1000);
  /*if(SD.exists("TEMP_OTA.HEX")){
    SD.remove("TEMP_OTA.HEX");
    Serial.println("OTA_temp.hex removed");
  }*/
  if(SD.exists("firmware.BIN")){
    SD.remove("firmware.BIN");
    Serial.println("firmware.bin removed");
  }
  delay(1000);
  //downloadBin();
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
  data = SD.open("temp_ota.hex", FILE_WRITE);
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
  /*while(Serial1.read() != '/');
  do{
    ch = Serial1.read();
  }while(ch < 48 || ch > 57);
  for(i = 0; ((ch >= 48 && ch <= 57) || ch == '.'); i++){
    temp_version[i] = ch;
    while(!Serial1.available());
    ch = Serial1.read();
  }
  temp_version[i] = '\0';*/
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
  data.seek(data.position() - 1);
  data.close();
  while(Serial1.available()) Serial1.read();
  Serial.println("Done");
  return true;
}

bool SD_copy(){
  unsigned char buff[45], ch;
  int i, j, temp_checksum;
  long int tt, checksum = 0;
  if(!SD.exists("TEMP_OTA.HEX")){
    return 0;
  }
  data_temp = SD.open("TEMP_OTA.HEX", FILE_READ);
  data = SD.open("firmware.bin", O_WRITE | O_CREAT);

  ch = data_temp.read();
  while((ch < 97 && ch > 58) || ch < 48 || ch > 102){
    ch = data_temp.read();
  }
  tt = millis();
  data_temp.seek(data_temp.position() - 1);
  j = 0;
  while(data_temp.available()){
    data_temp.read(buff, 45);
    for(i = 9, checksum = 0; i < 41; i++){
      //Serial.write(buff[i]);
      if((buff[i] >= 48 && buff[i] <= 58) || (buff[i] >= 97 && buff[i] <= 102)){
        if(buff[i] >= 97 && buff[i] <= 102){
          buff[i] -= 87;
        }
        else{
          buff[i] -= 48;
        }
        i++;
        if(buff[i] >= 97 && buff[i] <= 102){
          buff[i] -= 87;
        }
        else{
          buff[i] -= 48;
        }
        ch = buff[i - 1] * 16 + buff[i];
        checksum += ch;
        data.write(ch); 
        if(++j == 200){
          j = 0;
          data.flush();
        }
      }
    }
    if((buff[i] >= 48 && buff[i] <= 58) || (buff[i] >= 97 && buff[i] <= 102)){
      if(buff[i] >= 97 && buff[i] <= 102){
        buff[i] -= 87;
      }
      else{
        buff[i] -= 48;
      }
      i++;
      if(buff[i] >= 97 && buff[i] <= 102){
        buff[i] -= 87;
      }
      else{
        buff[i] -= 48;
      }
      temp_checksum = buff[i - 1] * 16 + buff[i];
    }
    if((checksum - temp_checksum) % 0x100 == 0) Serial.println("Checksum approved");
    else Serial.println("Checksum failed");
  }
  data.flush();
  data.close();
  data_temp.close();
  tt = millis() - tt;
  Serial.println("Time taken: " + String(float(tt) / 1000.0));
  return 1;
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
