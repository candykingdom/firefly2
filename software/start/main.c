// 24mhz
#define F_CPU 24000000

#include <atmel_start.h>
#include <hri_nvmctrl_d11.h>
#include "hal_gpio.h"

#define PA04 GPIO(GPIO_PORTA, 4)
#define PA08 GPIO(GPIO_PORTA, 8)
#define PA28 GPIO(GPIO_PORTA, 28)

/**
 * Delays for an arbitrary amount of time.
 * TODO: tune this.
 */
void delay(uint32_t cycles) {
  for (uint32_t i = 0; i < cycles; i++) {
    asm volatile("nop\n\t");
  }
}

/**
 * Someday this will delay for 1ms.
 * TODO: tune this
 */
void delayMs(uint32_t ms) {
  for (uint32_t i = 0; i < ms; i++) {
    delay(1000 * 4);
  }
}

/**
 * Writes a 0 bit to the serial LEDs.
 */
void inline ledLow() {
  gpio_set_pin_level(PA28, true);
  asm volatile(
      "nop\n\t"
      "nop\n\t");
  gpio_set_pin_level(PA28, false);
  asm volatile(
      "nop\n\t"
      "b .+2\n\t"
      "b .+2\n\t"
      "b .+2\n\t"
      "b .+2\n\t"
      "b .+2\n\t"
      "b .+2\n\t"
      "b .+2\n\t");
}

/**
 * Writes a 1 bit to the serial LEDs.
 */
void inline ledHigh() {
  gpio_set_pin_level(PA28, true);
  asm volatile(
      "nop\n\t"
      "b .+2\n\t"
      "b .+2\n\t"
      "b .+2\n\t"
      "b .+2\n\t"
      "b .+2\n\t"
      "b .+2\n\t"
      "b .+2\n\t"
      "b .+2\n\t"
      "b .+2\n\t");
  gpio_set_pin_level(PA28, false);
}

/**
 * Writes data to the onboard WS2812B LEDs, of the form:  0xBBRRGG
 * The LEDs act as shift buffers, so successive calls to this function shift
 * the data to the next LED in the chain. When you're finished, delay for at
 * least 50uS (according to the spec, in practice this is much lower).
 * See this writeup on the timing for more info:
 * https://cpldcpu.wordpress.com/2014/01/14/light_ws2812-library-v2-0-part-i-understanding-the-ws2812/
 */
void inline writeLed(uint32_t data) {
  for (uint32_t mask = 0x800000; mask >= 1; mask >>= 1) {
    if (data & mask) {
      ledHigh();
    } else {
      ledLow();
    }
  }
}

int main(void) {
  /* Initializes MCU, drivers and middleware */
  atmel_start_init();
  PM->EXTCTRL.bit.SETDIS = true;

  PORT->Group[0].PINCFG[PIN_PA28].bit.PMUXEN = 0;
  gpio_set_pin_direction(PA04, GPIO_DIRECTION_OUT);
  gpio_set_pin_direction(PA08, GPIO_DIRECTION_OUT);
  gpio_set_pin_direction(PA28, GPIO_DIRECTION_OUT);

  while (1) {
    // Ramps the LEDs up and down.
    for (int i = 0; i < 256; i++) {
      writeLed(i);
      writeLed(i << 8);
      writeLed(i << 16);
      writeLed(i | (i << 8) | (i << 16));
      delayMs(10);
    }
    for (int i = 255; i >= 0; i--) {
      writeLed(i);
      writeLed(i << 8);
      writeLed(i << 16);
      writeLed(i | (i << 8) | (i << 16));
      delayMs(10);
    }
  }
}
