#ifndef SD_h
#define SD_h

#include <SdFat.h>
#include "weatherData.h"

extern SdFat sd;

extern bool CheckOTA();
extern bool DownloadHex();
extern bool SDHexToBin();
extern bool WriteSD(weatherData);
extern bool WriteSD_old(weatherData);
extern bool WriteSD_NCML(weatherData);
extern bool ReadConfig();
extern long int GetPreviousFailedUploads(long int &n);
extern bool WriteFailedData(weatherData);
extern bool UploadFailedData();
extern bool UploadCSV();
extern bool WriteOldTime(int, realTime);
extern bool WriteOldID(int, int);
extern uint8_t CharToInt(unsigned char&);

#endif
