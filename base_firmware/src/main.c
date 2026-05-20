#include <zephyr/sys/printk.h>
#include "ble_central.h"
#include "protocol.h"
#include "minheap.h"
#include <zephyr/data/json.h>

void my_receive_callback(const void *data, uint16_t len)
{
    sensor_message_t received_data;
    char buf[len + 1];
    memcpy(buf, data, len);
    buf[len] = '\0';
    printk("%s\n", buf);
    int64_t parsed = json_obj_parse(buf, len, sensor_message_descr,
                                sensor_message_descr_len, &received_data);

    if (parsed < 0) {
        printk("json parse failed: %lld\n", parsed);
        return;
    }

    /* Optional: require that top-level "plane" object decoded (bit1) */
    if ((parsed & 0x2) == 0) {
        printk("No plane object in JSON\n");
        return;
    }

    /* Semantic guard: plane present but empty/default payload */
    if (received_data.plane.icao[0] == '\0' ||
        received_data.plane.timestamp[0] == '\0') {
        printk("No active plane data, skipping heap insert\n");
        return;
    }

    int rc = insert_into_heap(
        received_data.plane.icao,
        (float)received_data.plane.lon,
        (float)received_data.plane.lat,
        (float)received_data.gps.longitude,
        (float)received_data.gps.latitude
    );

    if (rc) {
        printk("insert_into_heap failed: %d\n", rc);
        return;
    }
    convert_heap_to_string();
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
