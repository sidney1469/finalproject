#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H

#include <stdint.h>

typedef void (*nus_data_cb_t)(const void *data, uint16_t len);

int ble_add(const char *addr_str, nus_data_cb_t cb);
int ble_start(void);

#endif
