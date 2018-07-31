#ifndef http_h
#define http_h

#include "weatherData.h"

extern bool GetVersion(int &id);
extern bool HttpInit();
extern bool UploadWeatherData(weatherData[], uint8_t, realTime&);
extern bool SendWeatherURL(weatherData);
extern bool GetTime(realTime&);
extern bool ReadTime(realTime&);
extern void UploadSMS();

#endif
