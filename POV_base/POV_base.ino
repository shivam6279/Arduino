void setup() {
  Serial.begin(9600);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  Serial.println("Started");
}

void loop() {
  digitalWrite(2, HIGH);
  digitalWrite(3, LOW);
  delayMicroseconds(200);
  digitalWrite(2, LOW);
  digitalWrite(3, HIGH);
  delayMicroseconds(200);
}
