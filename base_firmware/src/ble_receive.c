// "E9:E8:9E:1B:4B:0F"

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/nus.h>
#include <zephyr/sys/byteorder.h>

#include "ble_receive.h"

/* Target peripheral MAC addresses */
static const bt_addr_t target_mac = {.val = {0xE9, 0xE8, 0x9E, 0x1B, 0x4B, 0x0F}};

/* Active BLE connection used by the central device */
static struct bt_conn *default_conn;

/* Message queue for passing received Bluetooth data to main thread */
K_MSGQ_DEFINE(bt_data_msgq, sizeof(struct bt_data_received), 10, 4);

/* Nordic UART Service UUIDs */
static struct bt_uuid_128 nus_svc_uuid = BT_UUID_INIT_128(BT_UUID_NUS_SRV_VAL);
static struct bt_uuid_128 nus_tx_uuid = BT_UUID_INIT_128(BT_UUID_NUS_TX_CHAR_VAL);
static struct bt_uuid_128 nus_rx_uuid = BT_UUID_INIT_128(BT_UUID_NUS_RX_CHAR_VAL);

static struct bt_gatt_subscribe_params subscribe_params;
static struct bt_gatt_discover_params discover_params;

static void start_scan(void);

/*
 * Handles incoming NUS TX notifications.
 * Received data is copied into a queue so another thread can process it.
 */
static uint8_t on_received(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
                           const void *data, uint16_t length)
{
    if (!data) {
        printk("NUS TX unsubscribed\n");
        params->value_handle = 0;
        return BT_GATT_ITER_STOP;
    }

    const int8_t *bytes = (const uint8_t *)data;

    struct bt_data_received new_data = {0};
    uint16_t c = 0;

    for (int i = 0; i < length; i++) {
        new_data.data_buffer[i] = (int8_t)bytes[i];
        c++;
    }

    new_data.data_len = c;

    printk("RX [%d bytes]: ", length);
    for (int i = 0; i < length; i++) {
        printk("%c", bytes[i]);
    }
    printk("\n");

    k_msgq_put(&bt_data_msgq, &new_data, K_NO_WAIT);

    return BT_GATT_ITER_CONTINUE;
}
/*
 * Performs GATT discovery for the Nordic UART Service.
 * Once the TX characteristic is found, notifications are enabled.
 */
static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                             struct bt_gatt_discover_params *params)
{
    if (!attr) {
        printk("Discovery complete\n");
        return BT_GATT_ITER_STOP;
    }

    if (!bt_uuid_cmp(params->uuid, &nus_svc_uuid.uuid)) {
        params->uuid = NULL; // Discover all characteristics
        params->start_handle = attr->handle + 1;
        params->type = BT_GATT_DISCOVER_CHARACTERISTIC;
        bt_gatt_discover(conn, params);
        return BT_GATT_ITER_STOP;
    }

    struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;

    if (!bt_uuid_cmp(chrc->uuid, &nus_tx_uuid.uuid)) {
        printk("Found NUS TX Characteristic\n");

        subscribe_params.notify = on_received;
        subscribe_params.value = BT_GATT_CCC_NOTIFY;
        subscribe_params.value_handle = chrc->value_handle;
        subscribe_params.ccc_handle = chrc->value_handle + 1;

        int err = bt_gatt_subscribe(conn, &subscribe_params);
        if (err) {
            printk("Subscribe failed: %d\n", err);
        } else {
            printk("Subscribed to NUS TX\n");
        }
    }

    else if (!bt_uuid_cmp(chrc->uuid, &nus_rx_uuid.uuid)) {
        printk("Found NUS RX Characteristic (Phone can write to this)\n");
    }

    return BT_GATT_ITER_CONTINUE;
}

/* Starts service discovery on the connected peripheral */
static void start_discovery(struct bt_conn *conn)
{
    discover_params.uuid = &nus_svc_uuid.uuid;
    discover_params.func = discover_func;
    discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    discover_params.type = BT_GATT_DISCOVER_PRIMARY;

    int err = bt_gatt_discover(conn, &discover_params);
    if (err) {
        printk("Discovery failed to start: %d\n", err);
    }
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	int err;

	if (default_conn) {
		return;
	}

	/* We're only interested in connectable events */
	if (type != BT_GAP_ADV_TYPE_ADV_IND && type != BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		return;
    }
    if (bt_addr_cmp(&addr->a, &target_mac) != 0) {
        return;
    }


	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	printk("Device found: %s\n", addr_str);

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

/* Starts passive BLE scanning in central mode */
static void start_scan(void)
{
    int err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
    if (err) {
        printk("Scanning failed to start: %d\n", err);
        return;
    }

    printk("Scanning started\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s %u %s\n", addr, err, bt_hci_err_to_str(err));

		bt_conn_unref(default_conn);
		default_conn = NULL;

		start_scan();
		return;
	}

	if (conn != default_conn) {
		return;
	}

	printk("Connected: %s\n", addr);

	start_discovery(conn);
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

void ble_receive_thread(void *a, void *b, void *c)
{
    int err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed: %d\n", err);
        return;
    }

    printk("Bluetooth initialized\n");
    start_scan();

    while (1) {
        k_sleep(K_FOREVER);
    }
}
