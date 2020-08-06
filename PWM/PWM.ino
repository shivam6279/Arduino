#include <pwm_lib.h>

using namespace arduino_due::pwm_lib;

#define pole_count 7

#define ARR_SIZE 16

float rpm[ARR_SIZE] =       {25,  50,  75,  150, 300, 500, 750, 1000, 1255, 1506, 1757, 2000, 2255, 2510, 2760, 3017};
float delay_micro[ARR_SIZE];

#if pole_count == 7
//  float pwms1000[ARR_SIZE] = {650, 650, 650, 650, 600, 600, 600, 600,  550,  550,  500,  500,  500,  450,  450,  420}; // 0 - 1000
  float pwms1000[ARR_SIZE] = {650, 650, 650, 650, 600, 600, 550, 550,  500,  500,  450,  450,  400,  400,  350,  420}; // 0 - 1000
#elif pole_count == 2
  float pwms1000[ARR_SIZE] = {0,   0,   0,   0,   0,   0,   0,   0,    0,    0,    0,    0,    0,    0,    0,    0};
#endif
float motor_pwm[ARR_SIZE];

//const int pwmSin256[] = { 128,130,131,133,135,136,138,140,141,143,145,146,148,150,151,153,155,156,158,160,161,163,164,166,168,169,171,173,174,176,177,179,181,182,184,186,187,189,
//                          190,192,193,195,197,198,200,201,203,204,206,207,209,210,212,213,215,216,218,219,221,222,224,224,225,225,225,226,226,227,227,228,228,228,229,229,230,230,
//                          230,231,231,231,232,232,232,233,233,233,234,234,234,234,235,235,235,235,236,236,236,236,236,237,237,237,237,237,237,237,238,238,238,238,238,238,238,238,
//                          238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,237,237,237,237,237,237,237,236,236,236,236,236,235,235,235,235,234,
//                          234,234,234,233,233,233,232,232,232,231,231,231,230,230,230,229,229,228,228,228,227,227,226,226,225,225,225,224,224,224,225,225,225,226,226,227,227,228,
//                          228,228,229,229,230,230,230,231,231,231,232,232,232,233,233,233,234,234,234,234,235,235,235,235,236,236,236,236,236,237,237,237,237,237,237,237,238,238,
//                          238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,238,237,237,237,237,237,237,237,236,236,236,236,
//                          236,235,235,235,235,234,234,234,234,233,233,233,232,232,232,231,231,231,230,230,230,229,229,228,228,228,227,227,226,226,225,225,225,224,224,222,221,219,
//                          218,216,215,213,212,210,209,207,206,204,203,201,200,198,197,195,193,192,190,189,187,186,184,182,181,179,177,176,174,173,171,169,168,166,164,163,161,160,
//                          158,156,155,153,151,150,148,146,145,143,141,140,138,136,135,133,131,130,128,126,125,123,121,120,118,116,115,113,111,110,108,106,105,103,101,100,98, 96,
//                          95, 93, 92, 90, 88, 87, 85, 83, 82, 80, 79, 77, 75, 74, 72, 70, 69, 67, 66, 64, 63, 61, 59, 58, 56, 55, 53, 52, 50, 49, 47, 46, 44, 43, 41, 40, 38, 37,
//                          35, 34, 32, 32, 31, 31, 31, 30, 30, 29, 29, 28, 28, 28, 27, 27, 26, 26, 26, 25, 25, 25, 24, 24, 24, 23, 23, 23, 22, 22, 22, 22, 21, 21, 21, 21, 20, 20,
//                          20, 20, 20, 19, 19, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,
//                          18, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 24, 24, 24, 25, 25, 25, 26, 26, 26, 27, 27, 28, 28, 28,
//                          29, 29, 30, 30, 31, 31, 31, 32, 32, 32, 31, 31, 31, 30, 30, 29, 29, 28, 28, 28, 27, 27, 26, 26, 26, 25, 25, 25, 24, 24, 24, 23, 23, 23, 22, 22, 22, 22,
//                          21, 21, 21, 21, 20, 20, 20, 20, 20, 19, 19, 19, 19, 19, 19, 19, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 
//                          18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 21, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 24, 24, 24, 25, 25, 25, 26, 26, 
//                          26, 27, 27, 28, 28, 28, 29, 29, 30, 30, 31, 31, 31, 32, 32, 34, 35, 37, 38, 40, 41, 43, 44, 46, 47, 49, 50, 52, 53, 55, 56, 58, 59, 61, 63, 64, 66, 67, 
//                          69, 70, 72, 74, 75, 77, 79, 80, 82, 83, 85, 87, 88, 90, 92, 93, 95, 96, 98, 100,101,103,105,106,108,110,111,113,115,116,118,120,121,123,125,126        };
const int pwmSin256[] = {128, 132, 136, 140, 143, 147, 151, 155, 159, 162, 166, 170, 174, 178, 181, 185, 189, 192, 196, 200, 203, 207, 211, 214, 218, 221, 225, 228, 232, 235, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 248, 249, 250, 250, 251, 252, 252, 253, 253, 253, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 253, 253, 253, 252, 252, 251, 250, 250, 249, 248, 248, 247, 246, 245, 244, 243, 242, 241, 240, 239, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 248, 249, 250, 250, 251, 252, 252, 253, 253, 253, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 253, 253, 253, 252, 252, 251, 250, 250, 249, 248, 248, 247, 246, 245, 244, 243, 242, 241, 240, 239, 238, 235, 232, 228, 225, 221, 218, 214, 211, 207, 203, 200, 196, 192, 189, 185, 181, 178, 174, 170, 166, 162, 159, 155, 151, 147, 143, 140, 136, 132, 128, 124, 120, 116, 113, 109, 105, 101, 97, 94, 90, 86, 82, 78, 75, 71, 67, 64, 60, 56, 53, 49, 45, 42, 38, 35, 31, 28, 24, 21, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 8, 7, 6, 6, 5, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 8, 7, 6, 6, 5, 4, 4, 3, 3, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 24, 28, 31, 35, 38, 42, 45, 49, 53, 56, 60, 64, 67, 71, 75, 78, 82, 86, 90, 94, 97, 101, 105, 109, 113, 116, 120, 124};
const int svpwm_size = sizeof(pwmSin256)/sizeof(int);

int pwmSin[svpwm_size];

pwm<pwm_pin::PWMH0_PC3> A_HIGH; // D35
pwm<pwm_pin::PWMH1_PC5> B_HIGH; // D37
pwm<pwm_pin::PWMH2_PC7> C_HIGH; // D39

pwm<pwm_pin::PWML4_PC21> A_LOW; // D9
pwm<pwm_pin::PWML5_PC22> B_LOW; // D8
pwm<pwm_pin::PWML6_PC23> C_LOW; // D7

#define PWM_PERIOD_US 10.0f
const unsigned long PWM_PERIOD = 100 * PWM_PERIOD_US;

void setup() {
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
  unsigned long t, tt;
  unsigned long d, p;
  int c = 0;

  for(int i = 0; i < ARR_SIZE; i++) {
    delay_micro[i] = 10.0f / (float(pole_count) * rpm[i]) * 1000000.0f;
    motor_pwm[i] = pwms1000[i] / 1000.0f * float(PWM_PERIOD); 
  }

  for(int i = 0; i < svpwm_size; i++) {
    pwmSin[i] = PWM_PERIOD - (PWM_PERIOD * 0.3 * pwmSin256[i] / 255.0);
  }
  
   //foc(102);

  Serial.println(PWM_PERIOD);
  
  t = millis(); 
  d = delay_micro[c];
  p = motor_pwm[0];
  Serial.println("RPM, delay, pwm");
  Serial.println(String(rpm[c]) + ", " + String(d) + ", " + String(p));  
  while(1) {
    commutate(rpm[c], p);
    
    if(c < (ARR_SIZE - 1)) {
      if(millis() - t > 3000) {
        t = millis();
        c++;
        d = delay_micro[c];
        p = motor_pwm[c];
        Serial.println(String(rpm[c]) + ", " + String(d) + ", " + String(p));
      }
    }
  }
}

void foc(float rpm) {
  int currentStepA;
  int currentStepB;
  int currentStepC;
  int sineArraySize;
  int increment = 2;
  
  float d = 60.0f / (pole_count * float(svpwm_size) / increment * rpm) * 1000000.0f;

  unsigned long tt;

  sineArraySize = svpwm_size; // Find lookup table size
  int phaseShift = sineArraySize / 3;         // Find phase shift and initial A, B C phase values
  currentStepA = 0;
  currentStepB = currentStepA + phaseShift;
  currentStepC = currentStepB + phaseShift;
 
  sineArraySize--; // Convert from array Size to last PWM array number
  
  while(1) {
    tt = micros();

    A_HIGH.set_duty(pwmSin[currentStepA]);
    B_HIGH.set_duty(pwmSin[currentStepB]);
    C_HIGH.set_duty(pwmSin[currentStepC]);

    A_LOW.set_duty(pwmSin[currentStepA]);
    B_LOW.set_duty(pwmSin[currentStepB]);
    C_LOW.set_duty(pwmSin[currentStepC]);
   
    currentStepA = currentStepA + increment;
    currentStepB = currentStepB + increment;
    currentStepC = currentStepC + increment;
   
    //Check for lookup table overflow and return to opposite end if necessary
    if(currentStepA > sineArraySize)  currentStepA = 0;
    if(currentStepA < 0)  currentStepA = sineArraySize;
   
    if(currentStepB > sineArraySize)  currentStepB = 0;
    if(currentStepB < 0)  currentStepB = sineArraySize;
   
    if(currentStepC > sineArraySize)  currentStepC = 0;
    if(currentStepC < 0) currentStepC = sineArraySize; 
    
    /// Control speed by this delay
    while(micros() - tt < d);
  } 
}

void commutate(float rpm, unsigned int p) {
  unsigned long delay_phase = ceil(float(10.0f / (float(pole_count) * rpm) * 1000000.0f));
  unsigned long delay_six = 6 * delay_phase;
  unsigned long t, t6 ;
  
  t6 = micros();
  t = micros();
  
  phase1(p);
  while(micros() - t < delay_phase);
  t = micros();
  
  phase2(p);
  while(micros() - t < delay_phase);
  t = micros();
  
  phase3(p);
  while(micros() - t < delay_phase);
  t = micros();
  
  phase4(p);
  while(micros() - t < delay_phase);
  t = micros();
  
  phase5(p);
  while(micros() - t < delay_phase);
  
  phase6(p);
  while(micros() - t6 < delay_six);
}

void phase1(unsigned long p) {
  A_LOW.set_duty(0);
  B_LOW.set_duty(PWM_PERIOD);
  C_LOW.set_duty(0);
  
  A_HIGH.set_duty(p);
  B_HIGH.set_duty(PWM_PERIOD);
  C_HIGH.set_duty(PWM_PERIOD);  
}

void phase2(unsigned long p) {
  A_LOW.set_duty(0);
  B_LOW.set_duty(0);
  C_LOW.set_duty(PWM_PERIOD);
  
  A_HIGH.set_duty(p);
  B_HIGH.set_duty(PWM_PERIOD);
  C_HIGH.set_duty(PWM_PERIOD);
}

void phase3(unsigned long p) {
  A_LOW.set_duty(0);
  B_LOW.set_duty(0);
  C_LOW.set_duty(PWM_PERIOD);
  
  A_HIGH.set_duty(PWM_PERIOD);
  B_HIGH.set_duty(p);
  C_HIGH.set_duty(PWM_PERIOD);
}

void phase4(unsigned long p) {
  A_LOW.set_duty(PWM_PERIOD);
  B_LOW.set_duty(0);
  C_LOW.set_duty(0);
  
  A_HIGH.set_duty(PWM_PERIOD);
  B_HIGH.set_duty(p);
  C_HIGH.set_duty(PWM_PERIOD);
}

void phase5(unsigned long p) {
  A_LOW.set_duty(PWM_PERIOD);
  B_LOW.set_duty(0);
  C_LOW.set_duty(0);
  
  A_HIGH.set_duty(PWM_PERIOD);
  B_HIGH.set_duty(PWM_PERIOD);
  C_HIGH.set_duty(p);
}

void phase6(unsigned long p) {
  A_LOW.set_duty(0);
  B_LOW.set_duty(PWM_PERIOD);
  C_LOW.set_duty(0);
  
  A_HIGH.set_duty(PWM_PERIOD);
  B_HIGH.set_duty(PWM_PERIOD);
  C_HIGH.set_duty(p);
}

void phase_float() {
  A_LOW.set_duty(0);
  B_LOW.set_duty(0);
  C_LOW.set_duty(0);
  
  A_HIGH.set_duty(PWM_PERIOD);
  B_HIGH.set_duty(PWM_PERIOD);
  C_HIGH.set_duty(PWM_PERIOD);
}

void phase_brake() {
  A_LOW.set_duty(PWM_PERIOD);
  B_LOW.set_duty(PWM_PERIOD);
  C_LOW.set_duty(PWM_PERIOD);
  
  A_HIGH.set_duty(PWM_PERIOD);
  B_HIGH.set_duty(PWM_PERIOD);
  C_HIGH.set_duty(PWM_PERIOD);
}
