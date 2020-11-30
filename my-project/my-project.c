// #include "api.h"
// #include "api-asm.h"

#ifndef STM32F3
#define STM32F3
#endif

// #include "libopencm3/usb/hid.h"
#include <stdio.h>
#include <errno.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "setup.h"
#include "uart.h"
#include "systick.h"
#include "usb.h"


int main(void) {
    printf("Setting up clock...\n");
	clock_setup();
    printf("OK\n");
    printf("errno: %d\n", errno);
    printf("Setting up systick...\n");
    systick_setup();
    printf("OK\n");
    printf("errno: %d\n", errno);
    printf("Setting up USART...\n");
    usart_setup();
    printf("OK\n");
    printf("errno: %d\n", errno);
    printf("Setting up interrupts...\n");
    interrupts_setup();
    printf("OK\n");
    printf("errno: %d\n", errno);
    printf("Setting up USB...\n");
    usb_setup();
    printf("OK\n");
    printf("errno: %d\n", errno);

    // Now, let's forever toggle this LED back and forth
    /*while (1) {
        printf("[%lld] ON\n", millis());
        gpio_toggle(GPIOA, GPIO5);
        delay(1000);
        printf("[%lld] OFF\n", millis());
        gpio_toggle(GPIOA, GPIO5);
        delay(1000);
    }*/

}
