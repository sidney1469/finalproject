/* serial.h */
#ifndef SERIAL_H
#define SERIAL_H

/* Returns 0 on success, negative errno on failure */
int serial_init(void);

/*
 * Non-blocking line read.
 * Returns:
 * - pointer to internal null-terminated line buffer when a full line is ready
 * - NULL otherwise
 */
char *serial_read_line(void);

#endif