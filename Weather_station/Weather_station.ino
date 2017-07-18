#include <SoftwareSerial.h>
#include <String.h>
#include <Wire.h>
#include <BMP180.h>
#include <DHT.h>
#include <SHT2x.h>

#define rain_pin 2
#define led 4
#define wind_speed_pin 6
#define SIM900_DTR_pin 5

#define solar_radiation_pin A1

//------------------------------------Settings-------------------------------------
String id = "ws47";     

#define led_mode 2//0 for rain guage, 1 for anemometer, 2 to deactivate led -- regardless of this variable, the led always lights up when seending data
#define HT_mode 1// 0 for SHT21, 1 for DST, 2 for none
       
#define data_upload_frequency 1//Minutes -- should be a multiple of the read frequency
#define data_read_frequency 1//Minutes

#define radius 5.56//in centimeters

#define enable_GPS false//True to enable GPS    
#define enable_BMP180 false//True to enable BMP180

#define serial_output true//True to debug: display values through the serial port
//----------------------------------------------------------------------------------

SoftwareSerial SIM900(8, 7);

#if HT_mode == 1
  #define DHTPIN 3    
  #define DHTTYPE DHT22 
  DHT dht(DHTPIN, DHTTYPE);
#endif

//Rain variables
int rain_counter[data_upload_frequency / data_read_frequency];
boolean rain_flag = true, rain_pin_reading;

//Humidity/temperature variables
float temp, hum;
char str_temp[data_upload_frequency / data_read_frequency][10], str_hum[data_upload_frequency / data_read_frequency][10];

//BMP180 variables
#if enable_BMP180 == true
  long int BMP180_pressure;
  float BMP180_temp;
  char str_BMP180_temp[data_upload_frequency / data_read_frequency][10];
  BMP180 barometer;
#endif

//GPS variables
#if enable_GPS == true
  int GPS_wait;
  float lat, lon;
  char latitude[15], longitude[15];
#endif

//Wind Speed variables
int wind_speed_counter = 0;
float wind_speed;
boolean wind_flag = true, wind_temp;
char str_wind_speed[data_upload_frequency / data_read_frequency][15];

//Solar radiation variables
int solar_radiation[data_upload_frequency / data_read_frequency];

//Timer interrupt variables
int upload_counter, read_counter, i, c = 0;
boolean four_sec = true;

void reset_array(int a[]){
  for(i = 0; i < data_upload_frequency / data_read_frequency; i++){
    a[i] = 0;
  }
}

void setup(){
  pinMode(led, OUTPUT);
  pinMode(rain_pin, INPUT);
  pinMode(wind_speed_pin, INPUT);
  pinMode(SIM900_DTR_pin, OUTPUT);
  reset_array(rain_counter);
  
  SIM900.begin(19200); // the GPRS baud rate   
  Serial.begin(9600);  // the GPRS baud rate 
  #if serial_output
    Serial.println("Working");
  #endif

  #if enable_BMP180 == true || HT_mode == 0
    Wire.begin();
  #endif
  
  #if enable_BMP180 == true
    barometer = BMP180();
    barometer.SoftReset();
    barometer.Initialize();
  #endif
  
  digitalWrite(led, HIGH);
  delay(10000);//Wait for the SIM to log on to the network
  digitalWrite(led, LOW);

  //SIM900_wake();

  SIM900_wake();
  SIM900.print("AT+CMGF=1\r"); 
  SIM900.print("AT+CNMI=2,2,0,0,0\r"); 
  SIM900.print("AT+CLIP=1\r"); 
  delay(5000);
  SIM900_sleep();
  
  upload_counter = 0;
  read_counter = 0;
  i = 0;
  c = 0;
  hum = 0;
  temp = 0;
  #if enable_BMP180 == true
    BMP180_pressure = 0;
    BMP180_temp = 0;
  #endif
  
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
  OCR2A = 2000;
  TCCR2A |= (1 << WGM21);
  TCCR2B |= (1 << CS21);   
  TIMSK2 |= (1 << OCIE2A);
  //Timer1: 0.25hz, Timer2: 1000hz
  interrupts(); 
}
 
void loop(){
  rain_pin_reading = digitalRead(rain_pin);
  if(rain_pin_reading == 1 && rain_flag == true){
    #if led_mode == 0
      digitalWrite(led, HIGH);
    #endif
    rain_counter[i]++;
    rain_flag = false;
    delay(220);
  }
  else if(rain_pin_reading == 0){
    rain_flag = true;
    #if led_mode == 0
      digitalWrite(led, LOW);
    #endif
  }
  if(four_sec){
    four_sec = false;
    #if HT_mode == 0
      hum += SHT2x.GetHumidity();
      temp += SHT2x.GetTemperature();
    #endif
    #if HT_mode == 1
      hum += dht.readHumidity();
      temp += dht.readTemperature();
    #endif
    #if enable_BMP180 == true
      pressure += barometer.GetPressure();
      BMP180_temp += barometer.GetTemperature();
    #endif
    c++;
  }
  if(read_counter >= data_read_frequency * 15){
    read_counter = 0;
    digitalWrite(led, HIGH);
    #if serial_output
      Serial.println("Reading");
    #endif
    //Read data
    hum /= c;
    temp /= c;
    #if enable_BMP180 == true
      BMP180_pressure /= c;
      BMP180_temp /= c;
    #endif
    c = 0;
    wind_speed = ((wind_speed_counter) / data_upload_frequency) * radius * 0.37699169840664;
    solar_radiation[i] = analogRead(solar_radiation_pin);
    
    //Float to string conversion
    FloatToString(temp, str_temp[i]);
    FloatToString(hum, str_hum[i]);
    #if enable_BMP180 == true
      //FloatToString(BMP180_temp, str_temp[i]);    
      FloatToString(wind_speed, str_wind_speed[i]);
    #endif
    
    #if serial_output
      Serial.print("Humidity(%RH): ");
      Serial.print(hum);
      Serial.print("     Temperature(C): ");
      Serial.println(temp);
    #endif
    i++;
    wind_speed_counter = 0;
    temp = 0;
    hum = 0;
    #if enable_BMP180 == true
      BMP180_pressure = 0;
      BMP180_temp = 0;
    #endif
    digitalWrite(led, LOW);
    delay(200);
  }
  if(upload_counter >= data_upload_frequency * 15){
    SIM900_wake();
    upload_counter = 0;
    digitalWrite(led, HIGH);
    #if serial_output
      Serial.print("Uploading data");
    #endif
    SubmitHttpRequest();
    #if serial_output
      Serial.print("Done");
    #endif
    digitalWrite(led, LOW);
    reset_array(rain_counter);
    i = 0;
    SIM900_sleep();
  }
  #if enable_GPS
    if(upload_counter >= (data_upload_frequency * 15) - 4 && upload_counter < data_upload_frequency * 15){  
      GetGPS();      
      digitalWrite(led, LOW);
      delay(250);
      digitalWrite(led, HIGH);   
    } 
  #endif
}

ISR(TIMER1_COMPA_vect){ //Interrupt gets called every 4 seconds 
  #if serial_output
    Serial.println("*");
  #endif
  #if GPS
    if(++GPS_wait >= 2){
      GPS_wait = 2;
    }
  #endif
  four_sec = true;
  if(upload_counter < data_upload_frequency * 15){
    upload_counter++;
  }
  if(read_counter < data_read_frequency * 15){
    read_counter++;
  }
}

ISR(TIMER2_COMPA_vect){ 
  wind_temp = digitalRead(wind_speed_pin);
  if(wind_temp == 0 && wind_flag == true){
    wind_speed_counter++;
    wind_flag = false;
    #if led_mode == 1
      if(upload >= data_upload_frequency * 15){
        digitalWrite(led, HIGH);
      }
    #endif
  } 
  else if(wind_temp == 1){
    wind_flag = true;
    #if led_mode == 1
      if(upload >= data_upload_frequency * 15){
        digitalWrite(led, LOW);
      }
    #endif
  } 
}

void SubmitHttpRequest(){
  SIM900.println("AT+CSQ");
  delay(100);
  SIM900.println("AT+CGATT?");
  delay(500);
  SIM900.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"");//setting the SAPBR, the connection type is using gprs
  delay(1000);
  SIM900.println("AT+SAPBR=3,1,\"APN\",\"www\"");//setting the APN, the second need you fill in your local apn server
  delay(2000);
  SIM900.println("AT+SAPBR=1,1");//setting the SAPBR, for detail you can refer to the AT command mamual
  delay(4000);
  SIM900.println("AT+HTTPINIT"); //init the HTTP request
  delay(2000); 
  
  SIM900.println("AT+HTTPPARA=\"URL\",\"http://www.myvillageshop.com/agsense/add_data_ws.php?ws=" + id + "&air_temp=" + String(str_temp[0]) + "&humidity=" + String(str_hum[0]) + "&wind_speed=" + String(str_wind_speed[0]) + "&rainfall=" + String(rain_counter[0]) + "&air_pressure=N/A&solar_radiation=" + String(solar_radiation[0]) + "&latitude=N/A&longitude=N/A\""); 
  
  /*SIM900.write("AT+HTTPPARA=\"URL\",\"http://www.myvillageshop.com/agsense/add_data_ws.php?"); 
  SIM900.print("ws=" + id);
  SIM900.print("&air_temp=" + String(str_temp[0]));
  SIM900.print("&humidity=" + String(str_hum[0]));
  SIM900.print("&wind_speed=" + String(str_wind_speed[0]));
  SIM900.print("&rainfall=" + String(rain_counter[0]));
  #if enable_BMP180 == true
    SIM900.print("&air_pressure=" + String(BMP180_pressure));
  #else
   SIM900.print("&air_pressure=NotConnected");
  #endif
  SIM900.print("&solar_radiation=" + String(solar_radiation[0]));
  #if enable_GPS == true
     SIM900.println("&latitude=" + (String)latitude + "&longitude=" + (String)longitude + "\"");
  #else
     SIM900.println("&latitude=NotConnected&longitude=NotConnected\"");
  #endif*/
  delay(1000);
  SIM900.println("AT+HTTPACTION=0");//submit the request 
  delay(10000);//the delay is very important, the delay time is base on the return from the website, if the return datas are very large, the time required longer.
  SIM900.println("AT+HTTPREAD");// read the data from the website you access
  delay(100);
  SIM900.println("");
}

void SIM900_sleep(){
  SIM900.println("AT+CSCLK=1\r");
  delay(1000);
}

void SIM900_wake(){
  digitalWrite(SIM900_DTR_pin, HIGH);
  delay(1000);
  SIM900.println("AT+CSCLK=0\r");
  delay(1000);
  digitalWrite(SIM900_DTR_pin, LOW);
}

#if enable_GPS == true
void GetGPS(){
  int i, j, k;
  char ch;
  boolean GPS_flag;
  GPS_flag = false;
  GPS_wait = 0;
  do{
    delay(10);
    ch = Serial.read();
    if(ch == '$'){
      delay(10);
      ch = Serial.read();
      if(ch == 'G'){
        delay(10);
        ch = Serial.read();
        if(ch == 'P'){
          delay(10);
          ch = Serial.read();
          if(ch == 'R'){
            delay(10);
            ch = Serial.read();
            if(ch == 'M'){
              delay(10);
              ch = Serial.read();
              if(ch == 'C'){
                delay(10);
                ch = Serial.read();
                for(i = 0, j = 0, k = 0; k < 6;){
                  delay(10);
                  ch = Serial.read();
                  if(ch == 'V'){
                    break;
                  }
                  if(k > 1 && k < 4){  
                    latitude[i++] = ch;
                  }
                  else if(k > 3 && k < 6){
                    longitude[j++] = ch;
                  }
                  if(ch == ','){
                    k++;
                  }
                } 
                if(ch == 'V'){
                  strcpy(latitude, "No Signal");
                  strcpy(longitude, "No Signal");
                }
                else{
                  latitude[i - 1] = '\0';
                  longitude[j - 1] = '\0';
                  lat = GPS_StringToFloat(latitude);
                  lon = GPS_StringToFloat(longitude);
                  FloatToString(lat, latitude);
                  FloatToString(lon, longitude);
                }        
                GPS_flag = true;
              }  
            }
          }
        }
      }
    }  
  }while(GPS_flag == false && GPS_wait != 2);
}
#endif

double GPS_StringToFloat(char *str){
  double temp = 0, temp_decimal = 0;
  int i = 0, j;
  long int big;
  while(*str != '.'){
    str++;
    i++;
  }
  str -= 3;
  i -= 3;
  for(big = 1; i >= 0; i--, str--, big *= 10){
    temp += (*str - 48) * big;
  }
  while(*str != '.'){
    str++;
  }
  str -= 2;
  i = 0;
  while(isdigit(*str) || *str == '.'){
    str++;
    i++;
  }
  i--;
  j = i;
  str--;
  for(big = 1; i >= 0; i--, str--){
    if(*str != '.'){
      temp_decimal += (*str - 48) * big;
      big *= 10;
    }
  }
  temp_decimal = temp_decimal * 10 / 6;
  temp += (temp_decimal / pow_10(j));
  return temp;
}

long int pow_10(int a){
  long int t;
  int i;
  for(i = 0, t = 1; i < a; i++){
    t *= 10;
  }
  return t;
}

void FloatToString(float f, char str[]){
  double t, tt;
  int i = 0, j = 0, k, l;
  long int temp, temp2;
  t = f;
  temp = f;
  while((floor(t) - t) != 0){
    t *= 10;
    i++;
  }
  tt = floor(f);
  t = t - (tt * pow_10(i));
  while(temp > 0){
    temp /= 10;
    j++;
  }
  
  temp2 = t;
  temp = tt;
  if(j == 0){
    str[k++] = '0';
    j = 1;
  }
  else{
    for(k = 0; k < j; k++){
      str[k] = (temp / pow_10(j - k - 1)) % 10 + 48;
    }
  }
  str[j] = '.';
  for(k = j + 1; k <= (i + j); k++){
    str[k] = (temp2 / pow_10(j + i - k)) % 10 + 48;
  }
  if(k == (j + 1)){
    str[k++] = '0';
  }
  str[k] = '\0';
}

void ShowSerialData(){
  while(SIM900.available() != 0){
    Serial.write(SIM900.read());
  }
}
