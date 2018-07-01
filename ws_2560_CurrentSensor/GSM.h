#ifndef GSM_h
#define GSM_h

extern void InitGSM();
extern int GetSignalStrength();
extern bool CheckSMS();
extern void GetSMS(char*, char*);
extern void SendSMS(char*, char*);
extern bool CheckNetwork();
extern void GSMModuleRestart();
extern void GSMModuleSleep();
extern void GSMModuleWake();
extern void Talk();
extern void ShowSerialData();

#endif
