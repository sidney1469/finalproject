#include "minheap.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/min_heap.h>
#include <zephyr/data/json.h>
#include <zephyr/sys/util.h>
#include <zephyr/shell/shell.h>



#include <errno.h>
#include <stdbool.h>
#include <string.h>

int min_heap_count();


// static const struct json_obj_descr heap_json_descr[] = {
// 	JSON_OBJ_DESCR_ARRAY(struct HEAP_JSON,
// 			     flightName,
// 			     MAX_HEAP_SIZE,
// 			     heap_len,
// 			     JSON_TOK_STRING),

// 	JSON_OBJ_DESCR_ARRAY(struct HEAP_JSON,
// 			     longitude,
// 			     MAX_HEAP_SIZE,
// 			     heap_len,
// 			     JSON_TOK_FLOAT_FP),

// 	JSON_OBJ_DESCR_ARRAY(struct HEAP_JSON,
// 			     latitude,
// 			     MAX_HEAP_SIZE,
// 			     heap_len,
// 			     JSON_TOK_FLOAT_FP),

// 	JSON_OBJ_DESCR_ARRAY(struct HEAP_JSON,
// 			     loclong,
// 			     MAX_HEAP_SIZE,
// 			     heap_len,
// 			     JSON_TOK_FLOAT_FP),

// 	JSON_OBJ_DESCR_ARRAY(struct HEAP_JSON,
// 			     loclat,
// 			     MAX_HEAP_SIZE,
// 			     heap_len,
// 			     JSON_TOK_FLOAT_FP),
// };

/*
 * Returns the squared Euclidean distance between a plane's position and the
 * observer location in (longitude, latitude) space. Note: this is a flat-earth
 * approximation suitable for small geographic areas only; degree units are not
 * isotropic at non-zero latitudes.
 */
static float heap_node_distance_squared(const struct HEAP_NODE *node)
{
	float dx = node->longitude - node->loclong;
	float dy = node->latitude - node->loclat;

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

	return strcmp(node->flightName, flightName) == 0;
}

MIN_HEAP_DEFINE_STATIC(planeheap,
		       MAX_HEAP_SIZE,
		       sizeof(struct HEAP_NODE),
		       __alignof__(struct HEAP_NODE),
		       compare_heap_nodes);

int insert_into_heap(const char *flightName,
		     float longitude,
		     float latitude,
		     float loclong,
		     float loclat)
{
	struct HEAP_NODE node = {0};
	struct HEAP_NODE deletednode = {0};
	size_t index;

	if (flightName == NULL) {
		return -EINVAL;
	}

	strncpy(node.flightName, flightName, sizeof(node.flightName) - 1);
	node.flightName[sizeof(node.flightName) - 1] = '\0';

	node.longitude = longitude;
	node.latitude = latitude;
	node.loclong = loclong;
	node.loclat = loclat;

	int ret = min_heap_push(&planeheap, &node);

	if (ret != 0) {
		printk("insert_into_heap: failed to push %s: %d\n", flightName, ret);
		return ret;
	}

	return 0;
}
int min_heap_count(){
    void* elem;
	int count = 0;

    MIN_HEAP_FOREACH(&planeheap, elem) {
		struct HEAP_NODE *d = elem;

		count++;
	}
	return count;
}

int convert_heap_to_string(char *json_buf, size_t json_buf_size)
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
                    d->flightName,
                    (double)d->longitude,
                    (double)d->latitude,
                    (double)d->loclong,
                    (double)d->loclat);


        k_msleep(20);
        index++;
    }

    printk("]\n");

	struct HEAP_NODE *d = elem;

	while (min_heap_pop(&planeheap, &d)){ /*Empty Minheap*/}
    return 0;
}