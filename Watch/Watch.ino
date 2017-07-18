#include <SoftwareSerial.h>
#include <SPI.h>
#include "SH1106_SPI.h"

SoftwareSerial SIM900(8, 7);

SH1106_SPI lcd;

int year, month, day, hour, minute, second, frequency = 10, timer = 0, minute_counter = 16;
char admin_number[14];
float latitude, longitude;
uint8_t GPS_wait;
boolean flag = false;

String id = "110";
 
void setup(){
  pinMode(A1, OUTPUT);
  digitalWrite(A1, LOW);
  SIM900.begin(19200);    
  Serial.begin(19200);  
  //talk();
  lcd.begin();
  lcd.gotoXY(0,1);
  lcd.print("Booting");
  delay(10000);
  SIM900.print("AT+CMGF=1\r");
  delay(100);
  ShowSerialData();
  SIM900.print("AT+CNMI=1,2,0,0,0\r");
  delay(100);
  ShowSerialData();
  //while(SIM900.available()) SIM900.read(); 
  
  lcd.gotoXY(0,1);
  lcd.print("Done    ");
  delay(1000);
  lcd.clear();
  Serial.println("Started");

  noInterrupts();//stop interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A = 62500;
  TCCR1B |= (1 << WGM12);
  TCCR1B |= 5;
  TCCR1B &= 253;  
  TIMSK1 |= (1 << OCIE1A);
  interrupts();  
}

ISR(TIMER1_COMPA_vect){ //Interrupt gets called every 4 seconds 
  timer++;
  if(GPS_wait < 2) GPS_wait++;
  if(timer >= frequency * 15){
    flag = true;
    timer = 0;
  }
  if(minute_counter < 15) minute_counter++;
}
 
void loop(){
  int i, c;
  char ch, str_buffer[30];
  char number[14], number_temp[14], body_r[100], body_s[100];
  //strcpy(admin_number, "+919654315871");
  strcpy(admin_number, "+919650484721");
  admin_number[13] = '\0';
  while(1){
    if(minute_counter == 15){
      lcd.clear();
      minute_counter = 16;
    }
    if(flag){
      flag = false;
      GetGPS();
      lcd.clear();
      lcd.gotoXY(0,2);
      lcd.print("Uploading...");
      lcd.gotoXY(0,3);
      lcd.print("Lat: " );
      lcd.print(latitude);
      lcd.gotoXY(0,4);
      lcd.print("Lon: " );
      lcd.print(longitude);
      SubmitHttpRequest();
      lcd.clear();
    }
    if(check_sms()){
      get_sms(number, body_r);
      if(strcmp(number, admin_number) == 0){
        minute_counter = 0;
        //GetGPS();
        if((body_r[0] == 'F' || body_r[0] == 'f') && body_r[1] == '='){
          frequency = (body_r[2] - 48) * 10 + (body_r[3] - 48);
          lcd.gotoXY(0,2);
          lcd.print("Frequency changed to");
          lcd.gotoXY(0,3);
          lcd.print(frequency);
          lcd.print(" minutes");
        }
    
        else if((body_r[0] == 'C' || body_r[0] == 'c') && body_r[1] == '='){
          number_temp[0] = '+';
          number_temp[1] = '9';
          number_temp[2] = '1';
          if(body_r[2] == '+' && body_r[3] == '9' && body_r[4] == '1'){
            for(i = 0; i < 10; i++) number_temp[i + 3] = body_r[i + 5];
          }
          else if(body_r[2] == '9' && body_r[3] == '1'){
            for(i = 0; i < 10; i++) number_temp[i + 3] = body_r[i + 4];
          }
          else{
            for(i = 0; i < 10; i++) number_temp[i + 3] = body_r[i + 2];
          }
          number_temp[13] = '\0';
          strcpy(body_s, "Are you sure you want to change the admin phone number to ");
          strcat(body_s, number_temp);
          strcat(body_s, ". Reply with 'Y'(yes) or 'N'.");
          send_sms(admin_number, body_s);
          c = timer;
          do{
            if(check_sms()){
              get_sms(number, body_r);
              if(strcmp(admin_number, number) == 0){
                if(body_r[0] == 'y' || body_r[0] == 'Y'){
                  strcpy(admin_number, number_temp);
                  lcd.gotoXY(0,2);
                  lcd.print("Phone number changed");
                  lcd.gotoXY(0,3);
                  lcd.print(admin_number);
                  strcpy(body_s, "Admin phone number changed.");
                  send_sms(number, body_s);
                  strcpy(body_s, "This is now the admin phone number.");
                  send_sms(admin_number, body_s);
                }
                break;
              }
            }
          }while(timer - c < 30);
        }
    
        else{
          lcd.gotoXY(0,2);
          lcd.print("Message received    ");
          lcd.gotoXY(0,3);
          lcd.print(body_r);
          strcpy(body_s, "Location: ");
          String(latitude).toCharArray(str_buffer, 30);
          strcat(body_s, str_buffer);
          strcat(body_s, ", ");
          String(longitude).toCharArray(str_buffer, 30);
          strcat(body_s, str_buffer);
          send_sms(admin_number, body_s);
        }
        
        //SubmitHttpRequest();
    
        //Display Data
        Serial.println("Message Received");
        Serial.print("From: ");
        Serial.println(admin_number);
        Serial.print("Body: ");
        Serial.println(body_r);
        Serial.println("Location: " + String(latitude) + ", " + String(longitude));
        Serial.println();
      }
    }
  }
}

boolean check_sms(){
  if(SIM900.available()){
    if(SIM900.read() == '+'){
      delay(1);
      if(SIM900.read() == 'C'){
        delay(1);
        if(SIM900.read() == 'M'){
          delay(1);
          if(SIM900.read() == 'T'){
            return 1;
          }
        }
      }
    }
  }  
  return 0;
}

void get_sms(char n[], char b[]){
  char ch, i;
  ch = SIM900.read();
  while(ch != '"'){
    delay(1);
    ch = SIM900.read();
  }
  i = 0;
  while(1){
    delay(1);
    ch = SIM900.read();
    if(ch == '"') break;
    else{
      n[i++] = ch;
    }
  }
  n[i] = '\0';
  while(ch != '\n'){
    delay(1);
    ch = SIM900.read();
  }
  i = 0;
  while(1){
    delay(1);
    ch = SIM900.read();
    if(ch == '\n' || ch == '\r') break;
    else{
      b[i++] = ch;
    }
  }
  b[i] = '\0';
}

void send_sms(char *n, char *b){
  SIM900.print("AT+CMGS=\"" + String(n) + "\"\n");
  delay(100);
  SIM900.println(b);
  delay(100);
  SIM900.write(26);
  SIM900.write('\n');
  delay(100);
  SIM900.write('\n');
}

bool SubmitHttpRequest(){
  char str[60];
  uint16_t i, j;
  
  #if serial_output
  Serial.println("Uploading data");
  #endif
  while(SIM900.available()) SIM900.read();
  SIM900.println("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r");//setting the SAPBR, the connection type is using gprs
  delay(100);
  #if serial_response
  ShowSerialData();
  #endif
  SIM900.println("AT+SAPBR=3,1,\"APN\",\"www\"\r");//setting the APN, the second need you fill in your local apn server
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
  SIM900.print("1=<"); 
  SIM900.print("'id':" + id + ",");
  SIM900.print("'t1':" + String(frequency) + ",");
  SIM900.print("'t2':0,");
  SIM900.print("'h':0,");
  SIM900.print("'w':0,");
  SIM900.print("'r':0,");
  SIM900.print("'p':0,");
  SIM900.print("'s':0,");
  SIM900.print("'lt':0,");
  SIM900.print("'ln':0,");
  SIM900.print("'sg':0>");
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
    return false;
  }
  
  j = 100 * (str[40] - 48); j += 10 * (str[41] - 48); j += (str[42] - 48);
  if(j == 200) return true;
  else return false;
}

void GetGPS(){
  uint8_t c, k;
  char ch, valid, lat[16], lon[16], temp_time[6], temp_date[6];
  GPS_wait = 0;
  do{ ch = Serial.read();
    if(ch == '$'){ ch = Serial.read();
      if(ch == 'G'){ ch = Serial.read();
        if(ch == 'P'){ ch = Serial.read();
          if(ch == 'R'){ ch = Serial.read();
            if(ch == 'M'){ ch = Serial.read();
              if(ch == 'C'){ ch = Serial.read();
                for(c = 0, k = 0; k < 9;){
                  ch = Serial.read();
                  if(k == 1 && ch != ',') valid = ch;
                  if(k == 0 && ch != ',' && c < 6) temp_time[c++] = ch;
                  else if(k > 1 && k < 4) lat[c++] = ch;
                  else if(k > 3 && k < 6) lon[c++] = ch;
                  else if(k == 8 && ch != ',' && c < 6) temp_date[c++] = ch;
                  if(ch == ','){
                    if(k == 4) lat[c - 1] = '\0';
                    if(k == 6) lon[c - 1] = '\0';
                    k++;
                    c = 0;
                  }
                } 
                if(valid == 'V'){ latitude = 0.0; longitude = 0.0; }
                else{ latitude = GPS_StringToFloat(lat); longitude = GPS_StringToFloat(lon); }   
                hour = 10 * (temp_time[0] - 48) + (temp_time[1] - 48);
                minute = 10 * (temp_time[2] - 48) + (temp_time[3] - 48);
                second = 10 * (temp_time[4] - 48) + (temp_time[5] - 48);
                
                day = 10 * (temp_date[0] - 48) + (temp_date[1] - 48);
                month = 10 * (temp_date[2] - 48) + (temp_date[3] - 48);
                year = 10 * (temp_date[4] - 48) + (temp_date[5] - 48);
                
                minute += 30;
                if(minute >= 60){
                  minute -= 60;
                  hour++;
                }
                hour += 5;//IST = UTC + 5:30
                if(hour >= 24){
                  hour -= 60;
                  day++;
                }
                if(((month == 1 || month == 3 || month == 5 || month == 7 || month == 8 || month == 10 || month == 12) && day == 32) || ((month == 1 || month == 4 || month == 6 || month == 9 || month == 11) && day == 31) || (month == 2 && day == 29)){
                   day = 1;
                   if(++month == 13) year++;
                }
                return;
        }}}}}}  
  }while(GPS_wait != 2);
  latitude = 0.0;
  longitude = 0.0;
}

double GPS_StringToFloat(char *str){
  double temp = 0, temp_decimal = 0;
  int i, j;
  long int big;
  for(i = 0; *str != '.'; str++, i++);
  str -= 3; i -= 3;
  for(big = 1; i >= 0; i--, str--, big *= 10) temp += (*str - 48) * big;
  while(*str != '.') str++;
  str -= 2;
  for(i = 0; isdigit(*str) || *str == '.'; str++, i++);
  i--; j = i; str--;
  for(big = 1; i >= 0; i--, str--){ if(*str != '.'){ temp_decimal += (*str - 48) * big; big *= 10; } }
  temp_decimal = temp_decimal * 10 / 6; temp += (temp_decimal / pow_10(j));
  return temp;
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
    if(SIM900.available()) Serial.write(SIM900.read());
    if(Serial.available())SIM900.write(Serial.read());
  }
}

void ShowSerialData(){
  while(SIM900.available()) Serial.write(SIM900.read());
}
