#include <errno.h>

#include <libopencm3/stm32/usart.h>

#include "uart.h"

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

int _write(int file, const char *ptr, ssize_t len) {
    // If the target file isn't stdout/stderr, then return an error
    // since we don't _actually_ support file handles
    if (file != STDOUT_FILENO && file != STDERR_FILENO) {
        // Set the errno code (requires errno.h)
        errno = EIO;
        return -1;
    }

    // Keep i defined outside the loop so we can return it
    int i;
    for (i = 0; i < len; i++) {
        // If we get a newline character, also be sure to send the carriage
        // return character first, otherwise the serial console may not
        // actually return to the left.
        if (ptr[i] == '\n') {
            usart_send_blocking(USART2, '\r');
        }

        // Write the character to send to the USART1 transmit buffer, and block
        // until it has been sent.
        usart_send_blocking(USART2, ptr[i]);
    }

    // Return the number of bytes we sent
    return i;
}