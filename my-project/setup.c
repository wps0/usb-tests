//
// Created by piotr on 16.11.2020.
//

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>

#include <stdio.h>
#include <stddef.h>

#include "setup.h"

void clock_setup(void) {
    rcc_clock_setup_hsi(&rcc_hsi_configs[RCC_CLOCK_HSI_64MHZ]);

    /* Enable GPIOA clock */
    rcc_periph_clock_enable(RCC_GPIOA);
    // Enable the clock for USART2
    rcc_periph_clock_enable(RCC_USART2);
}

void usart_setup() {
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


void uart_puts(char *str) {
    while (*str) {
        usart_send_blocking(USART2, *str);
        str++;
    }
}

void uart_putln(char *str) {
    uart_puts(str);
    uart_puts("\r\n");
}