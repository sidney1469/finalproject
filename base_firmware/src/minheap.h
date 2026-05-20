#ifndef MINHEAP_H
#define MINHEAP_H

#include <stddef.h>

#define MAX_HEAP_SIZE 30
#define FLIGHT_NAME_MAX_LEN 16

struct HEAP_NODE {
    char flightName[FLIGHT_NAME_MAX_LEN];

    float longitude;
    float latitude;

    float loclong;
    float loclat;
};

void print_heap();

#define FLOAT_STR_LEN 12

struct HEAP_JSON {
    char *flightName[MAX_HEAP_SIZE];

    char longitudeStr[MAX_HEAP_SIZE][FLOAT_STR_LEN];
    char latitudeStr[MAX_HEAP_SIZE][FLOAT_STR_LEN];
    char loclongStr[MAX_HEAP_SIZE][FLOAT_STR_LEN];
    char loclatStr[MAX_HEAP_SIZE][FLOAT_STR_LEN];

    size_t heap_len;
};

int insert_into_heap(const char *flightName,
                     float longitude,
                     float latitude,
                     float loclong,
                     float loclat);

int convert_heap_to_string();

#endif /* MINHEAP_H */