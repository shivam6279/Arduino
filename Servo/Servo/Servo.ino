#include <Servo.h>

Servo ESC;     // create servo object to control the ESC
Servo brakes;     // create servo object to control the ESC

int potValue;  // value from the analog pin

void setup() {
    // Attach the ESC on pin 9
    Serial.begin(115200);
    ESC.attach(9,1000,2000); // (pin, min pulse width, max pulse width in microseconds) 
    brakes.attach(10);
    ESC.writeMicroseconds(1000);
    brakes.write(0);
    delay(7000);
}

void loop() {
    Serial.println("Start");
    ESC.writeMicroseconds(1300);
    delay(5000);
    // ESC.write(0);
    // delay(4000);
    Serial.println("go");
    delay(1000);
    ESC.writeMicroseconds(1000);
    brakes.write(60);
    delay(1000);
    brakes.write(0);
}