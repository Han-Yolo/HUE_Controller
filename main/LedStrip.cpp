#include "LedStrip.h"

#include "Adafruit_NeoPixel.h"
#include "esp8266.h"

#include "FreeRTOS.h"
#include "task.h"

#include <esp_log.h>

#include <math.h>


#define LED_PIN     4
#define LED_COUNT   8

#define LOG_TAG     "LedStrip"


static const uint8_t ctValues[LED_COUNT][3] = 
{
    {255, 254, 250},
    {255, 244, 233},
    {255, 232, 213},
    {255, 219, 190},
    {255, 204, 163},
    {255, 186, 128},
    {255, 165, 83},
    {255, 137, 14},
};

static Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);


void LedStrip::init(uint8_t brightness)
{
    initMicros();

    strip.begin();
    strip.setBrightness(brightness);
    strip.show();
}


void LedStrip::showHUE(uint8_t sat)
{
    uint16_t increment = 65535 / (LED_COUNT-1);

    for(uint8_t n = 0; n < LED_COUNT; n++)
    {
        uint16_t hue = (LED_COUNT - 1 - n) * increment;
        strip.setPixelColor(n, 
            strip.ColorHSV(hue, sat, 0xFF));
    }

    strip.show();
}


void LedStrip::showSaturation(uint16_t hue)
{
    uint8_t increment = 0xFF / (LED_COUNT - 1);

    for(uint8_t n = 0; n < LED_COUNT; n++)
    {
        uint8_t sat = (LED_COUNT - 1 - n) * increment;
        strip.setPixelColor(n, 
                strip.ColorHSV(hue, sat, 0xFF));
    }

    strip.show();
}


void LedStrip::showBrightness(uint16_t hue, uint8_t sat)
{
    uint8_t increment = 0xFF / (LED_COUNT - 1);

    for(uint8_t n = 0; n < LED_COUNT; n++)
    {
        uint8_t bri = (LED_COUNT - 1 - n) * increment;
        strip.setPixelColor(n, 
                strip.ColorHSV(hue, sat, bri));
    }

    strip.show();
}


void LedStrip::showBrightness(uint16_t ct)
{
    uint8_t increment = 0xFF / (LED_COUNT - 1);

    for(uint8_t n = 0; n < LED_COUNT; n++)
    {
        uint8_t bri = (LED_COUNT - 1 - n) * increment;
        strip.setPixelColor(n, colorCT(ct, bri));
    }

    strip.show();
}


void LedStrip::showColorTemperature(void)
{
    for(uint8_t n = 0; n < LED_COUNT; n++)
    {
        strip.setPixelColor(LED_COUNT - 1 - n, 
            ctValues[n][0], 
            ctValues[n][1], 
            ctValues[n][2]);
    }

    strip.show();
}


void LedStrip::showColor(uint16_t hue)
{
    for(uint8_t n = 0; n < LED_COUNT; n++)
    {
        strip.setPixelColor(n, strip.ColorHSV(hue));
    }

    strip.show();
}


uint32_t LedStrip::colorCT(uint16_t ct, uint8_t bri)
{
    uint8_t r, g, b;

    int32_t temperature = 
        (((((int32_t)ct - 500) * (-4500)) / 347) + 2000) / 100;

    /* Calculate red */
    if(temperature <= 66)
    {
        r = 255;
    }
    else
    {
        float red = 329.698727446F * 
            pow((float)(temperature - 60), -0.1332047592F);

        r = (uint8_t)round(red);
    }

    /* Calculate green */
    float green = 0;
    if(temperature <= 66)
    {
        green = (99.4708025861F * 
            log((float)temperature)) - 161.1195681661F;
    }
    else
    {
        green = 288.1221695283F *
            pow((float)(temperature - 60), -0.0755148492F);
    }
    g = (uint8_t)round(green);
    
    /* Calculate blue */
    if(temperature >= 66)
    {
        b = 255;
    }
    else
    {
        if(temperature <= 19)
        {
            b = 0;
        }
        else
        {
            float blue = (138.5177312231F *
                log((float)(temperature - 10))) - 305.0447927307F;

            b = (uint8_t)round(blue);
        }
    }

    return  ((((uint32_t)r * bri) / 255) << 16) |
            ((((uint32_t)g * bri) / 255) << 8)  |
             (((uint32_t)b * bri) / 255);
}


/*********************************** EFFECTS ***********************************/

// Fill strip pixels one after another with a color. Strip is NOT cleared
// first; anything there will be covered pixel by pixel. Pass in color
// (as a single 'packed' 32-bit value, which you can get by calling
// strip.Color(red, green, blue) as shown in the loop() function above),
// and a delay time (in milliseconds) between pixels.
void LedStrip::colorWipe(uint32_t color, int wait) 
{
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    vTaskDelay(wait / portTICK_PERIOD_MS); //  Pause for a moment
  }
}

// Theater-marquee-style chasing lights. Pass in a color (32-bit value,
// a la strip.Color(r,g,b) as mentioned above), and a delay time (in ms)
// between frames.
void LedStrip::theaterChase(uint32_t color, int wait) 
{
  for(int a=0; a<10; a++) {  // Repeat 10 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in steps of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show(); // Update strip with new contents
      vTaskDelay(wait / portTICK_PERIOD_MS);  // Pause for a moment
    }
  }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void LedStrip::rainbow(int wait) 
{
  // Hue of first pixel runs 5 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 5*65536. Adding 256 to firstPixelHue each time
  // means we'll make 5*65536/256 = 1280 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through strip.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    vTaskDelay(wait / portTICK_PERIOD_MS); // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void LedStrip::theaterChaseRainbow(int wait) 
{
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        // hue of pixel 'c' is offset by an amount to make one full
        // revolution of the color wheel (range 65536) along the length
        // of the strip (strip.numPixels() steps):
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      vTaskDelay(wait / portTICK_PERIOD_MS); // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}
