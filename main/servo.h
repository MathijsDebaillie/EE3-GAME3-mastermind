#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_err.h"

// Hardware Configuration
#define SERVO_GPIO          38 

// LEDC PWM Configuration
#define SPEED_MODE          LEDC_LOW_SPEED_MODE
#define TIMER_NUM           LEDC_TIMER_0
#define DUTY_RESOLUTION     LEDC_TIMER_13_BIT
#define FREQUENCY           50
#define CLK_CONFIG          LEDC_AUTO_CLK
#define CHANNEL             LEDC_CHANNEL_0
#define INTR_TYPE           LEDC_INTR_DISABLE

// Servo Parameters
#define SERVO_MIN_PULSEWIDTH    1000  // microseconds
#define SERVO_MAX_PULSEWIDTH    2000
#define SERVO_MAX_DEGREE        180

void setup_pwm(uint8_t pin);
void set_servo_angle(int angle);
void open_cover();
void close_cover(); 

#endif // SERVO_CONTROL_H   