#include <stdio.h>
#include "../src/gpio_ctrl.h"

int test_T5() {
    printf("Chay T5: Kiem tra tham so sai...\n");
    
    // Đưa vào 1 chân không tồn tại (chân 99)
    int res1 = gpio_init(99, GPIO_OUT);
    if (res1 != GPIO_ERR_PARAM) {
        printf("[FAIL] T5: Loi - gpio_init(99) khong tra ve GPIO_ERR_PARAM\n");
        return 1; // Báo lỗi
    }

    // Đưa vào chiều không hợp lệ (7)
    int res2 = gpio_init(17, 7);
    if (res2 != GPIO_ERR_PARAM) {
        printf("[FAIL] T5: Loi - gpio_init(17, 7) khong tra ve GPIO_ERR_PARAM\n");
        return 1; // Báo lỗi
    }

    printf("[PASS] T5 Tham so sai khong crash\n");
    return 0; // Thành công
}

int main() {
    int fail_count = 0;
    
    // Gọi thử T5
    fail_count += test_T5();
    
    // Có thể viết thêm test_T6() và gọi ở đây
    
    if (fail_count > 0) {
        printf("RESULT: FAIL\n");
        return 1; // Exit code 1 báo cho GitHub Runner biết là Test Fail
    }
    
    printf("RESULT: ALL PASS\n");
    return 0; // Exit code 0 báo GitHub Runner là Pass
}