#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "ble_central.h"

#define ACTUATOR_ADDR "C1:C4:D7:FA:FB:15" // Sidney's
#define SENSOR_ADDR                                                                                \
    "E9:83:9E:1B:4B:0F" // Xander's
                        // "C5:9A:5F:B3:A9:65" // Fiachra's

void my_receive_callback(const void *data, uint16_t len)
{
    char buf[len + 1];
    memcpy(buf, data, len);
    buf[len] = '\0';
    printk("%s\n", buf);
}

int main(void)
{
    ble_add(SENSOR_ADDR, my_receive_callback, false);
    ble_add(ACTUATOR_ADDR, NULL, true);

    int err = ble_start();
    if (err) {
        printk("BLE client start failed (err %d)\n", err);
        return 0;
    }

    printk("BLE client started\n");

    while (1) {
        k_sleep(K_SECONDS(1));
        err = ble_send(ACTUATOR_ADDR, "hello", strlen("hello"));
        if (err == -ENODEV) {
            printk("Sender not ready yet\n");
        } else if (err) {
            printk("Send failed (err %d)\n", err);
        } else {
            printk("Sent: hello\n");
        }
    }
    return 0;
}
