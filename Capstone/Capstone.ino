#include <SPI.h>
#include <Wire.h>

//#include <SD.h>

//File myFile;

#define LIS3DH_ADDRESS 0x18
#define CTRL_REG1 0x20
#define CTRL_REG2 0x21
#define CTRL_REG3 0x22
#define CTRL_REG4 0x23
#define CTRL_REG5 0x24
#define CTRL_REG6 0x25

#define MATRIX_SIZE 7

class led_matrix{
public:
  uint8_t r[MATRIX_SIZE][MATRIX_SIZE], g[MATRIX_SIZE][MATRIX_SIZE], b[MATRIX_SIZE][MATRIX_SIZE];

  void write_buffer() {
    int8_t i, j;
    start_frame();
    for(i = 0; i < MATRIX_SIZE; i++) {
      if(i % 2) {
        for(j = 0; i < MATRIX_SIZE; j++) {
          LED_frame(r[i][j], g[i][j], b[i][j]);
        }
      } else {
        for(j = MATRIX_SIZE - 1; j >= 0; j--) {
          LED_frame(r[i][j], g[i][j], b[i][j]);
        }
      }
    }
    end_frame();
  }

  void start_frame() {
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
  }

  void end_frame(){
    SPI.transfer(0xFF);
    SPI.transfer(0xFF);
    SPI.transfer(0xFF);
    SPI.transfer(0xFF);
  }

  void LED_frame(uint8_t red, uint8_t green, uint8_t blue){
    SPI.transfer(0xFF);
    SPI.transfer(blue);
    SPI.transfer(green);
    SPI.transfer(red);
  }
};

void setup() {
  Serial.begin(230400);
  
  Wire.begin();

  // enable all axes, High res 1.3 KHz
  LIS3DH_write8(CTRL_REG1, 0x97);

  // High res & BDU enabled
  LIS3DH_write8(CTRL_REG4, 0x88);

  // DRDY on INT1
  LIS3DH_write8(CTRL_REG3, 0x10);

  // enable adcs
  LIS3DH_write8(0x1F, 0x80);

//  SD.begin(10);
//  myFile = SD.open("data.csv", O_WRITE | O_CREAT);

//  start_frame();
//  for(int i = 0; i < 8; i++) {
//    LED_frame(50, 0, 50);
//  }
//  end_frame();
}

void loop() {
  float x, y, z;
  float mag;
  int i;
  
  int t, tt;
  uint16_t s = millis();

  uint16_t counter = 0;

  tt = millis();
  while(1) {
    t = micros();
    LIS3DH_read(&x, &y, &z);

    Serial.print((int)x);
    Serial.print(", ");
    Serial.print((int)y);
    Serial.print(", ");
    Serial.println((int)z);

    while((micros() - t) < 750);

    if(millis() - tt > 6000) {
      while(1);
    }
  }

//  for(i = 0, counter = 0; i < ARR_SIZE; i++, counter+= 25) {
//    myFile.print((int)x_arr[i]);
//    myFile.print(", ");
//    myFile.print((int)y_arr[i]);
//    myFile.print(", ");
//    myFile.println((int)z_arr[i]);
//
//    if(counter > 400) {
//      myFile.flush();
//      counter = 0;
//    }
//  }

//  for(i = 0; i < ARR_SIZE; i++) {
//    Serial.print((int)x_arr[i]);
//    Serial.print(", ");
//    Serial.print((int)y_arr[i]);
//    Serial.print(", ");
//    Serial.println((int)z_arr[i]);
//  }
//  myFile.flush();
//  myFile.close();

  while(1);

  /*mag = sqrt(pow(float(x), 2) + pow(float(y), 2) + pow(float(z), 2)) - 1.0;
  if(mag < 0.0) mag = 0.0;    
  
  if(mag > 0.1) {    
    hit = true;
    low_count = 0;
    
    if(mag > max_mag) {
      max_mag = mag;
      start_frame();
      for(i = 0; i < ((mag / 2.0) * 8.0) && i < 8; i++) {
        LED_frame(0, 255, 0);
      }
      for(; i < 8; i++) {
        LED_frame(0, 0, 0);
      }
      end_frame();
    }
  } else {
    if(low_count++ > 10) {
      if(hit) {
        delay(250);
      }
      hit = false;
      max_mag = 0.0;
  
      start_frame();
      for(i = 0; i < 8; i++) {
        LED_frame(0, 0, 0);
        }
      end_frame();
    }
  }*/
}

void LIS3DH_read(float *x, float *y, float *z) {
  static const float scale = 1.0 / 16.0;//2.0 /32768.0;
  
  Wire.beginTransmission(LIS3DH_ADDRESS);
  Wire.write(0x28 | 0x80);
  Wire.endTransmission();

  Wire.requestFrom(LIS3DH_ADDRESS, 6);

  int16_t tx, ty, tz;

  tx = Wire.read();
  tx |= ((uint16_t)Wire.read()) << 8;
  ty = Wire.read();
  ty |= ((uint16_t)Wire.read()) << 8;
  tz = Wire.read();
  tz |= ((uint16_t)Wire.read()) << 8;

  *x = float(tx) * scale;
  *y = float(ty) * scale;
  *z = float(tz) * scale;
}

void LIS3DH_write8(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(LIS3DH_ADDRESS);
  Wire.write((uint8_t)reg);
  Wire.write((uint8_t)value);
  Wire.endTransmission();
}

uint8_t LIS3DH_read8(uint8_t reg) {
  Wire.beginTransmission(LIS3DH_ADDRESS);
  Wire.write((uint8_t)reg);
  Wire.endTransmission();

  Wire.requestFrom(LIS3DH_ADDRESS, 1);
  return Wire.read();
}
