#ifndef GPIO_CTRL_H
#define GPIO_CTRL_H

#define GPIO_OK          0
#define GPIO_ERR_PARAM  -1   /* tham so khong hop le        */
#define GPIO_ERR_STATE  -2   /* chua init / sai chieu chan  */
#define GPIO_ERR_IO     -3   /* loi truy cap phan cung      */

#define GPIO_IN   0
#define GPIO_OUT  1
 
int gpio_init (int pin, int dir);
int gpio_write(int pin, int level);
int gpio_pulse(int pin, int count, int period_ms);
int gpio_close(int pin);

#endif