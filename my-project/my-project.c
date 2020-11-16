// #include "api.h"
// #include "api-asm.h"

#ifndef STM32F3
#define STM32F3
#endif

// #include "libopencm3/usb/hid.h"
#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/systick.h>

#include "setup.h"

void sys_tick_handler(void);

static volatile uint64_t _millis = 0;

static void systick_setup(void) {
    // Set the systick clock source to our main clock
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    // Clear the Current Value Register so that we start at 0
    // To jest ten reload register?
    // Cały kod źródłowy tego: STK_CVR = 0;
    systick_clear();
    // In order to trigger an interrupt every millisecond, we can set the reload
    // value to be the speed of the processor / 1000 - 1
    systick_set_reload(rcc_ahb_frequency / 1000 - 1);
    // Enable interrupts from the system tick clock
    systick_interrupt_enable();
    // Enable the system tick counter
    systick_counter_enable();
}

uint64_t millis(void) {
    return _millis;
}

void sys_tick_handler(void) {
    // Increment our monotonic clock
    _millis++;
}

void delay(uint64_t duration) {
    const uint64_t until = millis() + duration;
    while (millis() < until);
}

int main(void) {
	clock_setup();

	// Our test LED is connected to Port A pin 11, so let's set it as output
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);

    systick_setup();

    // Now, let's forever toggle this LED back and forth
    while (1) {
        gpio_toggle(GPIOA, GPIO5);
        delay(1000);
        gpio_toggle(GPIOA, GPIO5);
        delay(1000);
    }

}
