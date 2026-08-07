#include "fake_motor_driver.h"

static bool driver_enabled;
static bool brake_active;
static bool diagnostic_fault;
static bool unsafe_duty_preload;
static float driver_duty;
static MotorDirection_t driver_direction;
static unsigned int disable_count;

static FakeMotorDriverOperation_t failing_operation;
static MotorStatus_t failing_status;

static MotorStatus_t ConsumeFailure(
    FakeMotorDriverOperation_t operation)
{
    if (failing_operation != operation)
    {
        return MOTOR_STATUS_OK;
    }

    MotorStatus_t status = failing_status;

    failing_operation = FAKE_DRIVER_OPERATION_NONE;
    failing_status = MOTOR_STATUS_OK;

    return status;
}

void FakeMotorDriver_Reset(void)
{
    driver_enabled = false;
    brake_active = false;
    diagnostic_fault = false;
    unsafe_duty_preload = false;
    driver_duty = 0.0f;
    driver_direction = MOTOR_DIRECTION_FORWARD;
    disable_count = 0U;
    failing_operation = FAKE_DRIVER_OPERATION_NONE;
    failing_status = MOTOR_STATUS_OK;
}

void FakeMotorDriver_SetDiagnosticFault(bool active)
{
    diagnostic_fault = active;
}

void FakeMotorDriver_FailNext(
    FakeMotorDriverOperation_t operation,
    MotorStatus_t status)
{
    failing_operation = operation;
    failing_status = status;
}

bool FakeMotorDriver_IsEnabled(void)
{
    return driver_enabled;
}

bool FakeMotorDriver_IsBraking(void)
{
    return brake_active;
}

bool FakeMotorDriver_SawUnsafeDutyPreload(void)
{
    return unsafe_duty_preload;
}

float FakeMotorDriver_GetDuty(void)
{
    return driver_duty;
}

MotorDirection_t FakeMotorDriver_GetDirection(void)
{
    return driver_direction;
}

unsigned int FakeMotorDriver_GetDisableCount(void)
{
    return disable_count;
}

MotorStatus_t MotorDriver_Enable(void)
{
    MotorStatus_t status =
        ConsumeFailure(FAKE_DRIVER_OPERATION_ENABLE);

    if (status != MOTOR_STATUS_OK)
    {
        return status;
    }

    driver_enabled = true;
    brake_active = driver_duty == 0.0f;

    return MOTOR_STATUS_OK;
}

void MotorDriver_Disable(void)
{
    driver_enabled = false;
    brake_active = false;
    driver_duty = 0.0f;
    disable_count++;
}

MotorStatus_t MotorDriver_SetDuty(float duty_percent)
{
    MotorStatus_t status =
        ConsumeFailure(FAKE_DRIVER_OPERATION_SET_DUTY);

    if (status != MOTOR_STATUS_OK)
    {
        return status;
    }

    if ((!driver_enabled) && (duty_percent > 0.0f))
    {
        unsafe_duty_preload = true;
    }

    driver_duty = duty_percent;
    brake_active = driver_enabled && (duty_percent == 0.0f);

    return MOTOR_STATUS_OK;
}

MotorStatus_t MotorDriver_SetDirection(
    MotorDirection_t direction)
{
    MotorStatus_t status =
        ConsumeFailure(FAKE_DRIVER_OPERATION_SET_DIRECTION);

    if (status != MOTOR_STATUS_OK)
    {
        return status;
    }

    driver_direction = direction;
    driver_duty = 0.0f;

    return MOTOR_STATUS_OK;
}

MotorStatus_t MotorDriver_Brake(void)
{
    MotorStatus_t status =
        ConsumeFailure(FAKE_DRIVER_OPERATION_BRAKE);

    if (status != MOTOR_STATUS_OK)
    {
        return status;
    }

    driver_enabled = true;
    brake_active = true;
    driver_duty = 0.0f;

    return MOTOR_STATUS_OK;
}

bool MotorDriver_IsEnabled(void)
{
    return driver_enabled;
}

bool MotorDriver_IsFaultActive(void)
{
    return driver_enabled && diagnostic_fault;
}

float MotorDriver_GetDuty(void)
{
    return driver_duty;
}

MotorDirection_t MotorDriver_GetDirection(void)
{
    return driver_direction;
}
