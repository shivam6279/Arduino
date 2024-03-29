#include <arduino.h>
#include <Wire.h>

#include "SD.h"
#include <SdFat.h>

#include <RTClib.h>
#include <Adafruit_SHT31.h>
#include <SHT2x.h>
#include <SHT1X.h>

#include "general.h"
#include "settings.h"
#include "weatherData.h"

#if HT_MODE == 1
  #define DHTTYPE DHT22 
  DHT dht(DHTPIN, DHTTYPE);
#elif HT_MODE == 2
  SHT1x sht15(SHT15_DATA_PIN, SHT15_CLK_PIN);
#elif HT_MODE == 3
  Adafruit_SHT31 sht31 = Adafruit_SHT31();
#endif

//BMP180
#if ENABLE_BMP180 == true
  BMP180 barometer;
#endif

//SD
bool SD_flag = false;

//Wind speed
volatile int wind_speed_counter;
bool wind_flag = false, wind_temp;
double wind_speed_buffer[DATA_UPLOAD_FREQUENCY * 3 + 1];

//Davis wind speed
volatile int wind_speed_counter_davis;
bool wind_flag_davis = false, wind_temp_davis;
int VaneValue;
int Direction;
int CalDirection;

//SHT31.h Temperature
int temp;
float cTemp;
float fTemp;
float humidity;

//Rain guage
volatile int rain_counter;
bool rain_flag = false, rain_temp;

//Timer interrupt
uint16_t timer1_counter = 0;
volatile uint8_t GPS_wait;
volatile bool four_sec = false, read_flag = false, upload_flag = false;

//Weather data
int ws_id;
char imei_str[20] = {'N', 'U', 'L', 'L', '\0'};;

//Times
realTime current_time;
realTime startup_time;

RTC_DS3231 rtc;

bool startup = true;

//Compute the running average of sensor data
//Takes the number of previous data points as input (c)
void ReadData(weatherData &w, int c) {
  static float temp_temp, temp_hum;
    
  //BMP180
  #if ENABLE_BMP180 == true
    float temp_pressure = barometer.GetPressure();
    w.pressure = (w.pressure * float(c) + temp_pressure) / (float(c) + 1.0);
    //w.temp = (w.temp * c + barometer.GetTemperature()) / (float(c) + 1.0);
  #endif
  
  //w.solar_radiation = (w.solar_radiation * float(c) + (float(analogRead(BATTERY_PIN)) * 5.0 / 1024.0)) / (float(c) + 1.0);
  
  static float TR1 = 100000.0;
  static float TR2 = 10000.0;
  static int mVperAmp = 185; // use 100 for 20A Module and 66 for 30A Module
  static int ACSoffset = 2500; 
  static float temp_voltage = (float(analogRead(BATTERY_PIN)) * 5.0 / 1024.0 * (TR1 + TR2) / TR2);
  static float temp_current = (((float(analogRead(CURRENT_SENSOR_PIN)) / 1023.0 * 5000.0) - ACSoffset) / mVperAmp);
  static float temp_panel_voltage = (float(analogRead(CHARGE_PIN)) * 5.0 / 1024.0 * ((TR1 + TR2) / TR2));
  w.battery_voltage = (w.battery_voltage * float(c) + temp_voltage) / (float(c) + 1.0);
  w.amps = (w.amps * float(c) + temp_current) / (float(c) + 1.0);
  w.panel_voltage = (w.panel_voltage * float(c) + temp_panel_voltage) / (float(c) + 1.0);

  static int val = analogRead(LeafSensor1_PIN);           //Read Leaf Sensor Pin A3
  static int temp_leaf = map(val, 0, 1023, 0, 100);        //Convert to Percentage
  w.leaf_wetness = (w.leaf_wetness * float(c) + temp_leaf) / (float(c) + 1.0);

  //Soil Temperature
  static int Vo;
  static float R1 = 10000;
  static float logR2, R2, T, temp_soil, Tf;
  static float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
  Vo = analogRead(ThermistorPin_PIN);
  R2 = R1 * (1023.0 /Vo - 1.7);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  temp_soil = T - 273.15;
  w.ThermistorPin = (w.ThermistorPin * float(c) + temp_soil) / (float(c) + 1.0);

  static int uvLevel = analogRead(UVOUT_PIN);
  static int refLevel = analogRead(REF_3V3_PIN);
  static float outputVoltage = 3.3 / refLevel * uvLevel;
  static float uvIntensity = map(outputVoltage, 0.01, 2.5, 0.0, 700.0); //Convert the voltage to a UV intensity level
  w.uv_sensor = (w.uv_sensor * float(c) + uvIntensity) / (float(c) + 1.0);

  VaneValue = analogRead(DAVIS_WIND_DIRECTION_PIN); 
  Direction = map(VaneValue, 0, 1023, 0, 360); 
  CalDirection = Direction + DAVIS_WIND_DIRECTION_OFFSET; 
  
  static int DAVIS_vane;
  static int DAVIS_dir;
  static int DAVIS_cal_dir;
  DAVIS_vane = analogRead(DAVIS_WIND_DIRECTION_PIN); 
  DAVIS_dir = map(DAVIS_vane, 0, 1023, 0, 360); 
  DAVIS_cal_dir = DAVIS_dir + DAVIS_WIND_DIRECTION_OFFSET;   
  if(DAVIS_cal_dir > 360) 
    DAVIS_cal_dir = DAVIS_cal_dir - 360; 
  
  if(DAVIS_cal_dir < 0) 
    DAVIS_cal_dir = DAVIS_cal_dir + 360;   
  w.wind_direction = (w.wind_direction * float(c) + DAVIS_cal_dir) / (float(c) + 1.0);

  static unsigned int data[6];
  #if HT_MODE == 0
    SHT15_ReadTempHum(&temp_temp, &temp_hum);
  #elif HT_MODE == 1
    temp_temp = dht.readTemperature();
    temp_hum = dht.readHumidity();
  //DHT
  #elif HT_MODE == 2
    temp_temp = sht15.readTemperatureC();
    temp_hum = sht15.readHumidity();
  #elif HT_MODE == 3
    temp_temp = sht31.readTemperature();
    temp_hum = sht31.readHumidity();
  #endif  
  w.temp = (w.temp * float(c) + temp_temp) / (float(c) + 1.0);
  w.hum = (w.hum * float(c) + temp_hum) / (float(c) + 1.0);
  if(c == 0) {
    w.temp_max = w.temp;
    w.temp_min = w.temp;

    w.hum_max = w.hum;
    w.hum_min = w.hum;
  } else {
    if(temp_temp > w.temp_max) {
      w.temp_max = temp_temp;
    } else if (temp_temp < w.temp_min) {
      w.temp_min = temp_temp;
    }
    if(temp_hum > w.hum_max) {
      w.hum_max = temp_hum;
    } else if (temp_hum < w.hum_min) {
      w.hum_min = temp_hum;
    }
  }
  
  w.CheckIsNan();  

  if(SERIAL_OUTPUT) {
    Serial.println();
    Serial.println("Temperature: " + String(temp_temp) + " C");
    Serial.println("Humidity: " + String(temp_hum) + "%");
    #if ENABLE_BMP180 == true
      Serial.println("Pressure: " + String(temp_pressure) + " Pa");
    #endif
    Serial.println("Battery voltage: " + String(temp_voltage) + " V");
    Serial.println("Charge current: " + String(temp_current) + " A");
    Serial.println("Panel voltage: " + String(temp_panel_voltage) + " V");
    Serial.println("Leaf moisture: " + String(temp_leaf) + "%");
    Serial.println("Soil temperature: " + String(temp_soil) + " C");
    Serial.println("UV Intensity: " + String(uvIntensity) + " C");
    Serial.println("Wind direction: " + String(DAVIS_cal_dir) + " degrees");
  }
}

bool SHT15_ReadTempHum(float *temp_temp, float *temp_hum) {
  unsigned int data[6];
  
  Wire.beginTransmission(0x44);
  Wire.begin();
  Wire.write(0x2C);
  Wire.write(0x06);
  Wire.endTransmission();
  Wire.beginTransmission(0x44);
  Wire.endTransmission();

  Wire.requestFrom(Addr, 6);
  if (Wire.available() == 6) {
    data[0] = Wire.read();
    data[1] = Wire.read();
    data[2] = Wire.read();
    data[3] = Wire.read();
    data[4] = Wire.read();
    data[5] = Wire.read();
  } else {
    // Start I2C Transmission
    Wire.beginTransmission(0x45);
    // Send 16-bit command byte
    Wire.write(0x2C);
    Wire.write(0x06);
    // Stop I2C transmission
    Wire.endTransmission();
    delay(300);
  
    // Start I2C Transmission
    Wire.beginTransmission(0x45);
    // Stop I2C Transmission
    Wire.endTransmission();
  
    // Request 6 bytes of data
    Wire.requestFrom(0x45, 6);   

    if (Wire.available() == 6) {
      data[0] = Wire.read();
      data[1] = Wire.read();
      data[2] = Wire.read();
      data[3] = Wire.read();
      data[4] = Wire.read();
      data[5] = Wire.read();
    } else {
      *temp_temp = 0;
      *temp_hum = 0;
      return false;
    }
  }

  // Read 6 bytes of data
  // temp msb, temp lsb, temp crc, hum msb, hum lsb, hum crc
  
  int temp = (data[0] * 256) + data[1];
  *temp_temp = -45.0 + (175.0 * temp / 65535.0);
  *temp_hum = (100.0 * ((data[3] * 156.0) + data[4])) / 65535.0;
  
  return true;
}

void pinSetup() {
  pinMode(RAIN_LED, OUTPUT);
  pinMode(WIND_LED, OUTPUT);
  pinMode(UPLOAD_LED, OUTPUT);
  pinMode(SD_CARD_CS_PIN, OUTPUT);
  pinMode(RAIN_PIN, INPUT);

  #if board_version == 5
    pinMode(RAIN_CONNECT_PIN, INPUT);
    pinMode(DOOR_PIN, INPUT);
  #endif
  
  pinMode(WIND_PIN , INPUT);
  pinMode(GSM_DTR_PIN, OUTPUT);
  digitalWrite(GSM_DTR_PIN, LOW);
  #ifdef GSM_PWRKEY_PIN
    pinMode(GSM_PWRKEY_PIN, OUTPUT);
    digitalWrite(GSM_PWRKEY_PIN, LOW);
  #endif

  #ifdef DAVIS_WIND_SPEED_PIN
    pinMode(DAVIS_WIND_SPEED_PIN , INPUT);
  #endif
  #ifdef DAVIS_WIND_DIRECTION_PIN
    pinMode(DAVIS_WIND_DIRECTION_PIN , INPUT);
  #endif

  Serial.begin(115200); 
  Serial1.begin(115200);
  pinMode(19, INPUT);  
  digitalWrite(19, HIGH);
  Serial2.begin(9600);
  pinMode(17, INPUT);  
  digitalWrite(17, HIGH);
  //Serial3.begin(9600);

  Wire.begin();

  SD_flag = sd.begin(SD_CARD_CS_PIN);

  delay(100);

  #if HT_MODE == 1
    dht.begin();
  #elif HT_MODE == 3
    sht31.begin(0x44);
  #endif
}

void PrintInitial(RTC_DS3231 rtc) {
}
