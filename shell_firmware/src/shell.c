/*********************************** */
/*             shell.c               */
/*********************************** */
/* Authors                           */
/* Sidney Neil 47441952              */
/* Fiachra Richards 47450271         */
/*********************************** */

/********* Include Libraries ******* */
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>

#include <stdlib.h>
#include <errno.h>

#include "shell.h"
#include "minheap.h"
/*********************************** */

static int parse_float_arg(const struct shell *sh, const char *arg, float *out)
{
    char *end = NULL;

    if (arg == NULL || out == NULL) {
        shell_error(sh, "Invalid argument");
        return -EINVAL;
    }

    errno = 0;
    float value = strtof(arg, &end);

    if (errno != 0 || end == arg || *end != '\0') {
        shell_error(sh, "Invalid float value: %s", arg);
        return -EINVAL;
    }

    *out = value;
    return 0;
}

/*
 * Usage:
 * plane add <flightName> <longitude> <latitude> <loclong> <loclat>
 *
 * Example:
 * plane add QFA123 153.0251 -27.4698 153.0100 -27.4600
 */
static int cmd_plane_add(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);

    const char *flightName = argv[1];

    float longitude;
    float latitude;
    float loclong;
    float loclat;

    int ret;

    ret = parse_float_arg(sh, argv[2], &longitude);
    if (ret != 0) {
        return ret;
    }

    ret = parse_float_arg(sh, argv[3], &latitude);
    if (ret != 0) {
        return ret;
    }

    ret = parse_float_arg(sh, argv[4], &loclong);
    if (ret != 0) {
        return ret;
    }

    ret = parse_float_arg(sh, argv[5], &loclat);
    if (ret != 0) {
        return ret;
    }

    ret = insert_into_heap(flightName,
                           longitude,
                           latitude,
                           loclong,
                           loclat);

    if (ret != 0) {
        printk("Failed to add plane %s: %d", flightName, ret);
        return ret;
    }

    printk("Added PLane");

    return 0;
}

/*
 * Usage:
 * plane print
 */
static int cmd_plane_print(const struct shell *sh, size_t argc, char **argv)
{
    char json_buf[HEAP_JSON_BUF_SIZE];
    int ret = convert_heap_to_string(json_buf, sizeof(char) * HEAP_JSON_BUF_SIZE);

    if (ret != 0) {
        printk("Failed to encode heap JSON: %d", ret);
        return ret;
    }
    return 0;
}

/*
 * plane add   <flightName> <longitude> <latitude> <loclong> <loclat>
 * plane print
 */
SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_plane,

    SHELL_CMD_ARG(add,
                  NULL,
                  "Add plane to heap.\n"
                  "Usage: plane add <flightName> <longitude> <latitude> <loclong> <loclat>",
                  cmd_plane_add,
                  6,
                  0),

    SHELL_CMD_ARG(print,
                  NULL,
                  "Print current heap as JSON.\n"
                  "Usage: plane print",
                  cmd_plane_print,
                  1,
                  0),

    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(plane, &sub_plane, "Plane heap commands", NULL);