#ifndef settings_h
#define settings_h

#define rain_led 35
#define wind_led 41
#define upload_led 12
#define rain_pin 4
#define wind_pin A7
#define GSM_module_DTR_pin 24

//#define metal_sensor_pin 26
//#define solar_radiation_pin A3
//#define battery_pin A15
//#define batteryInput A14
//#define voltageInput A12

#define rain1_pin 14
#define rain2_pin 15
#define rain3_pin 25
#define rain4_pin 26
#define rain5_pin A12
#define rain6_pin 50
#define rain7_pin 51
#define rain8_pin 52

//------------------------------------Settings-------------------------------------  
String phone_number = "+919220592205";
String backup_id = "1001"; //Backup id in case there is no sd card
#define HT_mode 1// 0 for SHT21, 1 for DST, 2 for none
       
#define data_upload_frequency 60//Minutes -- should be a multiple of the read frequency
#define data_read_frequency 60//Minutes
#define number_of_readings (data_upload_frequency / data_read_frequency)

#define enable_GPS false//True to enable GPS   
#define enable_BMP180 false //True to enable BMP180

#define serial_output true//True to debug: display values through the serial port
#define serial_response true//True to see SIM900 serial response
//----------------------------------------------------------------------------------

#endif
