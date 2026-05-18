#ifndef BLE_TRANSFER_H
#define BLE_TRANSFER_H

#include <stddef.h>
#include <zephyr/kernel.h>

#define NUS_MAX_DATA_LEN 64

struct bt_data_received {
    size_t data_len;
    int8_t data_buffer[NUS_MAX_DATA_LEN];
};

extern struct k_msgq bt_data_msgq;

void ble_receive_thread(void *a, void *b, void *c);

#endif /* BLE_TRANSFER_H */

