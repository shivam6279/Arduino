#include <Servo.h>

Servo ESC;     // create servo object to control the ESC

int potValue;  // value from the analog pin

void setup() {
  Serial.begin(115200);
  ESC.attach(9,1000,2000); // (pin, min pulse width, max pulse width in microseconds) 
  ESC.writeMicroseconds(2000);
  delay(3000);
  ESC.writeMicroseconds(1000);
  delay(3000);
}



void loop() {
  if(Serial.available()) {
    int i = Serial.parseInt();
    Serial.println(i);
    ESC.writeMicroseconds(i);
  }
//  ESC.writeMicroseconds(1250); // Send signal to ESC.
}
