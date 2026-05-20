#ifndef GPS_DRIVER_H
#define GPS_DRIVER_H

#include "protocol.h"

/* Thread updates this continuously; caller just reads snapshot. */
gps_location_t get_location(void);

#endif