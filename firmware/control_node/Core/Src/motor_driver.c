#include "motor_driver.h"

#include <stddef.h>

static MotorDriverConfig_t motor_config;

static bool motor_initialized = false;
static bool motor_enabled = false;

static float motor_duty_percent = 0.0f;

static MotorDirection_t motor_direction =
    MOTOR_DIRECTION_FORWARD;


/* -------------------------------------------------------------------------- */
/* Private functions                                                          */
/* -------------------------------------------------------------------------- */

static float ClampDuty(float duty_percent)
{
    if (duty_percent < 0.0f)
    {
        return 0.0f;
    }

    if (duty_percent > 100.0f)
    {
        return 100.0f;
    }

    return duty_percent;
}


static uint32_t DutyToCompare(
    TIM_HandleTypeDef *timer,
    float duty_percent)
{
    uint32_t auto_reload;
    uint32_t compare;

    auto_reload = __HAL_TIM_GET_AUTORELOAD(timer);

    /*
     * Using ARR as the maximum compare value gives a duty cycle
     * extremely close to 100% without risking an overflow when
     * ARR is already at the timer's maximum value.
     */
    compare = (uint32_t)(
        ((float)auto_reload * duty_percent) / 100.0f
    );

    if (compare > auto_reload)
    {
        compare = auto_reload;
    }

    return compare;
}


static void SetPwm1Duty(float duty_percent)
{
    uint32_t compare = DutyToCompare(
        motor_config.pwm1_timer,
        duty_percent
    );

    __HAL_TIM_SET_COMPARE(
        motor_config.pwm1_timer,
        motor_config.pwm1_channel,
        compare
    );
}


static void SetPwm2Duty(float duty_percent)
{
    uint32_t compare = DutyToCompare(
        motor_config.pwm2_timer,
        duty_percent
    );

    __HAL_TIM_SET_COMPARE(
        motor_config.pwm2_timer,
        motor_config.pwm2_channel,
        compare
    );
}


static void SetBothPwmLow(void)
{
    __HAL_TIM_SET_COMPARE(
        motor_config.pwm1_timer,
        motor_config.pwm1_channel,
        0U
    );

    __HAL_TIM_SET_COMPARE(
        motor_config.pwm2_timer,
        motor_config.pwm2_channel,
        0U
    );
}


static void ApplyMotorOutput(void)
{
    if ((!motor_initialized) ||
        (!motor_enabled) ||
        (motor_duty_percent <= 0.0f))
    {
        SetBothPwmLow();
        return;
    }

    switch (motor_direction)
    {
        case MOTOR_DIRECTION_FORWARD:
            /*
             * Forward:
             * PWM1 = PWM signal
             * PWM2 = low
             */
            SetPwm2Duty(0.0f);
            SetPwm1Duty(motor_duty_percent);
            break;

        case MOTOR_DIRECTION_REVERSE:
            /*
             * Reverse:
             * PWM1 = low
             * PWM2 = PWM signal
             */
            SetPwm1Duty(0.0f);
            SetPwm2Duty(motor_duty_percent);
            break;

        default:
            SetBothPwmLow();
            break;
    }
}


/* -------------------------------------------------------------------------- */
/* Public functions                                                           */
/* -------------------------------------------------------------------------- */

MotorStatus_t MotorDriver_Init(const MotorDriverConfig_t *config)
{
    if ((config == NULL) ||
        (config->pwm1_timer == NULL) ||
        (config->pwm2_timer == NULL) ||
        (config->enable_port == NULL) ||
        (config->enable_bar_port == NULL) ||
        (config->diag_port == NULL))
    {
        return MOTOR_STATUS_INVALID_ARGUMENT;
    }

    motor_config = *config;

    motor_initialized = false;
    motor_enabled = false;
    motor_duty_percent = 0.0f;
    motor_direction = MOTOR_DIRECTION_FORWARD;

    /*
     * Immediately request the driver's disabled state:
     *
     * EN  = low
     * ENB = high
     */
    HAL_GPIO_WritePin(
        motor_config.enable_port,
        motor_config.enable_pin,
        GPIO_PIN_RESET
    );

    HAL_GPIO_WritePin(
        motor_config.enable_bar_port,
        motor_config.enable_bar_pin,
        GPIO_PIN_SET
    );

    SetBothPwmLow();

    /*
     * PWM channels run continuously after startup.
     * Duty is changed later by updating the compare registers.
     */
    if (HAL_TIM_PWM_Start(
            motor_config.pwm1_timer,
            motor_config.pwm1_channel) != HAL_OK)
    {
        return MOTOR_STATUS_HAL_ERROR;
    }

    if (HAL_TIM_PWM_Start(
            motor_config.pwm2_timer,
            motor_config.pwm2_channel) != HAL_OK)
    {
        HAL_TIM_PWM_Stop(
            motor_config.pwm1_timer,
            motor_config.pwm1_channel
        );

        return MOTOR_STATUS_HAL_ERROR;
    }

    motor_initialized = true;

    return MOTOR_STATUS_OK;
}


MotorStatus_t MotorDriver_Enable(void)
{
    if (!motor_initialized)
    {
        return MOTOR_STATUS_NOT_INITIALIZED;
    }

    /*
     * Keep both PWM inputs low while enabling the driver.
     */
    SetBothPwmLow();

    HAL_GPIO_WritePin(
        motor_config.enable_bar_port,
        motor_config.enable_bar_pin,
        GPIO_PIN_RESET
    );

    HAL_GPIO_WritePin(
        motor_config.enable_port,
        motor_config.enable_pin,
        GPIO_PIN_SET
    );

    motor_enabled = true;

    ApplyMotorOutput();

    return MOTOR_STATUS_OK;
}


void MotorDriver_Disable(void)
{
    if (!motor_initialized)
    {
        return;
    }

    SetBothPwmLow();

    /*
     * Disable using both enable inputs for a clear safe state.
     */
    HAL_GPIO_WritePin(
        motor_config.enable_port,
        motor_config.enable_pin,
        GPIO_PIN_RESET
    );

    HAL_GPIO_WritePin(
        motor_config.enable_bar_port,
        motor_config.enable_bar_pin,
        GPIO_PIN_SET
    );

    motor_enabled = false;
    motor_duty_percent = 0.0f;
}


MotorStatus_t MotorDriver_SetDuty(float duty_percent)
{
    if (!motor_initialized)
    {
        return MOTOR_STATUS_NOT_INITIALIZED;
    }

    motor_duty_percent = ClampDuty(duty_percent);

    ApplyMotorOutput();

    return MOTOR_STATUS_OK;
}


MotorStatus_t MotorDriver_SetDirection(
    MotorDirection_t direction)
{
    if (!motor_initialized)
    {
        return MOTOR_STATUS_NOT_INITIALIZED;
    }

    if ((direction != MOTOR_DIRECTION_FORWARD) &&
        (direction != MOTOR_DIRECTION_REVERSE))
    {
        return MOTOR_STATUS_INVALID_ARGUMENT;
    }

    if (direction != motor_direction)
    {
        /*
         * Stop before changing direction. The application can
         * command a new nonzero duty after the direction change.
         */
        SetBothPwmLow();
        motor_duty_percent = 0.0f;
        motor_direction = direction;
    }

    return MOTOR_STATUS_OK;
}


MotorStatus_t MotorDriver_Brake(void)
{
    if (!motor_initialized)
    {
        return MOTOR_STATUS_NOT_INITIALIZED;
    }

    motor_duty_percent = 0.0f;
    SetBothPwmLow();

    /*
     * Both PWM inputs low while EN = high and ENB = low
     * produces the TB9051 short-brake state.
     */
    HAL_GPIO_WritePin(
        motor_config.enable_bar_port,
        motor_config.enable_bar_pin,
        GPIO_PIN_RESET
    );

    HAL_GPIO_WritePin(
        motor_config.enable_port,
        motor_config.enable_pin,
        GPIO_PIN_SET
    );

    motor_enabled = true;

    return MOTOR_STATUS_OK;
}


bool MotorDriver_IsEnabled(void)
{
    return motor_initialized && motor_enabled;
}


bool MotorDriver_IsFaultActive(void)
{
    if ((!motor_initialized) || (!motor_enabled))
    {
        /*
         * DIAG is also low when the driver is intentionally disabled,
         * so do not classify that condition as a hardware fault.
         */
        return false;
    }

    return HAL_GPIO_ReadPin(
        motor_config.diag_port,
        motor_config.diag_pin
    ) == GPIO_PIN_RESET;
}


float MotorDriver_GetDuty(void)
{
    return motor_duty_percent;
}


MotorDirection_t MotorDriver_GetDirection(void)
{
    return motor_direction;
}
