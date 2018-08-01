#ifndef GSM_h
#define GSM_h

extern int SendATCommand(char*, char*, unsigned long);
extern bool GSMReadUntil(char*, unsigned long);
extern bool InitGSM();
extern bool CheckOtaSMS(char*);
extern bool GetSMS(char*, char*);
extern void SendSMS(char*, char*);
extern int GetSignalStrength();
extern bool CheckNetwork();
extern bool GetGSMLoc(float&, float&);
extern void GSMModuleRestart();
extern bool IsGSMModuleOn();
extern void GSMModuleWake();
extern void Talk();
extern void ShowSerialData();

#endif
