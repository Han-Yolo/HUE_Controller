/*
 * esp8266.h
 *
 *  Created on: Nov 5, 2019
 *      Author: hanyolo
 */

#ifndef NEOPIXEL_ESP8266_H_
#define NEOPIXEL_ESP8266_H_


#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

#include "esp_attr.h"

#define ESP8266

#define INPUT 1
#define OUTPUT 2

#define LOW 0
#define HIGH 1

void pinMode(uint16_t pin, uint8_t dir);
void digitalWrite(uint16_t pin, uint8_t level);
void IRAM_ATTR espShow(uint8_t pin, uint8_t *pixels, uint32_t numBytes, uint8_t is800KHz);

void initMicros(void);
uint32_t micros(void);

void noInterrupts(void);
void interrupts(void);


#ifdef __cplusplus
}
#endif

#endif /* NEOPIXEL_ESP8266_H_ */
