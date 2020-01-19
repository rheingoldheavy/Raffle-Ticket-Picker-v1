/*
   This code takes a CLOCK button press and retrieves a random value within the bounds of the upper and
   lower raffle ticket numbers entered below. The raffle ticket values selected are tracked in the
   "previousWinners" array so duplicates aren't issued.  If all available values are selected, code operation
   is moved into a while(1) loop to prevent re-execution of the selection code, and just displays "FULL" on
   subsequent button presses.

   While waiting for user input, the sparkleDisplay function is executed.

   Random Seed Generator Documentation:    https://rheingoldheavy.com/better-arduino-random-values/
   Display = I2C Display Add On:           https://rheingoldheavy.com/product/i2c-display-add/
   Shield  = I2C and SPI Education Shield: https://rheingoldheavy.com/product/i2c-and-spi-education-shield/
   ISSI 31FL3728 Datasheet:                http://www.issi.com/WW/pdf/31FL3728.pdf
*/

#include <I2C.h>

/*
   MODIFY THESE VALUES WITH HIGHEST AND LOWEST ISSUED RAFFLE TICKET NUMBERS
*/
const uint32_t raffleLowerBound = 1;  // The lowest sequential value raffle ticket number issued
const uint32_t raffleUpperBound = 5;  // The highest sequential value raffle ticket number issued

/*
   MODIFY THIS VALUE WITH AN UNCONNECTED ANALOG PIN FOR RANDOM SEED GENERATION
*/
const uint8_t  seedPin          = A0;  // Specifies the analog pin used to generate a random seed


// IS31FL3728 I2C Address and Registers. Driver Address Pin is Tied Low.
const uint8_t  IS31FL3728_I2C   = 0x60;
const uint8_t  TOP_R            = 0x01; // top red LED in RGB die
const uint8_t  TOP_G            = 0x02; // top grn LED in RGB die
const uint8_t  TOP_B            = 0x03; // top blu LED in RGB die
const uint8_t  DIG_D            = 0x04; // extra marking LEDs in display
const uint8_t  DIG_3            = 0x05; // Number 3 display in 0 1 2 3 order
const uint8_t  DIG_2            = 0x06; // Number 2 display in 0 1 2 3 order
const uint8_t  DIG_1            = 0x07; // Number 1 display in 0 1 2 3 order
const uint8_t  DIG_0            = 0x08; // Number 0 display in 0 1 2 3 order
const uint8_t  IS31_R_CONFIG    = 0x00;
const uint8_t  IS31_R_BRIGHT    = 0x0D;
const uint8_t  IS31_R_UPDATE    = 0x0C;
const uint8_t  DUMMY_DATA       = 0xCC;

// Display bit maps and brightness array
const uint8_t   numPatterns[10]  = {0x7E, 0x0C, 0xB6, 0x9E, 0xCC, 0xDA, 0xFA, 0x0E, 0xFE, 0xDE};
const uint8_t   brightValues[15] = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
const uint8_t   bitPosition[8]   = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
const uint8_t   font_L           = 0x70;
const uint8_t   font_A           = 0xEE;
const uint8_t   font_H           = 0xEC;
const uint8_t   font_F           = 0xE2;
const uint8_t   font_U           = 0x7C;

// Timing and bit position variables for the sparkleDisplay function
uint32_t currentMillis = 0;
uint32_t redTime       = 0;
uint32_t grnTime       = 0;
uint32_t bluTime       = 0;
uint8_t  redPos        = 0;
uint8_t  grnPos        = 0;
uint8_t  bluPos        = 0;

// Pin and button assignments. Clock, Latch and Data are on the board silk layer
const uint8_t  pinSpeaker  = A2;
const uint8_t  pinShutdown = A3;
const uint8_t  clkButton   = 6; 
const uint8_t  latButton   = 7;
const uint8_t  datButton   = 8;


const uint8_t  arraySize = (raffleUpperBound - raffleLowerBound) + 1;   // Determines the size of the previousWinners Array
uint32_t       previousWinners[arraySize];                              // An array to hold previously selected raffle numbers so they're not picked again
uint8_t        trackingIndex = 0;                                       // Holds the current position in the tracking index.

void setup() {

  // Serial.begin(115200);

  // Initialize Speaker and Shutdown Pin
  pinMode      (clkButton,   INPUT);
  pinMode      (latButton,   INPUT);
  pinMode      (datButton,   INPUT);
  pinMode      (pinSpeaker,  OUTPUT);
  pinMode      (pinShutdown, OUTPUT);
  digitalWrite (pinShutdown, HIGH);       // Shutdown Pin is Active Low

  // Configure I2C for communication with the HT16K33
  I2c.begin    ();       // Initialize the I2C library
  I2c.pullup   (0);      // Disable the internal pullup resistors
  I2c.setSpeed (1);      // Enable 400kHz I2C Bus Speed (0 = 100kHz / 1 = 400kHz)
  I2c.timeOut  (250);    // Set a 250ms timeout before the bus resets

  // Configure Driver for Normal Operation, Audio Disabled, 8x8 Matrix Mode and set brightness lowest
  I2c.write(IS31FL3728_I2C, IS31_R_CONFIG, 0x00);
  I2c.write(IS31FL3728_I2C, IS31_R_BRIGHT, 0x08);

  clearDisplay();
  displayLA3H();

  randomSeed(generateRandomSeed());
}

void loop()
{
  sparkleDisplay();

  if (digitalRead(clkButton))
  {
    uint16_t raffleTicket  = 0;
    uint8_t  duplicateFlag = 0;

    while (digitalRead(clkButton));                                        // Button Is Not Pulse Generator - Wait For It To Be Released
    raffleTicket = random(raffleLowerBound, (raffleUpperBound + 1));       // Generate Random Value

    for (int i = 0; i <= trackingIndex; i++)                               // Find a raffle ticket value that hasn't been picked yet
    {
      if (raffleTicket == previousWinners[i])
      {
        raffleTicket = random(raffleLowerBound, (raffleUpperBound + 1));
        i = -1;
      }
    }

    previousWinners[trackingIndex] = raffleTicket;                         // Add new raffle ticket number to previous winners array
    
    scrambleDisplay();                                                     // Scramble display then update with selected raffle ticket number

    uint8_t pos0 = 0;                                                      
    uint8_t pos1 = 0;
    uint8_t pos2 = 0;
    uint8_t pos3 = 0;

    pos0 = (previousWinners[trackingIndex] / 1000) % 10;
    pos1 = (previousWinners[trackingIndex] / 100)  % 10;
    pos2 = (previousWinners[trackingIndex] / 10)   % 10;
    pos3 = (previousWinners[trackingIndex])      % 10;

    clearDisplay();
    I2c.write(IS31FL3728_I2C, DIG_0, numPatterns[pos0]);
    I2c.write(IS31FL3728_I2C, DIG_1, numPatterns[pos1]);
    I2c.write(IS31FL3728_I2C, DIG_2, numPatterns[pos2]);
    I2c.write(IS31FL3728_I2C, DIG_3, numPatterns[pos3]);
    I2c.write(IS31FL3728_I2C, IS31_R_UPDATE, DUMMY_DATA);

    /*
      Serial.print   ("After Selection ");
      Serial.print   (trackingIndex);
      Serial.print   ("    Now Displaying Raffle Ticket # ");
      Serial.println (previousWinners[trackingIndex]);
    */

    trackingIndex++;                                  // Increment the trackingIndex
  }

  if (trackingIndex == arraySize)                     // If the tracking index is equal to the size of the previousWinners array
  {
    while (1)                                         // Force an endless loop
    {
      sparkleDisplay();
      if (digitalRead(clkButton))                     // on button press, display the phrase "FULL"
      {
        while (digitalRead(clkButton));
        //Serial.println ("FULL");
        displayFull();
        displayLA3H();
      }
    }
  }
}

/*
   Blank the I2C Display Add on

*/
void clearDisplay() {
  for (int i = 1; i < 9; i++) {
    I2c.write(IS31FL3728_I2C, i, 0x00);
  }
  I2c.write(IS31FL3728_I2C, IS31_R_UPDATE, DUMMY_DATA);
}

/*
   Generates a random seed using the analog pin specified by "seedPin" in the
   declarations areay.

*/
uint32_t generateRandomSeed()
{
  uint8_t  seedBitValue  = 0;
  uint8_t  seedByteValue = 0;
  uint32_t seedWordValue = 0;

  for (uint8_t wordShift = 0; wordShift < 4; wordShift++)     // 4 bytes in a 32 bit word
  {
    for (uint8_t byteShift = 0; byteShift < 8; byteShift++)   // 8 bits in a byte
    {
      for (uint8_t bitSum = 0; bitSum <= 8; bitSum++)         // 8 samples of analog pin
      {
        seedBitValue = seedBitValue + (analogRead(seedPin) & 0x01);                // Flip the coin eight times, adding the results together
      }
      delay(1);                                                                    // Delay a single millisecond to allow the pin to fluctuate
      seedByteValue = seedByteValue | ((seedBitValue & 0x01) << byteShift);        // Build a stack of eight flipped coins
      seedBitValue = 0;                                                            // Clear out the previous coin value
    }
    seedWordValue = seedWordValue | (uint32_t)seedByteValue << (8 * wordShift);    // Build a stack of four sets of 8 coins (shifting right creates a larger number so cast to 32bit)
    seedByteValue = 0;                                                             // Clear out the previous stack value
  }
  return (seedWordValue);
}

/*
   Displays the phrase "LA:3H" across the 7 Segment Display
*/
void displayLA3H()
{
  I2c.write(IS31FL3728_I2C, DIG_0, font_L);
  I2c.write(IS31FL3728_I2C, DIG_1, font_A);
  I2c.write(IS31FL3728_I2C, DIG_D, 0x06);
  I2c.write(IS31FL3728_I2C, DIG_2, numPatterns[3]);
  I2c.write(IS31FL3728_I2C, DIG_3, font_H);
  I2c.write(IS31FL3728_I2C, IS31_R_UPDATE, DUMMY_DATA);
}


/*
   Animates the word "FULL" across the 7 segment display.
   Too lazy to figure out a way to do this more efficiently. Code works.
*/
void displayFull()
{
  uint8_t animationDelay = 125;
  clearDisplay();

  // Display Static Red Bar At Top
  I2c.write(IS31FL3728_I2C, TOP_R, 0xFF);
  I2c.write(IS31FL3728_I2C, IS31_R_UPDATE, DUMMY_DATA);
  delay(250);

  for (int i = 0; i <= 8; i++)
  {
    if (i == 0)
    {
      I2c.write(IS31FL3728_I2C, DIG_3, font_F);
    }

    if (i == 1)
    {
      I2c.write(IS31FL3728_I2C, DIG_2, font_F);
      I2c.write(IS31FL3728_I2C, DIG_3, font_U);
    }
    if (i == 2)
    {
      I2c.write(IS31FL3728_I2C, DIG_1, font_F);
      I2c.write(IS31FL3728_I2C, DIG_2, font_U);
      I2c.write(IS31FL3728_I2C, DIG_3, font_L);
    }
    if (i == 3)
    {
      I2c.write(IS31FL3728_I2C, DIG_0, font_F);
      I2c.write(IS31FL3728_I2C, DIG_1, font_U);
      I2c.write(IS31FL3728_I2C, DIG_2, font_L);
      I2c.write(IS31FL3728_I2C, DIG_3, font_L);
    }
    if (i == 4)
    {
      I2c.write(IS31FL3728_I2C, DIG_0, font_U);
      I2c.write(IS31FL3728_I2C, DIG_1, font_L);
      I2c.write(IS31FL3728_I2C, DIG_2, font_L);
      I2c.write(IS31FL3728_I2C, DIG_3, 0x00);
    }
    if (i == 5)
    {
      I2c.write(IS31FL3728_I2C, DIG_0, font_L);
      I2c.write(IS31FL3728_I2C, DIG_1, font_L);
      I2c.write(IS31FL3728_I2C, DIG_2, 0x00);
    }
    if (i == 6)
    {
      I2c.write(IS31FL3728_I2C, DIG_0, font_L);
      I2c.write(IS31FL3728_I2C, DIG_1, 0x00);
    }
    if (i == 7)
    {
      I2c.write(IS31FL3728_I2C, DIG_0, 0x00);
    }
    if (i == 8)
    {

    }
    I2c.write(IS31FL3728_I2C, IS31_R_UPDATE, DUMMY_DATA);
    delay(animationDelay);
  }

}

/*
   Displays a quick scrambling animation on the seven segment display
*/

void scrambleDisplay()
{
  for (uint8_t i = 0; i < 10; i++)
  {
    for (uint8_t n = 0x01; n <= 0x08; n++)
    {
      I2c.write(IS31FL3728_I2C, n, random(0x00, (0xFF+1)));
      I2c.write(IS31FL3728_I2C, IS31_R_UPDATE, DUMMY_DATA);
      delay(5);
    }
  }

}

/*
   Display random pixel colors at random positions in the bar graph
*/
void sparkleDisplay()
{
  uint8_t delayLength = 100;
  currentMillis = millis();

  if (currentMillis - redTime >= delayLength)
  {
    I2c.write(IS31FL3728_I2C, TOP_R, bitPosition[redPos]);
    redPos = random(0, 8);
    redTime = currentMillis;
  }

  if (currentMillis - grnTime >= (delayLength + 05))
  {
    I2c.write(IS31FL3728_I2C, TOP_G, bitPosition[grnPos]);
    grnPos = random(0, 8);
    grnTime = currentMillis;
  }

  if (currentMillis - bluTime >= (delayLength + 10))
  {
    I2c.write(IS31FL3728_I2C, TOP_B, bitPosition[bluPos]);
    bluPos = random(0, 8);
    bluTime = currentMillis;
  }

  I2c.write(IS31FL3728_I2C, IS31_R_UPDATE, DUMMY_DATA);

}
