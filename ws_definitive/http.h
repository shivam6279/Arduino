#ifndef http_h
#define http_h

#include "weatherData.h"

extern bool GetID(int &id);
extern bool HttpInit();
extern bool UploadWeatherData(weatherData[], uint8_t, realTime&);
extern bool SendWeatherURL(weatherData);
extern bool SendTestPacket(int, realTime &wt);
extern bool GetTime(realTime&);
extern bool ReadTime(realTime&);
extern bool SendIdSMS(int, String);
extern void UploadSMS();

#endif
