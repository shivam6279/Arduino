#include <Arduino.h>
#include <SPI.h>
#include <esp_now.h>
#include <WiFi.h>

static const int spiClk = 1000000; // 1 MHz

#define VSPI_MISO   MISO
#define VSPI_MOSI   MOSI
#define VSPI_SCLK   SCK
#define VSPI_SS     SS

SPIClass * vspi = NULL;

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
int rx = 0;
int g[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void setup() {
  Serial.begin(115200);
  vspi = new SPIClass(VSPI);
  vspi->begin();
  pinMode(VSPI_SS, OUTPUT); //VSPI SS
  
  WiFi.mode(WIFI_STA);
  Serial.print("Mac Address in Station: "); 
  Serial.println(WiFi.macAddress());
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
  }

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  int t[10] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100}, i;
  
//  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &t, sizeof(t));

  if(rx) {
    rx = 0;
    for(i = 0; i < 10; i++) {
      Serial.print(g[i]);
      Serial.print('\t');
    }
    Serial.print('\n');
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  rx = 1;
  memcpy(&g, incomingData, len);
  Serial.print("Bytes received: ");
  Serial.println(len);
}

void LED_test() {
  while(1) { 
    int i = 0, k = 255;

    //use it as you would the regular arduino SPI API
    vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
  
    for(i = 0; i < 100; i++) {
      vspi->transfer(255);
      vspi->transfer(k);
      vspi->transfer(k);
      vspi->transfer(k);
    }
  
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->endTransaction();
  
    delay(50);
  }
}
