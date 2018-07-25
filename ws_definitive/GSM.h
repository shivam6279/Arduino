#ifndef GSM_h
#define GSM_h

extern bool InitGSM();
extern int SendATCommand(char*, char*, long int);
extern bool ReadUntil(char*, long int);
extern int GetSignalStrength();
extern bool CheckSMS();
extern bool GetSMS(char*, char*);
extern void SendSMS(char*, char*);
extern bool CheckNetwork();
extern void GSMModuleRestart();
extern bool IsGSMModuleOn();
extern void GSMModuleWake();
extern void Talk();
extern void ShowSerialData();

#endif
