#include <zephyr/sys/printk.h>
#include "ble_client.h"

int main(void)
{
    int err = ble_client_start();
    if (err) {
        printk("BLE client start failed (err %d)\n", err);
        return 0;
    }

    printk("BLE client started\n");
    return 0;
}
