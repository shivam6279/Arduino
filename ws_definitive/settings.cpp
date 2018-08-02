#include "settings.h"
#include "Arduino.h"

String URL = "http://enigmatic-caverns-27645.herokuapp.com/maytheforcebewithyou?";
String OTA_URL = "http://www.yobi.tech/ota/4";
String TIME_URL = "http://www.yobi.tech/IST";
String CHECK_ID_URL = "http://www.yobi.tech/check-id";
String CREATE_ID_URL = "http://www.yobi.tech/create-id?";

String SD_csv_header = "success,tf,id,ts,t1,t2,h,w,r,p,a,s,cv,bv,sg";

String SERVER_PHONE_NUMBER = "+919220592205";

bool SERIAL_OUTPUT = true;
bool SERIAL_RESPONSE = true;
