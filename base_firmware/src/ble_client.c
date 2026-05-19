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
#include <zephyr/sys/byteorder.h>
#include <zephyr/types.h>
#include <zephyr/sys/uuid.h>

#include "ble_client.h"

static void start_scan(void);

static struct bt_conn *default_conn;

// define the addresses of the possible mobile nodes
#define MOBILE_NODE_ADDR_MAL	"F4:4A:CE:71:27:25"
#define MOBILE_NODE_ADDR_XANDER "E9:83:9E:1B:4B:0F"
#define MOBILE_NODE_ADDR_DK 	"DC:56:43:F9:59:37"

/* Nordic UART Service UUID: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E */
#define BT_UUID_NUS_SERVICE_VAL \
    BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_NUS_SERVICE \
    BT_UUID_DECLARE_128(BT_UUID_NUS_SERVICE_VAL)

/* Nordic UART TX Characteristic (Central receives notifications from this): 6E400003... */
#define BT_UUID_NUS_TX_VAL \
    BT_UUID_128_ENCODE(0x6e400003, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_NUS_TX \
    BT_UUID_DECLARE_128(BT_UUID_NUS_TX_VAL)

// empty structs for later discovery and subscription
static struct bt_uuid_128 discover_uuid = BT_UUID_INIT_128(0);
static struct bt_uuid_16 discover_ccc_uuid = BT_UUID_INIT_16(0);

static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
             struct net_buf_simple *ad)
{
    char addr_str[BT_ADDR_LE_STR_LEN];
    int err;

    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

    if (default_conn) {
        return;
    }

    /* We're only interested in connectable events */
    if (type != BT_GAP_ADV_TYPE_ADV_IND &&
        type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
        return;
    }

    if (
        strncmp(addr_str, MOBILE_NODE_ADDR_XANDER, strlen(MOBILE_NODE_ADDR_XANDER)) &&
        strncmp(addr_str, MOBILE_NODE_ADDR_DK, strlen(MOBILE_NODE_ADDR_DK)) &&
        strncmp(addr_str, MOBILE_NODE_ADDR_MAL, strlen(MOBILE_NODE_ADDR_MAL))
    ) {
        return;
    }

    if (bt_le_scan_stop()) {
        return;
    }

    err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
                BT_LE_CONN_PARAM_DEFAULT, &default_conn);
    if (err) {
        printk("Create conn to %s failed (%d)\n", addr_str, err);
        start_scan();
    }
}

static uint8_t notify_func(struct bt_conn *conn,
               struct bt_gatt_subscribe_params *params,
               const void *data, uint16_t length)
{
    if (!data) {
        printk("[UNSUBSCRIBED]\n");
        params->value_handle = 0U;
        return BT_GATT_ITER_STOP;
    }

    char data_string[length + 1];
    memcpy(data_string, data, length);
    data_string[length] = '\0';

    printk("%s\n", data_string);

    return BT_GATT_ITER_CONTINUE;
}

static uint8_t discover_func(struct bt_conn *conn,
                 const struct bt_gatt_attr *attr,
                 struct bt_gatt_discover_params *params)
{
    int err;

    if (!attr) {
        printk("Discover complete\n");
        (void)memset(params, 0, sizeof(*params));
        return BT_GATT_ITER_STOP;
    }

    if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_NUS_SERVICE)) {
        // found NUS; find the tx characteristic
        memcpy(&discover_uuid, BT_UUID_NUS_TX, sizeof(discover_uuid));
        discover_params.uuid = &discover_uuid.uuid;
        discover_params.start_handle = attr->handle + 1;
        discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

        err = bt_gatt_discover(conn, &discover_params);
        if (err) {
            printk("Discover failed (err %d)\n", err);
        }
    } else if (!bt_uuid_cmp(discover_params.uuid,
                BT_UUID_NUS_TX)) {
        // found TX; find CCC
        memcpy(&discover_ccc_uuid.uuid, BT_UUID_GATT_CCC, sizeof(discover_ccc_uuid));
        discover_params.uuid = &discover_ccc_uuid.uuid;
        discover_params.start_handle = attr->handle + 2;
        discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
        subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

        err = bt_gatt_discover(conn, &discover_params);
        if (err) {
            printk("Discover failed (err %d)\n", err);
        }
    } else {
        subscribe_params.notify = notify_func;
        subscribe_params.value = BT_GATT_CCC_NOTIFY; // NUS uses noti's
        subscribe_params.ccc_handle = attr->handle;

        err = bt_gatt_subscribe(conn, &subscribe_params);
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

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
                struct bt_gatt_exchange_params *params)
{
    printk("%s: MTU exchange %s (%u)\n", __func__,
           err == 0U ? "successful" : "failed",
           bt_gatt_get_mtu(conn));
}

static struct bt_gatt_exchange_params mtu_exchange_params = {
    .func = mtu_exchange_cb
};

static int mtu_exchange(struct bt_conn *conn)
{
    int err;

    printk("%s: Current MTU = %u\n", __func__, bt_gatt_get_mtu(conn));

    printk("%s: Exchange MTU...\n", __func__);
    err = bt_gatt_exchange_mtu(conn, &mtu_exchange_params);
    if (err) {
        printk("%s: MTU exchange failed (err %d)", __func__, err);
    }

    return err;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        printk("Failed to connect to %s %u %s\n", addr, err,
                bt_hci_err_to_str(err));

        bt_conn_unref(default_conn);
        default_conn = NULL;

        start_scan();
        return;
    }

    if (conn != default_conn) {
        return;
    }

    printk("Connected: %s\n", addr);

    // exchange mtu info to increase uart send size
    (void)mtu_exchange(conn);

    // find the service and hand off to discover_func
    if (conn == default_conn) {
        memcpy(&discover_uuid, BT_UUID_NUS_SERVICE, sizeof(discover_uuid));
        discover_params.uuid = &discover_uuid.uuid;
        discover_params.func = discover_func;
        discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
        discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
        discover_params.type = BT_GATT_DISCOVER_PRIMARY;

        err = bt_gatt_discover(default_conn, &discover_params);
        if (err) {
            printk("Discover failed(err %d)\n", err);
            return;
        }
    }

}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    if (conn != default_conn) {
        return;
    }

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

    bt_conn_unref(default_conn);
    default_conn = NULL;

    start_scan();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

int ble_client_start(void)
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
