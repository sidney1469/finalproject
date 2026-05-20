/*********************************** */
/*              main.c               */
/*********************************** */
/* Authors                           */
/* Sidney Neil 47441952              */
/* Fiachra Richards  47450271        */
/*********************************** */

/********* Include Libraries ******* */
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/kernel.h>

#include "comms.h"
#include "servo.h"
/********************************* */

/*********** Global Defines ********** */

/* Thread configuration */
#define STACK_SIZE       2048
#define COMMS_PRIORITY   5
#define SERVO_PRIORITY 5

/* Thread entry functions defined in their respective modules */
extern void comms_thread(void *a, void *b, void *c);
extern void servo_thread(void* a, void* b, void* c);

/* Stack areas for sensing and communication threads */
K_THREAD_STACK_DEFINE(comms_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(servo_stack, STACK_SIZE);

/* Thread control blocks */
static struct k_thread comms_thread_data;
static struct k_thread servo_thread_data;

int main(void)
{
    /* Start the communications thread for sending collected data over BLE */
    k_thread_create(&comms_thread_data, comms_stack, K_THREAD_STACK_SIZEOF(comms_stack),
                    comms_thread, NULL, NULL, NULL, COMMS_PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&servo_thread_data, servo_stack, K_THREAD_STACK_SIZEOF(servo_stack),
                    servo_thread, NULL, NULL, NULL, SERVO_PRIORITY, 0, K_NO_WAIT);

    return 0;
}