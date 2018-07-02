#include "GSM.h"
#include "settings.h"

void InitGSM() {
  bool flag = 0;
  while(Serial1.available()) Serial1.read();
  Serial1.print("AT\r"); 
  delay(100);
  while(Serial1.available()) Serial1.read();
  Serial1.print("AT\r"); 
  delay(100);
  while(Serial1.available()) {
    if(Serial1.read() == 'O') {
      flag = 1;
      break;
    }
  }
  while(Serial1.available()) Serial1.read();
  if(flag == 1) {
    #if SERIAL_OUTPUT
      Serial.println("GSM Module on, turning off");
    #endif
    Serial1.print("AT+QPOWD=1\r"); 
    delay(4000);
    ShowSerialData();
  }
  #if SERIAL_OUTPUT
  Serial.println("Starting the GSM Module");
  #endif
  digitalWrite(GSM_PWRKEY_PIN, HIGH);
  delay(2000);
  digitalWrite(GSM_PWRKEY_PIN, LOW);

  delay(5000);
  
  Serial1.print("AT+QSCLK=2\r"); 
  delay(100);
  ShowSerialData();
  Serial1.print("AT+CMGF=1\r"); 
  delay(100);
  ShowSerialData();
  Serial1.print("AT+CNMI=2,2,0,0,0\r"); 
  delay(100); 
  ShowSerialData();
  Serial1.print("AT+CLIP=1\r"); 
  delay(100);
  ShowSerialData();
  delay(1000); 
  GSMModuleSleep();
}

int GetSignalStrength() {
  char ch, temp[3];
  int i = 0, tens, ret;
  while(Serial1.available()) Serial1.read();
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

bool CheckSMS(){
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

void GetSMS(char n[], char b[]) {
  char ch, i;
  ch = Serial1.read();
  while(ch != '"') {
  	delay(1); 
  	ch = Serial1.read(); 
  }
  i = 0;
  while(1) {
    delay(1);
    ch = Serial1.read();
    if(ch == '"') break;
    else n[i++] = ch;
  }
  n[i] = '\0';
  while(ch != '\n') {
  	delay(1); 
  	ch = Serial1.read(); 
  }
  i = 0;
  while(1) {
    delay(1);
    ch = Serial1.read();
    if(ch == '\n' || ch == '\r') break;
    else b[i++] = ch;
  }
  b[i] = '\0';
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
  while(Serial1.available()) Serial1.read();
}

bool CheckNetwork() {
  uint8_t i;
  while(Serial1.available()) Serial1.read();
  Serial1.print("AT+COPS?\r");
  delay(100);
  i = Serial1.available();
  ShowSerialData();
  if(i < 30) return 0;
  else return 1;
}

void GSMModuleRestart() {
  bool flag = 0;

  #if SERIAL_OUTPUT
  Serial.println("Restarting GSM Module");
  #endif

  while(Serial1.available()) Serial1.read();
  Serial1.print("AT\r"); 
  delay(100);
  while(Serial1.available()) Serial1.read();
  Serial1.print("AT\r"); 
  delay(100);

  while(Serial1.available()) {
    if(Serial1.read() == 'O') {
      flag = 1;
      break;
    }
  }
  while(Serial1.available()) Serial1.read();

  if(flag == 1) {
    #if SERIAL_OUTPUT
    Serial.println("GSM Module on, turning off");
    #endif
    Serial1.print("AT+QPOWD=1\r"); 
    delay(4000);
    #if SERIAL_RESPONSE
    ShowSerialData();
    #endif
    #if SERIAL_OUTPUT
    Serial.println("Turning on");
    #endif
  } 
  #if SERIAL_OUTPUT
  Serial.println("STarting the GSM Module");
  #endif
  digitalWrite(GSM_PWRKEY_PIN, HIGH);
  delay(2000);
  digitalWrite(GSM_PWRKEY_PIN, LOW);
  delay(5000);
}

void GSMModuleSleep() {
  while(Serial1.available()) Serial1.read();
  Serial1.print("AT+QSCLK=1\r");
  #if SERIAL_RESPONSE == true
  ShowSerialData();
  #endif
  delay(1000);
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
