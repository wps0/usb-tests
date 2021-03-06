//
// Created by piotr on 16.11.2020.
//

#ifndef STM32F3
#define STM32F3
#endif

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/systick.h>

#include <stdio.h>
#include <stddef.h>

#include "setup.h"
#include "systick.h"
#include "uart.h"

void clock_setup(void) {
    rcc_clock_setup_pll(&rcc_hse8mhz_configs[RCC_CLOCK_HSE8_72MHZ]);
    /* Enable GPIOA clock */
    rcc_periph_clock_enable(RCC_GPIOA);
    /* Enable USB clock */
    rcc_periph_clock_enable(RCC_USB); // chociaż i tak jest setupowany w usbd_init
    rcc_periph_clock_enable(RCC_GPIOC);
    /* Enable USART2 clock */
    rcc_periph_clock_enable(RCC_USART2);
}

void usart_setup(void) {
    // USART2 (AF7):
    //  PA3 - USART2_RX
    //  PA2 - USART2_TX
    //  PA1 - USART2_RTS
    //  PA0 - USART2_CTS

    // From the alternate function mapping from the datasheet for the STM32F303RE
    // board (Page 53, Table 14) we know the alternate function number.
    // In order to use the TX pin for the alternate function, we need to set the
    // mode to GPIO_MODE_AF (alternate function). We also do not need a pullup
    // or pulldown resistor on this pin, since the peripheral will handle
    // keeping the line high when nothing is being transmitted.
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);
    // Now that we have put the pin into alternate function mode, we need to
    // select which alternate function to use. PA2 can be used for several
    // alternate functions - Timer 2, USART2 TX, Timer 15.
    // Here, we want alternate function 7 (USART2_TX)
    gpio_set_af(GPIOA, GPIO_AF7, GPIO2);


    // Now that the pins are configured, we can configure the USART itself.
    // First, let's set the baud rate at 115200
    usart_set_baudrate(USART2, 115200);
    // The amount of data bits in single USART frame (L2 OSI?).
    usart_set_databits(USART2, 8);
    // Disable parity bit(s)
    usart_set_parity(USART2, USART_PARITY_NONE);
    // 1 stop bit
    usart_set_stopbits(USART2, USART_STOPBITS_1);
    // Unidirectional transmission
    usart_set_mode(USART2, USART_MODE_TX);
    // No flow control
    usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

    // Enable the peripheral
    usart_enable(USART2);

    // Optional extra - disable buffering on stdout.
    // Buffering doesn't save us any syscall overhead on embedded, and
    // can be the source of what seem like bugs.
    setbuf(stdout, NULL);
}

void systick_setup(void) {
    // Set the systick clock source to our main clock
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    // Clear the Current Value Register so that we start at 0
    systick_clear();
    // In order to trigger an interrupt every millisecond, we can set the reload
    // value to be the speed of the processor / 1000 - 1
    systick_set_reload(rcc_ahb_frequency / 1000 - 1);
    // Enable interrupts from the system tick clock
    systick_interrupt_enable();
    // Enable the system tick counter
    systick_counter_enable();
}

void interrupts_setup(void) {
    /* Enable USB RX interrupt */
    // nvic_enable_irq(NVIC_USB_HP_CAN1_TX_IRQ);
    // nvic_enable_irq(NVIC_USB_LP_CAN1_RX0_IRQ);
}