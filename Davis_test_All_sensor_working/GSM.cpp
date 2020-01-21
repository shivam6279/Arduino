#include "GSM.h"
#include "settings.h"
#include "weatherData.h"

int SendATCommand(char* cmd, char* resp, unsigned long timeout) {
  char ch;
  char *temp;

  unsigned long t;

  int error1_stage;
  char error1[] = "ERROR";

  int error2_stage;
  char error2[] = "FAIL";
  
  ShowSerialData();
  Serial1.print(String(cmd) + '\r');
  for(temp = resp, t = millis(), error1_stage = 0, error2_stage = 0; (millis() - t) < timeout;) {
    if(Serial1.available()) {
      ch = Serial1.read();
      if(SERIAL_RESPONSE) {
        Serial.print(ch);
      }
      if(ch == *resp) {
        resp++;
        if(*resp == '\0')
          return 1;
      }
      else
        resp = temp;
      if(ch == error1[error1_stage]) {
        error1_stage++;
        if(error1[error1_stage] == '\0')
          return 0;
      }
      else
        error1_stage = 0;
      if(ch == error2[error2_stage]) {
        error2_stage++;
        if(error2[error2_stage] == '\0')
          return 0;
      }
      else
        error2_stage = 0;
    }
  }
  return -1;
}

bool GSMReadUntil(char* resp, unsigned long timeout) {
  char ch;
  char *temp;

  unsigned long t;
  
  for(temp = resp, t = millis(); (millis() - t) < timeout;) {
    if(Serial1.available()) {
      ch = Serial1.read();
      if(SERIAL_RESPONSE) {
        Serial.print(ch);
      }
      if(ch == *resp) {
        resp++;
        if(*resp == '\0')
          return 1;
      }
      else
        resp = temp;
    }
  }
  return false;
}

bool InitGSM() {
  bool flag = true;

  #ifdef GSM_PWRKEY_PIN
    GSMModuleRestart();
  #endif
  
  GSMModuleWake();

  if(SendATCommand("AT+QSCLK=2", "OK", 1000) < 1) 
    flag = false;
  if(SendATCommand("AT+CMGF=1", "OK", 1000) < 1) 
    flag = false;
  if(SendATCommand("AT+CNMI=1,0,0,0,0", "OK", 1000) < 1) 
    flag = false;
  //if(SendATCommand("AT+QMGDA=\"DEL ALL\"", "OK", 3000) < 1) 
    //flag = false;
  if(SendATCommand("AT+CLIP=1", "OK", 3000) < 1) 
    flag = false;

  delay(50);
  ShowSerialData();
  delay(1000); 
  return flag;
}

bool CheckOtaSMS(char *number) {
  realTime t;
  int i;
  char ch;
  bool ota_flag = false, delete_flag = false, number_flag = false;
  long int timeout;
  char str[5];

  bool t_response = SERIAL_RESPONSE;

  SERIAL_RESPONSE = 0;

  GSMModuleWake();
  if(SendATCommand("AT+CMGL=\"ALL\"", "+", 1000) < 1) {
    SERIAL_RESPONSE = t_response;
    return false;
  }

  for(timeout = 0; timeout < 5000; timeout++) {
    if(GSMReadUntil("CMGL: ", 500) < 1) 
      break;
    delete_flag = true;
    if(GSMReadUntil(",", 100) < 1) 
        break;
    if(GSMReadUntil(",", 100) < 1) 
      break;

    if(number_flag == false) {
      do{
        ch = Serial1.read();
        if(isDigit(ch) || ch == '+')
          *(number++) = ch;
      }while(ch != ',');
      *number = '\0';
    } else {
      if(GSMReadUntil(",", 100) < 1) 
        break;
    }
    if(GSMReadUntil(",", 100) < 1) 
      break;

    i = 0;
    do{
      ch = Serial1.read();
      if(isDigit(ch))
        str[i++] = ch;
    }while(ch != '/');
    str[i] = '\0';
    t.year = (String(str)).toInt();
    if(i == 2)
      t.year += 2000;

    i = 0;
    do{
      ch = Serial1.read();
      if(isDigit(ch))
        str[i++] = ch;
    }while(ch != '/');
    str[i] = '\0';
    t.month = (String(str)).toInt();
      
    i = 0;
    do{
      ch = Serial1.read();
      if(isDigit(ch))
        str[i++] = ch;
    }while(ch != ' ');
    str[i] = '\0';
    t.day = (String(str)).toInt();

    i = 0;
    do{
      ch = Serial1.read();
      if(isDigit(ch))
        str[i++] = ch;
    }while(ch != ':');
    str[i] = '\0';
    t.hours = (String(str)).toInt();

    i = 0;
    do{
      ch = Serial1.read();
      if(isDigit(ch))
        str[i++] = ch;
    }while(ch != ':');
    str[i] = '\0';
    t.minutes = (String(str)).toInt();

    i = 0;
    do{
      ch = Serial1.read();
      if(isDigit(ch))
        str[i++] = ch;
    }while(ch != '+');
    str[i] = '\0';
    t.seconds = (String(str)).toInt();

    if(GSMReadUntil("\n", 100) < 1) 
      break;
    
    ch = Serial1.read();
    if(ch == 'O' || ch == 'o') {
      ch = Serial1.read();
      if(ch == 'T' || ch == 't') {
        ch = Serial1.read();
        if(ch == 'A' || ch == 'a') {
          ota_flag = true;
          number_flag = true;
        }
      }
    }
    if(GSMReadUntil("\n", 100) < 1) 
      break;
  }
  if(delete_flag) 
    SendATCommand("AT+QMGDA=\"DEL ALL\"", "OK", 3000);

  SERIAL_RESPONSE = t_response;

  if(timeout > 5000) 
    return false;

  return ota_flag;
}

void SendSMS(char *n, char *b) {
  bool t = SERIAL_RESPONSE;

  GSMModuleWake();
  
  SERIAL_RESPONSE = 1;
  Serial1.print("AT+CMGS=\"" + String(n) + "\"\r\n");
  delay(100);
  Serial1.println(b);
  delay(100);
  Serial1.write((char)26);
  Serial1.write('\n');
  delay(100);
  Serial1.write('\n');
  delay(100);
  ShowSerialData();
  SERIAL_RESPONSE = t;
}

bool CheckNetwork() {
  bool t_response = SERIAL_RESPONSE;
  SERIAL_RESPONSE = 0;
  
  SendATCommand("AT+QIDNSIP=1", "OK", 1000);
  ShowSerialData();
  SendATCommand("AT+QICLOSE", "OK", 1000);
  ShowSerialData();
  if(SendATCommand("AT+QIOPEN=\"TCP\",\"www.yobi.tech\",\"80\"", "CONNECT OK", 20000) == 1) {
    SERIAL_RESPONSE = t_response;
    return true;
  }
  SERIAL_RESPONSE = t_response;
  return false;
}

int GetSignalStrength() {
  char ch, temp[3];
  int i = 0, tens, ret;
  GSMModuleWake();
  Serial1.print("AT+CSQ\r");
  delay(200);
  do {
    ch = Serial1.read();
    delay(1);
    i++;
  }while((ch < 48 || ch > 57) && i < 1000);
  if(i == 1000) return 0;
  for(i = 0; ch >= 48 && ch <= 57 && i < 4; i++) {
    delay(1);
    temp[i] = ch;
    ch = Serial1.read();
  }
  if(i == 4) return 0;
  i--;
  for(tens = 1, ret = 0; i >= 0; i--, tens *= 10) {
    ret += (temp[i] - 48) * tens;
  }
  while(Serial1.available()) Serial1.read();
  return ret;
}

bool GetGSMLoc(float &lat, float &lon) {
  char lat_str[20], lon_str[20];
  char ch;
  int i;
  if(SendATCommand("AT+QGSMLOC", "+QGSMLOC:", 10000) < 1)
    return false;

  GSMReadUntil(",", 100);
  delay(50);

  for(i = 0, ch = Serial1.read(); isDigit(ch) || ch == '.' || ch == '-' || ch == '+';) {
    lon_str[i++] = ch;
    ch = Serial1.read();
  }
  lon_str[i] = '\0';
  lon = String(lon_str).toFloat(); 

  for(i = 0, ch = Serial1.read(); isDigit(ch) || ch == '.' || ch == '-' || ch == '+';) {
    lat_str[i++] = ch;
    ch = Serial1.read();
  }
  lat_str[i] = '\0';
  lat = String(lat_str).toFloat(); 

  GSMReadUntil("OK", 100);
  GSMReadUntil("\n", 100);

  return true;
}

#ifdef GSM_PWRKEY_PIN
void GSMModuleRestart() {
  bool flag = 0;

  if(SERIAL_OUTPUT) {
    Serial.println("Restarting GSM Module");
  }

  GSMModuleWake();

  if(IsGSMModuleOn()) {
    if(SERIAL_OUTPUT) {
      Serial.println("GSM Module on, turning off");
    }
    SendATCommand("AT+QPOWD=1", "POWER DOWN", 10000);
    GSMReadUntil("\n", 500);
    ShowSerialData();
  } 
  if(SERIAL_OUTPUT) {
    Serial.println("Starting the GSM Module");
  }
  digitalWrite(GSM_PWRKEY_PIN, HIGH);
  delay(2000);
  digitalWrite(GSM_PWRKEY_PIN, LOW);
  delay(5000);
  while(Serial1.available()) Serial1.read();
  if(!IsGSMModuleOn()) {
    digitalWrite(GSM_PWRKEY_PIN, HIGH);
    delay(2000);
    digitalWrite(GSM_PWRKEY_PIN, LOW);
    delay(5000);
    while(Serial1.available()) Serial1.read();
  }
}
#endif

bool IsGSMModuleOn() {
  bool t_response = SERIAL_RESPONSE;
  SERIAL_RESPONSE = 0;
  GSMModuleWake();
  if(SendATCommand("AT", "OK", 500) < 1) {
    SERIAL_RESPONSE = t_response;
    return false;
  }
  SERIAL_RESPONSE = t_response;
  return true;
}

void GSMModuleWake() {
  bool t_response = SERIAL_RESPONSE;
  SERIAL_RESPONSE = 0;
  if(SendATCommand("AT", "OK", 100) == 1) {
    SERIAL_RESPONSE = t_response;
    ShowSerialData();
    return;
  }
  SendATCommand("AT", "OK", 100);
  ShowSerialData();
  SERIAL_RESPONSE = t_response;
}


void Talk() {
  char ch;
  SERIAL_RESPONSE = 1;
  #ifdef GSM_PWRKEY_PIN
    if(!IsGSMModuleOn()) 
      GSMModuleRestart();
  #endif
  while(Serial.available()) 
    Serial.read();
  Serial.println("Talk");
  while(1){
    if(Serial1.available()){ Serial.write(Serial1.read()); }
    if(Serial.available()) { 
      ch = Serial.read();
      if(ch == '|') {
        while(Serial.available()) 
          Serial.read();
        Serial1.write(0x1A);
      }
      else if(ch == '~') {
        while(Serial.available()) 
          Serial.read();
        Serial.println("End");
        return;
      }
      else {
        Serial1.write(ch); 
      }
    }
  }
}

void ShowSerialData() {
  if(SERIAL_RESPONSE) {
    while(Serial1.available() != 0) Serial.write(Serial1.read());
  } else {
    while(Serial1.available() != 0) Serial1.read();
  }
}
