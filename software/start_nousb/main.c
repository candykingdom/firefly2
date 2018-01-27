#include <atmel_start.h>
#include "examples/driver_examples.h"
#include "hal_gpio.h"

int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();



	/* Replace with your application code */
	while (1) {
    //SPI_0_example();
    ioport_toggle_pin_level(PORTA.4);
	}
}
