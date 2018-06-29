#ifndef SD_h
#define SD_h

#include <SdFat.h>
#include "weatherData.h"

const SdFat sd;
const SdFile datalog, data_temp;

extern bool DownloadHex();
extern bool SDHexToBin();
extern bool WriteSD(weatherData, wtime);
extern void CharToInt(unsigned char&);

#endif
