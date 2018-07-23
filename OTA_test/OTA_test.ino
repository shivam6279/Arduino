#include "settings.h"

#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <String.h>
#include <Wire.h>
//#include <BMP180.h>
#include <DHT.h>
#include <SHT2x.h>
#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#include "http.h"
#include "weatherData.h"
#include "GSM.h"
#include "SD.h"

//All pin definitions and settings are in "settings.h"

#if HT_MODE == 1
  #define DHTTYPE DHT22 
  DHT dht(DHTPIN, DHTTYPE);
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
double wind_speed_buffer[DATA_READ_FREQUENCY * 15 + 1];

//Rain guage
volatile int rain_counter;
bool rain_flag = false, rain_temp;

//Voltage
float R1 = 100000.0;
float R2 = 10000.0;
int mVperAmp = 185; // use 100 for 20A Module and 66 for 30A Module
int ACSoffset = 2500; 

//Timer interrupt
uint8_t timer1_counter = 0;
volatile uint8_t GPS_wait;
volatile bool four_sec = false, read_flag = false, upload_flag = false;

//Weather data
int ws_id;

//Times
realTime current_time;
realTime startup_time;

bool startup = true;

OneWire oneWire(METAL_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

void setup() {  
  unsigned int i;
  char str[10];

  pinMode(RAIN_LED, OUTPUT);
  pinMode(WIND_LED, OUTPUT);
  pinMode(UPLOAD_LED, OUTPUT);
  pinMode(SD_CARD_CS_PIN, OUTPUT);
  pinMode(RAIN_PIN, INPUT);
  pinMode(WIND_PIN , INPUT);
  pinMode(GSM_DTR_PIN, OUTPUT);
  digitalWrite(GSM_DTR_PIN, LOW);
  pinMode(GSM_PWRKEY_PIN, OUTPUT);
  digitalWrite(GSM_PWRKEY_PIN, LOW);

  Serial.begin(115200); 
  Serial1.begin(19200);
  pinMode(19, INPUT);  
  digitalWrite(19, HIGH);
  Serial2.begin(9600);
  pinMode(17, INPUT);  
  digitalWrite(17, HIGH);
  //Serial3.begin(9600);

  SD_flag = sd.begin(SD_CARD_CS_PIN);
  delay(100);
  if(!sd.exists("id.txt")){
    #if SERIAL_OUTPUT
      Serial.println("\nNo id file found on sd card, using bakup id");
    #endif
    ws_id = BACKUP_ID.toInt();
  } else {
    #if SERIAL_OUTPUT
      Serial.println("\nSD Card detected");
    #endif
    datalog.open("id.txt", FILE_READ);
    while(datalog.available()) {
      str[i++] = datalog.read();
    }
    str[i] = '\0';
    ws_id = String(str).toInt();
    datalog.close();
  }
  
  #if HT_MODE == 0 || ENABLE_BMP180 == true
    Wire.begin();
  #endif
  #if HT_MODE == 1
    dht.begin();
  #endif
  sensors.begin();
  
  #if ENABLE_BMP180
    barometer = BMP180();
    barometer.SoftReset();
    barometer.Initialize(); 
  #endif

  #if SERIAL_OUTPUT
    Serial.println("Sensor id: " + String(ws_id));
    Serial.println("Data upload frequency: " + (String)DATA_UPLOAD_FREQUENCY + " minutes");
    Serial.println("Data read frequency: " + (String)DATA_READ_FREQUENCY + " minutes");
    if(HT_MODE == 0) Serial.println("Using SHT21"); else if(HT_MODE == 1) Serial.println("Using DHT22"); else Serial.println("No HT sensor");
    if(ENABLE_BMP180) Serial.println("BMP180 enabled"); else Serial.println("BMP180 disabled");
    if(ENABLE_GPS) Serial.println("GPS enabled"); else Serial.println("GPS disabled");
    Serial.println();
  #endif
  
  //Talk();

  digitalWrite(UPLOAD_LED, HIGH);
  delay(2000);  //Wait for the GSM module to boot up
  digitalWrite(UPLOAD_LED, LOW);

  //Initialize GSM module
  InitGSM();  
  
  //Interrupt initialization  
  InitInterrupt();  //Timer1: 0.25hz, Timer2: 8Khz

  test();
}
 
void loop() {
  int i, j;
  unsigned long int avg_counter = 0;
  bool temp_read, temp_upload, temp_network, temp_sd;

  uint16_t reading_number = 0;
  uint16_t number_of_fail_uploads = 0;
  uint16_t initial_sd_card_uploads = 0;

  weatherData w[BUFFER_SIZE];

  realTime temp_time;
  
  rain_counter = 0;
  wind_speed_counter = 0;
  reading_number = 0;

  for(i = 0; i < BUFFER_SIZE; i++) {
    w[i].Reset(ws_id);
  }
  while(Serial1.available()) Serial1.read();

  #if SERIAL_OUTPUT == true
    Serial.println("\nSeconds till next upload:");
  #endif
  while(1) {
    CheckOTA();
  }
}

//Interrupt gets called every 4 seconds
ISR(TIMER1_COMPA_vect) { 
  #if enable_GPS
    if(GPS_wait < 2) GPS_wait++;
  #endif
  timer1_counter++;
  if(timer1_counter == DATA_UPLOAD_FREQUENCY * 15) { 
    timer1_counter = 0;
    upload_flag = true;
  }
  if(timer1_counter % (DATA_READ_FREQUENCY * 15) == 0) read_flag = true;
  four_sec = true;

  //Increment time by 4 seconds and handle overflow
  current_time.seconds += 4;
  current_time.HandleTimeOverflow();

}

//Interrupt gets called every 125 micro seconds
ISR(TIMER2_COMPA_vect) {
  rain_temp = digitalRead(RAIN_PIN);
  if(rain_temp == 1 && rain_flag == true) {
    digitalWrite(RAIN_LED, HIGH);
    rain_counter++;
    rain_flag = false;
  }
  else if(rain_temp == 0) {
    rain_flag = true;
    digitalWrite(RAIN_LED, LOW);
  }
  wind_temp = digitalRead(WIND_PIN);
  if(wind_temp == 0 && wind_flag == true) {
    wind_speed_counter++;
    wind_flag = false;
    digitalWrite(WIND_LED, HIGH);
  } 
  else if(wind_temp == 1) {
    wind_flag = true;
    digitalWrite(WIND_LED, LOW);
  } 
}

void ReadData(weatherData &w, int c) {
  //SHT
  #if HT_MODE == 0
    w.hum = (w.hum * float(c) + SHT2x.GetHumidity()) / (float(c) + 1.0);
    w.temp1 = (w.temp1 * float(c) + SHT2x.GetTemperature()) / (float(c) + 1.0);
  #endif
  //DHT
  #if HT_MODE == 1
    w.hum = (w.hum * float(c) + dht.readHumidity()) / (float(c) + 1.0);
    w.temp1 = (w.temp1 * float(c) + dht.readTemperature()) / (float(c) + 1.0);
  #endif
  #if ENABLE_BMP180 == true
    w.pressure = (w.pressure * float(c) + barometer.GetPressure()) / (float(c) + 1.0);
    //w.temp2 = (w.temp2 * c + barometer.GetTemperature()) / (float(c) + 1.0);
  #endif
  //w.solar_radiation = (w.solar_radiation * float(c) + (float(analogRead(BATTERY_PIN)) * 5.0 / 1024.0)) / (float(c) + 1.0);
  
  w.battery_voltage = (w.battery_voltage * float(c) + (float(analogRead(BATTERY_PIN)) * 5.0 / 1024.0 * (R1 + R2) / R2)) / (float(c) + 1.0);
  
  w.amps = (w.amps * float(c) + (((float(analogRead(CURRENT_SENSOR_PIN)) / 1023.0 * 5000.0) - ACSoffset) / mVperAmp)) / (float(c) + 1.0);

  w.panel_voltage = (w.panel_voltage * float(c) + (float(analogRead(CHARGE_PIN)) * 5.0 / 1024.0 * ((R1 + R2) / R2))) / (float(c) + 1.0);
  
  sensors.requestTemperatures();
  w.temp2 = (w.temp2 * float(c) + sensors.getTempCByIndex(0)) / (float(c) + 1.0);

  w.CheckIsNan();
  
}

void InitInterrupt() {
  noInterrupts();//stop interrupts
  TCCR1A = 0; 
  TCCR1B = 0; 
  TCNT1  = 0; 
  OCR1A = 62500; 
  TCCR1B |= (1 << WGM12); 
  TCCR1B |= 5;
  TCCR1B &= 253; 
  TIMSK1 |= (1 << OCIE1A); 
  TCCR2A = 0; 
  TCCR2B = 0; 
  TCNT2  = 0;
  OCR2A = 249; 
  TCCR2A |= (1 << WGM21); 
  TCCR2B |= (1 << CS21); 
  TIMSK2 |= (1 << OCIE2A);
  interrupts();
}

void test() {
  String str;
  
  Serial.println(F("\nStarting connection to server..."));

  HttpInit();
  delay(1000);
  Serial1.print("AT+QHTTPURL=32,30\r");
  delay(1000);
  ShowSerialData();
  Serial1.print("http://www.yobi.tech/bulk-upload\r");
  delay(500);
  ShowSerialData();
  
  //str = "POST /bulk-upload HTTP/1.1\n";
  //str += "Host: www.yobi.tech\n";
  //str += "Origin: http://www.yobi.tech\n";
  //str += "User-Agent: Frank/1.0\n";
  //str += "Accept-Encoding: gzip, deflate\n";
  //str += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\n";
  //str += "Referer: http://www.yobi.tech/bulk-upload\n";
  //str += "Connection: keep-alive\n";
  //str += "Content-Length: ";
  //str += "9";
  //str += "\n";
  //str += "Content-Type: multipart/form-data;     boundary=710ff0c6cf2d4c73b12db64cab12e58c\n";
  //str += "\n";
  //str += "--710ff0c6cf2d4c73b12db64cab12e58c\n";
  str = "Content-Disposition:  form-data; name=\"file\"; filename=\"test.csv\"\n";
  str += "Content-Type: text/plain\n\n";

  str += "1,2,3,4,5";

  //str += "\n--710ff0c6cf2d4c73b12db64cab12e58c--\n\r";
  
  Serial.println();
  Serial.println(str);
  Serial.println();

  Serial1.print("AT+QHTTPPOST=" + String(str.length()) + ",30,30\r");
  delay(5000);
  ShowSerialData();
  Serial1.print(str);
  delay(15000);
  ShowSerialData();

  Talk();
  
}

