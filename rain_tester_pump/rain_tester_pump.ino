#define LOW_PIN 14
#define HIGH_PIN 15
#define VALVE_PIN 26
#define PUMP_PIN 25

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
  /*while(1){
    digitalWrite(VALVE_PIN, HIGH);
    delay(1000);
    digitalWrite(VALVE_PIN, LOW);
    digitalWrite(PUMP_PIN, HIGH);
    delay(1000);
    digitalWrite(PUMP_PIN, LOW);
  }*/
  /*while(1) {
    Serial.print(digitalRead(HIGH_PIN));
    Serial.print('\t');
    Serial.println(digitalRead(LOW_PIN));
    delay(250);
  }*/
  
  if(digitalRead(HIGH_PIN) == 0) {
    digitalWrite(PUMP_PIN, HIGH);
    while(digitalRead(HIGH_PIN) == 0) {
      Serial.print(digitalRead(HIGH_PIN));
      Serial.print('\t');
      Serial.println(digitalRead(LOW_PIN));
      delay(250);
    }
    digitalWrite(PUMP_PIN, LOW);
  } else {
    digitalWrite(VALVE_PIN, HIGH);
    while(digitalRead(HIGH_PIN) == 1) {
      Serial.print(digitalRead(HIGH_PIN));
      Serial.print('\t');
      Serial.println(digitalRead(LOW_PIN));
      delay(250);
    }
    digitalWrite(VALVE_PIN, LOW);
  }
  delay(1000);
  Serial.println("Started");
  while(1) {
    digitalWrite(VALVE_PIN, HIGH);
    while(digitalRead(LOW_PIN) == 1) {
      Serial.print(digitalRead(HIGH_PIN));
      Serial.print('\t');
      Serial.println(digitalRead(LOW_PIN));
      delay(250);
    }
    digitalWrite(VALVE_PIN, LOW);
    delay(1000);
    
    digitalWrite(PUMP_PIN, HIGH);
    while(digitalRead(HIGH_PIN) == 0) {
      Serial.print(digitalRead(HIGH_PIN));
      Serial.print('\t');
      Serial.println(digitalRead(LOW_PIN));
      delay(250);
    }
    digitalWrite(PUMP_PIN, LOW);
    delay(1000);
  }
}
