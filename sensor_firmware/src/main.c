#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include "ble_transfer.h"
#include "gps_driver.h"
#include "serial.h"

// int main(void)
// {
//     int err = serial_init();
//     if (err) {
//         printk("serial_init failed: %d\n", err);
//         return 0;
//     }

//     while (1) {
//         char *line = serial_read_line();
//         if (line) {
//             send_over_nus(line);
//             printk("USB->NUS: %s\n", line);
//         }

//         k_msleep(5);
//     }
//     return 0;
// }

int main(void)
{
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
            send_over_nus("TEST MESSAGE since GPS isn't working");
        }

        k_sleep(K_SECONDS(2));  /* Send every 10 seconds */
    }

    return 0;
}