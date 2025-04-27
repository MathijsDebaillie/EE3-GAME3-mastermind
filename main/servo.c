#include "servo.h"

void setup_pwm(uint8_t pin) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = SPEED_MODE,
        .timer_num        = TIMER_NUM,
        .duty_resolution  = DUTY_RESOLUTION,
        .freq_hz          = FREQUENCY,
        .clk_cfg          = CLK_CONFIG
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = SPEED_MODE,
        .channel        = CHANNEL,
        .timer_sel      = TIMER_NUM,
        .intr_type      = INTR_TYPE,
        .gpio_num       = pin,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
}

void set_servo_angle(int angle) {
    if (angle < 0) angle = 0;
    //if (angle > 120) angle = 120;

    int pulsewidth = SERVO_MIN_PULSEWIDTH +
                     ((SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) * angle) / SERVO_MAX_DEGREE;

    int duty = (pulsewidth * (1 << DUTY_RESOLUTION)) / 20000;
    ledc_set_duty(SPEED_MODE, CHANNEL, duty);
    ledc_update_duty(SPEED_MODE, CHANNEL);
}

void open_cover() {
    printf("Opening cover\n");
    set_servo_angle(220);
}

void close_cover() {
    printf(" Closing cover\n");
    set_servo_angle(0);
}