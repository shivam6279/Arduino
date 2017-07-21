#include <EEPROM.h>
#include <SoftwareSerial.h>
#include <String.h>
#include <Wire.h>
//#include <BMP180.h>
#include <DHT.h>
#include <SHT2x.h>
#include <SPI.h>
#include <SdFat.h>
#include <avr/wdt.h>

#define rain_led 35
#define wind_led 41
#define upload_led 12
#define rain_pin 4
#define wind_pin A1
#define GSM_module_DTR_pin 24
#define solar_radiation_pin A3
#define battery_pin A15

//------------------------------------Settings-------------------------------------  
String phone_number = "+919220592205";

#define HT_mode 1// 0 for SHT21, 1 for DST, 2 for none
       
#define data_upload_frequency 1//Minutes -- should be a multiple of the read frequency
#define data_read_frequency 1//Minutes
#define number_of_readings (data_upload_frequency / data_read_frequency)

#define enable_GPS false//True to enable GPS    
#define enable_BMP180 false //True to enable BMP180

#define serial_output true//True to debug: display values through the serial port
#define serial_response true//True to see SIM900 serial response
//----------------------------------------------------------------------------------

SdFat sd;
SdFile datalog, data_temp;
bool SD_flag = false;

#if HT_mode == 1
#define DHTPIN 25    
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
boolean wind_flag = false, wind_temp;

//Rain guage
volatile int rain_counter;
boolean rain_flag = false, rain_temp;

//Timer interrupt
uint8_t timer1_counter = 0;
volatile uint8_t GPS_wait;
volatile boolean four_sec = false, read_flag = false, upload_flag = false;

//SIM
float signal_strength;

weather_data w;
uint8_t reading_number = 0, number_of_fail_uploads = 0;

uint8_t seconds = 0, minutes = 0, hours = 0, day = 0, month = 0;
uint16_t year = 0;
boolean startup = true;
char id[4];

void setup(){
  pinMode(rain_led, OUTPUT);
  pinMode(wind_led, OUTPUT);
  pinMode(upload_led, OUTPUT);
  pinMode(53, OUTPUT);
  pinMode(rain_pin, INPUT);
  pinMode(wind_pin , INPUT);
  pinMode(GSM_module_DTR_pin, OUTPUT);
  digitalWrite(GSM_module_DTR_pin, LOW);

  Serial.begin(9600); 
  Serial1.begin(19200);
  pinMode(19, INPUT);  
  digitalWrite(19, HIGH);
  Serial2.begin(9600);
  pinMode(17, INPUT);  
  digitalWrite(17, HIGH);
  //Serial3.begin(9600);
  
  uint8_t i = 0;
  SD_flag = sd.begin(53);
  delay(100);
  if(!sd.exists("id.txt")){
    #if serial_output
    Serial.println("No id file found!!!!!!");
    #endif
    while(1);
  }
  datalog.open("id.txt", FILE_READ);
  while(datalog.available()){
    id[i++] = datalog.read();
  }
  id[i] = '\0';
  datalog.close();
  
  //Wire.begin();
  
  #if enable_BMP180
  barometer = BMP180();
  barometer.SoftReset();
  barometer.Initialize();
  #endif
  
  #if serial_output
  Serial.println("Sensor id: " + String(id));
  Serial.println("Data upload frequency: " + (String)data_upload_frequency + " minutes");
  Serial.println("Data read frequency: " + (String)data_read_frequency + " minutes");
  if(HT_mode == 0) Serial.println("Using SHT21"); else if(HT_mode == 1) Serial.println("Using DHT22"); else Serial.println("No HT sensor");
  if(enable_BMP180) Serial.println("BMP180 enabled"); else Serial.println("BMP180 disabled");
  if(enable_GPS) Serial.println("GPS enabled"); else Serial.println("GPS disabled");
  Serial.println();
  #endif

  //talk();

  //GSM_module_reset();
  digitalWrite(upload_led, HIGH);
  delay(10000);//Wait for the SIM to log on to the network
  digitalWrite(upload_led, LOW);
  //GSM_module_wake();
  Serial1.print("AT+QSCLK=2\r"); delay(100);
  #if serial_response
  ShowSerialData();
  #endif
  Serial1.print("AT+CMGF=1\r"); delay(100);
  #if serial_response
  ShowSerialData();
  #endif
  Serial1.print("AT+CNMI=2,2,0,0,0\r"); delay(100); 
  #if serial_response
  ShowSerialData();
  #endif
  Serial1.print("AT+CLIP=1\r"); delay(100);
  #if serial_response
  ShowSerialData();
  #endif
  delay(1000); 
  //GSM_module_sleep();
  
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
  char body_r[40], number[14];
  
  rain_counter = 0;
  wind_speed_counter = 0;
  reading_number = 0;

  w.temp1 = 0.0;
  w.temp2 = 0.0;
  w.hum = 0.0;
  w.solar_radiation = 0;
  w.pressure = 0;
  
  while(1){
    if(check_sms()){
      get_sms(number, body_r);
      if(toupper(body_r[0]) == 'O' &&  toupper(body_r[1]) == 'T' &&toupper(body_r[2]) == 'A' && sd.begin(53)){
        send_sms(number, "Downloading new firmware");
        #if serial_output
        Serial.println("Updating firmware");
        #endif
        delay(500);
        if(sd.exists("TEMP_OTA.HEX")) sd.remove("TEMP_OTA.HEX");
        if(sd.exists("firmware.BIN")) sd.remove("firmware.BIN");
        if(download_hex()){
          #if serial_output
          Serial.println("\nNew firmware downloaded");
          Serial.println("Converting .hex file to .bin");
          #endif
          if(SD_copy()){
            #if serial_output
            Serial.println("Done\nRestarting and re-programming");
            #endif
            while(Serial1.available()) Serial1.read();
            EEPROM.write(0x1FF,0xF0);
            wdt_enable(WDTO_500MS);
            wdt_reset();
            delay(600);
          }
          else{
            #if serial_output
            Serial.println("SD card copy error- hex file checksum failed");
            #endif
          }
        }
        else{
          #if serial_output
          Serial.println("Download failed");
          #endif
        }
      }
    }
    if(four_sec){
      four_sec = false;
      temp_read = read_flag;
      temp_upload = upload_flag;
  
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
      w.battery_voltage = (w.battery_voltage * float(avg_counter) + (float(analogRead(battery_pin)) * 10.0 / 1023.0)) / (float(avg_counter) + 1.0);
      avg_counter++;

      if(isnan(w.hum)) w.hum = 0;
      if(isnan(w.temp1)) w.temp1 = 0;
      if(isnan(w.temp2)) w.temp2 = 0;
      if(isnan(w.pressure)) w.pressure = 0;
  
      if(temp_read){//Read data
        read_flag = false;
        digitalWrite(upload_led, HIGH);
        
        //Read data
        avg_counter = 0;
        
        w.wind_speed = ((wind_speed_counter) / data_upload_frequency);
        wind_speed_counter = 0;
        
        w.rain = rain_counter;
        rain_counter = 0;
        
        #if enable_GPS == true
        //GetGPS();
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
        
        digitalWrite(upload_led, LOW);
      }
      if(temp_upload){//Upload data
        digitalWrite(upload_led, HIGH);
        
        signal_strength = get_signal_strength();
        #if serial_output
        Serial.println("Signal strength: " + String(signal_strength));
        #endif
        
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
          startup = false;
          reading_number = 0;
          number_of_fail_uploads = 0;
          #if serial_output
          Serial.println("\nUpload successful");
          #endif
        }
        delay(2000);
        /*#if serial_output
        Serial.println("\nGetting Time");
        #endif
        get_time();*/
        #if serial_output
        Serial.println("\nWriting to SD card");
        #endif
        write_SD();
        
        digitalWrite(upload_led, LOW);
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
  rain_temp = digitalRead(rain_pin);
  if(rain_temp == 1 && rain_flag == true){
    digitalWrite(rain_led, HIGH);
    rain_counter++;
    rain_flag = false;
  }
  else if(rain_temp == 0){
    rain_flag = true;
    digitalWrite(rain_led, LOW);
  }
  wind_temp = digitalRead(wind_pin);
  if(wind_temp == 0 && wind_flag == true){
    wind_speed_counter++;
    wind_flag = false;
    digitalWrite(wind_led, HIGH);
  } 
  else if(wind_temp == 1){
    wind_flag = true;
    digitalWrite(wind_led, LOW);
  } 
}

bool SubmitHttpRequest(){
  uint8_t j;
  uint16_t i;
  int str_len;
  char t[4];
  
  #if serial_output
  Serial.println("Uploading data");
  #endif
  while(Serial1.available()) Serial1.read();
  Serial1.println("AT+QIFGCNT=0\r");
  delay(100);
  #if serial_response
  ShowSerialData();
  #endif
  Serial1.println("AT+QICSGP=1,\"CMNET\"\r");
  delay(100);
  #if serial_response
  ShowSerialData();
  #endif
  Serial1.println("AT+QIREGAPP\r");
  delay(100);
  #if serial_response
  ShowSerialData();
  #endif
  Serial1.println("AT+QIACT\r");
  delay(2000);
  #if serial_response
  ShowSerialData();
  #endif
  while(Serial1.available()) Serial1.read();
  str_len = 66 + String(id).length()+4 + String(w.temp1).length()+4 + String(w.temp2).length()+4 + String(w.hum).length()+3 + String(w.wind_speed).length()+3 + String(w.rain).length()+3 + String(w.pressure).length()+3 + String(w.solar_radiation).length()+3 + String(w.latitude).length()+4 + String(w.longitude).length()+4 + String(signal_strength).length()+4;
  t[0] = ((str_len / 100) % 10) + 48;
  t[1] = ((str_len / 10) % 10) + 48;
  t[2] = (str_len % 10) + 48;
  t[3] = '\0';
  Serial1.print("AT+QHTTPURL=");
  Serial1.print(t);
  Serial1.print(",30\r");
  delay(1000);
  #if serial_response
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif    
  
  Serial1.print("http://enigmatic-caverns-27645.herokuapp.com/maytheforcebewithyou?");   
  //Serial1.print(String(row_number + 1) + "=<"); 
  Serial1.print("id=" + String(id) + "&");
  Serial1.print("t1=" + String(w.temp1) + "&");
  Serial1.print("t2=" + String(w.temp2) + "&");
  Serial1.print("h=" + String(w.hum) + "&");
  Serial1.print("w=" + String(w.wind_speed) + "&");
  Serial1.print("r=" + String(w.rain) + "&");
  Serial1.print("p=" + String(w.pressure) + "&");
  Serial1.print("s=" + String(w.solar_radiation) + "&");
  Serial1.print("lt=" + String(w.latitude) + "&");
  Serial1.print("ln=" + String(w.longitude) + "&");
  Serial1.print("sg=" + String(signal_strength) + ">\r");
  delay(300);
  
  #if serial_response
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif   
  Serial1.println("AT+QHTTPGET=30\r");
  delay(100);
  #if serial_response
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif   
  for(i = 0; Serial1.available() < 3 && i < 200; i++) delay(100);
  //delay(1000);
  #if serial_response
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif   
  Serial1.println("AT+QHTTPREAD=30\r");
  delay(400);
  j = Serial1.available();
  #if serial_response
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif   
  if(j > 70 && i < 200) return true;
  else return false;
}

bool download_hex(){  
  int i;
  char ch;
  if(!sd.exists("id.txt")){
    if(!sd.begin(53)) return false;
  }
  datalog.open("temp_ota.hex", FILE_WRITE);
  while(Serial1.available()) Serial1.read();
  Serial1.println("AT+QIFGCNT=0\r");
  delay(100);
  #if serial_response
  ShowSerialData();
  #endif
  Serial1.println("AT+QICSGP=1,\"CMNET\"\r");
  delay(100);
  #if serial_response
  ShowSerialData();
  #endif
  Serial1.println("AT+QIREGAPP\r");
  delay(100);
  #if serial_response
  ShowSerialData();
  #endif
  Serial1.println("AT+QIACT\r");
  delay(2000);
  #if serial_response
  ShowSerialData();
  #endif
  Serial1.print("AT+QHTTPURL=26,30\r");
  delay(1000);
  #if serial_response
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif  
  Serial1.print("http://www.yobi.tech/ota/4\r");
  delay(100);
  #if serial_response
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif  
  while(Serial1.available()) Serial1.read();
  Serial1.println("AT+QHTTPGET=30\r");
  delay(700);
  do{
    if(Serial1.available()){
      ch = Serial1.read();
      Serial.write(ch);
    }
  }while(!isdigit(ch));
  do{
    if(Serial1.available()){
      ch = Serial1.read();
      Serial.write(ch);
    }
  }while(!isAlpha(ch));
  delay(100);
  #if serial_response
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif  
  if(ch != 'O') return false;
  Serial1.println("AT+QHTTPREAD=30\r");
  for(i = 0; i < 3;){
    if(Serial1.read() ==  '>') i++;
  }
  ch = Serial1.read();
  while(!isDigit(ch) && ch != ':') ch = Serial1.read();
  datalog.write(ch);
  i = 0;
  while(1){
    while(Serial1.available()){
      ch = Serial1.read();
      if(ch == 'O') break;
      datalog.write(ch);
      i = 0;
    }
    if(ch == 'O') break;
    while(Serial1.available() == 0){
      i++;
      delay(1);
      if(i > 3000) break;
    }
    if(i > 3000) break;
  }
  datalog.seekCur(-1);
  datalog.close();
  while(Serial1.available()) Serial1.read();
  return true;
}

bool SD_copy(){
  unsigned char buff[37], ch, n, t_ch[2];
  uint16_t i, j, temp_checksum;
  long int checksum;
  if(!sd.exists("id.txt")){
    if(!sd.begin(53)) return false;
  }
  if(!sd.exists("TEMP_OTA.HEX")) return 0;
  data_temp.open("TEMP_OTA.HEX", O_READ);
  datalog.open("firmware.bin", O_WRITE | O_CREAT);

  ch = data_temp.read();
  while((ch < 97 && ch > 58) || ch < 48 || ch > 102){
    ch = data_temp.read();
  }
  data_temp.seekCur(-1);
  j = 0;
  while(data_temp.available()){
    data_temp.read(buff, 9);
    for(i = 1, checksum = 0; i < 9; i++){
      t_ch[0] = buff[i++];
      t_ch[1] = buff[i];
      if((t_ch[0] >= 48 && t_ch[0] <= 57) || (t_ch[0] >= 97 && t_ch[0] <= 102) || (t_ch[0] >= 65 && t_ch[0] <= 70)){
        if((t_ch[0] >= 48 && t_ch[0] <= 57)) t_ch[0] -= 48;
        else{
          if(t_ch[0] >= 97 && t_ch[0] <= 102) t_ch[0] -= 87;
          else t_ch[0] -= 55;
        }
        if((t_ch[1] >= 48 && t_ch[1] <= 57)) t_ch[1] -= 48;
        else{
          if(t_ch[1] >= 97 && t_ch[1] <= 102) t_ch[1] -= 87;
          else t_ch[1] -= 55;
        }
        ch = t_ch[0] * 16 + t_ch[1];
        checksum += ch;
        if(i == 2) n = ch;
      }
    }
    data_temp.read(buff, (n*2)+4);
    for(i = 0; i < (n*2); i++){
      t_ch[0] = buff[i++];
      t_ch[1] = buff[i];
      if((t_ch[0] >= 48 && t_ch[0] <= 57) || (t_ch[0] >= 97 && t_ch[0] <= 102) || (t_ch[0] >= 65 && t_ch[0] <= 70)){
        if((t_ch[0] >= 48 && t_ch[0] <= 57)) t_ch[0] -= 48;
        else{
          if(t_ch[0] >= 97 && t_ch[0] <= 102) t_ch[0] -= 87;
          else t_ch[0] -= 55;
        }
        if((t_ch[1] >= 48 && t_ch[1] <= 57)) t_ch[1] -= 48;
        else{
          if(t_ch[1] >= 97 && t_ch[1] <= 102) t_ch[1] -= 87;
          else t_ch[1] -= 55;
        }
        ch = t_ch[0] * 16 + t_ch[1];
        checksum += ch;
        datalog.write(ch);
        if(++j == 500){
          j = 0;
          datalog.sync();
        }
      }
    }
    t_ch[0] = buff[(n*2)];
    t_ch[1] = buff[(n*2) + 1];
    if((t_ch[0] >= 48 && t_ch[0] <= 57) || (t_ch[0] >= 97 && t_ch[0] <= 102) || (t_ch[0] >= 65 && t_ch[0] <= 70)){
      if((t_ch[0] >= 48 && t_ch[0] <= 57)){
        t_ch[0] -= 48;
      }
      else{
        if(t_ch[0] >= 97 && t_ch[0] <= 102) t_ch[0] -= 87;
        else t_ch[0] -= 55;
      }
      if((t_ch[1] >= 48 && t_ch[1] <= 57)){
        t_ch[1] -= 48;
      }
      else{
        if(t_ch[1] >= 97 && t_ch[1] <= 102) t_ch[1] -= 87;
        else t_ch[1] -= 55;
      }
      temp_checksum = t_ch[0] * 16 + t_ch[1];
    }
    if((checksum + temp_checksum) % 0x100 != 0) return false;
  }
  datalog.sync();
  datalog.close();
  data_temp.close();
  return true;
}

boolean check_sms(){
  if(Serial1.available() > 3){
    if(Serial1.read() == '+'){
      if(Serial1.read() == 'C'){
        if(Serial1.read() == 'M'){
          if(Serial1.read() == 'T'){
            return 1;
  }}}}}  
  return 0;
}

void get_sms(char n[], char b[]){
  char ch, i;
  ch = Serial1.read();
  while(ch != '"'){ delay(1); ch = Serial1.read(); }
  i = 0;
  while(1){
    delay(1);
    ch = Serial1.read();
    if(ch == '"') break;
    else n[i++] = ch;
  }
  n[i] = '\0';
  while(ch != '\n'){ delay(1); ch = Serial1.read(); }
  i = 0;
  while(1){
    delay(1);
    ch = Serial1.read();
    if(ch == '\n' || ch == '\r') break;
    else b[i++] = ch;
  }
  b[i] = '\0';
}

void send_sms(char *n, char *b){
  Serial1.print("AT+CMGS=\"" + String(n) + "\"\n");
  delay(100);
  Serial1.println(b);
  delay(100);
  Serial1.write(26);
  Serial1.write('\n');
  delay(100);
  Serial1.write('\n');
  delay(100);
  while(Serial1.available()) Serial1.read();
}

void upload_sms(){
  Serial1.print("AT+CMGS=\"" + phone_number + "\"\n");
  delay(100);
  Serial1.print("HK9D7 ");
  Serial1.print("'id':" + String(id) + ",");
  Serial1.print("'t1':" + String(w.temp1) + ",");
  Serial1.print("'t2':" + String(w.temp2) + ",");
  Serial1.print("'h':" + String(w.hum) + ",");
  Serial1.print("'w':" + String(w.wind_speed) + ",");
  Serial1.print("'r':" + String(w.rain) + ",");
  Serial1.print("'p':" + String(w.pressure) + ",");
  Serial1.print("'s':" + String(w.solar_radiation) + ",");
  Serial1.print("'lt':" + String(w.latitude) + ",");
  Serial1.print("'ln':" + String(w.longitude) + ",");
  Serial1.print("'sg':" + String(signal_strength));
  delay(100);
  Serial1.write(26);
  Serial1.write('\n');
  delay(100);
  Serial1.write('\n');
  #if serial_response
  ShowSerialData();
  #endif
}

boolean get_time(){
  uint8_t i;
  while(Serial1.available()) Serial1.read();
  Serial1.print("AT+QHTTPURL=24,30\r");
  delay(100);
  Serial1.print("http://www.yobi.tech/IST\r");
  delay(500);
  while(Serial1.available()) Serial1.read();
  Serial1.println("AT+QHTTPGET=30\r");
  delay(100);
  while(Serial1.available()) Serial1.read();
  for(i = 0; Serial1.available() < 3 && i < 200; i++) delay(100);
  while(Serial1.available()) Serial1.read();
  Serial1.println("AT+QHTTPREAD=30\r");
  delay(600);
  for(i = 0; Serial1.read() != '\n' && i < 200; i++) delay(100);
  for(i = 0; Serial1.read() != '\n' && i < 200; i++) delay(100);
  year = (Serial1.read() - 48) * 1000; year += (Serial1.read() - 48) * 100; year += (Serial1.read() - 48) * 10; year += (Serial1.read() - 48);
  Serial1.read();
  month = (Serial1.read() - 48) * 10; month += (Serial1.read() - 48);
  Serial1.read();
  day = (Serial1.read() - 48) * 10; day += (Serial1.read() - 48);
  Serial1.read();
  hours = (Serial1.read() - 48) * 10; hours += (Serial1.read() - 48);
  Serial1.read();
  minutes = (Serial1.read() - 48) * 10; minutes += (Serial1.read() - 48);
  Serial1.read();
  seconds = (Serial1.read() - 48) * 10; seconds += (Serial1.read() - 48);
  while(Serial1.available()) Serial1.read();
  #if serial_output
  Serial.println("Date");
  Serial.write((day / 10) % 10 + 48); Serial.write((day % 10) + 48);
  Serial.write('/');
  Serial.write((month / 10) % 10 + 48); Serial.write((month % 10) + 48);
  Serial.write('/');
  Serial.write((year / 1000) % 10 + 48); Serial.write((year / 100) % 10 + 48); Serial.write((year / 10) % 10 + 48); Serial.write((year % 10) + 48);
  Serial.println("\nTime");
  Serial.write((hours / 10) % 10 + 48); Serial.write((hours % 10) + 48);
  Serial.write(':');
  Serial.write((minutes / 10) % 10 + 48); Serial.write((minutes % 10) + 48);
  Serial.write(':');
  Serial.write((seconds / 10) % 10 + 48); Serial.write((seconds % 10) + 48);
  Serial.println();
  #endif
  return true;
}

boolean write_SD(){
  if(!sd.exists("id.txt")){
    if(!sd.begin(53)) return false;
  }
  datalog.open("datalog.txt", FILE_WRITE);
  datalog.seekEnd();
  datalog.print("1=<"); 
  datalog.print("'id':" + String(id) + ",");
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
  datalog.println("'sg':" + String(signal_strength) + ">");
  datalog.close();
  return true;
}

int get_signal_strength(){
  char ch, temp[3];
  int i = 0, tens;
  while(Serial1.available()) Serial1.read();
  Serial1.print("AT+CSQ\r");
  delay(200);
  for(i = 0, ch = Serial1.read(); (ch < '0' || ch > '9') && i < 100; i++){
    ch = Serial1.read();
    delay(10);
  }
  if(i == 100) return 0;
  for(i = 0; ch >= '0' && ch <= '9' && i < 4; i++){
    delay(1);
    temp[i] = ch;
    ch = Serial1.read();
  }
  if(i == 4) return 0;
  i--;
  for(tens = 1, ch = 0; i >= 0; i--, tens *= 10){
    ch += temp[i] * tens;
  }
  return ch;
}

bool check_network(){
  uint8_t i;
  while(Serial1.available()) Serial1.read();
  Serial1.print("AT+COPS?\r");
  delay(100);
  i = Serial1.available();
  #if serial_response
  ShowSerialData();
  #else
  while(Serial1.available()) Serial1.read();
  #endif
  if(i < 30) return 0;
  else return 1;
}

void GSM_module_sleep(){
  while(Serial1.available()) Serial1.read();
  Serial1.print("AT+QSCLK=1\r");
  #if serial_response == true
  ShowSerialData();
  #endif
  delay(1000);
}
void GSM_module_wake(){
  while(Serial1.available()) Serial1.read();
  digitalWrite(GSM_module_DTR_pin, HIGH);
  delay(1000);
  Serial1.print("AT+QSCLK=0\r");
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
  Serial.println("Talk");
  while(1){
    if(Serial1.available()){ Serial.write(Serial1.read()); }
    if(Serial.available()){ Serial1.write(Serial.read()); }
  }
}

void ShowSerialData(){
  while(Serial1.available() != 0) Serial.write(Serial1.read());
}
