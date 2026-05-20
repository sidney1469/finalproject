#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "ble_central.h"
#include "protocol.h"
#include "minheap.h"
#include <zephyr/data/json.h>

#define ACTUATOR_ADDR "C1:C4:D7:FA:FB:15" // Sidney's
#define SENSOR_ADDR                                                                                \
    "E9:83:9E:1B:4B:0F" // Xander's
                        // "C5:9A:5F:B3:A9:65" // Fiachra's

#define CONTROL_STACK_SIZE 2048
#define RECEIVER_STACK_SIZE 4096
#define SENDER_STACK_SIZE   2048

K_THREAD_STACK_DEFINE(control_stack_area, CONTROL_STACK_SIZE);
K_THREAD_STACK_DEFINE(receiver_stack_area, RECEIVER_STACK_SIZE);
K_THREAD_STACK_DEFINE(sender_stack_area, SENDER_STACK_SIZE);

#define CONTROL_PRIORITY 9
#define RECEIVER_PRIORITY 5
#define SENDER_PRIORITY 7

struct k_thread control_thread_data;
struct k_thread receiver_thread_data;
struct k_thread sender_thread_data;

struct bt_data_received {
    size_t data_len;
    uint8_t data_buffer[256];
};

K_MSGQ_DEFINE(sensor_msgq, sizeof(struct bt_data_received), 10, 4);

void my_receive_callback(const void *data, uint16_t len)
{
    printk("callback called, len=%d\n", len);  // add this
    struct bt_data_received msg;
    msg.data_len = len;
    memcpy(msg.data_buffer, data, len);
    k_msgq_put(&sensor_msgq, &msg, K_NO_WAIT);
  
  
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
        received_data.plane.ts[0] == '\0') {
        printk("No active plane data, skipping heap insert\n");
        return;
    }

    int rc = insert_into_heap(received_data.gps, received_data.plane);

    if (rc) {
        printk("insert_into_heap failed: %d\n", rc);
        return;
    }
    convert_heap_to_string();
}

void init_bluetooth(void)
{
    ble_add(SENSOR_ADDR, my_receive_callback, false);
    ble_add(ACTUATOR_ADDR, NULL, true);

    int err = ble_start();
    if (err) {
        printk("BLE client start failed (err %d)\n", err);
        return;
    }

    printk("BLE client started\n");
    return;
}

void control_thread(void *a, void *b, void *c)
{
    while (1) {
        k_sleep(K_FOREVER);
    }
}

void receiver_thread(void *a, void *b, void *c)
{
    printk("Receiver thread started\n");
    struct bt_data_received msg;

    while (1) {
        k_msgq_get(&sensor_msgq, &msg, K_FOREVER);
        char buf[257];
        memcpy(buf, msg.data_buffer, msg.data_len);
        buf[msg.data_len] = '\0';
        printk("%s\n", buf);
    }
}

void sender_thread(void *a, void *b, void *c)
{
    while (1) {
        k_sleep(K_SECONDS(1));
        int err = ble_send(ACTUATOR_ADDR, "hello", strlen("hello"));
        if (err == -ENODEV) {
            printk("Sender not ready yet\n");
        } else if (err) {
            printk("Send failed (err %d)\n", err);
        } else {
            printk("Sent: hello\n");
        }
    }
}

int main(void)
{
    init_bluetooth();

    k_tid_t control_tid = k_thread_create(&control_thread_data, control_stack_area,
                                  K_THREAD_STACK_SIZEOF(control_stack_area), control_thread, NULL,
                                  NULL, NULL, CONTROL_PRIORITY, 0, K_NO_WAIT);

    if (!control_tid) {
        printk("Failed to create control thread!\n");
    }

    k_tid_t receiver_tid = k_thread_create(&receiver_thread_data, receiver_stack_area,
                                  K_THREAD_STACK_SIZEOF(receiver_stack_area), receiver_thread, NULL,
                                  NULL, NULL, RECEIVER_PRIORITY, 0, K_NO_WAIT);

    if (!receiver_tid) {
        printk("Failed to create receiver thread!\n");
    }

    k_tid_t sender_tid = k_thread_create(&sender_thread_data, sender_stack_area,
                                  K_THREAD_STACK_SIZEOF(sender_stack_area), sender_thread, NULL,
                                  NULL, NULL, SENDER_PRIORITY, 0, K_NO_WAIT);

    if (!sender_tid) {
        printk("Failed to create sender thread!\n");
    }

    while(1) {
        k_sleep(K_FOREVER);
    }

    return 0;
}
