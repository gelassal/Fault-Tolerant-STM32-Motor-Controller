#ifndef FAKE_MOTOR_DRIVER_H
#define FAKE_MOTOR_DRIVER_H

#include "motor_driver_runtime.h"

typedef enum
{
    FAKE_DRIVER_OPERATION_NONE = 0,
    FAKE_DRIVER_OPERATION_ENABLE,
    FAKE_DRIVER_OPERATION_SET_DUTY,
    FAKE_DRIVER_OPERATION_SET_DIRECTION,
    FAKE_DRIVER_OPERATION_BRAKE
} FakeMotorDriverOperation_t;

void FakeMotorDriver_Reset(void);
void FakeMotorDriver_SetDiagnosticFault(bool active);
void FakeMotorDriver_FailNext(
    FakeMotorDriverOperation_t operation,
    MotorStatus_t status);

bool FakeMotorDriver_IsEnabled(void);
bool FakeMotorDriver_IsBraking(void);
bool FakeMotorDriver_SawUnsafeDutyPreload(void);
float FakeMotorDriver_GetDuty(void);
MotorDirection_t FakeMotorDriver_GetDirection(void);
unsigned int FakeMotorDriver_GetDisableCount(void);

#endif
