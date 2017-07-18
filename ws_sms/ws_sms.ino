#include <SoftwareSerial.h>
#include <String.h>
#include <Wire.h>
#include <BMP180.h>
#include <DHT.h>
#include <SHT2x.h>

#define rain_pin 2
#define led 4
#define wind_speed_pin 3
#define GSM_module_DTR_pin 9
#define GSM_module_RST_pin 5

#define solar_radiation_pin A1
#define battery_pin A6

//------------------------------------Settings-------------------------------------
#define _1280 //_328p or _1280/_2560

#ifndef _328p
#include <SPI.h>
#include <SD.h>
File datalog;
#endif

String id = "207";     
String phone_number = "+919220592205";
uint8_t network = 1;//1 for Vodafone, 2 for Aircel, 3 for Airtel
#define GSM_model 1//0 for SIM900, 1 for Quectel

#define led_mode 1//0 for rain guage, 1 for anemometer, 2 to deactivate led -- regardless of this variable, the led always lights up when sending data
#define HT_mode 0// 0 for SHT21, 1 for DST, 2 for none
       
uint16_t data_upload_frequency = 1;//Minutes -- should be a multiple of the read frequency
#define data_read_frequency 1//Minutes
#define number_of_readings (data_upload_frequency / data_read_frequency)

#define enable_GPS false//True to enable GPS    
#define enable_BMP180 false //True to enable BMP180

#define R1 6800.0f
#define R2 10000.0f
#define voltage_factor (R1 + R2) / R2 

#define serial_output true//True to debug: display values through the serial port
#define serial_response true//True to see SIM900 serial response
//----------------------------------------------------------------------------------

SoftwareSerial GSM_module(8, 7);

#if HT_mode == 1
#define DHTPIN 6    
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);
#endif

//BMP180
#if enable_BMP180 == true
BMP180 barometer;
#endif

struct weather_data{
  float temp1, temp2, hum, solar_radiation, latitude, longitude, battery_voltage;
  int rain, wind_speed;
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

uint8_t seconds = 0, minutes = 0, hours = 0, day = 0, month = 0;
uint16_t year = 0;

void setup(){
  pinMode(led, OUTPUT);
  pinMode(rain_pin, INPUT);
  pinMode(wind_speed_pin, INPUT);
  pinMode(GSM_module_DTR_pin, OUTPUT);
  pinMode(GSM_module_RST_pin, OUTPUT);
  digitalWrite(GSM_module_DTR_pin, LOW);
  digitalWrite(GSM_module_RST_pin, LOW);
  GSM_module.begin(19200); // SIM900 baud rate   
  Serial.begin(9600);  // GPS baud rate 
  Wire.begin();

  #ifndef _328p
  SD.begin();
  delay(100);
  datalog = SD.open("datalog.txt", FILE_WRITE);
  datalog.close();
  #endif  
  
  #if enable_BMP180
  barometer = BMP180();
  barometer.SoftReset();
  barometer.Initialize();
  #endif
  
  #if serial_output
  Serial.println("Sensor id: " + id);
  Serial.println("Data upload frequency: " + (String)data_upload_frequency + " minutes");
  Serial.println("Data read frequency: " + (String)data_read_frequency + " minutes");
  if(GSM_model == 0) Serial.println("Using SIM900"); else if(GSM_model == 1) Serial.println("Using Quectel M95"); else if(GSM_model == 2) Serial.println("Using U-Blox SARA G340");
  if(led_mode == 0) Serial.println("Led mode: rain guage"); else if(led_mode == 1) Serial.println("Led mode: anemometer"); else Serial.println("Led disabled (lights up during data upload)");
  if(HT_mode == 0) Serial.println("Using SHT21"); else if(HT_mode == 1) Serial.println("Using DHT22"); else Serial.println("No HT sensor");
  if(enable_BMP180) Serial.println("BMP180 enabled"); else Serial.println("BMP180 disabled");
  if(enable_GPS) Serial.println("GPS enabled"); else Serial.println("GPS disabled");
  Serial.println();
  #endif

  //GSM_module_reset();
  digitalWrite(led, HIGH);
  delay(10000);//Wait for the SIM to log on to the network
  digitalWrite(led, LOW);
  //GSM_module_wake();
  GSM_module.print("AT+CMGF=1\r"); delay(200);
  #if serial_response
  ShowSerialData();
  #endif
  GSM_module.print("AT+CNMI=2,2,0,0,0\r"); delay(200); 
  #if serial_response
  ShowSerialData();
  #endif
  GSM_module.print("AT+CLIP=1\r"); delay(200);
  #if serial_response
  ShowSerialData();
  #endif
  delay(1000); //GSM_module_sleep();
  
  //Interrupt initialization
  noInterrupts();//stop interrupts
  TCCR1A = 0; TCCR1B = 0; TCNT1  = 0; OCR1A = 62500; TCCR1B |= (1 << WGM12); TCCR1B |= 5;
  TCCR1B &= 253; TIMSK1 |= (1 << OCIE1A); TCCR2A = 0; TCCR2B = 0; TCNT2  = 0;
  OCR2A = 249; TCCR2A |= (1 << WGM21); TCCR2B |= (1 << CS21); TIMSK2 |= (1 << OCIE2A);
  #if serial_output == true
  Serial.println("\nSeconds till next upload:");
  #endif
  interrupts();//Timer1: 0.25hz, Timer2: 8Khz
}
 
void loop(){
  int i = 0, j, avg_counter = 0;
  boolean temp_read, temp_upload, temp_network;
  char body_r[20], number[14];
  strcpy(admin_number, "+919650484721");
  admin_number[13] = '\0';
  
  rain_counter = 0;
  wind_speed_counter = 0;
  reading_number = 0;

  w.temp1 = 0.0;
  w.temp2 = 0.0;
  w.hum = 0.0;
  w.solar_radiation = 0;
  w.pressure = 0;

  while(1){
    /*if(check_sms()){
      get_sms(number, body_r);
      if(strcmp(number, admin_number) == 0){
        if((body_r[0] == 'F' || body_r[0] == 'f') && body_r[1] == '='){ // Change upload frequency
          for(i = 2; isdigit(body_r[i]); i++); i--;
          for(data_upload_frequency = 0, j = 1; i > 1; i--, j *= 10) data_upload_frequency += (body_r[i] - 48) * j;
          Serial.println("Data upload frequency changed to " + String(data_upload_frequency) + " minutes");
          GSM_module.print("AT+CMGS=\"" + String(admin_number) + "\"\n");  delay(100);
          GSM_module.println("Data upload frequency changed to " + String(data_upload_frequency) + " minutes");          
          delay(100); GSM_module.write(26); GSM_module.write('\n');  delay(100);  GSM_module.write('\n');
        }
        if((body_r[0] == 'N' || body_r[0] == 'n') && body_r[1] == '='){ //Change Newtork
          if(body_r[2] == '1'){ network = 1; Serial.println("Network changed to Vodafone"); send_sms(admin_number, "Network changed to Vodafone"); }
          else if(body_r[2] == '2'){ network = 2; Serial.println("Network changed to Aircel"); send_sms(admin_number, "Network changed to Aircel"); }
          else if(body_r[2] == '3'){ network = 3; Serial.println("Network changed to Airtel"); send_sms(admin_number, "Network changed to Airtel"); }
        }
        else if((body_r[0] == 'C' || body_r[0] == 'c') && body_r[1] == '='){
          number[0] = '+'; number[1] = '9'; number[2] = '1';
          if(body_r[2] == '+' && body_r[3] == '9' && body_r[4] == '1'){ for(i = 0; i < 10; i++) number[i + 3] = body_r[i + 5]; }
          else if(body_r[2] == '9' && body_r[3] == '1'){ for(i = 0; i < 10; i++) number[i + 3] = body_r[i + 4]; }
          else{ for(i = 0; i < 10; i++) number[i + 3] = body_r[i + 2]; }
          number[13] = '\0'; 
          Serial.println("Admin number changed to " + String(number));
          GSM_module.print("AT+CMGS=\"" + String(admin_number) + "\"\n");  delay(100);
          GSM_module.println("Admin number changed to " + String(number));          
          delay(100); GSM_module.write(26); GSM_module.write('\n');  delay(100);  GSM_module.write('\n');
          delay(5000);
          GSM_module.print("AT+CMGS=\"" + String(number) + "\"\n"); delay(100);
          GSM_module.println("This is the new admin number for ws id " + id + ", changed from" + String(admin_number));
          delay(100); GSM_module.write(26); GSM_module.write('\n');  delay(100); GSM_module.write('\n');
          strcpy(admin_number, number);
        }
      }
    }*/
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
      #endif
      #if enable_BMP180 == true
      w.pressure = (w.pressure * float(avg_counter) + barometer.GetPressure()) / (float(avg_counter) + 1.0);
      w.temp2 = (w.temp2 * avg_counter + barometer.GetTemperature()) / (float(avg_counter) + 1.0);
      #endif
      w.solar_radiation = (w.solar_radiation * float(avg_counter) + float(analogRead(solar_radiation_pin))) / (float(avg_counter) + 1.0);
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
  
        //w.battery_voltage = float(analogRead(battery_pin) * 5.0 / 1023.0 * voltage_factor);
        
        #if enable_GPS == true
        GetGPS();
        #endif
  
        #if serial_output
        Serial.println("Reading number: " + String(reading_number + 1));
        Serial.println("Humidity(%RH): " + String(w.hum));
        Serial.println("Temperature(C): SHT: " + String(w.temp1));
        #if enable_BMP180
        Serial.println(", BMP: " + String(w.temp2)); 
        #endif
        Serial.println("Solar radiation(Voltage): " + String(float(w.solar_radiation) * 5.0 / 1023.0)); 
        #if enable_BMP180
        Serial.println("Pressure: " + String(w.pressure) + "atm"); 
        #endif
        #endif
        
        reading_number++;
        
        digitalWrite(led, LOW);
      }
      if(temp_upload){//Upload data
        digitalWrite(led, HIGH);
        
        //GSM_module_wake();
        
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
          Serial.println("\nUpload successful");
          #endif
        }

        get_time();
        #ifndef _328p
        write_SD();
        #endif

        //GSM_module_sleep();
        
        digitalWrite(led, LOW);
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

  seconds += 4;
  if(seconds >= 60){ seconds = 0; minutes++;
    if(minutes >= 60){  minutes = 0; hours++;
      if(hours >= 24){ hours = 0; day++;
        if((month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) && day > 31){ day = 1; month++;}
        else if((month == 4 || month == 6 || month == 9 || month == 11) && day > 30){ day = 1; month++;}
        else if(month == 2 && day > 28){ day = 1; month++;}
        if(month > 12){  month = 1; year++;
  } } } }
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
  uint8_t row_number = 0, j;
  uint16_t i;
  #if GSM_model == 1
  int str_len;
  char t[4];
  #endif
  
  #if serial_output
  Serial.println("Uploading data");
  #endif
  while(GSM_module.available()) GSM_module.read();

  #if GSM_model == 0//SIM900
    GSM_module.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r");//setting the SAPBR, the connection type is using gprs
    delay(100);
    #if serial_response
    ShowSerialData();
    #endif
    #if network == 1
    GSM_module.println("AT+SAPBR=3,1,\"APN\",\"www\"\r");//setting the APN, the second need you fill in your local apn server
    #elif network == 2
    GSM_module.println("AT+SAPBR=3,1,\"APN\",\"aircelgprs.po\"\r");//setting the APN, the second need you fill in your local apn server
    #elif network == 3
    GSM_module.println("AT+SAPBR=3,1,\"APN\",\"airtelgprs.com\"\r");//setting the APN, the second need you fill in your local apn server
    #endif
    delay(100);
    #if serial_response
    ShowSerialData();
    #endif
    GSM_module.println("AT+SAPBR=1,1\r");//setting the SAPBR, for detail you can refer to the AT command mamual
    delay(2000);
    #if serial_response
    ShowSerialData();
    #endif  
    GSM_module.println("AT+HTTPINIT\r"); //init the HTTP request
    delay(100);
    #if serial_response
    ShowSerialData();
    #endif
    while(GSM_module.available()) GSM_module.read();
    GSM_module.print("AT+HTTPPARA=\"URL\",\"");
  #elif GSM_model == 1//Quectel M95
    GSM_module.println("AT+QIFGCNT=0\r");
    delay(100);
    #if serial_response
    ShowSerialData();
    #endif
    GSM_module.println("AT+QICSGP=1,\"CMNET\"\r");
    delay(100);
    #if serial_response
    ShowSerialData();
    #endif
    GSM_module.println("AT+QIREGAPP\r");
    delay(100);
    #if serial_response
    ShowSerialData();
    #endif
    GSM_module.println("AT+QIACT\r");
    delay(1000);
    #if serial_response
    ShowSerialData();
    #endif
    str_len = 54 + String(row_number + 1).length() + 7 + id.length() + 6 + String(w.temp1).length() + 6 + String(w.temp2).length() + 5 + String(w.hum).length() + 5 + String(w.wind_speed).length() + 5 + String(w.rain).length() + 5 + String(w.pressure).length() + 5 + String(w.solar_radiation).length() + 6 + String(w.latitude).length() + 6 + String(w.longitude).length() + 6 + String(signal_strength).length() + 1;
    t[0] = ((str_len / 100) % 10) + 48;
    t[1] = ((str_len / 10) % 10) + 48;
    t[2] = (str_len % 10) + 48;
    t[3] = '\0';
    GSM_module.print("AT+QHTTPURL=");
    GSM_module.print(t);
    GSM_module.print(",30\r");
    delay(1000);
    #if serial_response
    ShowSerialData();
    #else
    while(GSM_module.available()) GSM_module.read();
    #endif    
  #elif GSM_model == 2
  #endif
  
  GSM_module.print("http://enigmatic-caverns-27645.herokuapp.com/add_data/");   
  GSM_module.print(String(row_number + 1) + "=<"); 
  GSM_module.print("'id':" + id + ",");
  GSM_module.print("'t1':" + String(w.temp1) + ",");
  GSM_module.print("'t2':" + String(w.temp2) + ",");
  GSM_module.print("'h':" + String(w.hum) + ",");
  GSM_module.print("'w':" + String(w.wind_speed) + ",");
  GSM_module.print("'r':" + String(w.rain) + ",");
  GSM_module.print("'p':" + String(w.pressure) + ",");
  GSM_module.print("'s':" + String(w.solar_radiation) + ",");
  GSM_module.print("'lt':" + String(w.latitude) + ",");
  GSM_module.print("'ln':" + String(w.longitude) + ",");
  GSM_module.print("'sg':" + String(signal_strength) + ">\r");
  
  #if GSM_model == 0
    GSM_module.print("\"");
    #endif
    GSM_module.println("\r");
    delay(400);
    #if serial_response
    ShowSerialData();
    #else
    while(GSM_module.available()) GSM_module.read();
    #endif
    #if GSM_model == 0
    GSM_module.println("AT+HTTPACTION=0\r");//submit the request 
    for(i = 0; GSM_module.available() < 35 && i < 200; i++) delay(100);
    if(i < 200){
      for(i = 0; GSM_module.available(); i++) str[i] = GSM_module.read();
      str[i] = '\0';
      #if serial_response
      Serial.println(String(str));
      GSM_module.println("AT+HTTPREAD\r");
      delay(100);
      ShowSerialData();
      #endif
    }
    else{
      #if serial_response
      ShowSerialData();
      #else
      while(GSM_module.available()) GSM_module.read();
      #endif
      return 0;
    }
    delay(5000);  
    j = 100 * (str[40] - 48); j += 10 * (str[41] - 48); j += (str[42] - 48);
    if(j == 200) return true;
    else return false;
  #elif GSM_model == 1
    #if serial_response
    ShowSerialData();
    #else
    while(GSM_module.available()) GSM_module.read();
    #endif   
    GSM_module.println("AT+QHTTPGET=30\r");
    delay(10);
    #if serial_response
    ShowSerialData();
    #else
    while(GSM_module.available()) GSM_module.read();
    #endif   
    for(i = 0; GSM_module.available() < 3 && i < 200; i++) delay(100);
    //delay(1000);
    #if serial_response
    ShowSerialData();
    #else
    while(GSM_module.available()) GSM_module.read();
    #endif   
    GSM_module.println("AT+QHTTPREAD=30\r");
    delay(400);
    j = GSM_module.available();
    #if serial_response
    ShowSerialData();
    #else
    while(GSM_module.available()) GSM_module.read();
    #endif   
    if(j > 70 && i < 200) return 1;
    else return 0;
  #endif
}

boolean check_sms(){
  if(GSM_module.available() > 3){
    if(GSM_module.read() == '+'){
      if(GSM_module.read() == 'C'){
        if(GSM_module.read() == 'M'){
          if(GSM_module.read() == 'T'){
            return 1;
  }}}}}  
  return 0;
}

void get_sms(char n[], char b[]){
  char ch, i;
  ch = GSM_module.read();
  while(ch != '"'){ delay(1); ch = GSM_module.read(); }
  i = 0;
  while(1){
    delay(1);
    ch = GSM_module.read();
    if(ch == '"') break;
    else n[i++] = ch;
  }
  n[i] = '\0';
  while(ch != '\n'){ delay(1); ch = GSM_module.read(); }
  i = 0;
  while(1){
    delay(1);
    ch = GSM_module.read();
    if(ch == '\n' || ch == '\r') break;
    else b[i++] = ch;
  }
  b[i] = '\0';
}

void send_sms(char *n, String b){
  GSM_module.print("AT+CMGS=\"" + String(n) + "\"\n");
  delay(100);
  GSM_module.println(b);
  delay(100);
  GSM_module.write(26);
  GSM_module.write('\n');
  delay(100);
  GSM_module.write('\n');
}

void upload_sms(){
  GSM_module.print("AT+CMGS=\"" + phone_number + "\"\n");
  delay(100);
  GSM_module.print("HK9D7 ");
  GSM_module.print("'id':" + id + ",");
  GSM_module.print("'t1':" + String(w.temp1) + ",");
  GSM_module.print("'t2':" + String(w.temp2) + ",");
  GSM_module.print("'h':" + String(w.hum) + ",");
  GSM_module.print("'w':" + String(w.wind_speed) + ",");
  GSM_module.print("'r':" + String(w.rain) + ",");
  GSM_module.print("'p':" + String(w.pressure) + ",");
  GSM_module.print("'s':" + String(w.solar_radiation) + ",");
  GSM_module.print("'lt':" + String(w.latitude) + ",");
  GSM_module.print("'ln':" + String(w.longitude) + ",");
  GSM_module.print("'sg':" + String(signal_strength));
  delay(100);
  GSM_module.write(26);
  GSM_module.write('\n');
  delay(100);
  GSM_module.write('\n');
  #if serial_response
  ShowSerialData();
  #endif
}

boolean get_time(){
  uint8_t i;
  GSM_module.print("AT+QHTTPURL=24,30\r");
  delay(100);
  #if serial_response
  ShowSerialData();
  #else
  while(GSM_module.available()) GSM_module.read();
  #endif  
  GSM_module.print("http://www.yobi.tech/IST\r");
  delay(100);
  #if serial_response
  ShowSerialData();
  #else
  while(GSM_module.available()) GSM_module.read();
  #endif  
  GSM_module.print("AT+QHTTPGET=30\r");
  delay(10);
  #if serial_response
  ShowSerialData();
  #else
  while(GSM_module.available()) GSM_module.read();
  #endif   
  for(i = 0; GSM_module.available() < 3 && i < 200; i++) delay(100);
  if(i >= 200) return false;
  while(GSM_module.read() != 'p');
  GSM_module.read();
  year = (GSM_module.read() - 48) * 1000; year += (GSM_module.read() - 48) * 100; year += (GSM_module.read() - 48) * 10; year += (GSM_module.read() - 48);
  GSM_module.read();
  month = (GSM_module.read() - 48) * 10; month += (GSM_module.read() - 48);
  GSM_module.read();
  day = (GSM_module.read() - 48) * 10; day += (GSM_module.read() - 48);
  GSM_module.read();
  hours = (GSM_module.read() - 48) * 10; hours += (GSM_module.read() - 48);
  GSM_module.read();
  minutes = (GSM_module.read() - 48) * 10; minutes += (GSM_module.read() - 48);
  GSM_module.read();
  seconds = (GSM_module.read() - 48) * 10; seconds += (GSM_module.read() - 48);
  while(GSM_module.available()) GSM_module.read();
  #if serial_output
  Serial.println("Date");
  Serial.write((day / 10) % 10 + 48); Serial.write((day % 10) + 48);
  Serial.write('/');
  Serial.write((month / 10) % 10 + 48); Serial.write((month % 10) + 48);
  Serial.write('/');
  Serial.write((year / 1000) % 10 + 48); Serial.write((year / 100) % 10 + 48); Serial.write((year / 10) % 10 + 48); Serial.write((year % 10) + 48);
  Serial.println("\nDate");
  Serial.write((hours / 10) % 10 + 48); Serial.write((hours % 10) + 48);
  Serial.write(':');
  Serial.write((minutes / 10) % 10 + 48); Serial.write((minutes % 10) + 48);
  Serial.write(':');
  Serial.write((seconds / 10) % 10 + 48); Serial.write((seconds % 10) + 48);
  Serial.println();
  #endif
  return true;
}

#ifndef _328p
void write_SD(){
  datalog = SD.open("datalog.txt", FILE_WRITE);
  datalog.seek(datalog.size());
  datalog.print("1=<"); 
  datalog.print("'id':" + id + ",");
  datalog.print("'ts':");
  datalog.write((day / 10) % 10 + 48); datalog.write((day % 10) + 48);
  datalog.write('/');
  datalog.write((month / 10) % 10 + 48); datalog.write((month % 10) + 48);
  datalog.write('/');
  datalog.write((year / 1000) % 10 + 48); datalog.write((year / 100) % 10 + 48); datalog.write((year / 10) % 10 + 48); datalog.write((year % 10) + 48);
  datalog.print(" ");
  datalog.write((hours / 10) % 10 + 48); datalog.write((hours % 10) + 48);
  datalog.write(':');
  datalog.write((minutes / 10) % 10 + 48); datalog.write((minutes % 10) + 48);
  datalog.write(':');
  datalog.write((seconds / 10) % 10 + 48); datalog.write((seconds % 10) + 48);
  datalog.print(",'t1':" + String(w.temp1) + ",");
  datalog.print("'t2':" + String(w.temp2) + ",");
  datalog.print("'h':" + String(w.hum) + ",");
  datalog.print("'w':" + String(w.wind_speed) + ",");
  datalog.print("'r':" + String(w.rain) + ",");
  datalog.print("'p':" + String(w.pressure) + ",");
  datalog.print("'s':" + String(w.solar_radiation) + ",");
  datalog.print("'lt':" + String(w.latitude) + ",");
  datalog.print("'ln':" + String(w.longitude) + ",");
  datalog.print("'sg':" + String(signal_strength) + ">\n");
  datalog.close();
}
#endif

int get_signal_strength(){
  char temp[20];
  uint8_t i = 0;
  while(GSM_module.available()) GSM_module.read();
  GSM_module.print("AT+CSQ\r");
  delay(100);
  while(GSM_module.available()) temp[i++] = GSM_module.read();  
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
  while(GSM_module.available()) GSM_module.read();
  GSM_module.print("AT+COPS?\r");
  delay(100);
  i = GSM_module.available();
  #if serial_response
  ShowSerialData();
  #else
  while(GSM_module.available()) GSM_module.read();
  #endif
  if(i < 30) return 0;
  else return 1;
}

void GSM_module_reset(){
  digitalWrite(GSM_module_RST_pin, HIGH);
  delay(1000);
  digitalWrite(GSM_module_RST_pin, LOW);
}
void GSM_module_sleep(){
  GSM_module.print("AT+CSCLK=1\r");
  #if serial_response == true
  ShowSerialData();
  #endif
  delay(1000);
}
void GSM_module_wake(){
  digitalWrite(GSM_module_DTR_pin, HIGH);
  delay(1000);
  GSM_module.print("AT+CSCLK=0\r");
  #if serial_response == true
  ShowSerialData();
  #endif
  delay(1000);
  digitalWrite(GSM_module_DTR_pin, LOW);
}

long int pow_10(int a){
  long int t;
  int i;
  for(i = 0, t = 1; i < a; i++) t *= 10;
  return t;
}

void talk(){
  while(1){
    if(GSM_module.available()) Serial.write(GSM_module.read());
    if(Serial.available()) GSM_module.write(Serial.read());
  }
}

void ShowSerialData(){
  while(GSM_module.available() != 0) Serial.write(GSM_module.read());
}

