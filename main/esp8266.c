// This is a mash-up of the Due show() code + insights from Michael Miller's
// ESP8266 work for the NeoPixelBus library: github.com/Makuna/NeoPixelBus
// Needs to be a separate .c file to enforce ICACHE_RAM_ATTR execution.

#include "esp8266.h"

#include "FreeRTOS.h"
#include "task.h"

#include <xtensa/xtruntime.h>
#include "driver/gpio.h"
#include "driver/hw_timer.h"

#define F_CPU 80000000

static uint32_t microsSeconds = 0;

static uint32_t _getCycleCount(void) __attribute__((always_inline));
static inline uint32_t _getCycleCount(void) {
  uint32_t ccount;
  __asm__ __volatile__("rsr %0,ccount":"=a" (ccount));
  return ccount;
}


void IRAM_ATTR espShow(
 uint8_t pin, uint8_t *pixels, uint32_t numBytes, uint8_t is800KHz) {

#define CYCLES_800_T0H  (F_CPU / 2500000) // 0.4us
#define CYCLES_800_T1H  (F_CPU / 1250000) // 0.8us
#define CYCLES_800      (F_CPU /  800000) // 1.25us per bit
#define CYCLES_400_T0H  (F_CPU / 2000000) // 0.5uS
#define CYCLES_400_T1H  (F_CPU /  833333) // 1.2us
#define CYCLES_400      (F_CPU /  400000) // 2.5us per bit

  uint8_t *p, *end, pix, mask;
  uint32_t t, time0, time1, period, c, startTime, pinMask;

  pinMask   = BIT(pin);
  p         =  pixels;
  end       =  p + numBytes;
  pix       = *p++;
  mask      = 0x80;
  startTime = 0;

#ifdef NEO_KHZ400
  if(is800KHz) {
#endif
    time0  = CYCLES_800_T0H;
    time1  = CYCLES_800_T1H;
    period = CYCLES_800;
#ifdef NEO_KHZ400
  } else { // 400 KHz bitstream
    time0  = CYCLES_400_T0H;
    time1  = CYCLES_400_T1H;
    period = CYCLES_400;
  }
#endif

  for(t = time0;; t = time0) {
    if(pix & mask) t = time1;                             // Bit high duration
    while(((c = _getCycleCount()) - startTime) < period); // Wait for bit start

    GPIO_REG_WRITE(GPIO_OUT_W1TS_ADDRESS, pinMask);       // Set high

    startTime = c;                                        // Save start time
    while(((c = _getCycleCount()) - startTime) < t);      // Wait high duration

    GPIO_REG_WRITE(GPIO_OUT_W1TC_ADDRESS, pinMask);       // Set low

    if(!(mask >>= 1)) {                                   // Next bit/byte
      if(p >= end) break;
      pix  = *p++;
      mask = 0x80;
    }
  }
  while((_getCycleCount() - startTime) < period); // Wait for last bit
}


void pinMode(uint16_t pin, uint8_t dir)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = dir;
    //bit mask of the pins that you want to set,e.g.GPIO15/16
    io_conf.pin_bit_mask = BIT(pin);
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    gpio_set_level(pin, 0);
}

void digitalWrite(uint16_t pin, uint8_t level)
{
    gpio_set_level(pin, level);
}


static void microsCallback(void *arg)
{
    microsSeconds++;
}

void initMicros(void)
{
    hw_timer_init(microsCallback, NULL);
    hw_timer_alarm_us(1000000, true);
}

uint32_t micros(void)
{
    return ((microsSeconds * 1000000) + ((5000000 - hw_timer_get_count_data()) / 5));
}


void noInterrupts(void)
{
    portDISABLE_INTERRUPTS();

    do {
        REG_WRITE(INT_ENA_WDEV, WDEV_TSF0_REACH_INT);
    } while(REG_READ(INT_ENA_WDEV) != WDEV_TSF0_REACH_INT);
}

void interrupts(void)
{
    extern uint32_t WDEV_INTEREST_EVENT;
    REG_WRITE(INT_ENA_WDEV, WDEV_INTEREST_EVENT);

    portENABLE_INTERRUPTS();
}
