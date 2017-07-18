#include <SoftwareSerial.h>
SoftwareSerial mySerial(8, 7);

#define buttonPin 2
 
void setup(){
  mySerial.begin(19200);               // the GPRS baud rate   
  Serial.begin(19200);    // the GPRS baud rate 
  delay(500);
  pinMode(buttonPin, INPUT);
}
 
void loop(){
  int buttonState = 0; 
  buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) {
    SendTextMessage("919999124846", "2541d938b0a58946090d7abdde0d3890");
    delay(5000);
  }
}
 
void SendTextMessage(String number, String message){
  mySerial.print("AT+CMGF=1\r");    //Because we want to send the SMS in text mode
  delay(10);
  mySerial.println("AT+CMGS=\"+" + number + "\""); //send sms message, be careful need to add a country code before the cellphone number
  delay(10);
  mySerial.println(message);//the content of the message
  delay(10);
  mySerial.println((char)26);//the ASCII code of the ctrl+z is 26
  delay(10);
  mySerial.println();
}

void ShowSerialData(){
  while(mySerial.available()) Serial.write(mySerial.read());
}
