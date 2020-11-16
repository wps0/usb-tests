//
// Created by piotr on 16.11.2020.
//

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#include "setup.h"

void clock_setup(void) {
    rcc_clock_setup_hsi(&rcc_hsi_configs[RCC_CLOCK_HSI_64MHZ]);

    /* Enable GPIOA clock. */
    rcc_periph_clock_enable(RCC_GPIOA);
}

