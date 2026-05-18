#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "ble_receive.h"

#define STACK_SIZE       2048
#define RECEIVE_PRIORITY 5

extern void ble_receive_thread(void *a, void *b, void *c);

K_THREAD_STACK_DEFINE(receive_stack, STACK_SIZE);


static struct k_thread ble_receive_thread_data;

int main(void)
{
    // Start Bluetooth central receive thread
    k_tid_t ble_receive_tid = k_thread_create(&ble_receive_thread_data, receive_stack, K_THREAD_STACK_SIZEOF(receive_stack),
                    ble_receive_thread, NULL, NULL, NULL, RECEIVE_PRIORITY, 0, K_NO_WAIT);

    if (!ble_receive_tid) {
        printk("Failed to create BLE receive thread!\n");
    }

    return 0;
}
