/**
 * Pico Logic analyzer
 * -- this is just a very simple test to check gpio_get_all
 **/

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/stdio/driver.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/structs/bus_ctrl.h"
#include "logic.pio.h"


void print_binary(uint32_t num) {
    uint32_t left_mask = 0x80000000;
    for (int i=0; i<32; i++) {
        if (num & left_mask)
            printf("1");
        else
            printf("0");
        num <<= 1;
    }
}

int main() {
    stdio_init_all();
    uint32_t gpio_current = 0;
    uint32_t gpio_last = 0;
    while (true) {
        gpio_current = gpio_get_all();
        if (gpio_current != gpio_last) {
            print_binary(gpio_current);
            printf("\n");
        }
        gpio_last = gpio_current;
    }
}
