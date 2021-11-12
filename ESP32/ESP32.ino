#include <Arduino.h>
#include <SPI.h>
#include <esp_now.h>
#include <WiFi.h>

#include <FS.h>
#include <SD.h>

SPIClass spiSD(HSPI);

File root;
uint8_t gif_list(String arr[]) {  
  String n;
  uint8_t r = 0;
  File root = SD.open("/");
  
  while(1) {
    File entry =  root.openNextFile();
  
    if(!entry) {      
      break;
    }

    String n = entry.name();
    if(n.endsWith(".gif")) {
      arr[r] = n;
      r++;      
    }
    entry.close();
  }
  return r;
}

void setup() {
  Serial.begin(115200);
  pinMode(15, OUTPUT); //VSPI SS

//  spiSD.begin();
  spiSD.begin(14, 12, 13, -1);

  if(!SD.begin(15, spiSD)) {
    Serial.println("Card Mount Failed");    
  }   

  root = SD.open("/");
  
//  WiFi.mode(WIFI_STA);
  Serial.print("Mac Address in Station: "); 
  Serial.println(WiFi.macAddress());
}

void loop() {
  uint8_t r;
  String arr[15];
  r = gif_list(arr);

  for(uint8_t i = 0; i < r; i++) {
    Serial.println(arr[i]);
  }
  
  while(1);
}
