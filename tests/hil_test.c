#include <stdio.h>
#include "../src/gpio_ctrl.h"

// ---------------------------------------------------------
// MACRO KIỂM THỬ: Tự động so sánh và in báo cáo chi tiết
// ---------------------------------------------------------
#define EXPECT_EQ(actual, expected, msg, fail_counter) \
    do { \
        if ((actual) != (expected)) { \
            printf("  [FAIL] %s\n", msg); \
            printf("         -> Ky vong (Expected): %d\n", expected); \
            printf("         -> Thuc te (Actual)  : %d\n", actual); \
            (*fail_counter)++; \
        } else { \
            printf("  [ OK ] %s\n", msg); \
        } \
    } while(0)

// ---------------------------------------------------------
// TESTCASE T5: Tham số sai không crash
// ---------------------------------------------------------
int test_T5() {
    printf("\n--- Chay T5: Kiem tra tham so sai (Khong crash) ---\n");
    int fails = 0;
    int res;

    // 1. Kiểm tra init chân không tồn tại
    res = gpio_init(99, GPIO_OUT);
    EXPECT_EQ(res, GPIO_ERR_PARAM, "gpio_init(99, OUT) phai bao loi PARAM", &fails);

    // 2. Kiểm tra init sai chiều
    res = gpio_init(17, 7);
    EXPECT_EQ(res, GPIO_ERR_PARAM, "gpio_init(17, 7) phai bao loi PARAM", &fails);

    // 3. Kiểm tra write sai mức logic
    // (Giả sử 17 đã được init hợp lệ trước đó, nhưng ta test hàm write)
    gpio_init(17, GPIO_OUT); 
    res = gpio_write(17, 2);
    EXPECT_EQ(res, GPIO_ERR_PARAM, "gpio_write(17, 2) phai bao loi PARAM", &fails);
    
    // 4. Giải phóng chân 17 để không ảnh hưởng test khác
    gpio_close(17);

    // Tổng kết T5
    if (fails == 0) {
        printf("[PASS] T5 hoan thanh. Khong co loi.\n");
    } else {
        printf("[FAIL] T5 that bai voi %d loi chi tiet nhu tren.\n", fails);
    }
    return fails;
}

// ---------------------------------------------------------
// TESTCASE T6: Giải phóng chân bằng gpio_close
// ---------------------------------------------------------
int test_T6() {
    printf("\n--- Chay T6: Kiem tra giai phong chan ---\n");
    int fails = 0;
    int res;

    // 1. Init và Write bình thường
    res = gpio_init(17, GPIO_OUT);
    EXPECT_EQ(res, GPIO_OK, "gpio_init(17) trong T6 phai tra ve OK", &fails);
    
    res = gpio_write(17, 1);
    EXPECT_EQ(res, GPIO_OK, "gpio_write(17, 1) trong T6 phai tra ve OK", &fails);

    // 2. Gọi hàm close
    res = gpio_close(17);
    EXPECT_EQ(res, GPIO_OK, "gpio_close(17) phai tra ve OK", &fails);

    // 3. Cố tình write vào chân đã close
    res = gpio_write(17, 1);
    EXPECT_EQ(res, GPIO_ERR_STATE, "gpio_write(17) sau khi close phai bao loi STATE", &fails);

    // Tổng kết T6
    if (fails == 0) {
        printf("[PASS] T6 hoan thanh. Khong co loi.\n");
    } else {
        printf("[FAIL] T6 that bai voi %d loi chi tiet.\n", fails);
    }
    return fails;
}

// ---------------------------------------------------------
// HÀM MAIN: Chạy bộ test và trả Exit Code cho GitHub Actions
// ---------------------------------------------------------
int main() {
    printf("==========================================\n");
    printf("BAT DAU CHAY HIL TEST TREN RASPBERRY PI 4\n");
    printf("==========================================\n");

    int total_fails = 0;
    
    // Gọi lần lượt các testcase
    total_fails += test_T5();
    total_fails += test_T6();

    printf("\n==========================================\n");
    if (total_fails > 0) {
        printf("RESULT: FAIL - Phat hien %d loi logic trong code!\n", total_fails);
        return 1; // Exit 1 sẽ làm GitHub Actions báo đỏ (Failed)
    }
    
    printf("RESULT: ALL PASS - Hoan hao!\n");
    return 0; // Exit 0 sẽ làm GitHub Actions báo xanh (Success)
}