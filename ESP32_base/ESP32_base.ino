#include <Arduino.h>
#include <SPI.h>
#include <esp_now.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#include <FS.h>
#include <SD.h>

SPIClass spiSD(HSPI);

const char* ssid = "POV-Access-Point";
const char* password = "POVDisplay";

const char* slave_ip = "192.168.4.2";

bool SD_present = 0;

WiFiServer server(80);

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
  } else {
    SD_present = 1;
  }

  root = SD.open("/");
  
//  WiFi.mode(WIFI_STA);
//  Serial.print("Mac Address in Station: "); 
//  Serial.println(WiFi.macAddress());

    
//  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.softAP(ssid, password);  
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
}

WiFiClient wifiClient;
IPAddress client_ip(192, 168, 4, 2);

void loop() {
  uint8_t r;
  String arr[15];
  r = gif_list(arr);

  for(uint8_t i = 0; i < r; i++) {
    Serial.println(arr[i]);
  }  

  Serial.println("***");
  
  while(1) {
    wifiClient = server.available();
    if (wifiClient) {
      wifiClient.setTimeout(5000);
      Serial.println("New Client: " + String(wifiClient.remoteIP()));
      while(wifiClient.connected()) {
        
        server.streamFile(root, "mario.gif");
        wifiClient.flush();
        
        if (wifiClient.available()) {
          char c = wifiClient.read();
          Serial.write(c); 
        }
        delay(1000);
      }
      wifiClient.stop();
      Serial.println("Client disconnected.");
      Serial.println("");
    }
    delay(1000);
  }
}
