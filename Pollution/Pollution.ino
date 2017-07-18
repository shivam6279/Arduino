#include <SoftwareSerial.h>
#include <String.h>
//#include <Wire.h>
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 12
#define rain_pin 2
#define led 4
#define wind_speed_pin 3
#define SIM900_DTR_pin 9
#define SIM900_RST_pin 5

#define solar_radiation_pin A1
#define battery_pin A6

//------------------------------------Settings-------------------------------------
String id = "56";     
#define network 2//1 for Vodafone, 2 for Aircel, 3 for Airtel

String phone_number = "+919220592205";
#define led_mode 1//0 for rain guage, 1 for anemometer, 2 to deactivate led -- regardless of this variable, the led always lights up when sending data
#define HT_mode 1// 0 for SHT21, 1 for DST, 2 for none
       
#define data_upload_frequency 5//Minutes -- should be a multiple of the read frequency
#define data_read_frequency 5//Minutes
#define number_of_readings (data_upload_frequency / data_read_frequency)

#define enable_GPS false//True to enable GPS    
#define enable_BMP180 false //True to enable BMP180

#define R1 6800.0f
#define R2 10000.0f
#define voltage_factor (R1 + R2) / R2 

#define serial_output true//True to debug: display values through the serial port
#define serial_response true//True to see SIM900 serial response
//----------------------------------------------------------------------------------

SoftwareSerial SIM900(8, 7);

#if HT_mode == 1
#define DHTPIN 6    
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);
#endif

struct weather_data{
  float temp1, temp2, hum, solar_radiation, latitude, longitude, battery_voltage;
  int rain, wind_speed;
  int pm1, pm2_5, pm10;
  long int pressure;
};

//Wind speed
volatile int wind_speed_counter;
boolean wind_flag = true, wind_temp;

//Rain guage
volatile int rain_counter;
boolean rain_flag = true, rain_temp;

//Timer interrupt
uint8_t timer1_counter = 0;
volatile uint8_t GPS_wait;
volatile boolean four_sec = false, read_flag = false, upload_flag = false, network_flag = false;

//SIM
float signal_strength;

char admin_number[14];

weather_data w;
uint8_t reading_number = 0, number_of_fail_uploads = 0;

uint8_t u_seconds = 0, minutes = 0, hours = 0, day = 0, month = 0;
uint16_t year = 0;
boolean net_time = false;

int PM01Value=0;          //define PM1.0 value of the air detector module
int PM2_5Value=0;         //define PM2.5 value of the air detector module
int PM10Value=0; 

void setup(){
  pinMode(led, OUTPUT);
  pinMode(rain_pin, INPUT);
  pinMode(wind_speed_pin, INPUT);
  pinMode(SIM900_DTR_pin, OUTPUT);
  pinMode(SIM900_RST_pin, OUTPUT);
  digitalWrite(SIM900_DTR_pin, LOW);
  digitalWrite(SIM900_RST_pin, LOW);
  SIM900.begin(19200); // SIM900 baud rate   
  Serial.begin(9600);  // GPS baud rate 
  Serial.setTimeout(1500); 
  //Wire.begin();

  #if enable_BMP180
  barometer = BMP180();
  barometer.SoftReset();
  barometer.Initialize();
  #endif
  
  #if serial_output
  Serial.println("Sensor id: " + id);
  Serial.println("Data upload frequency: " + (String)data_upload_frequency + " minutes");
  Serial.println("Data read frequency: " + (String)data_read_frequency + " minutes");
  if(led_mode == 0) Serial.println("Led mode: rain guage"); else if(led_mode == 1) Serial.println("Led mode: anemometer"); else Serial.println("Led disabled (lights up during data upload)");
  if(HT_mode == 0) Serial.println("Using SHT21"); else if(HT_mode == 1) Serial.println("Using DHT22"); else Serial.println("No HT sensor");
  if(enable_BMP180) Serial.println("BMP180 enabled"); else Serial.println("BMP180 disabled");
  if(enable_GPS) Serial.println("GPS enabled"); else Serial.println("GPS disabled");
  #endif

  //SIM900_reset();
  digitalWrite(led, HIGH);
  delay(10000);//Wait for the SIM to log on to the network
  digitalWrite(led, LOW);
  SIM900_wake();
  SIM900.print("AT+CMGF=1\r"); 
  delay(200);
  #if serial_response
  ShowSerialData();
  #endif
  SIM900.print("AT+CNMI=2,2,0,0,0\r");
  delay(200); 
  #if serial_response
  ShowSerialData();
  #endif
  SIM900.print("AT+CLIP=1\r"); 
  delay(200);
  #if serial_response
  ShowSerialData();
  #endif
  delay(1000);
  SIM900_sleep();
  
  //Interrupt initialization
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
  #if serial_output == true
  Serial.println("\nSeconds till next upload:");
  #endif
  interrupts();//Timer1: 0.25hz, Timer2: 8Khz
}
 
void loop(){
  int i = 0, j, avg_counter = 0;
  boolean temp_read, temp_upload, temp_network;
  
  rain_counter = 0;
  wind_speed_counter = 0;
  reading_number = 0;

  w.temp1 = 0.0;
  w.temp2 = 0.0;
  w.hum = 0.0;
  w.solar_radiation = 0;
  w.pressure = 0;
  w.pm1 = 0;
  w.pm2_5 = 0;
  w.pm10 = 0;
  while(Serial.available() != 0) Serial.read();
  while(1){
    if(Serial.read() == 0x42){
      delay(1);
      if(Serial.read() == 0x4d){
        Serial.read(); delay(1);
        Serial.read(); delay(1);
        PM01Value = Serial.read() << 8; delay(1);
        PM01Value |= Serial.read(); delay(1);
        PM2_5Value = Serial.read() << 8; delay(1);
        PM2_5Value |= Serial.read(); delay(1);
        PM10Value = Serial.read() << 8; delay(1);
        PM10Value |= Serial.read();
      }
    }
    
    if(four_sec){
      four_sec = false;
      temp_read = read_flag;
      temp_upload = upload_flag;
      temp_network = network_flag;
  
      #if serial_output
      if(timer1_counter) Serial.println(((data_upload_frequency * 15) - timer1_counter + 1) * 4);
      else Serial.println("4");
      #endif
        
      #if HT_mode == 0
      w.hum = (w.hum * float(avg_counter) + SHT2x.GetHumidity()) / (float(avg_counter) + 1.0);
      w.temp1 = (w.temp1 * float(avg_counter) + SHT2x.GetTemperature()) / (float(avg_counter) + 1.0);
      #endif
      #if HT_mode == 1
      w.hum = (w.hum * float(avg_counter) + dht.readHumidity()) / (float(avg_counter) + 1.0);
      w.temp1 = (w.temp1 * float(avg_counter) + dht.readTemperature()) / (float(avg_counter) + 1.0);
      if(isnan(w.hum)) w.hum = 0.0;
      if(isnan(w.temp1)) w.temp1 = 0.0;
      #endif
      #if enable_BMP180 == true
      w.pressure = (w.pressure * float(avg_counter) + barometer.GetPressure()) / (float(avg_counter) + 1.0);
      w.temp2 = (w.temp2 * avg_counter + barometer.GetTemperature()) / (float(avg_counter) + 1.0);
      #endif
      w.solar_radiation = (w.solar_radiation * float(avg_counter) + float(analogRead(solar_radiation_pin))) / (float(avg_counter) + 1.0);
      
      w.pm1 = (w.pm1 * float(avg_counter) + PM01Value)/ (float(avg_counter) + 1.0);
      w.pm2_5 = (w.pm2_5 * float(avg_counter) + PM2_5Value)/ (float(avg_counter) + 1.0);
      w.pm10 = (w.pm10 * float(avg_counter) + PM10Value)/ (float(avg_counter) + 1.0);
      avg_counter++;
  
      if(temp_read){//Read data
        read_flag = false;
        digitalWrite(led, HIGH);
        
        //Read data
        avg_counter = 0;
        
        w.wind_speed = ((wind_speed_counter) / data_upload_frequency);
        wind_speed_counter = 0;
        
        w.rain = rain_counter;
        rain_counter = 0;
        
        #if enable_GPS == true
        GetGPS();
        #endif
        
        while(Serial.available() != 0) Serial.read();
        
        #if serial_output
        Serial.println("Reading number: " + String(reading_number + 1));
        Serial.println("Humidity(%RH): " + String(w.hum));
        Serial.println("Temperature(C): SHT: " + String(w.temp1));
        Serial.println("PM1.0: " + String(w.pm1));
        Serial.println("PM2.5: " + String(w.pm2_5));
        Serial.println("PM10: SHT: " + String(w.pm10));
        #if enable_BMP180
        Serial.println(", BMP: " + String(w.temp2)); 
        #endif
        Serial.println("Solar radiation(Voltage): " + String(float(w.solar_radiation) * 5.0 / 1023.0)); 
        #if enable_BMP180
        Serial.println("Pressure: " + String(w.pressure) + "atm"); 
        #endif
        #endif
        
        reading_number++;
        
        digitalWrite(led, HIGH);
      }
      if(temp_upload){//Upload data
        digitalWrite(led, HIGH);
        
        SIM900_wake();
        
        signal_strength = get_signal_strength();
        
        if(!SubmitHttpRequest()){
          number_of_fail_uploads++;
          reading_number = 0;
          #if serial_output
          Serial.println("\nUpload failed - sending sms");
          #endif
          delay(100);
          upload_sms();
        }
        else{
          reading_number = 0;
          number_of_fail_uploads = 0;
          #if serial_output
          Serial.println("Upload successful");
          #endif
        }

          w.temp1 = 0.0;
          w.temp2 = 0.0;
          w.hum = 0.0;
          w.solar_radiation = 0;
          w.pressure = 0;
          w.pm1 = 0;
          w.pm2_5 = 0;
          w.pm10 = 0;

        
        //GSM_module_sleep();
        
        digitalWrite(led, HIGH);
        upload_flag = false;
        #if serial_output == true
        Serial.println("Seconds till next upload:");
        #endif
      }
    }
  }
}

ISR(TIMER1_COMPA_vect){ //Interrupt gets called every 4 seconds 
  #if enable_GPS
  if(GPS_wait < 2) GPS_wait++;
  #endif
  timer1_counter++;
  if(timer1_counter == data_upload_frequency * 15){ 
    timer1_counter = 0;
    upload_flag = true;
  }
  if(timer1_counter % (data_read_frequency * 15) == 0) read_flag = true;
  if(timer1_counter == (data_upload_frequency - 1) * 15) network_flag = true;
  four_sec = true;
}

ISR(TIMER2_COMPA_vect){ 
  rain_temp = (PIND >> rain_pin) & 1;
  if(rain_temp == 1 && rain_flag == true){
    #if led_mode == 0
      if(!upload_flag) digitalWrite(led, HIGH);
    #endif
    rain_counter++;
    rain_flag = false;
  }
  else if(rain_temp == 0){
    rain_flag = true;
    #if led_mode == 0
      if(!upload_flag) digitalWrite(led, LOW);
    #endif
  }
  wind_temp = (PIND >> wind_speed_pin) & 1;
  if(wind_temp == 0 && wind_flag == true){
    wind_speed_counter++;
    wind_flag = false;
    #if led_mode == 1
      if(!upload_flag) digitalWrite(led, HIGH);
    #endif
  } 
  else if(wind_temp == 1){
    wind_flag = true;
    #if led_mode == 1
      if(!upload_flag) digitalWrite(led, LOW);
    #endif
  } 
}

bool SubmitHttpRequest(){
  char str[60];
  uint8_t row_number, j;
  uint16_t i;
  
  #if serial_output
  Serial.println("Uploading data");
  #endif
  while(SIM900.available()) SIM900.read();
  SIM900.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r");//setting the SAPBR, the connection type is using gprs
  delay(100);
  #if serial_response
  ShowSerialData();
  #endif
  #if network == 1
  SIM900.println("AT+SAPBR=3,1,\"APN\",\"www\"\r");//setting the APN, the second need you fill in your local apn server
  #elif network == 2
  SIM900.println("AT+SAPBR=3,1,\"APN\",\"aircelgprs.po\"\r");//setting the APN, the second need you fill in your local apn server
  #elif network == 3
  SIM900.println("AT+SAPBR=3,1,\"APN\",\"airtelgprs.com\"\r");//setting the APN, the second need you fill in your local apn server
  #endif
  delay(100);
  #if serial_response
  ShowSerialData();
  #endif
  SIM900.println("AT+SAPBR=1,1\r");//setting the SAPBR, for detail you can refer to the AT command mamual
  delay(2000);
  #if serial_response
  ShowSerialData();
  #endif  
  SIM900.println("AT+HTTPINIT\r"); //init the HTTP request
  delay(100);
  #if serial_response
  ShowSerialData();
  #else
  while(SIM900.available()) SIM900.read();
  #endif
  SIM900.print("AT+HTTPPARA=\"URL\",\"http://enigmatic-caverns-27645.herokuapp.com/add_data/");      
  SIM900.print(String(row_number + 1) + "=<"); 
  SIM900.print("'id':" + id + ",");
  SIM900.print("'t1':" + String(w.temp1) + ",");
  SIM900.print("'t2':" + String(w.temp2) + ",");
  SIM900.print("'h':" + String(w.hum) + ",");
  SIM900.print("'w':" + String(w.wind_speed) + ",");
  SIM900.print("'r':" + String(w.rain) + ",");
  SIM900.print("'p':" + String(w.pressure) + ",");
  SIM900.print("'s':" + String(w.solar_radiation) + ",");
  SIM900.print("'lt':" + String(w.latitude) + ",");
  SIM900.print("'ln':" + String(w.longitude) + ",");
  SIM900.print("'sg':" + String(signal_strength) + ",");
  SIM900.print("'pm01':" + String(w.pm1) + ",");
  SIM900.print("'pm25':" + String(w.pm2_5) + ",");
  SIM900.print("'pm10':" + String(w.pm10) + ">");
  SIM900.println("\"\r");
  delay(400);
  #if serial_response
  ShowSerialData();
  #else
  while(SIM900.available()) SIM900.read();
  #endif
  SIM900.println("AT+HTTPACTION=0\r");//submit the request 
  for(i = 0; SIM900.available() < 35 && i < 200; i++) delay(100);
  if(i < 200){
    for(i = 0; SIM900.available(); i++) str[i] = SIM900.read();
    str[i] = '\0';
    #if serial_response
    Serial.println(String(str));
    SIM900.println("AT+HTTPREAD\r");
    delay(100);
    ShowSerialData();
    #endif
  }
  else{
    #if serial_response
    ShowSerialData();
    #else
    while(SIM900.available()) SIM900.read();
    #endif
    return 0;
  }
  delay(5000);
  
  j = 100 * (str[40] - 48); j += 10 * (str[41] - 48); j += (str[42] - 48);
  if(j == 200) return true;
  else return false;
}


int get_signal_strength(){
  char temp[20];
  uint8_t i = 0;
  while(SIM900.available()) SIM900.read();
  SIM900.print("AT+CSQ\r");
  delay(100);
  while(SIM900.available()) temp[i++] = SIM900.read();  
  temp[i] = '\0';
  #if serial_response
  Serial.println(String(temp));
  #endif
  i = 10 * (temp[15] - 48);
  i += (temp[16] - 48);
  return i;
}

bool check_network(){
  uint8_t i;
  while(SIM900.available()) SIM900.read();
  SIM900.print("AT+COPS?\r");
  delay(100);
  i = SIM900.available();
  #if serial_response
  ShowSerialData();
  #else
  while(SIM900.available()) SIM900.read();
  #endif
  if(i < 30) return 0;
  else return 1;
}

void SIM900_reset(){
  digitalWrite(SIM900_RST_pin, HIGH);
  delay(1000);
  digitalWrite(SIM900_RST_pin, LOW);
}
void SIM900_sleep(){
  SIM900.print("AT+CSCLK=1\r");
  #if serial_response == true
  ShowSerialData();
  #endif
  delay(1000);
}
void SIM900_wake(){
  digitalWrite(SIM900_DTR_pin, HIGH);
  delay(1000);
  SIM900.print("AT+CSCLK=0\r");
  #if serial_response == true
  ShowSerialData();
  #endif
  delay(1000);
  digitalWrite(SIM900_DTR_pin, LOW);
}

long int pow_10(int a){
  long int t;
  int i;
  for(i = 0, t = 1; i < a; i++) t *= 10;
  return t;
}

void upload_sms(){
  SIM900.print("AT+CMGS=\"" + phone_number + "\"\n");
  delay(100);
  SIM900.print("HK9D7 ");
  SIM900.print("'id':" + id + ",");
  SIM900.print("'t1':" + String(w.temp1) + ",");
  SIM900.print("'t2':" + String(w.temp2) + ",");
  SIM900.print("'h':" + String(w.hum) + ",");
  SIM900.print("'w':" + String(w.wind_speed) + ",");
  SIM900.print("'r':" + String(w.rain) + ",");
  SIM900.print("'p':" + String(w.pressure) + ",");
  SIM900.print("'s':" + String(w.solar_radiation) + ",");
  SIM900.print("'lt':" + String(w.latitude) + ",");
  SIM900.print("'ln':" + String(w.longitude) + ",");
  SIM900.print("'sg':" + String(signal_strength));
  delay(100);
  SIM900.write(26);
  SIM900.write('\n');
  delay(100);
  SIM900.write('\n');
  #if serial_response
  ShowSerialData();
  #endif
}

char checkValue(unsigned char *thebuf, char leng)
{  
  char receiveflag=0;
  int receiveSum=0;

  for(int i=0; i<(leng-2); i++){
  receiveSum=receiveSum+thebuf[i];
  }
  receiveSum=receiveSum + 0x42;
 
  if(receiveSum == ((thebuf[leng-2]<<8)+thebuf[leng-1]))  //check the serial data 
  {
    receiveSum = 0;
    receiveflag = 1;
  }
  return receiveflag;
}

void ShowSerialData(){
  while(SIM900.available() != 0) Serial.write(SIM900.read());
}

int transmitPM01(unsigned char *thebuf)
{
  int PM01Val;
  PM01Val=((thebuf[3]<<8) + thebuf[4]); //count PM1.0 value of the air detector module
  return PM01Val;
}

//transmit PM Value to PC
int transmitPM2_5(unsigned char *thebuf)
{
  int PM2_5Val;
  PM2_5Val=((thebuf[5]<<8) + thebuf[6]);//count PM2.5 value of the air detector module
  return PM2_5Val;
  }

//transmit PM Value to PC
int transmitPM10(unsigned char *thebuf)
{
  int PM10Val;
  PM10Val=((thebuf[7]<<8) + thebuf[8]); //count PM10 value of the air detector module  
  return PM10Val;
}
