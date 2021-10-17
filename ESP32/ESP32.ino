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

void setup() {
  Serial.begin(115200);
  vspi = new SPIClass(VSPI);
  vspi->begin();
  pinMode(VSPI_SS, OUTPUT); //VSPI SS
  
  WiFi.mode(WIFI_STA);
  Serial.print("Mac Address in Station: "); 
  Serial.println(WiFi.macAddress());
}

void loop() {
  //Power_test(96, 128);
  LED_test(96, 10);
}

void Power_test(int len, int b) {
  while(1) { 
    int i = 0;
    
    //use it as you would the regular arduino SPI API
    vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
  
    for(i = 0; i < len; i++) {
      vspi->transfer(255);
      vspi->transfer(b);
      vspi->transfer(b);
      vspi->transfer(b);
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

void LED_test(int len, int b) {
  while(1) { 
    int i = 0;
    vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
  
    for(i = 0; i < len; i++) {
      vspi->transfer(255);
      vspi->transfer(b);
      vspi->transfer(0);
      vspi->transfer(0);
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
    delay(1000);

    vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
  
    for(i = 0; i < len; i++) {
      vspi->transfer(255);
      vspi->transfer(0);
      vspi->transfer(b);
      vspi->transfer(0);
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
    delay(1000);

    vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
    vspi->transfer(0);
  
    for(i = 0; i < len; i++) {
      vspi->transfer(255);
      vspi->transfer(0);
      vspi->transfer(0);
      vspi->transfer(b);
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
    delay(1000);
  }
}
