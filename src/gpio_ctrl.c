#include "gpio_ctrl.h"
#include <gpiod.h>
#include <stdio.h>
#include <unistd.h> // dùng cho hàm usleep (tạo delay)

#define MAX_PINS 30
#define GPIO_CHIP_NAME "gpiochip4" // Chip GPIO tiêu chuẩn trên Raspberry Pi 4

// Biến toàn cục để lưu trạng thái của từng chân GPIO
static struct gpiod_chip *chip = NULL;
static struct gpiod_line *lines[MAX_PINS] = {NULL};

// Mảng lưu chiều của chân: -1 (chưa khởi tạo), 0 (IN), 1 (OUT)
static int pin_dirs[MAX_PINS] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

int gpio_init(int pin, int dir) {
    // 1. Kiểm tra tham số đầu vào (Pass kịch bản T5)
    if (pin < 2 || pin > 27) return GPIO_ERR_PARAM;
    if (dir != GPIO_IN && dir != GPIO_OUT) return GPIO_ERR_PARAM;

    // 2. Mở chip GPIO nếu chưa mở
    if (chip == NULL) {
        chip = gpiod_chip_open_by_name(GPIO_CHIP_NAME);
        if (chip == NULL) return GPIO_ERR_IO;
    }

    // 3. Giải phóng chân nếu đã được cấp phát trước đó
    if (lines[pin] != NULL) {
        gpiod_line_release(lines[pin]);
    }

    // 4. Lấy line tương ứng với chân
    lines[pin] = gpiod_chip_get_line(chip, pin);
    if (lines[pin] == NULL) return GPIO_ERR_IO;

    // 5. Cấu hình chiều (Input hoặc Output)
    int ret;
    if (dir == GPIO_OUT) {
        // Yêu cầu là Output, mặc định mức 0 theo đặc tả
        ret = gpiod_line_request_output(lines[pin], "gpio_ctrl", 0);
    } else {
        ret = gpiod_line_request_input(lines[pin], "gpio_ctrl");
    }

    if (ret < 0) {
        lines[pin] = NULL;
        return GPIO_ERR_IO;
    }

    pin_dirs[pin] = dir;
    return GPIO_OK;
}

int gpio_write(int pin, int level) {
    // 1. Kiểm tra tham số (Pass kịch bản T5)
    if (pin < 2 || pin > 27) return GPIO_ERR_PARAM;
    if (level != 0 && level != 1) return GPIO_ERR_PARAM;

    // 2. Kiểm tra trạng thái chân (Pass kịch bản T6)
    if (lines[pin] == NULL || pin_dirs[pin] != GPIO_OUT) {
        return GPIO_ERR_STATE;
    }

    // 3. Xuất mức logic
    int ret = gpiod_line_set_value(lines[pin], level);
    if (ret < 0) return GPIO_ERR_IO;

    return GPIO_OK;
}

int gpio_pulse(int pin, int count, int period_ms) {
    // 1. Kiểm tra tham số (Pass kịch bản T5)
    if (pin < 2 || pin > 27) return GPIO_ERR_PARAM;
    if (count < 1 || count > 1000) return GPIO_ERR_PARAM;
    if (period_ms < 2 || period_ms > 1000) return GPIO_ERR_PARAM;

    // 2. Kiểm tra trạng thái
    if (lines[pin] == NULL || pin_dirs[pin] != GPIO_OUT) {
        return GPIO_ERR_STATE;
    }

    // 3. Tính toán thời gian nửa chu kỳ (micro-giây)
    int half_period_us = (period_ms * 1000) / 2;

    // 4. Phát xung bằng vòng lặp
    for (int i = 0; i < count; i++) {
        gpiod_line_set_value(lines[pin], 1);
        usleep(half_period_us);
        gpiod_line_set_value(lines[pin], 0);
        usleep(half_period_us);
    }

    return GPIO_OK;
}

int gpio_close(int pin) {
    if (pin < 2 || pin > 27) return GPIO_ERR_PARAM;

    // Kiểm tra xem chân đã init chưa
    if (lines[pin] == NULL) {
        return GPIO_ERR_STATE;
    }

    // Yêu cầu chuyển về Input (an toàn phần cứng) rồi giải phóng line
    gpiod_line_request_input(lines[pin], "gpio_ctrl_close");
    gpiod_line_release(lines[pin]);
    
    // Đặt lại trạng thái để hàm write báo lỗi (Pass kịch bản T6)
    lines[pin] = NULL;
    pin_dirs[pin] = -1;

    return GPIO_OK;
}