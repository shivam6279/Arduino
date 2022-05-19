/* This example shows how to display a moving gradient pattern on
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
const uint16_t ledCount = 180;

// Create a buffer for holding the colors (3 bytes per color).
rgb_color colors[ledCount];

// Set the brightness to use (the maximum is 31).
const uint8_t brightness = 5;

void setup()
{
}

void loop()
{
  for(uint16_t i = 0; i < ledCount; i++)
    colors[i] = rgb_color(255, 0, 0);
  ledStrip.write(colors, ledCount, brightness);
  delay(1000);

  for(uint16_t i = 0; i < ledCount; i++)
    colors[i] = rgb_color(0, 255, 0);
  ledStrip.write(colors, ledCount, brightness);
  delay(1000);

  for(uint16_t i = 0; i < ledCount; i++)
    colors[i] = rgb_color(0, 0, 255);
  ledStrip.write(colors, ledCount, brightness);
  delay(1000);
}
