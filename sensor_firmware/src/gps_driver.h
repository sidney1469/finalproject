#ifndef GPS_DRIVER_H
#define GPS_DRIVER_H

int gps_poll(void);

gps_location_t get_location(void);

#endif