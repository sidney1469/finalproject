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
//     k_sleep(K_SECONDS(5));
//     gps_location_t current_loc;
//     current_loc.latitude = -27.497358;
//     current_loc.longitude = 153.013259;
//     current_loc.altitude_m = 10.00;
//     current_loc.valid = true;

//     while (1) {
//         gps_location_t new_loc = get_location();

//         if (new_loc.valid) {
//             // Update current location with new gps info
//             char msg[200];
//             current_loc.latitude = new_loc.latitude;
//             current_loc.longitude = new_loc.longitude;
//             current_loc.altitude_m = new_loc.altitude_m;
//             snprintf(msg, sizeof(msg),
//                      "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.2f}",
//                      new_loc.latitude, new_loc.longitude, new_loc.altitude_m);
//             printk("Received GPS: %s\n", msg);
//         } else {
//             printk("Waiting for GPS fix...\n");
//         }
//         char* plane_data = serial_read_line();

//         char ble_msg[247];
//         snprintf(ble_msg, sizeof(ble_msg),
//             "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.2f, \"air\": [%s]}",
//         current_loc.latitude, current_loc.longitude, current_loc.altitude_m, plane_data);
//         send_over_nus(ble_msg);
//         k_sleep(K_SECONDS(2));
//     }

//     return 0;
// }

int main(void)
{
    int err = serial_init();
    if (err) {
        printk("serial_init failed: %d\n", err);
        return 0;
    }

    printk("serial_init OK, waiting for lines...\n");

    while (1) {
        char *line = serial_read_line();

        if (line) {
            printk("SERIAL RX: %s\n", line);
            send_over_nus(line);
            printk("SENT OVER NUS: %s\n", line);
        }

        k_msleep(5);
    }

    return 0;
}
