#include "settings.h"
#include "Arduino.h"

String URL = "http://enigmatic-caverns-27645.herokuapp.com/maytheforcebewithyou?";
String OTA_URL = "http://www.yobi.tech/ota/4";
String TIME_URL = "http://www.yobi.tech/IST";
String CHECK_ID_URL = "http://www.yobi.tech/check-id";
String CREATE_ID_URL = "http://www.yobi.tech/create-id?";

String SD_csv_header = "success,tf,id,ts,t1,t2,h,w,r,p,a,s,cv,bv,sg";

String PHONE_NUMBER = "+919220592205";
String BACKUP_ID = "201"; //Backup id in case there is no sd card

bool SERIAL_OUTPUT = true;
bool SERIAL_RESPONSE = true;
