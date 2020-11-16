#include <stdint.h>

#include <libopencm3/cm3/systick.h>


void sys_tick_handler(void);

static volatile uint64_t _millis = 0;

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
