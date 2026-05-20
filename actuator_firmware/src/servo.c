#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include "servo.h"

static const struct pwm_dt_spec pwm_spec_0 =
    PWM_DT_SPEC_GET(DT_NODELABEL(pwm0_out));

static const struct pwm_dt_spec pwm_spec_1 =
    PWM_DT_SPEC_GET(DT_NODELABEL(pwm1_out));

K_MSGQ_DEFINE(servo_msgq, sizeof(struct angle_struct), 1, 4);

/* Maps 0-180 degrees to 1000-2000us pulse width, returned in nanoseconds */
static uint32_t angle_to_pulse_ns(float theta)
{
    float pulse_us = 1000.0f + (theta / 180.0f) * 1000.0f;
    return (uint32_t)(pulse_us * 1000.0f);
}

static void drive_to_theta(float theta, float phi)
{
    uint32_t pan_pulse_ns  = angle_to_pulse_ns(theta);
    uint32_t tilt_pulse_ns = angle_to_pulse_ns(phi);

    printk("%d, %d\n", pan_pulse_ns, tilt_pulse_ns);

    pwm_set_pulse_dt(&pwm_spec_0, pan_pulse_ns);
    pwm_set_pulse_dt(&pwm_spec_1, tilt_pulse_ns);
}

void servo_thread(void *a, void *b, void *c)
{
    if (!pwm_is_ready_dt(&pwm_spec_0)) {
        printk("PWM0 not ready\n");
        return;
    }

    if (!pwm_is_ready_dt(&pwm_spec_1)) {
        printk("PWM1 not ready\n");
        return;
    }

    struct angle_struct angles = {0};

    while (1) {
        k_msgq_get(&servo_msgq, &angles, K_FOREVER);
        drive_to_theta(angles.theta, angles.phi);
        k_sleep(K_MSEC(100));
    }
}