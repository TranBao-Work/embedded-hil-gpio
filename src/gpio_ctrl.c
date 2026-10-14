#include "gpio_ctrl.h"
#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>

#define MAX_PINS 30
#define CHIP_PATH "/dev/gpiochip4"

static struct gpiod_chip *chip = NULL;
static struct gpiod_line_request *requests[MAX_PINS] = {NULL};
static int pin_dirs[MAX_PINS] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

int gpio_init(int pin, int dir) {
    if (pin < 2 || pin > 27) return GPIO_ERR_PARAM;
    if (dir != GPIO_IN && dir != GPIO_OUT) return GPIO_ERR_PARAM;

    if (chip == NULL) {
        chip = gpiod_chip_open(CHIP_PATH);
        if (!chip) return GPIO_ERR_IO;
    }

    if (requests[pin] != NULL) {
        gpiod_line_request_release(requests[pin]);
        requests[pin] = NULL;
    }

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    if (dir == GPIO_OUT) {
        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
        gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);
    } else {
        gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    }

    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    
    // FIX: Ép kiểu sang unsigned int để tương thích hoàn toàn kiến trúc 64-bit ARM
    unsigned int offset = pin; 
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "gpio_ctrl");

    requests[pin] = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);

    if (!requests[pin]) return GPIO_ERR_IO;

    pin_dirs[pin] = dir;
    return GPIO_OK;
}

int gpio_write(int pin, int level) {
    if (pin < 2 || pin > 27) return GPIO_ERR_PARAM;
    if (level != 0 && level != 1) return GPIO_ERR_PARAM;
    if (requests[pin] == NULL || pin_dirs[pin] != GPIO_OUT) return GPIO_ERR_STATE;

    int val = (level == 1) ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;
    int ret = gpiod_line_request_set_value(requests[pin], pin, val);
    if (ret < 0) return GPIO_ERR_IO;

    return GPIO_OK;
}

int gpio_pulse(int pin, int count, int period_ms) {
    if (pin < 2 || pin > 27) return GPIO_ERR_PARAM;
    if (count < 1 || count > 1000) return GPIO_ERR_PARAM;
    if (period_ms < 2 || period_ms > 1000) return GPIO_ERR_PARAM;
    if (requests[pin] == NULL || pin_dirs[pin] != GPIO_OUT) return GPIO_ERR_STATE;

    int half_period_us = (period_ms * 1000) / 2;

    for (int i = 0; i < count; i++) {
        gpiod_line_request_set_value(requests[pin], pin, GPIOD_LINE_VALUE_ACTIVE);
        usleep(half_period_us);
        gpiod_line_request_set_value(requests[pin], pin, GPIOD_LINE_VALUE_INACTIVE);
        usleep(half_period_us);
    }

    return GPIO_OK;
}

int gpio_close(int pin) {
    if (pin < 2 || pin > 27) return GPIO_ERR_PARAM;
    if (requests[pin] == NULL) return GPIO_ERR_STATE;

    // FIX: Chỉ gọi release để trả tài nguyên, OS sẽ tự dọn dẹp phần cứng. Tránh Race Condition.
    gpiod_line_request_release(requests[pin]);
    requests[pin] = NULL;
    pin_dirs[pin] = -1;

    return GPIO_OK;
}