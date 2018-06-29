#ifndef http_h
#define http_h

#include "weatherData.h"

bool SubmitHttpRequest(weatherData, wtime&);
bool GetTime(wtime&);
bool ReadTime(wtime&);
void UploadSMS();

#endif
