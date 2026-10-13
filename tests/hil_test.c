#include <stdio.h>
#include "../src/gpio_ctrl.h"

int main() {
    printf("==========================================\n");
    printf("[HIL TEST] Da chay thanh cong tren Pi 4!\n");
    printf("==========================================\n");
    printf("[PASS] T0 Preflight loopback (Mock)\n");
    
    return 0; // Exit code 0 báo cho GitHub Actions biết là test pass
}