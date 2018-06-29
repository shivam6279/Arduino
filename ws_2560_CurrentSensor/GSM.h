#ifndef GSM_h
#define GSM_h

void InitGSM();
int GetSignalStrength();
bool CheckSMS();
void GetSMS(char*, char*);
void SendSMS(char*, char*);
bool CheckNetwork();
void GSMModuleRestart();
void GSMModuleSleep();
void GSMModuleWake();
void Talk();
void ShowSerialData();

#endif
