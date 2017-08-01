#define time_period 500000 // microseconds

uint8_t sin_table[] = {
0, 4, 9, 13, 18, 22, 27, 31, 35, 40, 
44, 49, 53, 57, 62, 66, 70, 75, 79, 83, 
87, 91, 96, 100, 104, 108, 112, 116, 120, 124, 
128, 131, 135, 139, 143, 146, 150, 153, 157, 160, 
162, 167, 171, 174, 177, 180, 183, 186, 190, 192, 
196, 198, 201, 204, 206, 209, 211, 214, 216, 219, 
221, 223, 225, 227, 229, 231, 233, 235, 236, 238, 
240, 241, 243, 244, 245, 246, 247, 248, 249, 250, 
251, 252, 253, 253, 254, 254, 254, 255, 255, 255 
};


void setup() {
  Serial.begin(9600);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
  Serial.println("Started");
  setPwmFrequency(9, 1);
  setPwmFrequency(10, 1);
  digitalWrite(9, LOW);
  digitalWrite(10, LOW);
}

void loop() {
  int i;
  long int t;
  digitalWrite(9, HIGH);
  delayMicroseconds(150);
  digitalWrite(9, LOW);
  digitalWrite(10, HIGH);
  delayMicroseconds(150);
  digitalWrite(10, LOW);
  /*analogWrite(10, 0);
  for(i = 0; i < 18; i++){
    t = micros();
    analogWrite(9, sin_table[i * 5]);
    while((micros() - t) < 320);
  }
  for(i = 18; i > 0; i--){
    t = micros();
    if((i * 5) < 90) analogWrite(9, sin_table[i * 5]);
    else analogWrite(9, sin_table[89]);
    while((micros() - t) < 320);
  }
  analogWrite(9, 0);
  for(i = 0; i < 18; i++){
    t = micros();
    analogWrite(10, sin_table[i * 5]);
    while((micros() - t) < 320);
  }
  for(i = 18; i > 0; i--){
    t = micros();
    if((i * 5) < 90) analogWrite(10, sin_table[i * 5]);
    else analogWrite(10, sin_table[89]);
    while((micros() - t) < 320);
  }*/
  
  
}

void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 64: mode = 0x03; break;
      case 256: mode = 0x04; break;
      case 1024: mode = 0x05; break;
      default: return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    } else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  } else if(pin == 3 || pin == 11) {
    switch(divisor) {
      case 1: mode = 0x01; break;
      case 8: mode = 0x02; break;
      case 32: mode = 0x03; break;
      case 64: mode = 0x04; break;
      case 128: mode = 0x05; break;
      case 256: mode = 0x06; break;
      case 1024: mode = 0x07; break;
      default: return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}
