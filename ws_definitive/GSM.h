#ifndef GSM_h
#define GSM_h

extern int SendATCommand(char*, char*, long int);
extern bool GSMReadUntil(char*, long int);
extern bool InitGSM();
extern bool CheckOtaSMS(char*);
extern bool GetSMS(char*, char*);
extern void SendSMS(char*, char*);
extern int GetSignalStrength();
extern bool CheckNetwork();
extern void GSMModuleRestart();
extern bool IsGSMModuleOn();
extern void GSMModuleWake();
extern void Talk();
extern void ShowSerialData();

#endif
