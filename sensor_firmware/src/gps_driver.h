#ifndef GPS_DRIVER_H
#define GPS_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    double latitude;
    double longitude;
    double altitude_m;
    bool valid;
    uint32_t fix_seq;
} gps_location_t;

/* Thread updates this continuously; caller just reads snapshot. */
gps_location_t get_location(void);

#endif