#ifndef SD_h
#define SD_h

#include <SdFat.h>
#include "weatherData.h"

extern SdFat sd;
extern bool SD_connected;

extern bool OTA();
extern bool DownloadHex();
extern bool SDHexToBin();
extern bool WriteSD(weatherData);
extern long int GetPreviousFailedUploads(long int &n);
extern bool UploadCSV();
extern bool WriteOldTime(int, realTime);
extern bool WriteOldID(int, int);
extern uint8_t CharToInt(unsigned char&);

#endif
