#define time_period 500000 // microseconds
#define a 10
#define b 15
#define c 11
#define d 50 - a - b - c

void setup() {
  Serial.begin(9600);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  Serial.println("Started");
  digitalWrite(7, LOW);
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
}
//10 - 462 - 38 - 490
void loop() {
  /*PORTD &= ~(1 << PORTD7);
  delayMicroseconds(5);
  PORTB |= (1 << PORTB0);
  delayMicroseconds(21);
  PORTB &= ~(1 << PORTB0);
  delayMicroseconds(19);
  PORTD |= (1 << PORTD7);
  delayMicroseconds(45);*/
  
  PORTD &= ~(1 << PORTD7);
  delayMicroseconds(a);
  PORTB |= (1 << PORTB0);
  
  delayMicroseconds(b);
  PORTB &= ~(1 << PORTB0);
  delayMicroseconds(c);
  PORTD |= (1 << PORTD7);
  delayMicroseconds(d);
}
