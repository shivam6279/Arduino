#include <Arduino.h>
#include <SPI.h>
#include <esp_now.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <FS.h>

SPIClass spiSD(HSPI);

const char* ssid = "POV-Access-Point";
const char* password = "POVDisplay";

const char* master_ip = "192.168.4.1";

uint8_t gif_arr[250000];

void setup() {
  Serial.begin(115200);
  
//  WiFi.mode(WIFI_STA);
//  Serial.print("Mac Address in Station: "); 
//  Serial.println(WiFi.macAddress());

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to "); Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.print(F("Connected. My IP address is: "));
  Serial.println(WiFi.localIP());
}

WiFiClient wifiClient;
IPAddress server(192, 168, 4, 1);

void loop() {  
//  gif_arr[200000] = 0;
  
  while(1) {
    if(wifiClient.connect(server, 80)) { // Connection to the server
      while(wifiClient.connected()) {
        wifiClient.setTimeout(5000);
  //      wifiClient.println("Hello!"); 
  //      String answer = wifiClient.readStringUntil('\r');
        wifiClient.flush();
  //      Serial.println(answer);
        delay(100);
        while(wifiClient.available()) {
          char c = wifiClient.read();
          Serial.write(c); 
        }
      }
      delay(1000);
    }
  }
}
