/*
Simple bluetooth nus helper
*/
#include <string.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/services/nus.h>
#include <zephyr/bluetooth/hci.h>

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN     (sizeof(DEVICE_NAME) - 1)

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_SRV_VAL),
};

static bool nus_ready = false;

static int init_nus(void) {
    int err;
    if (nus_ready) {
        return 0;
    }

    err = bt_enable(NULL);
    if (err) {
        return err;
    }

    err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err && err != -EALREADY) return err;

    nus_ready = true;
    return 0;
}

int send_over_nus(char *msg) {
    int err = init_nus();
    if (err) {
        return err;
    }
    if (msg == NULL) {
        return -EINVAL;
    }
    err = bt_nus_send(NULL, msg, strlen(msg));
    if (err == -ENOTCONN) {
         bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    }
    return err;
}
