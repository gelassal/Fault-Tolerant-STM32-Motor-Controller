#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include "motor_types.h"

#include <stdbool.h>


typedef enum
{
    MOTOR_CONTROLLER_STATE_UNINITIALIZED = 0,
    MOTOR_CONTROLLER_STATE_DISABLED,
    MOTOR_CONTROLLER_STATE_READY,
    MOTOR_CONTROLLER_STATE_RUNNING,
    MOTOR_CONTROLLER_STATE_BRAKING,
    MOTOR_CONTROLLER_STATE_FAULT
} MotorControllerState_t;


typedef enum
{
    MOTOR_CONTROLLER_FAULT_NONE = 0,
    MOTOR_CONTROLLER_FAULT_DRIVER_DIAGNOSTIC,
    MOTOR_CONTROLLER_FAULT_DRIVER_ERROR,
    MOTOR_CONTROLLER_FAULT_SOFTWARE,
    MOTOR_CONTROLLER_FAULT_CONTROL_TIMEOUT,
    MOTOR_CONTROLLER_FAULT_ENCODER_LOSS,
    MOTOR_CONTROLLER_FAULT_OVERCURRENT,
    MOTOR_CONTROLLER_FAULT_OVERSPEED
} MotorControllerFault_t;


typedef enum
{
    MOTOR_CONTROLLER_STATUS_OK = 0,
    MOTOR_CONTROLLER_STATUS_NOT_INITIALIZED,
    MOTOR_CONTROLLER_STATUS_INVALID_ARGUMENT,
    MOTOR_CONTROLLER_STATUS_INVALID_STATE,
    MOTOR_CONTROLLER_STATUS_DRIVER_ERROR,
    MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE,
    MOTOR_CONTROLLER_STATUS_FAULT_STILL_PRESENT
} MotorControllerStatus_t;


/**
 * Initializes the high-level motor controller.
 *
 * MotorDriver_Init() must be called before this function.
 * The controller starts in the DISABLED state with zero duty.
 */
MotorControllerStatus_t MotorController_Init(void);


/**
 * Performs periodic safety monitoring.
 *
 * Call repeatedly from the main loop. Later, this can be called from
 * a periodic FreeRTOS motor-control task.
 */
void MotorController_Process(void);


/**
 * Arms the controller while leaving the H-bridge disabled and coasting.
 *
 * DISABLED -> READY
 */
MotorControllerStatus_t MotorController_Enable(void);


/**
 * Immediately clears duty and disables the motor driver.
 *
 * Any non-fault state -> DISABLED
 *
 * If the controller is already in FAULT, the hardware is disabled,
 * but the fault remains latched.
 */
MotorControllerStatus_t MotorController_Disable(void);


/**
 * Sets motor duty from 0.0% through 100.0%.
 *
 * READY + nonzero duty -> RUNNING
 * RUNNING + zero duty  -> READY
 */
MotorControllerStatus_t MotorController_SetDuty(
    float duty_percent);


/**
 * Changes motor direction.
 *
 * Direction changes are allowed only in READY, where duty is zero.
 */
MotorControllerStatus_t MotorController_SetDirection(
    MotorDirection_t direction);


/**
 * Commands the low-level driver to electrically brake the motor.
 *
 * READY or RUNNING -> BRAKING
 */
MotorControllerStatus_t MotorController_Brake(void);


/**
 * Releases electrical braking by disabling the H-bridge to coast.
 *
 * BRAKING -> READY
 */
MotorControllerStatus_t MotorController_ReleaseBrake(void);


/**
 * Latches a fault reported by another subsystem.
 *
 * This can later be used by encoder monitoring, current monitoring,
 * CAN timeout detection, and other safety logic.
 */
MotorControllerStatus_t MotorController_ReportFault(
    MotorControllerFault_t fault);


/**
 * Attempts to clear a latched fault.
 *
 * The driver is briefly enabled at zero duty so the diagnostic input
 * can be checked while the driver is not intentionally disabled.
 *
 * FAULT -> DISABLED when the hardware fault is gone.
 */
MotorControllerStatus_t MotorController_ClearFault(void);


/* State and telemetry accessors. */

MotorControllerState_t MotorController_GetState(void);

MotorControllerFault_t MotorController_GetFault(void);

float MotorController_GetDuty(void);

MotorDirection_t MotorController_GetDirection(void);

bool MotorController_IsFaultLatched(void);


#endif
