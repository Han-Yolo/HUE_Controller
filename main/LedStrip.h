#ifndef LEDSTRIP_H
#define LEDSTRIP_H


#include <stdint.h>


class LedStrip
{
public:

    static void init(uint8_t brightness);

    static void showHUE(uint8_t sat);
    static void showSaturation(uint16_t hue);
    static void showBrightness(uint16_t hue, uint8_t sat);
    static void showBrightness(uint16_t ct);
    static void showColorTemperature(void);
    static void showColor(uint16_t hue);
    
    static void colorWipe(uint32_t color, int wait);
    static void theaterChase(uint32_t color, int wait);
    static void rainbow(int wait);
    static void theaterChaseRainbow(int wait);

private:

    static uint32_t colorCT(uint16_t ct, uint8_t bri); 
};


#endif /* LEDSTRIP_H */
