#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include "main.h"
#include "motor_driver_runtime.h"

#include <stdint.h>

typedef struct
{
    TIM_HandleTypeDef *pwm1_timer;
    uint32_t pwm1_channel;

    TIM_HandleTypeDef *pwm2_timer;
    uint32_t pwm2_channel;

    GPIO_TypeDef *enable_port;
    uint16_t enable_pin;

    GPIO_TypeDef *enable_bar_port;
    uint16_t enable_bar_pin;

    GPIO_TypeDef *diag_port;
    uint16_t diag_pin;
} MotorDriverConfig_t;

/**
 * Initializes the software driver and places the motor in a safe,
 * disabled state.
 */
MotorStatus_t MotorDriver_Init(const MotorDriverConfig_t *config);

#endif
