#include <SPI.h>
#include <Wire.h>

#define MATRIX_SIZE 7

struct color{
  uint8_t r, g, b;
};

class led_matrix{
public:
  color rgb[MATRIX_SIZE][MATRIX_SIZE];

  led_matrix() {
    clear_buffer();
  }

  void start_frame() {
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
    SPI.transfer(0x00);
  }

  void end_frame(){
    SPI.transfer(0xFF);
    SPI.transfer(0xFF);
    SPI.transfer(0xFF);
    SPI.transfer(0xFF);
  }

  void LED_frame(color c){
    SPI.transfer(0xFF);
    SPI.transfer(c.b);
    SPI.transfer(c.g);
    SPI.transfer(c.r);
  }

  void clear_buffer() {
    uint8_t i, j;
    for(i = 0; i < MATRIX_SIZE; i++) {
      for(j = 0; j < MATRIX_SIZE; j++) {
        rgb[i][j] = color{0, 0, 0};
      }
    }
  }

  void write_buffer() {
    int8_t i, j;
    start_frame();
    for(i = 0; i < MATRIX_SIZE; i++) {
      if(i % 2 == 1) {
        for(j = 0; j < MATRIX_SIZE; j++) {
          LED_frame(rgb[i][j]);
        }
      } else {
        for(j = MATRIX_SIZE - 1; j >= 0; j--) {
          LED_frame(rgb[i][j]);
        }
      }
    }
    end_frame();
  }

  void arrow_north(color c) {
    clear_buffer();
    rgb[0][3] = c;

    rgb[1][2] = c;
    rgb[1][3] = c;
    rgb[1][4] = c;

    rgb[2][1] = c;
    rgb[2][2] = c;
    rgb[2][3] = c;
    rgb[2][4] = c;
    rgb[2][5] = c;

    rgb[3][2] = c;
    rgb[3][3] = c;
    rgb[3][4] = c;

    rgb[4][2] = c;
    rgb[4][3] = c;
    rgb[4][4] = c;

    rgb[5][2] = c;
    rgb[5][3] = c;
    rgb[5][4] = c;

    rgb[6][2] = c;
    rgb[6][3] = c;
    rgb[6][4] = c;
  }

  void arrow_northeast(color c) {
    clear_buffer();
    rgb[0][3] = c;
    rgb[0][4] = c;
    rgb[0][5] = c;
    rgb[0][6] = c;

    rgb[1][4] = c;
    rgb[1][5] = c;
    rgb[1][6] = c;

    rgb[2][3] = c;
    rgb[2][4] = c;
    rgb[2][5] = c;
    rgb[2][6] = c;

    rgb[3][2] = c;
    rgb[3][3] = c;
    rgb[3][4] = c;
    rgb[3][6] = c;

    rgb[4][1] = c;
    rgb[4][2] = c;
    rgb[4][3] = c;

    rgb[5][0] = c;
    rgb[5][1] = c;
    rgb[5][2] = c;

    rgb[6][0] = c;
    rgb[6][1] = c;
  }

  void cross(color c) {
    clear_buffer();

    rgb[0][0] = c;
    rgb[0][6] = c;

    rgb[1][1] = c;
    rgb[1][5] = c;

    rgb[2][2] = c;
    rgb[2][4] = c;

    rgb[3][3] = c;

    rgb[4][2] = c;
    rgb[4][4] = c;

    rgb[5][1] = c;
    rgb[5][5] = c;

    rgb[6][0] = c;
    rgb[6][6] = c;    
  }

  void line_(color c, uint8_t, t=1) {
    int8_t i, j;
    if(t > MATRIX_SIZE/2) {
      t = 1;
    }
    for(i = 0; i < MATRIX_SIZE; i++) {
      for(j = -t; j <= t; j++) {
        rgb[i][j] = c;
      }
    }
  }

  // Rotate: 0 = 0 degrees, 1 = 90 degrees, 2 = 180 degrees, 3 = 270 degrees - Clockwise
  void rotate_buffer(uint8_t dir) {
    if(dir == 0 || dir > 3) {
      return;
    }
    dir = 4-dir;
    int8_t i, j, k;
    for(i = 0; i < MATRIX_SIZE / 2; i++) { 
      for(j = i; j < MATRIX_SIZE-i-1; j++) { 
        for(k = 0; k < dir; k++) {
          color temp = rgb[i][j]; 
          rgb[i][j] = rgb[j][MATRIX_SIZE-1-i]; 
          rgb[j][MATRIX_SIZE-1-i] = rgb[MATRIX_SIZE-1-i][MATRIX_SIZE-1-j]; 
          rgb[MATRIX_SIZE-1-i][MATRIX_SIZE-1-j] = rgb[MATRIX_SIZE-1-j][i];
          rgb[MATRIX_SIZE-1-j][i] = temp; 
        }
      } 
    }
  }

  //dir = 0 - 7: 0 = 0 degrees, 1 = 45 degrees, ... , 7 = 315 degrees
  void draw_arrow(uint8_t dir, color c) {
    if(dir % 2 == 0) {
      arrow_north(c);      
    } else {
      arrow_northeast(c);
    }
    rotate_buffer(dir/2);
    write_buffer();
  }

  void draw_cross(color c) {
    cross(c);
    write_buffer();
  }
  
  //dir = 0 - 7: 0 = 0 degrees, 1 = 45 degrees, ... , 7 = 315 degrees
  void draw_line(uint8_t dir, uint8_t t, color c) {
    if(dir % 2 == 0) {
      line_north(c, t);      
    } else {
      line_northeast(c, t);
    }
    rotate_buffer(dir/2);
    write_buffer();
  }
};

void setup() {
  Serial.begin(9600);  
  Wire.begin();
  SPI.begin();
}

void loop() {
  int8_t i, j;

  led_matrix m;
  
  m.draw_arrow(7, color{255,255,255});
  m.draw_cross(color{255,255,255});
  
  for(i = 0; i < MATRIX_SIZE; i++) { 
    for(j = 0; j < MATRIX_SIZE; j++) { 
      if(m.rgb[i][j].r != 0) {
        Serial.print("* ");
      } else {
        Serial.print("_ ");
      }
    }
    Serial.println();
  }  
  while(1);
}
