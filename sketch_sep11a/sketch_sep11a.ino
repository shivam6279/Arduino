void setup() {
  Serial.begin(9600);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
}

#define T1 25
#define T3 2

void loop() {
  PORTB = B10;
  delayMicroseconds(T1);
//  PORTB = B00;
//  delayMicroseconds(T3);
  PORTB = B01;
  delayMicroseconds(T1);
//  PORTB = B00;

//  delayMicroseconds(T3);
}
