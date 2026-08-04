#ifndef MOTOR_DRIVER_H
#define MOTOR_DRIVER_H

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    MOTOR_DIRECTION_FORWARD = 0,
    MOTOR_DIRECTION_REVERSE
} MotorDirection_t;

typedef enum
{
    MOTOR_STATUS_OK = 0,
    MOTOR_STATUS_INVALID_ARGUMENT,
    MOTOR_STATUS_NOT_INITIALIZED,
    MOTOR_STATUS_HAL_ERROR
} MotorStatus_t;

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

/**
 * Enables the TB9051 power outputs.
 */
MotorStatus_t MotorDriver_Enable(void);

/**
 * Sets duty to zero and disables the TB9051 outputs.
 */
void MotorDriver_Disable(void);

/**
 * Sets requested duty cycle from 0.0% to 100.0%.
 * Values outside the range are clamped.
 */
MotorStatus_t MotorDriver_SetDuty(float duty_percent);

/**
 * Selects forward or reverse operation.
 * Changing direction forces duty to zero.
 */
MotorStatus_t MotorDriver_SetDirection(MotorDirection_t direction);

/**
 * Applies short braking by holding both PWM inputs low while enabled.
 */
MotorStatus_t MotorDriver_Brake(void);

/**
 * Returns true when the driver is enabled.
 */
bool MotorDriver_IsEnabled(void);

/**
 * Returns true when DIAG indicates a fault while the driver is enabled.
 */
bool MotorDriver_IsFaultActive(void);

float MotorDriver_GetDuty(void);
MotorDirection_t MotorDriver_GetDirection(void);

#endif
