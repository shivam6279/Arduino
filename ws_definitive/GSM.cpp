#include "GSM.h"
#include "settings.h"

bool InitGSM() {
  bool flag = true;

  #ifdef GSM_PWRKEY_PIN
  if(!IsGSMModuleOn()) GSMModuleRestart();
  #endif
  
  GSMModuleWake();

  if(SendATCommand("AT+QSCLK=2", "OK", 1000) < 0) 
    flag = false;
  if(SendATCommand("AT+CMGF=1", "OK", 1000) < 0) 
    flag = false;
  if(SendATCommand("AT+CNMI=2,2,0,0,0", "OK", 1000) < 0) 
    flag = false;
  if(SendATCommand("AT+CLIP=1", "OK", 3000) < 0) 
    flag = false;

  ShowSerialData();
  delay(1000); 
  return flag;
}

int SendATCommand(char* cmd, char* resp, long int timeout) {
  int stage;
  char ch;

  int error_stage;
  char error[] = {'E', 'R', 'R', 'O', 'R', '\0'};
  ShowSerialData();
  Serial1.print(cmd);
  Serial1.print('\r');
  for(stage = 0, error_stage = 0; timeout > 0; timeout--) {
    delay(1);
    if(Serial1.available()) {
      ch = Serial1.read();
      #if SERIAL_RESPONSE
        Serial.print(ch);
      #endif
      if(ch == *(resp + stage)) {
        stage++;
        if(*(resp + stage) == '\0')
          return 1;
      }
      else
        stage = 0;
      if(ch == error[error_stage]) {
        stage++;
        if(resp[stage] == '\0')
          return 0;
      }
      else
        stage = 0;
    }
  }
  return -1;
}

bool ReadUntil(char* resp, long int timeout) {
  int stage;
  char ch;
  
  for(stage = 0; timeout > 0; timeout--) {
    delay(1);
    if(Serial1.available()) {
      ch = Serial1.read();
      #if SERIAL_RESPONSE
        Serial.print(ch);
      #endif
      if(ch == *(resp + stage)) {
        stage++;
        if(*(resp + stage) == '\0')
          return 1;
      }
      else
        stage = 0;
    }
  }
  return false;
}

bool CheckSMS() {
  if(Serial1.available() > 3) {
    if(Serial1.read() == '+') {
      if(Serial1.read() == 'C') {
        if(Serial1.read() == 'M') {
          if(Serial1.read() == 'T') {
            return 1;
          }
        }
      }
    }
  }
  return 0;
}

bool GetSMS(char n[], char b[]) {
  char ch, i;
  uint16_t timeout;
  ch = Serial1.read();
  while(ch != '"') {
  	delay(1); 
  	ch = Serial1.read(); 
  }
  i = 0;
  timeout = 0;
  while(1) {
    delay(1);
    if(timeout++ > 1000) break;
    ch = Serial1.read();
    if(ch == '"') break;
    else n[i++] = ch;
  }
  if(timeout > 1000) {
    b[0] = '\0';
    ShowSerialData();
    return false;
  }
  n[i] = '\0';
  timeout = 0;
  while(ch != '\n') {
  	delay(1); 
    if(timeout++ > 1000) break;
  	ch = Serial1.read(); 
  }
  if(timeout > 1000) {
    b[0] = '\0';
    ShowSerialData();
    return false;
  }
  i = 0;
  timeout = 0;
  while(1) {
    delay(1);
    if(timeout++ > 1000) break;
    ch = Serial1.read();
    if(ch == '\n' || ch == '\r') break;
    else b[i++] = ch;
  }
  if(timeout > 1000) {
    b[0] = '\0';
    ShowSerialData();
    return false;
  }
  b[i] = '\0';
  return true;
}

void SendSMS(char *n, char *b) {
  Serial1.print("AT+CMGS=\"" + String(n) + "\"\n");
  delay(100);
  Serial1.println(b);
  delay(100);
  Serial1.write(26);
  Serial1.write('\n');
  delay(100);
  Serial1.write('\n');
  delay(100);
  ShowSerialData();
}

bool CheckNetwork() {
  uint8_t i;
  while(Serial1.available()) Serial1.read();
  Serial1.print("AT+COPS?\r");
  delay(100);
  i = Serial1.available();
  ShowSerialData();
  if(i < 30) return false;
  else return true;
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

#ifdef GSM_PWRKEY_PIN
void GSMModuleRestart() {
  bool flag = 0;

  #if SERIAL_OUTPUT
    Serial.println("Restarting GSM Module");
  #endif

  GSMModuleWake();

  if(IsGSMModuleOn()) {
    #if SERIAL_OUTPUT
    Serial.println("GSM Module on, turning off");
    #endif
    SendATCommand("AT+QPOWD=1", "POWER DOWN", 10000);
    ShowSerialData();
  } 
  #if SERIAL_OUTPUT
    Serial.println("Starting the GSM Module");
  #endif
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
  GSMModuleWake();

  if(SendATCommand("AT", "OK", 500) < 1) {
    return false;
  }
  return true;
}

void GSMModuleWake() {
  Serial1.print("AT\r");
  delay(100);
  Serial1.print("AT\r");
  delay(100);
  while(Serial1.available()) Serial1.read();
}


void Talk() {
  GSMModuleRestart();
  Serial.println("Talk");
  while(1){
    if(Serial1.available()){ Serial.write(Serial1.read()); }
    if(Serial.available()){ Serial1.write(Serial.read()); }
  }
}

void ShowSerialData() {
  #if SERIAL_RESPONSE
    while(Serial1.available() != 0) Serial.write(Serial1.read());
  #else
    while(Serial1.available() != 0) Serial1.read();
  #endif
}
