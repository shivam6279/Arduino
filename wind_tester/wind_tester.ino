#include <LiquidCrystal.h>

#define wind_pin 7

//-------------------------------------------------
const float radius = 63.7 / 1000.0;
const float scaler = 2.5;
//-------------------------------------------------

//Wind speed
boolean wind_flag = true, wind_temp;

//Rain guage
volatile int rain_counter;
boolean rain_flag = true, rain_temp;

volatile uint32_t timer_counter = 0;
volatile uint32_t wind_speed_counter1 = 0, wind_speed_counter2 = 0;

double wind_speed1 = 0.0, wind_speed2 = 0.0;

volatile int timer = 0;

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
  pinMode(wind_pin, INPUT);
  digitalWrite(wind_pin, HIGH);

  Serial.begin(9600); 

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.println("Count: ");
  lcd.setCursor(0, 1);
  lcd.println("Speed: ");
  
  
  //Interrupt initialization
  noInterrupts();//stop interrupts
  TCCR1A = 0; TCCR1B = 0; TCNT1  = 0; OCR1A = 62500; TCCR1B |= (1 << WGM12); TCCR1B |= 5;
  TCCR1B &= 253; TIMSK1 |= (1 << OCIE1A); TCCR2A = 0; TCCR2B = 0; TCNT2  = 0;
  OCR2A = 249; TCCR2A |= (1 << WGM21); TCCR2B |= (1 << CS21); TIMSK2 |= (1 << OCIE2A);
  #if serial_output == true
  Serial.println("\nSeconds till next upload:");
  #endif
  interrupts();//Timer1: 0.25hz, Timer2: 8Khz
}
 
void loop() {
  if(timer) {
    timer = 0;
    wind_speed2 = wind_speed_counter2 * radius * 6.2831 / 4.0 * scaler;
    wind_speed_counter2 = 0;
  }
  lcd.setCursor(7, 0);
  lcd.print(wind_speed1, 4);
  lcd.setCursor(7, 1);
  lcd.print(wind_speed2, 4);
}

ISR(TIMER1_COMPA_vect){ //Interrupt gets called every 4 seconds 
  timer++;
}

ISR(TIMER2_COMPA_vect) { 
  timer_counter++;
  wind_temp = digitalRead(wind_pin);
  if(wind_temp == 0 && wind_flag == true) {
    if(timer_counter) wind_speed1 = 8000.0 / float(timer_counter) * radius * 6.2831 * scaler;
    timer_counter = 0;
    wind_speed_counter1++;
    wind_speed_counter2++;
    wind_flag = false;
  } 
  else if(wind_temp == 1) {
    wind_flag = true;
  } 
}

