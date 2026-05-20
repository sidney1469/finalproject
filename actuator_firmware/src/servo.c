
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

int angle_to_pwm(float theta) 
{    
    return (int)(0.5 + (double)(theta/180) * 2.0);
}

void drive_to_theta(float theta, float phi) {
    int pan_pwm = angle_to_pwm(theta);
    int tilt_pwm = angle_to_pwm(phi);
    
    float period1 = 1 / pan_pwm;
    float period2 = 1 / tilt_pwm;

    pwm_set_pulse_dt(&pwm_spec_0, period1);
    pwm_set_pulse_dt(&pwm_spec_1, period2);

    return;
}

void servo_thread(void* a, void* b, void* c) {

    if (!pwm_is_ready_dt(&pwm_spec_0)) {
        printk("PWM0 not ready\n");
        return;
    }

    if (!pwm_is_ready_dt(&pwm_spec_1)) {
        printk("PWM1 not ready\n");
        return;
    }

    struct angle_struct angles = {0};

    while(1) {
        k_msgq_get(&servo_msgq, &angles, K_FOREVER);
        drive_to_theta(angles.theta, angles.phi);
        k_sleep(K_MSEC(100));
    }
    return;
}