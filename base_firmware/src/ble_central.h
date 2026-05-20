#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H

#include <stdint.h>
#include <stdbool.h>

typedef void (*nus_data_cb_t)(const void *data, uint16_t len);

int ble_add(const char *addr_str, nus_data_cb_t cb, bool is_sender);
int ble_send(const char *addr_str, const void *data, uint16_t len);
int ble_start(void);

#endif
