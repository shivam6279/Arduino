#ifndef SMS_h
#define SMS_h

#include "Arduino.h"

struct SMS_node{
  char number[14];
  char body[100];
  SMS_node *next;
};

class SMS{
public:
  SMS_node *head;
  int current_size;

  SMS() {
    head = NULL;
    current_size = 0;
  }
  void Add(char*, char*);
  bool DeleteIndex(int);
  void Print();
};

extern SMS sms;

extern bool CheckOtaSMS(char*);
extern void SendSMS(char*, char*);
extern uint8_t CheckSMS();

#endif
