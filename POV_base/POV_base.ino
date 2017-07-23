#define time_period 200 // microseconds

void setup() {
  Serial.begin(9600);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  Serial.println("Started");
}

void loop() {
  digitalWrite(2, HIGH);
  digitalWrite(3, LOW);
  delayMicroseconds(time_period);
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);
  delayMicroseconds(time_period);
  digitalWrite(2, LOW);
  digitalWrite(3, HIGH);
  delayMicroseconds(time_period);
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);
  delayMicroseconds(time_period);
}
