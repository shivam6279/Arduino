
#include <APA102.h>

const uint8_t dataPin = 11;
const uint8_t clockPin = 12;

APA102<dataPin, clockPin> ledStrip;

const uint16_t ledCount = 198;

const uint16_t brightness = 1;
const uint16_t drawer_len = 40;

void setup()
{
}

void loop()
{
  uint16_t i, j, k;
  while(1) {
    ledStrip.startFrame();
    for(i = 0; i < (ledCount-drawer_len); i++)
    {
      ledStrip.sendColor(0, 0, 0, brightness);
    }
    for(i = 0; i < drawer_len; i++)
    {
      ledStrip.sendColor(255, 255, 255, brightness);
    }
    ledStrip.endFrame(ledCount);
    delay(100);
  }
}
