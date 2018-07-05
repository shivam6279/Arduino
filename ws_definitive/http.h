#ifndef http_h
#define http_h

#include "weatherData.h"

extern bool SendHttpGet();
extern bool HttpInit();
extern bool UploadWeatherData(weatherData[], uint8_t, real_time&);
extern bool SendWeatherURL(weatherData);
extern bool GetTime(real_time&);
extern bool ReadTime(real_time&);
extern void UploadSMS();

#endif
