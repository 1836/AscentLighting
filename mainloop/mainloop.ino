/* SparkFun WS2812 Breakout Board Example
 SparkFun Electronics
 date: July 25, 2013
 license: GNU GENERAL PUBLIC LICENSE
 
 Requires the Adafruit NeoPixel library. It's awesome, go get it.
 https://github.com/adafruit/Adafruit_NeoPixel
 
 This simple example code runs three sets of animations on a group of WS2812
 breakout boards. The more boards you link up, the better these animations
 will look. 
 
 For help linking WS2812 breakouts, checkout our hookup guide:
 https://learn.sparkfun.com/tutorials/ws2812-breakout-hookup-guide
 
 Before uploading the code, make sure you adjust the two defines at the
 top of this sketch: PIN and LED_COUNT. Pin should be the Arduino pin
 you've got connected to the first pixel's DIN pin. By default it's
 set to Arduino pin 4. LED_COUNT should be the number of breakout boards
 you have linked up.
 
 I didn't copy this...
 
 Nathan Hakkakzadeh
 */
#include <Adafruit_NeoPixel.h>
#include "WS2812_Definitions.h"
#include <Wire.h>
#include "LPD8806.h"
#include "SPI.h"


#define PIN 4
#define LED_COUNT 55

// LED modes
#define IDLE_MODE 0
#define SHOOT_MODE 1
#define NO_MODE 2

#define I2C_ADDRESS 0x52

// Number of RGB LEDs in strand:
int nLEDs = 22;

// Chose 2 pins for output; can be any valid output pins:
int dataPin  = 5;
int clockPin = 6;

//setup stuff for longshoot loop
uint16_t wait = 500;
int maxshots = 3;  //there is only allowed to be 3 shots on the strip at any one time
int shotcount[3];  //this is a counter to keep track of the places the shots are
int arraycount = 0;  //this makes sure that the shotcount array does not go past it's boundary

char buf [100];
volatile byte pos;
volatile boolean process_it;


LPD8806 strip = LPD8806(nLEDs, dataPin, clockPin);

// Create an instance of the Adafruit_NeoPixel class called "leds".
// That'll be what we refer to from here on...
Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, PIN, NEO_GRB + NEO_KHZ800);

// Speed of the shooter (eventually we'll get this from I2C...)
int shooter_speed = 0;

// LED mode
int led_mode = NO_MODE;

void setup()
{
  leds.begin();  // Call this to start up the LED strip.

  // Startup I2C
  /*
  Wire.begin(I2C_ADDRESS);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
  */
  
   // have to send on master in, *slave out*
  pinMode(MISO, OUTPUT);
  
  // turn on SPI in slave mode
  SPCR |= _BV(SPE);
  
  // get ready for an interrupt 
  pos = 0;   // buffer empty
  process_it = false;

  // now turn on interrupts
  SPI.attachInterrupt();
  
  Serial.begin(9600);           // start serial for output

  clearLEDs();   // This function, defined below, turns all LEDs off...
  leds.show();   // ...but the LEDs don't actually update until you call this.

  // Start up the LED strip
  strip.begin();

  // Update the strip, to start they are all 'off'
  strip.show();

}

// SPI interrupt routine
ISR (SPI_STC_vect)
{
byte c = SPDR;  // grab byte from SPI Data Register
  
  // add to buffer if room
  if (pos < sizeof buf)
    {
    buf [pos++] = c;
    
    // example: newline means time to process buffer
    if (c == '\n')
      process_it = true;
      
    }  // end of room available
}  // end of interrupt routine SPI_STC_vect



void loop()
{
  
  if (process_it)
    {
    buf [pos] = 0;  
    if (buf == "idle") {
      led_mode = IDLE_MODE;
    } else {
      led_mode = SHOOT_MODE;
      shooter_speed = buf[0] * 100;
    }
    pos = 0;
    process_it = false;
    }  // end of flag set
  
  // What we do depends on what mode we are in...

  if (led_mode == IDLE_MODE)
  {
    cascade(RED, TOP_DOWN, 25);
  }

  else if (led_mode == SHOOT_MODE)
  {
    shootingLoop();
  }

  else
  {
    
    for (int i=0; i<10; i++)
    {
      cascade(RED, TOP_DOWN, 25);
    }
    shooter_speed = 0;
    shootingLoop();
    delay(5000);
    for (int i=0; i<10; i++)
    {
      shooter_speed += 300;
      shootingLoop();
    }
  }


}

void receiveEvent(int howMany)
{
  while(1 < Wire.available()) // loop through all but the last
  {
    char c = Wire.read(); // receive byte as a character
    if (c == 255 || c == 0)
    {
    }
    else if(c == 1)
    {
      led_mode = IDLE_MODE;
      Serial.println("This should go to idle mode.");         // print the character     
    }
    else
    {
      led_mode = SHOOT_MODE;
      shooter_speed = c * 100;
      Serial.println(shooter_speed);
    }
  }
  int x = Wire.read();    // receive byte as an 
  if (x == 255 || x == 0)
    {
    }
    else if(x == 1)
    {
      led_mode = IDLE_MODE;
      Serial.println("This should go to idle mode.");         // print the character     
    }
    else
    {
      led_mode = SHOOT_MODE;
      shooter_speed = x * 100;
      Serial.println("Shooter Mode");
    }

}

// Implements a little larson "cylon" sanner.
// This'll run one full cycle, down one way and back the other
void cylon(unsigned long color, byte wait)
{
  // weight determines how much lighter the outer "eye" colors are
  const byte weight = 4;  
  // It'll be easier to decrement each of these colors individually
  // so we'll split them out of the 24-bit color value
  byte red = (color & 0xFF0000) >> 16;
  byte green = (color & 0x00FF00) >> 8;
  byte blue = (color & 0x0000FF);

  // Start at closest LED, and move to the outside
  for (int i=0; i<=LED_COUNT-1; i++)
  {
    clearLEDs();
    leds.setPixelColor(i, red, green, blue);  // Set the bright middle eye
    // Now set two eyes to each side to get progressively dimmer
    for (int j=1; j<3; j++)
    {
      if (i-j >= 0)
        leds.setPixelColor(i-j, red/(weight*j), green/(weight*j), blue/(weight*j));
      if (i-j <= LED_COUNT)
        leds.setPixelColor(i+j, red/(weight*j), green/(weight*j), blue/(weight*j));
    }
    leds.show();  // Turn the LEDs on
    delay(wait);  // Delay for visibility
  }

  // Now we go back to where we came. Do the same thing.
  for (int i=LED_COUNT-2; i>=1; i--)
  {
    clearLEDs();
    leds.setPixelColor(i, red, green, blue);
    for (int j=1; j<3; j++)
    {
      if (i-j >= 0)
        leds.setPixelColor(i-j, red/(weight*j), green/(weight*j), blue/(weight*j));
      if (i-j <= LED_COUNT)
        leds.setPixelColor(i+j, red/(weight*j), green/(weight*j), blue/(weight*j));
    }

    leds.show();
    delay(wait);
  }
}

// Cascades a single direction. One time.
void cascade(unsigned long color, byte direction, byte wait)
{
  if (direction == TOP_DOWN)
  {
    for (int i=LED_COUNT-1; i>=0; i--)
    {
      clearLEDs();  // Turn off all LEDs
      leds.setPixelColor(i, color);  // Set just this one
      //strip.setPixelColor(i, color);

      // Slowly drop off...
      for (int j=1; j<17; j++)
      {
        unsigned long dimColor;
        if (j < 15)
        {
          dimColor = color - (0x110000 * j);
        }
        else
        {
          dimColor = 0x110000;
        }

        leds.setPixelColor((i+j) % LED_COUNT, dimColor);
        leds.setPixelColor((i-j) % LED_COUNT, dimColor);

        strip.setPixelColor((i+j) % LED_COUNT, dimColor);
        strip.setPixelColor((i-j) % LED_COUNT, dimColor);
      }

      leds.show();
      strip.show();
      delay(wait);
    }
  }
  else
  {
    for (int i=LED_COUNT-1; i>=0; i--)
    {
      clearLEDs();
      leds.setPixelColor(i, color);
      leds.show();
      delay(wait);
    }
  }
}

void shootingLoop()
{
  if (shooter_speed > 0)
  {
    // Loop through LEDs...
    for (int i=6; i<LED_COUNT; i++)
    {
      clearLEDs();
      setRainbowBottom();
      leds.setPixelColor(i % LED_COUNT+6, RED);
      leds.setPixelColor((i+1) % LED_COUNT+6, 0x880000);
      leds.setPixelColor((i-1) % LED_COUNT+6, 0x880000);

      leds.setPixelColor((i+2) % LED_COUNT+6, 0x110000);
      leds.setPixelColor((i-2) % LED_COUNT+6, 0x110000);
      leds.show();
      delay(25000/shooter_speed);
    }
  }
  else {
    clearLEDs();
    setRainbowBottom();
    leds.setPixelColor(6, RED);
    leds.setPixelColor(7, RED);
    leds.setPixelColor(8, RED);
    leds.show();
  }
}

void setRainbowBottom()
{
  leds.setPixelColor(0, WHITE);
  leds.setPixelColor(1, VIOLET);
  leds.setPixelColor(2, BLUE);
  leds.setPixelColor(3, GREEN);
  leds.setPixelColor(4, YELLOW);
  leds.setPixelColor(5, ORANGE);

}

// Sets all LEDs to off, but DOES NOT update the display;
// call leds.show() to actually turn them off after this.
void clearLEDs()
{
  for (int i=0; i<LED_COUNT; i++)
  {
    leds.setPixelColor(i, 0);
  }
}

// Prints a rainbow on the ENTIRE LED strip.
//  The rainbow begins at a specified position. 
// ROY G BIV!
void rainbow(byte startPosition) 
{
  // Need to scale our rainbow. We want a variety of colors, even if there
  // are just 10 or so pixels.
  int rainbowScale = 192 / LED_COUNT;

  // Next we setup each pixel with the right color
  for (int i=0; i<LED_COUNT; i++)
  {
    // There are 192 total colors we can get out of the rainbowOrder function.
    // It'll return a color between red->orange->green->...->violet for 0-191.
    leds.setPixelColor(i, rainbowOrder((rainbowScale * (i + startPosition)) % 192));
  }
  // Finally, actually turn the LEDs on:
  leds.show();
}

// Input a value 0 to 191 to get a color value.
// The colors are a transition red->yellow->green->aqua->blue->fuchsia->red...
//  Adapted from Wheel function in the Adafruit_NeoPixel library example sketch
uint32_t rainbowOrder(byte position) 
{
  // 6 total zones of color change:
  if (position < 31)  // Red -> Yellow (Red = FF, blue = 0, green goes 00-FF)
  {
    return leds.Color(0xFF, position * 8, 0);
  }
  else if (position < 63)  // Yellow -> Green (Green = FF, blue = 0, red goes FF->00)
  {
    position -= 31;
    return leds.Color(0xFF - position * 8, 0xFF, 0);
  }
  else if (position < 95)  // Green->Aqua (Green = FF, red = 0, blue goes 00->FF)
  {
    position -= 63;
    return leds.Color(0, 0xFF, position * 8);
  }
  else if (position < 127)  // Aqua->Blue (Blue = FF, red = 0, green goes FF->00)
  {
    position -= 95;
    return leds.Color(0, 0xFF - position * 8, 0xFF);
  }
  else if (position < 159)  // Blue->Fuchsia (Blue = FF, green = 0, red goes 00->FF)
  {
    position -= 127;
    return leds.Color(position * 8, 0, 0xFF);
  }
  else  //160 <position< 191   Fuchsia->Red (Red = FF, green = 0, blue goes FF->00)
  {
    position -= 159;
    return leds.Color(0xFF, 0x00, 0xFF - position * 8);
  }




}
