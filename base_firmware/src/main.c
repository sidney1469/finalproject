#include <zephyr/sys/printk.h>
#include "ble_central.h"
#include "protocol.h"
#include <zephyr/data/json.h>

void my_receive_callback(const void *data, uint16_t len)
{
    int err;
    sensor_message_t received_data;
    char buf[len + 1];
    memcpy(buf, data, len);
    buf[len] = '\0';
    printk("%s\n", buf);
    err = json_obj_parse(buf, strlen(buf), sensor_message_descr, sensor_message_descr_len, &received_data);
    printk("%f\n", received_data.gps.latitude);
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
