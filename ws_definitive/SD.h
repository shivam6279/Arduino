#ifndef SD_h
#define SD_h

#include <SdFat.h>
#include "weatherData.h"

extern SdFat sd;
extern SdFile datalog, data_temp;

extern bool CheckOTA();
extern bool DownloadHex();
extern bool SDHexToBin();
extern bool WriteSD(weatherData);
extern long int GetPreviousFailedUploads(long int &n);
extern bool UploadCSV();
extern bool WriteOldTime(int, realTime);
extern void CharToInt(unsigned char&);

#endif
