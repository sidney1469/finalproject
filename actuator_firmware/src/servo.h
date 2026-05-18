#ifndef SERVO_H
#define SERVO_H

#include <zephyr/kernel.h>

/* Thread entry point for scanning known BLE beacons and collecting RSSI data */
void servo_thread(void* a, void* b, void* c);

/* Shared queue containing the latest RSSI table for the communications thread */
extern struct k_msgq servo_msgq;
struct angle_struct {
    float theta;
    float phi;
};

#endif /* SENSOR_H */