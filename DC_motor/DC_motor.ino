#include <pwm_lib.h>

using namespace arduino_due::pwm_lib;

pwm<pwm_pin::PWMH0_PC3> A_HIGH; // D35
pwm<pwm_pin::PWMH1_PC5> B_HIGH; // D37
pwm<pwm_pin::PWMH2_PC7> C_HIGH; // D39

pwm<pwm_pin::PWML4_PC21> A_LOW; // D9
pwm<pwm_pin::PWML5_PC22> B_LOW; // D8
pwm<pwm_pin::PWML6_PC23> C_LOW; // D7

#define PWM_PERIOD_US 10.0f
const unsigned long PWM_PERIOD = 100 * PWM_PERIOD_US;

void setup() {
  pinMode(31, OUTPUT);
  pinMode(33, OUTPUT);

  digitalWrite(31, 1);
  
  A_HIGH.start(PWM_PERIOD, 0);
  B_HIGH.start(PWM_PERIOD, 0);
  C_HIGH.start(PWM_PERIOD, 0);
  
  A_LOW.start(PWM_PERIOD, 0);
  B_LOW.start(PWM_PERIOD, 0);
  C_LOW.start(PWM_PERIOD, 0);
  
  Serial.begin(115200);
  Serial.println("*********************");
}

void loop() {  
  A_HIGH.set_duty(PWM_PERIOD);
  A_LOW.set_duty(PWM_PERIOD);
  
  while(1) {
    B_HIGH.set_duty(100);
    B_LOW.set_duty(0);
    delay(1);
    B_HIGH.set_duty(PWM_PERIOD);
    B_LOW.set_duty(PWM_PERIOD);
    delay(99);
  }  
}
