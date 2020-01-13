/* This example shows how to display a moving rainbow pattern on
 * an APA102-based LED strip. */

/* By default, the APA102 library uses pinMode and digitalWrite
 * to write to the LEDs, which works on all Arduino-compatible
 * boards but might be slow.  If you have a board supported by
 * the FastGPIO library and want faster LED updates, then install
 * the FastGPIO library and uncomment the next two lines: */
// #include <FastGPIO.h>
// #define APA102_USE_FAST_GPIO

#include <APA102.h>

// Define which pins to use.
const uint8_t dataPin = 11;
const uint8_t clockPin = 12;

// Create an object for writing to the LED strip.
APA102<dataPin, clockPin> ledStrip;

// Set the number of LEDs to control.
const uint16_t ledCount = 49;

// Create a buffer for holding the colors (3 bytes per color).
rgb_color colors[ledCount];

// Set the brightness to use (the maximum is 31).
const uint8_t brightness = 6;

void setup()
{
}

/* Converts a color from HSV to RGB.
 * h is hue, as a number between 0 and 360.
 * s is the saturation, as a number between 0 and 255.
 * v is the value, as a number between 0 and 255. */
rgb_color hsvToRgb(uint16_t h, uint8_t s, uint8_t v)
{
    uint8_t f = (h % 60) * 255 / 60;
    uint8_t p = (255 - s) * (uint16_t)v / 255;
    uint8_t q = (255 - f * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t t = (255 - (255 - f) * (uint16_t)s / 255) * (uint16_t)v / 255;
    uint8_t r = 0, g = 0, b = 0;
    switch((h / 60) % 6){
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
    return rgb_color(r, g, b);
}

void loop()
{
  for(uint16_t i = 0; i < ledCount; i++)
  {
      colors[i] = rgb_color(0, 0, 0);
  }
  colors[3] = rgb_color(0, 255, 0);
  colors[9] = rgb_color(0, 255, 0);
  colors[10] = rgb_color(0, 255, 0);
  colors[11] = rgb_color(0, 255, 0);

  colors[15] = rgb_color(0, 255, 0);
  colors[16] = rgb_color(0, 255, 0);
  colors[17] = rgb_color(0, 255, 0);
  colors[18] = rgb_color(0, 255, 0);
  colors[19] = rgb_color(0, 255, 0);

  colors[23] = rgb_color(0, 255, 0);
  colors[24] = rgb_color(0, 255, 0);
  colors[25] = rgb_color(0, 255, 0);

  colors[30] = rgb_color(0, 255, 0);
  colors[31] = rgb_color(0, 255, 0);
  colors[32] = rgb_color(0, 255, 0);

  colors[37] = rgb_color(0, 255, 0);
  colors[38] = rgb_color(0, 255, 0);
  colors[39] = rgb_color(0, 255, 0);

  colors[44] = rgb_color(0, 255, 0);
  colors[45] = rgb_color(0, 255, 0);
  colors[46] = rgb_color(0, 255, 0);

  ledStrip.write(colors, ledCount, brightness);

  delay(1000);

  for(uint16_t i = 0; i < ledCount; i++)
  {
      colors[i] = rgb_color(0, 0, 0);
  }

  colors[3] = rgb_color(0, 255, 0);
  colors[4] = rgb_color(0, 255, 0);
  colors[5] = rgb_color(0, 255, 0);
  colors[6] = rgb_color(0, 255, 0);

  colors[7] = rgb_color(0, 255, 0);
  colors[8] = rgb_color(0, 255, 0);
  colors[9] = rgb_color(0, 255, 0);
  colors[17] = rgb_color(0, 255, 0);
  colors[18] = rgb_color(0, 255, 0);

  colors[19] = rgb_color(0, 255, 0);
  colors[20] = rgb_color(0, 255, 0);
  colors[21] = rgb_color(0, 255, 0);

  colors[23] = rgb_color(0, 255, 0);
  colors[24] = rgb_color(0, 255, 0);
  colors[25] = rgb_color(0, 255, 0);

  colors[29] = rgb_color(0, 255, 0);
  colors[30] = rgb_color(0, 255, 0);
  colors[31] = rgb_color(0, 255, 0);

  colors[41] = rgb_color(0, 255, 0);
  colors[40] = rgb_color(0, 255, 0);
  colors[39] = rgb_color(0, 255, 0);

  colors[43] = rgb_color(0, 255, 0);
  colors[42] = rgb_color(0, 255, 0);


  ledStrip.write(colors, ledCount, brightness);

  delay(1000);
}
