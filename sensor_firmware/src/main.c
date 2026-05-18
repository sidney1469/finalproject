#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include "ble_transfer.h"
#include "gps_driver.h"

int main(void)
{
    /* Give GPS thread time to get first fix */
    k_sleep(K_SECONDS(5));

    while (1) {
        gps_location_t loc = get_location();

        if (loc.valid) {
            char msg[200];
            snprintf(msg, sizeof(msg),
                     "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.2f}",
                     loc.latitude, loc.longitude, loc.altitude_m);
            
            send_over_nus(msg);
            printk("Sent: %s\n", msg);
        } else {
            printk("Waiting for GPS fix...\n");
        }

        k_sleep(K_SECONDS(10));  /* Send every 10 seconds */
    }

    return 0;
}