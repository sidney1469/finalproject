#include "minheap.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/min_heap.h>
#include <zephyr/data/json.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>

#include <errno.h>
#include <stdbool.h>
#include <string.h>

/*
 * Returns the squared Euclidean distance between a plane's position and the
 * observer location in (longitude, latitude) space. Note: this is a flat-earth
 * approximation suitable for small geographic areas only; degree units are not
 * isotropic at non-zero latitudes.
 */
static float heap_node_distance_squared(const struct HEAP_NODE *node)
{
    float dx = node->plane_data.lon - node->gps_data.lon;
    float dy = node->plane_data.lat - node->gps_data.lat;

    return (dx * dx) + (dy * dy);
}

static int compare_heap_nodes(const void *a, const void *b)
{
    const struct HEAP_NODE *da = a;
    const struct HEAP_NODE *db = b;

    float difa = heap_node_distance_squared(da);
    float difb = heap_node_distance_squared(db);

    if (difa < difb) {
        return -1;
    }

    if (difa > difb) {
        return 1;
    }

    return 0;
}

static bool match_flight_name(const void *a, const void *b)
{
    const struct HEAP_NODE *node = a;
    const char *flightName = b;

    if (flightName == NULL) {
        return false;
    }

    return strcmp(node->plane_data.icao, flightName) == 0;
}

MIN_HEAP_DEFINE_STATIC(planeheap,
               MAX_HEAP_SIZE,
               sizeof(struct HEAP_NODE),
               __alignof__(struct HEAP_NODE),
               compare_heap_nodes);

               
int insert_into_heap(gps_location_t gps_data, airplane_t plane_data)
{
    struct HEAP_NODE node = {0};
    struct HEAP_NODE deletednode = {0};
    size_t index;

    node.gps_data = gps_data;
    node.plane_data = plane_data;

    int ret = min_heap_push(&planeheap, &node);

    if (ret != 0) {
        printk("insert_into_heap: failed to push %s: %d\n", plane_data.icao, ret);
        return ret;
    }

    return 0;
}

int convert_heap_to_string()
{

    void *elem;
    size_t index = 0;
    size_t newindex = 0;
    struct HEAP_NODE deletednode = {0};


    printk("[");

    MIN_HEAP_FOREACH(&planeheap, elem) {
        struct HEAP_NODE *d = elem;

        printk("%s{\"f\":\"%s\",\"lo\":%.4f,\"la\":%.4f,\"ll\":%.4f,\"lt\":%.4f}",
                    index > 0 ? "," : "",
                    d->plane_data.icao,
                    d->plane_data.lon,
                    d->plane_data.lat,
                    d->gps_data.lon,
                    d->gps_data.lat);
        k_msleep(20);
        index++;
    }

    printk("]\n");

    struct HEAP_NODE out;
    while (min_heap_pop(&planeheap, &out) == 0) {
        /* popped one */
    }
    return 0;
}