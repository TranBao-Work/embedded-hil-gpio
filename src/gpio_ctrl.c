#include "gpio_ctrl.h"

int gpio_init(int pin, int dir) {
    return GPIO_OK;
}

int gpio_write(int pin, int level) {
    return GPIO_OK;
}

int gpio_pulse(int pin, int count, int period_ms) {
    return GPIO_OK;
}

int gpio_close(int pin) {
    return GPIO_OK;
}