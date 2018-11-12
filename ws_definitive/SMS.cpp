#include "SMS.h"
#include "Arduino.h"
#include "GSM.h"
#include "weatherData.h"
#include "settings.h"

SMS sms;

void SMS::Add(char *n, char *b) {
  SMS_node *temp = head;
  if(temp == NULL) 
    temp = new SMS_node;
  else {
    while(temp->next != NULL);
    temp->next = new SMS_node;
    temp = temp->next;
  }
  strcpy(temp->number, n);
  strcpy(temp->body, b);
  temp->next = NULL;
  current_size++;
}

bool SMS::DeleteIndex(int index) {
  if(index >= current_size)
    return 0;
  SMS_node *temp = head, *prev;
  for(int i = 0; i < index; i++) {
    prev = temp;
    temp = temp->next;
  }
  prev->next = temp->next;
  delete temp;
}

void SMS::Print() {
  SMS_node *temp = head;
  while(temp != NULL && current_size != 0) {
    Serial.println(temp->number);
    Serial.println(temp->body);
    temp = temp->next;
  }
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
  SERIAL_RESPONSE = 0;
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
  SERIAL_RESPONSE = t;
}

uint8_t CheckSMS() {
  unsigned long timeout;
  char ch;
  int i;
  char number[14], body[100];

  bool ota_flag = 0, id_flag = 0, test_flag = 0;

  bool t_response = SERIAL_RESPONSE;
  SERIAL_RESPONSE = 0;
  
  if(SendATCommand("AT+CMGL=\"REC UNREAD\"", "+", 1000) < 1) {
    SERIAL_RESPONSE = t_response;
    return false;
  }
  while(1) {
   if(GSMReadUntil("CMGL: ", 500) < 1) 
      break;
    if(GSMReadUntil(",", 100) < 1) 
        break;
    if(GSMReadUntil(",", 100) < 1) 
      break;

    i = 0;
    do{
      ch = Serial1.read();
      if(isDigit(ch) || ch == '+')
        number[i++] = ch;
    }while(ch != ',');
    number[i] = '\0';

    if(GSMReadUntil(",", 100) < 1) 
      break;

    i = 0;
    ch = 0;
    for(i = 0, timeout = 0; ch != '\n' && timeout < 500; i++) {
      ch = Serial1.read();
      body[i++] = ch;
      delay(1);
    }
    body[i] = '\0';

    if(timeout >= 500) 
      break;

    if(i >= 3) {
      if(toupper(body[0]) == 'O' && toupper(body[1]) == 'T' && toupper(body[2]) == 'A') {
        ota_flag = 1;
      }
    }
    if(i >= 9) {
      if(toupper(body[0]) == 'S' && toupper(body[1]) == 'E' && toupper(body[2]) == 'N' && toupper(body[3]) == 'D' && toupper(body[4]) == ' ' && toupper(body[5]) == 'T' && toupper(body[6]) == 'E' && toupper(body[7]) == 'S' && toupper(body[7]) == 'T') {
        test_flag = 1;
      }
    }
    if(i >= 6) {
      if(toupper(body[0]) == 'N' && toupper(body[1]) == 'E' && toupper(body[2]) == 'W' && toupper(body[3]) == ' ' && toupper(body[4]) == 'I' && toupper(body[5]) == 'D') {
        id_flag = 1;
      }
    }
    //sms.Add(number, body);
  }
  return (ota_flag << 2 | test_flag << 1 | id_flag);
}
