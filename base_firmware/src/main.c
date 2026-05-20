#include <zephyr/sys/printk.h>
#include "ble_central.h"

void my_receive_callback(const void *data, uint16_t len)
{
    char buf[len + 1];
    memcpy(buf, data, len);
    buf[len] = '\0';
    printk("%s\n", buf);
}

int main(void)
{
    ble_add("E9:83:9E:1B:4B:0F", my_receive_callback);

    int err = ble_start();
    if (err) {
        printk("BLE client start failed (err %d)\n", err);
        return 0;
    }

    printk("BLE client started\n");
    return 0;
}
