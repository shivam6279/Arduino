void setup() {
  pinMode(9, OUTPUT);
  digitalWrite(9, 0);

  noInterrupts();
  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // prescaler: 8
  TCCR1B |= (1 << CS11);  
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  interrupts();

  set_rpm(50);  
  Serial.begin(9600);
}

volatile bool toggle = 1;
volatile int rpm;

ISR(TIMER1_COMPA_vect) {
  if (toggle){
    PORTB = PORTB | 0b00000010;
    toggle  = 0;
  }
  else{
    PORTB = PORTB & 0b11111101;
    toggle = 1;
  }
}

const uint16_t rpms[] = {10, 25, 50, 75, 100, 150, 250, 500, 1000, 1500, 2000, 2500, 3000};
const int ARR_SIZE = sizeof(rpms) / sizeof(rpms[0]);

void loop() {
  int i;
  unsigned long t, tt;
  while(1) {
    for(i = 0; i < ARR_SIZE; i++) {
      ramp(rpms[i], 10);
      if(rpms[i] <= 24) {
        delay(10000);
      } else {
        delay(5000);
      }
    }
  }
}

void set_rpm(int r) {
  if(r == 0) {
    TIMSK1 &= ~(1 << OCIE1A);
    toggle = 1;
    PORTB = PORTB & 0b11111101;
  } else {
    float temp = (float(r) / 60.0) * 200.0 * 2.0;

    // prescaler: 8
    temp = 2000000.0 / temp - 1.0;
    
    TCNT1 = 0;
    OCR1A = int(temp); 
  } 
  rpm = r;
}

void ramp(int r, int acc) { 
  int i;
  if(fabs(r - rpm) > acc) {
    if(r > set_rpm) {
      for(i = rpm + acc; i < r; i+=acc) {
        set_rpm(i);
        delay(15);
      }
    } else {
      for(i = rpm - acc; i > r; i-=acc) {
        set_rpm(i);
        delay(15);
      }    
    }
  }
  set_rpm(r);
}
