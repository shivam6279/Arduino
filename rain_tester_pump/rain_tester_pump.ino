#define LOW_PIN 9
#define HIGH_PIN 10
#define VALVE_PIN 7
#define PUMP_PIN 8

void setup() {
  pinMode(LOW_PIN, INPUT);
  pinMode(HIGH_PIN, INPUT);
  digitalWrite(LOW_PIN, HIGH);
  digitalWrite(HIGH_PIN, HIGH);
  
  pinMode(VALVE_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(VALVE_PIN, LOW);
  digitalWrite(PUMP_PIN, LOW);

  Serial.begin(9600);
  Serial.println("Rain guage pump");
}

void loop() {
  if(digitalRead(HIGH_PIN) == 0) {
    digitalWrite(PUMP_PIN, HIGH);
    while(digitalRead(HIGH_PIN) == 0);
    digitalWrite(PUMP_PIN, LOW);
  }
  while(1) {
    digitalWrite(VALVE_PIN, HIGH);
    while(digitalRead(LOW_PIN) == 0);
    digitalWrite(VALVE_PIN, LOW);
    
    digitalWrite(PUMP_PIN, HIGH);
    while(digitalRead(HIGH_PIN) == 1);
    digitalWrite(PUMP_PIN, LOW);
  }
}
