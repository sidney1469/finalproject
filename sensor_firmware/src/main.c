#include <zephyr/sys/printk.h>
#include "ble_transfer.h"
#include "gps_driver.h"

int main(void)
{
    char* msg = "TEST";
    send_over_nus(msg);
    return 0;
}