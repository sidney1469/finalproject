/* serial.c */
#include "serial.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/devicetree.h>
#include <errno.h>
#include <stdint.h>

#define SERIAL_LINE_MAX 128

static const struct device *usb_uart;
static char line_buf[SERIAL_LINE_MAX];
static size_t line_idx;

int serial_init(void)
{
    usb_uart = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
    if (!device_is_ready(usb_uart)) {
        return -ENODEV;
    }

    if (usb_enable(NULL) != 0) {
        return -EIO;
    }

    /* Optional but useful: wait for terminal to open port (DTR) */
    uint32_t dtr = 0;
    while (!dtr) {
        (void)uart_line_ctrl_get(usb_uart, UART_LINE_CTRL_DTR, &dtr);
        k_msleep(100);
    }

    line_idx = 0;
    return 0;
}

char *serial_read_line(void)
{
    if (!usb_uart) {
        return NULL;
    }

    uint8_t c;
    if (uart_poll_in(usb_uart, &c) != 0) {
        return NULL; /* no byte available */
    }

    if (c == '\n' || c == '\r') {
        if (line_idx == 0) {
            return NULL;
        }

        line_buf[line_idx] = '\0';
        line_idx = 0;
        return line_buf;
    }

    if (line_idx < (SERIAL_LINE_MAX - 1)) {
        line_buf[line_idx++] = (char)c;
    } else {
        /* overflow: reset and start fresh */
        line_idx = 0;
    }

    return NULL;
}