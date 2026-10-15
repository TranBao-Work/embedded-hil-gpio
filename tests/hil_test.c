#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <gpiod.h>
#include "../src/gpio_ctrl.h"

#define CHIP_PATH "/dev/gpiochip0" // ĐỔI SỐ NÀY NẾU CHIP CỦA BẠN LÀ SỐ KHÁC (VD: gpiochip0)

// ---------------------------------------------------------
// MACRO VÀ BIẾN TOÀN CỤC CHO HỆ THỐNG TEST
// ---------------------------------------------------------
#define EXPECT_EQ(actual, expected, msg, fail_counter) \
    do { \
        if ((actual) != (expected)) { \
            printf("  [FAIL] %s\n         -> Ky vong: %d | Thuc te: %d\n", msg, expected, actual); \
            (*fail_counter)++; \
        } else { \
            printf("  [ OK ] %s\n", msg); \
        } \
    } while(0)

// Đối tượng libgpiod v2 để giám sát chân 27 độc lập với gpio_ctrl
static struct gpiod_line_request *req27 = NULL;

// Biến dùng cho luồng ngầm (T4)
volatile int edge_count = 0;
volatile int monitoring = 0;
volatile long long first_edge_time = 0;
volatile long long last_edge_time = 0;

// ---------------------------------------------------------
// CÁC HÀM TIỆN ÍCH HỖ TRỢ ĐO LƯỜNG (Chân 27)
// ---------------------------------------------------------
int init_monitor_pin() {
    struct gpiod_chip *chip = gpiod_chip_open(CHIP_PATH);
    if (!chip) return -1;

    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
    
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offset = 27; // Chân vật lý 13
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);

    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "hil_monitor");

    req27 = gpiod_chip_request_lines(chip, req_cfg, line_cfg);

    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);

    return req27 ? 0 : -1;
}

int read_monitor_pin() {
    if (!req27) return -1;
    int val = gpiod_line_request_get_value(req27, 27);
    return (val == GPIOD_LINE_VALUE_ACTIVE) ? 1 : 0;
}

// Helper cho T0 (Viết mức logic trực tiếp không qua gpio_ctrl)
void direct_write_17(int level) {
    struct gpiod_chip *chip = gpiod_chip_open(CHIP_PATH);
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, level ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
    
    struct gpiod_line_config *line_cfg = gpiod_line_config_new();
    unsigned int offset = 17;
    gpiod_line_config_add_line_settings(line_cfg, &offset, 1, settings);
    
    struct gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "hil_direct_t0");
    
    struct gpiod_line_request *req = gpiod_chip_request_lines(chip, req_cfg, line_cfg);
    if (req) gpiod_line_request_release(req);
    
    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);
}

// ---------------------------------------------------------
// TESTCASES T0 -> T6
// ---------------------------------------------------------

int test_T0() {
    printf("\n--- Chay T0: Preflight kiem tra phan cung loopback ---\n");
    if (init_monitor_pin() != 0) {
        printf("  [FATAL] Khong the mo chan 27 (Monitor). Loi ha tang!\n");
        exit(2);
    }

    direct_write_17(1);
    usleep(10000);
    int val1 = read_monitor_pin();

    direct_write_17(0);
    usleep(10000);
    int val2 = read_monitor_pin();

    if (val1 == 1 && val2 == 0) {
        printf("  [PASS] T0 Preflight loopback 17->27 (Muc 1 va 0 deu thong).\n");
        return 0;
    } else {
        printf("  [FAIL] T0 that bai (Doc %d, %d). Vui long kiem tra lai dien tro/day cam!\n", val1, val2);
        exit(2); // Dừng hẳn CI/CD, trả exit code 2 (Lỗi hạ tầng)
    }
}

int test_T1() {
    printf("\n--- Chay T1: gpio_write muc cao ---\n");
    int fails = 0;
    EXPECT_EQ(gpio_init(17, GPIO_OUT), GPIO_OK, "Init chan 17 OUT", &fails);
    EXPECT_EQ(gpio_write(17, 1), GPIO_OK, "Ghi muc 1 ra chan 17", &fails);
    usleep(10000);
    EXPECT_EQ(read_monitor_pin(), 1, "Doc chan 27 phai ra muc 1", &fails);
    return fails;
}

int test_T2() {
    printf("\n--- Chay T2: gpio_write muc thap ---\n");
    int fails = 0;
    EXPECT_EQ(gpio_write(17, 0), GPIO_OK, "Ghi muc 0 ra chan 17", &fails);
    usleep(10000);
    EXPECT_EQ(read_monitor_pin(), 0, "Doc chan 27 phai ra muc 0", &fails);
    return fails;
}

int test_T3() {
    printf("\n--- Chay T3: Dao muc lien tuc 100 lan ---\n");
    int fails = 0, matches = 0;
    for (int i = 0; i < 100; i++) {
        int expected = i % 2;
        gpio_write(17, expected);
        usleep(5000); // Cho 5ms theo dac ta
        if (read_monitor_pin() == expected) matches++;
    }
    EXPECT_EQ(matches, 100, "100 lan dao muc phai dung 100 lan", &fails);
    return fails;
}

// Luồng giám sát xung cho T4
void* monitor_thread_func(void* arg) {
    int last_val = read_monitor_pin();
    while (monitoring) {
        int val = read_monitor_pin();
        if (val == 1 && last_val == 0) { // Phat hien canh len
            edge_count++;
            struct timeval tv;
            gettimeofday(&tv, NULL);
            long long ms = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
            if (edge_count == 1) first_edge_time = ms;
            last_edge_time = ms;
        }
        last_val = val;
        usleep(1000); // Quet 1ms/lan
    }
    return NULL;
}

int test_T4() {
    printf("\n--- Chay T4: Phat xung gpio_pulse ---\n");
    int fails = 0;
    gpio_write(17, 0); // Khoi diem muc thap
    
    edge_count = 0;
    monitoring = 1;
    pthread_t tid;
    pthread_create(&tid, NULL, monitor_thread_func, NULL);

    usleep(10000);

    // Phat 10 xung, chu ky 100ms
    EXPECT_EQ(gpio_pulse(17, 10, 100), GPIO_OK, "Lenh phat xung phai tra ve OK", &fails);

    usleep(50000); // Cho 50ms sau khi phat xung
    monitoring = 0;
    pthread_join(tid, NULL);

    EXPECT_EQ(edge_count, 10, "So canh len dem duoc phai la 10", &fails);
    EXPECT_EQ(read_monitor_pin(), 0, "Xung phai ket thuc o muc 0", &fails);

    if (edge_count > 1) {
        long long total_time = last_edge_time - first_edge_time;
        long long avg_period = total_time / (edge_count - 1);
        printf("  [INFO] Chu ky trung binh do duoc: %lld ms\n", avg_period);
        int in_range = (avg_period >= 80 && avg_period <= 120) ? 1 : 0;
        EXPECT_EQ(in_range, 1, "Chu ky phai nam trong dung sai 80-120ms", &fails);
    }
    
    gpio_close(17); // Don dep cho T4
    return fails;
}

int test_T5() {
    printf("\n--- Chay T5: Kiem tra tham so sai (Khong crash) ---\n");
    int fails = 0;
    EXPECT_EQ(gpio_init(99, GPIO_OUT), GPIO_ERR_PARAM, "init(99) bao loi PARAM", &fails);
    EXPECT_EQ(gpio_init(17, 7), GPIO_ERR_PARAM, "init(17, 7) bao loi PARAM", &fails);
    
    gpio_init(17, GPIO_OUT); 
    EXPECT_EQ(gpio_write(17, 2), GPIO_ERR_PARAM, "write muc 2 bao loi PARAM", &fails);
    gpio_close(17);
    
    return fails;
}

int test_T6() {
    printf("\n--- Chay T6: Kiem tra giai phong chan ---\n");
    int fails = 0;
    EXPECT_EQ(gpio_init(17, GPIO_OUT), GPIO_OK, "init chan 17", &fails);
    EXPECT_EQ(gpio_write(17, 1), GPIO_OK, "write muc 1", &fails);
    EXPECT_EQ(gpio_close(17), GPIO_OK, "close chan 17", &fails);
    EXPECT_EQ(gpio_write(17, 1), GPIO_ERR_STATE, "write sau khi close phai bao loi STATE", &fails);
    
    return fails;
}

// ---------------------------------------------------------
// HÀM MAIN
// ---------------------------------------------------------
int main() {
    printf("====================================================\n");
    printf("   BAT DAU HIL TEST KHEP KIN (LOOPBACK 17 -> 27)    \n");
    printf("====================================================\n");

    int total_fails = 0;
    
    // T0 - Neu failed se thoat (Exit 2) luon tu ben trong ham
    test_T0(); 
    
    // Neu qua duoc T0, chay cac testcase cham diem
    total_fails += test_T1();
    total_fails += test_T2();
    total_fails += test_T3();
    total_fails += test_T4();
    total_fails += test_T5();
    total_fails += test_T6();

    // Giai phong tai nguyen monitor T0
    if (req27) gpiod_line_request_release(req27);

    printf("\n====================================================\n");
    if (total_fails > 0) {
        printf("RESULT: %d LOI PHAT HIEN! -> EXIT 1 (CHAN MERGE)\n", total_fails);
        return 1;
    }
    
    printf("RESULT: ALL PASS! -> EXIT 0 (DUOC PHEP MERGE)\n");
    return 0;
}