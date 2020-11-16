//
// Created by piotr on 16.11.2020.
//

#ifndef USB_TESTS_UART_H
#define USB_TESTS_UART_H

#include <unistd.h>

void uart_puts(char *);
void uart_putln(char *);
int _write(int, const char *, ssize_t);

#endif