// #include "api.h"
// #include "api-asm.h"

#ifndef STM32F3
#define STM32F3
#endif

// #include "libopencm3/usb/hid.h"
#include <stdio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "setup.h"
#include "uart.h"
#include "systick.h"


int main(void) {
	clock_setup();

    systick_setup();
    usart_setup();

    // Now, let's forever toggle this LED back and forth
    while (1) {
        printf("[%lld] ON\n", millis());
        gpio_toggle(GPIOA, GPIO5);
        delay(1000);
        printf("[%lld] OFF\n", millis());
        gpio_toggle(GPIOA, GPIO5);
        delay(1000);
    }

}
