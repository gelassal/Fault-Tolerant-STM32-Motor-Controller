#include "motor_controller.h"
#include "motor_driver_runtime.h"


static bool controller_initialized = false;

static MotorControllerState_t controller_state =
    MOTOR_CONTROLLER_STATE_UNINITIALIZED;

static MotorControllerFault_t latched_fault =
    MOTOR_CONTROLLER_FAULT_NONE;

static float commanded_duty_percent = 0.0f;

static MotorDirection_t commanded_direction =
    MOTOR_DIRECTION_FORWARD;


/* -------------------------------------------------------------------------- */
/* Private functions                                                          */
/* -------------------------------------------------------------------------- */

static bool IsDirectionValid(
    MotorDirection_t direction)
{
    return
        (direction == MOTOR_DIRECTION_FORWARD) ||
        (direction == MOTOR_DIRECTION_REVERSE);
}


static bool IsDutyValid(
    float duty_percent)
{
    return
        (duty_percent >= 0.0f) &&
        (duty_percent <= 100.0f);
}


static bool IsDriverActiveState(void)
{
    return
        (controller_state ==
            MOTOR_CONTROLLER_STATE_RUNNING) ||
        (controller_state ==
            MOTOR_CONTROLLER_STATE_BRAKING);
}


/**
 * Immediately places the controller into its safest state.
 *
 * The fault remains latched until MotorController_ClearFault()
 * succeeds.
 */
static void EnterFaultState(
    MotorControllerFault_t fault)
{
    MotorDriver_Disable();

    commanded_duty_percent = 0.0f;
    latched_fault = fault;

    controller_state =
        MOTOR_CONTROLLER_STATE_FAULT;
}


/**
 * Converts a low-level driver failure into a controller fault.
 */
static MotorControllerStatus_t HandleDriverFailure(
    MotorStatus_t driver_status)
{
    if (driver_status == MOTOR_STATUS_OK)
    {
        return MOTOR_CONTROLLER_STATUS_OK;
    }

    EnterFaultState(
        MOTOR_CONTROLLER_FAULT_DRIVER_ERROR
    );

    return MOTOR_CONTROLLER_STATUS_DRIVER_ERROR;
}


/**
 * Checks the physical driver diagnostic input only while the driver
 * is expected to be active.
 *
 * The TB9051 diagnostic signal can indicate the disabled condition,
 * so it should not be interpreted as a fault while intentionally
 * disabled.
 */
static bool CheckForDriverFault(void)
{
    if (!IsDriverActiveState())
    {
        return false;
    }

    if (!MotorDriver_IsFaultActive())
    {
        return false;
    }

    EnterFaultState(
        MOTOR_CONTROLLER_FAULT_DRIVER_DIAGNOSTIC
    );

    return true;
}


/* -------------------------------------------------------------------------- */
/* Public functions                                                           */
/* -------------------------------------------------------------------------- */

MotorControllerStatus_t MotorController_Init(void)
{
    /*
     * Verify that the low-level motor driver has already been
     * initialized by requesting a safe zero-duty output.
     */
    MotorStatus_t driver_status =
        MotorDriver_SetDuty(0.0f);

    if (driver_status != MOTOR_STATUS_OK)
    {
        controller_initialized = false;

        controller_state =
            MOTOR_CONTROLLER_STATE_UNINITIALIZED;

        return MOTOR_CONTROLLER_STATUS_DRIVER_ERROR;
    }

    /*
     * Force the hardware into a known safe condition.
     */
    MotorDriver_Disable();

    commanded_duty_percent = 0.0f;

    commanded_direction =
        MotorDriver_GetDirection();

    latched_fault =
        MOTOR_CONTROLLER_FAULT_NONE;

    controller_state =
        MOTOR_CONTROLLER_STATE_DISABLED;

    controller_initialized = true;

    return MOTOR_CONTROLLER_STATUS_OK;
}


void MotorController_Process(void)
{
    if (!controller_initialized)
    {
        return;
    }

    /*
     * Ignore DIAG while intentionally disabled or already faulted.
     */
    (void)CheckForDriverFault();
}


MotorControllerStatus_t MotorController_Enable(void)
{
    if (!controller_initialized)
    {
        return
            MOTOR_CONTROLLER_STATUS_NOT_INITIALIZED;
    }

    if (controller_state ==
        MOTOR_CONTROLLER_STATE_FAULT)
    {
        return
            MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE;
    }

    /*
     * Enabling an already-ready controller is harmless.
     */
    if (controller_state ==
        MOTOR_CONTROLLER_STATE_READY)
    {
        return MOTOR_CONTROLLER_STATUS_OK;
    }

    /*
     * Enable is only valid from DISABLED.
     */
    if (controller_state !=
        MOTOR_CONTROLLER_STATE_DISABLED)
    {
        return
            MOTOR_CONTROLLER_STATUS_INVALID_STATE;
    }

    /*
     * READY means armed but electrically coasting. Keep the
     * H-bridge disabled until a nonzero duty or brake command.
     */
    MotorDriver_Disable();

    commanded_duty_percent = 0.0f;

    controller_state =
        MOTOR_CONTROLLER_STATE_READY;

    return MOTOR_CONTROLLER_STATUS_OK;
}


MotorControllerStatus_t MotorController_Disable(void)
{
    if (!controller_initialized)
    {
        return
            MOTOR_CONTROLLER_STATUS_NOT_INITIALIZED;
    }

    MotorDriver_Disable();

    commanded_duty_percent = 0.0f;

    /*
     * Disabling the output must not silently clear a fault.
     */
    if (controller_state !=
        MOTOR_CONTROLLER_STATE_FAULT)
    {
        controller_state =
            MOTOR_CONTROLLER_STATE_DISABLED;
    }

    return MOTOR_CONTROLLER_STATUS_OK;
}


MotorControllerStatus_t MotorController_SetDuty(
    float duty_percent)
{
    if (!controller_initialized)
    {
        return
            MOTOR_CONTROLLER_STATUS_NOT_INITIALIZED;
    }

    if (!IsDutyValid(duty_percent))
    {
        return
            MOTOR_CONTROLLER_STATUS_INVALID_ARGUMENT;
    }

    if (controller_state ==
        MOTOR_CONTROLLER_STATE_FAULT)
    {
        return
            MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE;
    }

    /*
     * Duty commands are accepted only when the motor is ready
     * or already running.
     */
    if ((controller_state !=
            MOTOR_CONTROLLER_STATE_READY) &&
        (controller_state !=
            MOTOR_CONTROLLER_STATE_RUNNING))
    {
        return
            MOTOR_CONTROLLER_STATUS_INVALID_STATE;
    }

    if ((controller_state ==
            MOTOR_CONTROLLER_STATE_RUNNING) &&
        CheckForDriverFault())
    {
        return
            MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE;
    }

    if (duty_percent == 0.0f)
    {
        MotorDriver_Disable();

        commanded_duty_percent = 0.0f;
        controller_state =
            MOTOR_CONTROLLER_STATE_READY;

        return MOTOR_CONTROLLER_STATUS_OK;
    }

    MotorStatus_t driver_status;

    if (controller_state ==
        MOTOR_CONTROLLER_STATE_READY)
    {
        /*
         * Never preload nonzero duty into a disabled driver. Enable
         * at zero duty, validate DIAG, then apply the command.
         */
        driver_status =
            MotorDriver_SetDuty(0.0f);

        if (driver_status != MOTOR_STATUS_OK)
        {
            return HandleDriverFailure(
                driver_status
            );
        }

        driver_status =
            MotorDriver_Enable();

        if (driver_status != MOTOR_STATUS_OK)
        {
            return HandleDriverFailure(
                driver_status
            );
        }

        controller_state =
            MOTOR_CONTROLLER_STATE_RUNNING;

        if (CheckForDriverFault())
        {
            return
                MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE;
        }
    }

    driver_status =
        MotorDriver_SetDuty(duty_percent);

    if (driver_status != MOTOR_STATUS_OK)
    {
        return HandleDriverFailure(
            driver_status
        );
    }

    commanded_duty_percent =
        duty_percent;

    controller_state =
        MOTOR_CONTROLLER_STATE_RUNNING;

    return MOTOR_CONTROLLER_STATUS_OK;
}


MotorControllerStatus_t MotorController_SetDirection(
    MotorDirection_t direction)
{
    if (!controller_initialized)
    {
        return
            MOTOR_CONTROLLER_STATUS_NOT_INITIALIZED;
    }

    if (!IsDirectionValid(direction))
    {
        return
            MOTOR_CONTROLLER_STATUS_INVALID_ARGUMENT;
    }

    if (controller_state ==
        MOTOR_CONTROLLER_STATE_FAULT)
    {
        return
            MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE;
    }

    /* Direction may change only while READY and electrically coasting. */
    if (controller_state !=
        MOTOR_CONTROLLER_STATE_READY)
    {
        return
            MOTOR_CONTROLLER_STATUS_INVALID_STATE;
    }

    if (commanded_duty_percent != 0.0f)
    {
        return
            MOTOR_CONTROLLER_STATUS_INVALID_STATE;
    }

    MotorStatus_t driver_status =
        MotorDriver_SetDirection(direction);

    if (driver_status != MOTOR_STATUS_OK)
    {
        return HandleDriverFailure(
            driver_status
        );
    }

    commanded_direction = direction;
    commanded_duty_percent = 0.0f;

    return MOTOR_CONTROLLER_STATUS_OK;
}


MotorControllerStatus_t MotorController_Brake(void)
{
    if (!controller_initialized)
    {
        return
            MOTOR_CONTROLLER_STATUS_NOT_INITIALIZED;
    }

    if (controller_state ==
        MOTOR_CONTROLLER_STATE_FAULT)
    {
        return
            MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE;
    }

    if ((controller_state !=
            MOTOR_CONTROLLER_STATE_READY) &&
        (controller_state !=
            MOTOR_CONTROLLER_STATE_RUNNING))
    {
        return
            MOTOR_CONTROLLER_STATUS_INVALID_STATE;
    }

    if ((controller_state ==
            MOTOR_CONTROLLER_STATE_RUNNING) &&
        CheckForDriverFault())
    {
        return
            MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE;
    }

    MotorStatus_t driver_status =
        MotorDriver_Brake();

    if (driver_status != MOTOR_STATUS_OK)
    {
        return HandleDriverFailure(
            driver_status
        );
    }

    commanded_duty_percent = 0.0f;

    controller_state =
        MOTOR_CONTROLLER_STATE_BRAKING;

    if (CheckForDriverFault())
    {
        return
            MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE;
    }

    return MOTOR_CONTROLLER_STATUS_OK;
}


MotorControllerStatus_t MotorController_ReleaseBrake(void)
{
    if (!controller_initialized)
    {
        return
            MOTOR_CONTROLLER_STATUS_NOT_INITIALIZED;
    }

    if (controller_state ==
        MOTOR_CONTROLLER_STATE_FAULT)
    {
        return
            MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE;
    }

    if (controller_state !=
        MOTOR_CONTROLLER_STATE_BRAKING)
    {
        return
            MOTOR_CONTROLLER_STATUS_INVALID_STATE;
    }

    /*
     * Releasing the short brake disables the H-bridge so the
     * motor is electrically coasting in READY.
     */
    MotorDriver_Disable();

    commanded_duty_percent = 0.0f;

    controller_state =
        MOTOR_CONTROLLER_STATE_READY;

    return MOTOR_CONTROLLER_STATUS_OK;
}


MotorControllerStatus_t MotorController_ReportFault(
    MotorControllerFault_t fault)
{
    if (!controller_initialized)
    {
        return
            MOTOR_CONTROLLER_STATUS_NOT_INITIALIZED;
    }

    if ((fault <= MOTOR_CONTROLLER_FAULT_NONE) ||
        (fault > MOTOR_CONTROLLER_FAULT_OVERSPEED))
    {
        return
            MOTOR_CONTROLLER_STATUS_INVALID_ARGUMENT;
    }

    EnterFaultState(fault);

    return MOTOR_CONTROLLER_STATUS_OK;
}


MotorControllerStatus_t MotorController_ClearFault(void)
{
    if (!controller_initialized)
    {
        return
            MOTOR_CONTROLLER_STATUS_NOT_INITIALIZED;
    }

    if (controller_state !=
        MOTOR_CONTROLLER_STATE_FAULT)
    {
        return
            MOTOR_CONTROLLER_STATUS_INVALID_STATE;
    }

    /*
     * Fault entry disables the driver and clears its duty.
     * Explicitly request zero duty again before briefly enabling.
     */
    MotorStatus_t driver_status =
        MotorDriver_SetDuty(0.0f);

    if (driver_status != MOTOR_STATUS_OK)
    {
        MotorDriver_Disable();

        return
            MOTOR_CONTROLLER_STATUS_DRIVER_ERROR;
    }

    /*
     * Briefly enable at zero duty so DIAG represents a genuine
     * hardware fault instead of the expected disabled condition.
     */
    driver_status = MotorDriver_Enable();

    if (driver_status != MOTOR_STATUS_OK)
    {
        MotorDriver_Disable();

        return
            MOTOR_CONTROLLER_STATUS_DRIVER_ERROR;
    }

    bool fault_still_present =
        MotorDriver_IsFaultActive();

    /*
     * A successful clear always returns to DISABLED, requiring
     * a separate explicit enable command afterward.
     */
    MotorDriver_Disable();

    commanded_duty_percent = 0.0f;

    if (fault_still_present)
    {
        return
            MOTOR_CONTROLLER_STATUS_FAULT_STILL_PRESENT;
    }

    latched_fault =
        MOTOR_CONTROLLER_FAULT_NONE;

    controller_state =
        MOTOR_CONTROLLER_STATE_DISABLED;

    return MOTOR_CONTROLLER_STATUS_OK;
}


MotorControllerState_t MotorController_GetState(void)
{
    return controller_state;
}


MotorControllerFault_t MotorController_GetFault(void)
{
    return latched_fault;
}


float MotorController_GetDuty(void)
{
    return commanded_duty_percent;
}


MotorDirection_t MotorController_GetDirection(void)
{
    return commanded_direction;
}


bool MotorController_IsFaultLatched(void)
{
    return
        controller_state ==
        MOTOR_CONTROLLER_STATE_FAULT;
}
