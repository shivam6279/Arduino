/*
 *    Example source code for an Arduino to show how
 *    to use SPI to communicate with an Allegro A1339
 *
 *    Written by K. Robert Bate, Allegro MicroSystems, LLC.
 *
 *    A1339SPIExample is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <SPI.h>

#define kNOERROR 0
#define kPRIMARYREADERROR 1
#define kEXTENDEDREADTIMEOUTERROR 2
#define kPRIMARYWRITEERROR 3
#define kEXTENDEDWRITETIMEOUTERROR 4
#define kCRCERROR 5

const uint16_t ChipSelectPin = 10;
const uint16_t LEDPin = 13;

const uint32_t WRITE = 0x40;
const uint32_t READ = 0x00;
const uint32_t COMMAND_MASK = 0xC0;
const uint32_t ADDRESS_MASK = 0x3F;

unsigned long nextTime;
bool ledOn = false;

bool includeCRC = false;
bool seventeenBitTransfer = false;

int16_t linearization_data[32] = {  0, // 0 degrees
                                    7, // 11.25 degrees
                                    6, // 22.5 degrees
                                    7, // 33.75 degrees
                                    11, // 45 degrees
                                    13, // 56.25 degrees
                                    12, // 67.5 degrees
                                    8, // 78.75 degrees
                                    6, // 90 degrees
                                    5, // 101.25 degrees
                                    -7, // 112.5 degrees
                                    -5, // 123.75 degrees
                                    -12, // 135 degrees
                                    -10, // 146.25 degrees
                                    -8, // 157.50 degrees
                                    -3, // 168.75 degrees
                                    -2, // 180 degrees
                                    6, // 191.25 degrees
                                    4, // 202.5 degrees
                                    5, // 213.75 degrees
                                    13, // 225 degrees
                                    13, // 236.25 degrees
                                    13, // 247.5 degrees
                                    14, // 258.75 degrees
                                    13, // 270 degrees
                                    16, // 281.25 degrees
                                    3, // 292.5 degrees
                                    -1, // 303.75 degrees
                                    4, // 315 degrees
                                    -5, // 326.25 degrees
                                    -7, // 337.5 degrees
                                    -1, // 348.75 degrees
                                    };

void setup()
{
    uint16_t flags;
    uint16_t angle;
    uint32_t flagsAndZeroOffset;
    uint8_t i;

    // Initialize SPI
    SPI.begin(); 

    pinMode(ChipSelectPin, OUTPUT);
    //    SPI.setSCK(14);

    pinMode(LEDPin, OUTPUT);

    // Initialize serial
    Serial.begin(115200);
    while (!Serial);  // Wait for the serial connection to be established

    nextTime = millis();
    digitalWrite(LEDPin, LOW);
    digitalWrite(ChipSelectPin, HIGH);
    digitalWrite(14, HIGH);

    // Unlock the device
    PrimaryWrite(ChipSelectPin, 0x3C, 0x2700);
    PrimaryWrite(ChipSelectPin, 0x3C, 0x8100);
    PrimaryWrite(ChipSelectPin, 0x3C, 0x1F00);
    PrimaryWrite(ChipSelectPin, 0x3C, 0x7700);

    // Make sure the device is unlocked
    PrimaryRead(ChipSelectPin, 0x3C, flags);
    if ((flags & 0x0001) != 0x0001)
    {
    Serial.println("Device is not Unlocked");
    }

    // Zero the angle
    // Extended location 0x1C contains flags in the MSW and the Zero Angle values in the LSW
    // so get both and zero out ZeroAngle
    // ExtendedRead(ChipSelectPin, 0x1C, flagsAndZeroOffset);
    // flagsAndZeroOffset = flagsAndZeroOffset & 0x00FFF000;
    // ExtendedWrite(ChipSelectPin, 0x1C, flagsAndZeroOffset); // Zero out the Shadow

    // Get the current angle. It is now without the ZeroAngle correction
    // PrimaryRead(ChipSelectPin, 0x20, angle);

    // Copy the read angle into location 0x5C preserving the flags
    // flagsAndZeroOffset = flagsAndZeroOffset | (angle & 0x000FFF);
    // ExtendedWrite(ChipSelectPin, 0x1C, flagsAndZeroOffset);

    delay(100);

    uint32_t t = 0;

    uint32_t slew_time = 0b000000;
    uint32_t inv = 0b0;
    uint32_t ahe = 0b0;
    uint32_t index_mode = 0b00;
    uint32_t wdh = 0b0;
    uint32_t plh = 0b0;
    uint32_t ioe = 0b1;
    uint32_t uvw = 0b0;
    uint32_t resolution_pairs = 0x4;
    // 0x3: 8192
    // 0x4: 4096
    uint32_t ABI = (slew_time << 16) | (inv << 15) | (ahe << 12) | (index_mode << 8) | (wdh << 7) | (plh << 6) | (ioe << 5) | (uvw << 4) | (resolution_pairs << 0);
    ExtendedRead(ChipSelectPin, 0x19, t);
    Serial.print("0x19: 0x");
    Serial.println(t & 0xFFFFFF, HEX);
    ExtendedWrite(ChipSelectPin, 0x19, ABI);
    ExtendedRead(ChipSelectPin, 0x19, t);
    Serial.println(t & 0xFFFFFF, HEX);

    ExtendedRead(ChipSelectPin, 0x1B, t);
    Serial.print("0x1B: 0x");
    Serial.println(t & 0xFFFFFF, HEX);
    // ExtendedWrite(ChipSelectPin, 0x1B, t | (1<<11));
    // ExtendedWrite(ChipSelectPin, 0x1B, 0x670100);
    // ExtendedRead(ChipSelectPin, 0x1B, t);
    // Serial.println(t & 0xFFFFFF);

    uint32_t orate = 0x0;
    uint32_t hysteresis = 0x4; // 0x3F = 1.384 degrees, 0x1 = 0.02197265625 degrees
    uint32_t ANG = (orate << 20) | (hysteresis << 12);
    
    ExtendedRead(ChipSelectPin, 0x1C, t);
    Serial.print("0x1C: 0x");
    Serial.println(t & 0xFFFFFF, HEX);
    ExtendedWrite(ChipSelectPin, 0x1C, ANG);
    ExtendedRead(ChipSelectPin, 0x1C, t);
    Serial.println(t & 0xFFFFFF, HEX);

    // for(i = 0; i < 32; i += 2) {
    //     ExtendedWrite(ChipSelectPin, 0x20+i/2, linearization_data[i+1] << 12 | linearization_data[i]);
    //     Serial.print(0x20+i/2, HEX);
    //     Serial.print(": ");
    //     Serial.print(linearization_data[i+1] << 12 | linearization_data[i]);
    // }

    while(1);
}

void loop()
{
    uint16_t angle;
    uint16_t temperature;
    uint16_t fieldStrength;
    uint16_t  turnsCount;
    float i;
    char ch;

    /*while(1) {
        for(i = 0; i < 360; i+= 11.25) {
            do{
                PrimaryRead(ChipSelectPin, 0x20, angle);
                angle = angle & (0xFFF);
                int32_t t = angle;
                Serial.print(i);
                Serial.print(", ");
                Serial.print(i * 4096/360);
                Serial.print(", ");
                Serial.println(i * 4096/360 - angle);
                delay(250);
                ch = 0;
                if(Serial.available()) {
                    ch = Serial.read();
                }
            }while(ch != 'A');
        }
    }*/

    // Every second, read the angle, temperature, field strength and turns count
    if (nextTime < millis()) {
        if (PrimaryRead(ChipSelectPin, 0x20, angle) == kNOERROR) {
            if (CalculateParity(angle)) {
                Serial.print("Angle = ");
                Serial.print((float)(angle & 0x0FFF) * 360.0 / 4096.0);
                Serial.println(" Degrees");
            } else {
                Serial.println("Parity error on Angle read");
            }
        } else {
            Serial.println("Unable to read Angle");
        }

        /*
        if (PrimaryRead(ChipSelectPin, 0x28, temperature) == kNOERROR) {
            Serial.print("Temperature = ");
            Serial.print(((float)(temperature & 0x0FFF) / 8.0) + 25.0);
            Serial.println(" C");
        } else {
            Serial.println("Unable to read Temperature");
        }

        if (PrimaryRead(ChipSelectPin, 0x2A, fieldStrength) == kNOERROR) {
            Serial.print("Field Strength = ");
            Serial.print(fieldStrength & 0x0FFF);
            Serial.println(" Gauss");
        } else {
            Serial.println("Unable to read Field Strength");
        }

        if (PrimaryRead(ChipSelectPin, 0x2C, turnsCount) == kNOERROR) {
            if (CalculateParity(turnsCount)) {
                Serial.print("Turns Count = ");
                Serial.println(SignExtendBitfield(turnsCount, 12));
            } else {
                 Serial.println("Parity error on Turns Count read");
            }
        } else {
            Serial.println("Unable to read Turns Count");
        }
        */

        nextTime = millis() + 500L;

    // Blink the LED every half second
        if (ledOn) {
            digitalWrite(LEDPin, LOW);
            ledOn = false;
        } else {
            digitalWrite(LEDPin, HIGH);
            ledOn = true;
        }
    }
}

/*
 * PrimaryRead
 * 
 * Read from the primary serial registers
 */
uint16_t PrimaryRead(uint16_t cs, uint16_t address, uint16_t& value)
{
    if (includeCRC)
    {
        uint8_t crcValue;
        uint8_t crcCommand;

        SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));

        // Combine the register address and the command into one byte
        uint16_t command = ((address & ADDRESS_MASK) | READ) << 8;

        crcCommand = CalculateCRC(command);

        // take the chip select low to select the device
        digitalWrite(cs, LOW);

        // send the device the register you want to read
        SPI.transfer16(command);
        SPI.transfer(crcCommand);

        digitalWrite(cs, HIGH);
        digitalWrite(cs, LOW);

        // send the command again to read the contents
        value = SPI.transfer16(command);
        crcValue = SPI.transfer(crcCommand);

        // take the chip select high to de-select
        digitalWrite(cs, HIGH);

        SPI.endTransaction();

        // Check the CRC value
        if (CalculateCRC(value) != crcValue)
        {
            return kCRCERROR;
        }
    }
    else if (seventeenBitTransfer)
    {
        uint16_t value8;
        uint16_t value9;

        // Combine the register address and the command into one word
        uint16_t command = (((address & ADDRESS_MASK) | READ) << 8)  | ((value >> 8) & 0x0FF);

        // take the chip select low to select the device:
        digitalWrite(cs, LOW);

        SPI.transfer((uint8_t)(command >> 8)); // Send the upper byte
        SPI.transfer16(0x01FE & (command << 1)); // Send the lower byte and an extra bit

        // take the chip select high to de-select:
        digitalWrite(cs, HIGH);
        // take the chip select low to re-select the device:
        digitalWrite(cs, LOW);

        value8 = SPI.transfer((uint8_t)(command >> 8)); // Send the upper byte, get back an unknown bit then the upper 7
        value9 = SPI.transfer16(0x01FE & (command << 1)); // Send the lower byte and an extra bit, get back the lower 9 bits

        value = (0xFE00 & (value8 << 9)) & (0x01FF & value9);   // Fuse the bits together

        // take the chip select high to de-select:
        digitalWrite(cs, HIGH);
    }
    else
    {
        SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));

        // Combine the register address and the command into one byte
        uint16_t command = ((address & ADDRESS_MASK) | READ) << 8;

        // take the chip select low to select the device
        digitalWrite(cs, LOW);

        // send the device the register you want to read
        SPI.transfer16(command);

        digitalWrite(cs, HIGH);
        digitalWrite(cs, LOW);

        // send the command again to read the contents
        value = SPI.transfer16(command);

        // take the chip select high to de-select
        digitalWrite(cs, HIGH);

        SPI.endTransaction();
    }

    return kNOERROR;
}

/*
 * PrimaryWrite
 * 
 * Write to the primary serial registers
 */
uint16_t PrimaryWrite(uint16_t cs, uint16_t address, uint16_t value)
{
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));

    if (includeCRC)
    {

        // Combine the register address and the command into one byte
        uint16_t command = (((address & ADDRESS_MASK) | WRITE) << 8)  | ((value >> 8) & 0x0FF);
        uint8_t crcCommand = CalculateCRC(command);

        // take the chip select low to select the device:
        digitalWrite(cs, LOW);

        SPI.transfer16(command); // Send most significant byte of register data
        SPI.transfer(crcCommand); // Send the crc

        // take the chip select high to de-select:
        digitalWrite(cs, HIGH);

        command = ((((address + 1) & ADDRESS_MASK) | WRITE) << 8 ) | (value & 0x0FF);
        crcCommand = CalculateCRC(command);

        // take the chip select low to select the device:
        digitalWrite(cs, LOW);

        SPI.transfer16(command); // Send least significant byte of register data
        SPI.transfer(crcCommand); // Send the crc

        // take the chip select high to de-select:
        digitalWrite(cs, HIGH);
    }
    else if (seventeenBitTransfer)
    {

        // Combine the register address and the command into one word
        uint16_t command = (((address & ADDRESS_MASK) | WRITE) << 8)  | ((value >> 8) & 0x0FF);

        // take the chip select low to select the device:
        digitalWrite(cs, LOW);

        SPI.transfer((uint8_t)(command >> 8)); // Send the upper byte
        SPI.transfer16(0x01FE & (command << 1)); // Send the lower byte and an extra bit

        // take the chip select high to de-select:
        digitalWrite(cs, HIGH);

        command = ((((address + 1) & ADDRESS_MASK) | WRITE) << 8 ) | (value & 0x0FF);

        // take the chip select low to select the device:
        digitalWrite(cs, LOW);

        SPI.transfer((uint8_t)(command >> 8)); // Send the upper byte
        SPI.transfer16(0x01FE & (command << 1)); // Send the lower byte and an extra bit

        // take the chip select high to de-select:
        digitalWrite(cs, HIGH);
    }
    else
    {
        // Combine the register address and the command into one byte
        uint16_t command = ((address & ADDRESS_MASK) | WRITE) << 8;

        // take the chip select low to select the device:
        digitalWrite(cs, LOW);

        SPI.transfer16(command | ((value >> 8) & 0x0FF)); // Send most significant
                                                          // byte of register data

        // take the chip select high to de-select:
        digitalWrite(cs, HIGH);

        command = (((address + 1) & ADDRESS_MASK) | WRITE) << 8;
        // take the chip select low to select the device:
        digitalWrite(cs, LOW);

        SPI.transfer16(command | (value & 0x0FF));  // Send least significant byte
                                                    // of register data

        // take the chip select high to de-select:
        digitalWrite(cs, HIGH);
    }

    SPI.endTransaction();

    return kNOERROR;
}

/*
 * ExtendedRead
 * 
 * Read from the EEPROM, Shadow or AUX
 */
uint16_t ExtendedRead(uint16_t cs, uint16_t address, uint32_t& value)
{
    uint16_t results;
    uint16_t readFlags;
    uint32_t timeout;
    uint16_t valueMSW;
    uint16_t valueLSW;
    uint32_t currentTime;

    // Write the address to the Extended Read Address register
    results = PrimaryWrite(cs, 0x0A, address & 0xFFFF);

    if (results != kNOERROR)
    {
        return results;
    }

    // Initiate the extended read
    results = PrimaryWrite(cs, 0x0C, 0x8000);
        
    if (results != kNOERROR)
    {
        return results;
    }

    timeout = millis() + 100L;

    do  // Wait for the read to be complete
    {
        results = PrimaryRead(cs, 0x0C, readFlags);
    
        if (results != kNOERROR)
        {
            return results;
        }

        // Make sure the read is not taking too long
        currentTime = millis();
        if (timeout < currentTime)
        {
            return kEXTENDEDREADTIMEOUTERROR;
        }
    } while ((readFlags & 0x0001) != 0x0001);
    
    // Read the most significant word from the extended read data
    results = PrimaryRead(cs, 0x0E, valueMSW);

    if (results != kNOERROR)
    {
        return results;
    }

    // Read the least significant word from the extended read data
    results = PrimaryRead(cs, 0x10, valueLSW);

    // Combine them
    value = ((uint32_t)valueMSW << 16) + valueLSW;

    return results;
}

/*
 * ExtendedWrite
 * 
 * Write to the EEPROM, Shadow or AUX
 */
uint16_t ExtendedWrite(uint16_t cs, uint16_t address, uint32_t value)
{
    uint16_t results;
    uint16_t writeFlags;
    uint32_t timeout;

  // Write into the extended address register
    results = PrimaryWrite(cs, 0x02, address & 0xFFFF);
    
    if (results != kNOERROR)
    {
        return results;
    }

  // Write the MSW (Most significant word) into the high order write data register
    results = PrimaryWrite(cs, 0x04, (value >> 16) & 0xFFFF);
        
    if (results != kNOERROR)
    {
        return results;
    }

  // Write the LSW (Least significant word) into the low order write data register
    results = PrimaryWrite(cs, 0x06, value & 0xFFFF);
        
    if (results != kNOERROR)
    {
        return results;
    }

  // Start the write process
    results = PrimaryWrite(cs, 0x08, 0x8000);
        
    if (results != kNOERROR)
    {
        return results;
    }

    timeout = millis() + 100;

  // Wait for the write to complete
    do
    {
        results = PrimaryRead(cs, 0x08, writeFlags);
    
        if (results != kNOERROR)
        {
            return results;
        }

        if (timeout < millis())
        {
            return kEXTENDEDWRITETIMEOUTERROR;
        }
    } while ((writeFlags & 0x0001) != 0x0001);

    return results;
}

/*
 * CalculateParity
 *
 * From the 16 bit input, calculate the parity
 */
bool CalculateParity(uint16_t input)
{
    uint16_t count = 0;
    
    // Count up the number of 1s in the input
    for (int index = 0; index < 16; ++index)
    {
        if ((input & 1) == 1)
        {
            ++count;
        }

        input >>= 1;
    }
    
    // return true if there is an odd number of 1s
    return (count & 1) != 0;
}

/*
 * CalculateCRC
 *
 * Take the 16 bit input and generate a 4bit CRC
 * Polynomial = x^4 + x^1 + 1
 * LFSR preset to all 1's
 */
uint8_t CalculateCRC(uint16_t input)
{
    bool CRC0 = true;
    bool CRC1 = true;
    bool CRC2 = true;
    bool CRC3 = true;
    int  i;
    bool DoInvert;
    uint16_t mask = 0x8000;
   
    for (i = 0; i < 16; ++i)
    {
        DoInvert = ((input & mask) != 0) ^ CRC3;         // XOR required?

        CRC3 = CRC2;
        CRC2 = CRC1;
        CRC1 = CRC0 ^ DoInvert;
        CRC0 = DoInvert;
        mask >>= 1;
    }

    return (CRC3 ? 8U : 0U) + (CRC2 ? 4U : 0U) + (CRC1 ? 2U : 0U) + (CRC0 ? 1U : 0U);
}

/*
 * SignExtendBitfield
 *
 * Sign extend a bitfield which is right justified
 */
int16_t SignExtendBitfield(uint16_t data, int width)
{
  int32_t x = (int32_t)data;
  int32_t mask = 1L << (width - 1);

  x = x & ((1 << width) - 1); // make sure the upper bits are zero

  return (int16_t)((x ^ mask) - mask);
}
