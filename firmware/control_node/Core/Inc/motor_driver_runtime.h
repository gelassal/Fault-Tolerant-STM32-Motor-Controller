#ifndef MOTOR_DRIVER_RUNTIME_H
#define MOTOR_DRIVER_RUNTIME_H

#include "motor_types.h"

#include <stdbool.h>

typedef enum
{
    MOTOR_STATUS_OK = 0,
    MOTOR_STATUS_INVALID_ARGUMENT,
    MOTOR_STATUS_NOT_INITIALIZED,
    MOTOR_STATUS_HAL_ERROR
} MotorStatus_t;

MotorStatus_t MotorDriver_Enable(void);
void MotorDriver_Disable(void);
MotorStatus_t MotorDriver_SetDuty(float duty_percent);
MotorStatus_t MotorDriver_SetDirection(MotorDirection_t direction);
MotorStatus_t MotorDriver_Brake(void);

bool MotorDriver_IsEnabled(void);
bool MotorDriver_IsFaultActive(void);
float MotorDriver_GetDuty(void);
MotorDirection_t MotorDriver_GetDirection(void);

#endif
