#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/nus.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/types.h>
#include <zephyr/sys/uuid.h>

#include "ble_central.h"

#define MAX_CONNECTIONS 2

typedef void (*nus_data_cb_t)(const void *data, uint16_t len);

typedef struct {
    bt_addr_le_t target_addr;
    struct bt_conn *conn;
    struct bt_uuid_128 discover_uuid;
    struct bt_uuid_16 discover_ccc_uuid;
    struct bt_gatt_discover_params discover_params;
    struct bt_gatt_subscribe_params subscribe_params;
    struct bt_gatt_exchange_params mtu_params;
    nus_data_cb_t data_cb;
    bool in_use;
    uint16_t rx_handle;
    bool is_sender;
} ble_ctx_t;

static ble_ctx_t clients[MAX_CONNECTIONS];
static uint8_t volatile conn_count;

/* Nordic UART Service UUIDs */
static struct bt_uuid_128 nus_svc_uuid = BT_UUID_INIT_128(BT_UUID_NUS_SRV_VAL);
static struct bt_uuid_128 nus_tx_uuid = BT_UUID_INIT_128(BT_UUID_NUS_TX_CHAR_VAL);
static struct bt_uuid_128 nus_rx_uuid = BT_UUID_INIT_128(BT_UUID_NUS_RX_CHAR_VAL);

static void start_scan(void);

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
                         struct net_buf_simple *ad)
{
    char addr_str[BT_ADDR_LE_STR_LEN];
    int err;

    if (type != BT_GAP_ADV_TYPE_ADV_IND && type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        return;
    }

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

    /* check if this address matches any registered client slot */
    ble_ctx_t *ctx = NULL;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (clients[i].in_use && clients[i].conn == NULL) {
            char target_str[BT_ADDR_LE_STR_LEN];
            bt_addr_le_to_str(&clients[i].target_addr, target_str, sizeof(target_str));
            if (strncmp(addr_str, target_str, strlen(target_str)) == 0) {
                ctx = &clients[i];
                break;
            }
        }
    }

    if (!ctx) {
        return;
    }

    if (bt_le_scan_stop()) {
        return;
    }

    err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT, &ctx->conn);
    if (err) {
        printk("Create conn to %s failed (%d)\n", addr_str, err);
        start_scan();
    }
}

static uint8_t notify_func(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
                           const void *data, uint16_t length)
{
    ble_ctx_t *ctx = CONTAINER_OF(params, ble_ctx_t, subscribe_params);

    if (!data) {
        printk("[UNSUBSCRIBED]\n");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    if (ctx->data_cb) {
        ctx->data_cb(data, length);
    }

    return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params)
{
    ble_ctx_t *ctx = CONTAINER_OF(params, ble_ctx_t, discover_params);

    int err;

    if (!attr) {
        printk("Discover complete\n");
        (void)memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    if (!bt_uuid_cmp(ctx->discover_params.uuid, &nus_svc_uuid.uuid)) {

        if (ctx->is_sender) {
            memcpy(&ctx->discover_uuid, &nus_rx_uuid.uuid, sizeof(ctx->discover_uuid));
        } else {
            memcpy(&ctx->discover_uuid, &nus_tx_uuid.uuid, sizeof(ctx->discover_uuid));
        }

        ctx->discover_params.uuid = &ctx->discover_uuid.uuid;
        ctx->discover_params.start_handle = attr->handle + 1;
        ctx->discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

        err = bt_gatt_discover(conn, &ctx->discover_params);
        if (err) {
            printk("Discover failed (err %d)\n", err);
        }
    } else if (!bt_uuid_cmp(ctx->discover_params.uuid, &nus_rx_uuid.uuid)) {
        ctx->rx_handle = bt_gatt_attr_value_handle(attr);
        printk("[SENDER READY] rx_handle = %u\n", ctx->rx_handle);
        return BT_GATT_ITER_STOP;
    } else if (!bt_uuid_cmp(ctx->discover_params.uuid, &nus_tx_uuid.uuid)) {
        // found TX; find CCC
        memcpy(&ctx->discover_ccc_uuid.uuid, BT_UUID_GATT_CCC, sizeof(ctx->discover_ccc_uuid));
        ctx->discover_params.uuid = &ctx->discover_ccc_uuid.uuid;
        ctx->discover_params.start_handle = bt_gatt_attr_value_handle(attr) + 1;
        ctx->discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
        ctx->subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

        err = bt_gatt_discover(conn, &ctx->discover_params);
        if (err) {
            printk("Discover failed (err %d)\n", err);
        }
    } else {
        ctx->subscribe_params.notify = notify_func;
        ctx->subscribe_params.value = BT_GATT_CCC_NOTIFY;
        ctx->subscribe_params.ccc_handle = attr->handle;

        err = bt_gatt_subscribe(conn, &ctx->subscribe_params);
        if (err && err != -EALREADY) {
            printk("Subscribe failed (err %d)\n", err);
        } else {
            printk("[SUBSCRIBED]\n");
        }

        return BT_GATT_ITER_STOP;
    }

    return BT_GATT_ITER_STOP;
}

static void start_scan(void)
{
    int err;
    err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
    if (err) {
        printk("Scanning failed to start (err %d)\n", err);
        return;
    }
}

static ble_ctx_t *get_ctx_by_conn(struct bt_conn *conn)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (clients[i].conn == conn) {
            return &clients[i];
        }
    }
    return NULL;
}

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
                            struct bt_gatt_exchange_params *params)
{
    printk("MTU exchange %s (%u)\n", err == 0U ? "successful" : "failed", bt_gatt_get_mtu(conn));
}

static int mtu_exchange(struct bt_conn *conn, ble_ctx_t *ctx)
{

    ctx->mtu_params.func = mtu_exchange_cb;
    int err = bt_gatt_exchange_mtu(conn, &ctx->mtu_params);

    if (err) {
        printk("MTU exchange failed (err %d)\n", err);
    }

    return err;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    ble_ctx_t *ctx = get_ctx_by_conn(conn);
    if (!ctx) {
        return;
    }

    if (err) {
        printk("Failed to connect to %s %u %s\n", addr, err, bt_hci_err_to_str(err));
        bt_conn_unref(ctx->conn);
        ctx->conn = NULL;
        start_scan();
        return;
    }

    printk("Connected: %s\n", addr);
    conn_count++;

    mtu_exchange(conn, ctx);

    memcpy(&ctx->discover_uuid, BT_UUID_NUS_SERVICE, sizeof(ctx->discover_uuid));
    ctx->discover_params.uuid = &ctx->discover_uuid.uuid;
    ctx->discover_params.func = discover_func;
    ctx->discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    ctx->discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    ctx->discover_params.type = BT_GATT_DISCOVER_PRIMARY;

    err = bt_gatt_discover(conn, &ctx->discover_params);
    if (err) {
        printk("Discover failed(err %d)\n", err);
        return;
    }

    if (conn_count < MAX_CONNECTIONS) {
        start_scan();
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    ble_ctx_t *ctx = get_ctx_by_conn(conn);

    if (!ctx) {
        return;
    }

    printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

    bt_conn_unref(ctx->conn);
    ctx->conn = NULL;
    conn_count--;

    start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

int ble_add(const char *addr_str, nus_data_cb_t cb, bool is_sender)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!clients[i].in_use) {
            bt_addr_le_from_str(addr_str, "random", &clients[i].target_addr);
            clients[i].data_cb = cb;
            clients[i].in_use = true;
            clients[i].conn = NULL;
            clients[i].is_sender = is_sender;
            clients[i].rx_handle = 0;
            return 0;
        }
    }
    return -ENOMEM;
}

int ble_start(void)
{
    int err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return err;
    }

    printk("Bluetooth initialized\n");
    start_scan();
    return 0;
}

int ble_send(const char *addr_str, const void *data, uint16_t len)
{
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!clients[i].in_use || !clients[i].conn || clients[i].rx_handle == 0) {
            continue;
        }
        char target_str[BT_ADDR_LE_STR_LEN];
        bt_addr_le_to_str(&clients[i].target_addr, target_str, sizeof(target_str));
        if (strncmp(target_str, addr_str, strlen(addr_str)) == 0) {
            return bt_gatt_write_without_response(clients[i].conn, clients[i].rx_handle, data, len,
                                                  false);
        }
    }
    return -ENODEV;
}
