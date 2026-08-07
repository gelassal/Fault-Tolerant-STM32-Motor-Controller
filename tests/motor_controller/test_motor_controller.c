#include "fake_motor_driver.h"
#include "motor_controller.h"

#include <math.h>
#include <stdbool.h>
#include <stdio.h>

static unsigned int assertion_count;
static unsigned int failure_count;

#define EXPECT_TRUE(condition) \
    ExpectTrue((condition), #condition, __FILE__, __LINE__)

#define EXPECT_STATUS(expected, expression) \
    ExpectInteger((expected), (expression), #expression, __FILE__, __LINE__)

#define EXPECT_STATE(expected) \
    ExpectInteger((expected), MotorController_GetState(), \
        "MotorController_GetState()", __FILE__, __LINE__)

#define EXPECT_FAULT(expected) \
    ExpectInteger((expected), MotorController_GetFault(), \
        "MotorController_GetFault()", __FILE__, __LINE__)

static void ExpectTrue(
    bool condition,
    const char *expression,
    const char *file,
    int line)
{
    assertion_count++;

    if (condition)
    {
        return;
    }

    failure_count++;
    (void)printf(
        "FAIL %s:%d: %s\n",
        file,
        line,
        expression
    );
}

static void ExpectInteger(
    int expected,
    int actual,
    const char *expression,
    const char *file,
    int line)
{
    assertion_count++;

    if (expected == actual)
    {
        return;
    }

    failure_count++;
    (void)printf(
        "FAIL %s:%d: %s expected %d, got %d\n",
        file,
        line,
        expression,
        expected,
        actual
    );
}

static void ResetController(void)
{
    FakeMotorDriver_Reset();
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_Init()
    );
}

static void ArmController(void)
{
    ResetController();
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_Enable()
    );
}

static void TestPreInitialization(void)
{
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_NOT_INITIALIZED,
        MotorController_Enable()
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_NOT_INITIALIZED,
        MotorController_SetDuty(10.0f)
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_UNINITIALIZED);
}

static void TestInitializationAndArming(void)
{
    ResetController();

    EXPECT_STATE(MOTOR_CONTROLLER_STATE_DISABLED);
    EXPECT_FAULT(MOTOR_CONTROLLER_FAULT_NONE);
    EXPECT_TRUE(MotorController_GetDuty() == 0.0f);
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());
    EXPECT_TRUE(!FakeMotorDriver_IsBraking());

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_STATE,
        MotorController_SetDuty(25.0f)
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_STATE,
        MotorController_SetDirection(MOTOR_DIRECTION_REVERSE)
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_STATE,
        MotorController_Brake()
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_STATE,
        MotorController_ReleaseBrake()
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_STATE,
        MotorController_ClearFault()
    );

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_Enable()
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_READY);
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());
    EXPECT_TRUE(!FakeMotorDriver_IsBraking());

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_Enable()
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_READY);
}

static void TestRunCoastAndDirectionInterlock(void)
{
    ArmController();

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_SetDirection(MOTOR_DIRECTION_REVERSE)
    );
    EXPECT_TRUE(
        FakeMotorDriver_GetDirection() ==
        MOTOR_DIRECTION_REVERSE
    );

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_SetDuty(25.0f)
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_RUNNING);
    EXPECT_TRUE(FakeMotorDriver_IsEnabled());
    EXPECT_TRUE(!FakeMotorDriver_IsBraking());
    EXPECT_TRUE(FakeMotorDriver_GetDuty() == 25.0f);
    EXPECT_TRUE(!FakeMotorDriver_SawUnsafeDutyPreload());

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_STATE,
        MotorController_SetDirection(MOTOR_DIRECTION_FORWARD)
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_STATE,
        MotorController_Enable()
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_STATE,
        MotorController_ReleaseBrake()
    );
    EXPECT_TRUE(
        MotorController_GetDirection() ==
        MOTOR_DIRECTION_REVERSE
    );

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_SetDuty(50.0f)
    );
    EXPECT_TRUE(FakeMotorDriver_GetDuty() == 50.0f);

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_SetDuty(0.0f)
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_READY);
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());
    EXPECT_TRUE(!FakeMotorDriver_IsBraking());
    EXPECT_TRUE(MotorController_GetDuty() == 0.0f);

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_SetDirection(MOTOR_DIRECTION_FORWARD)
    );
}

static void TestBrakeAndRelease(void)
{
    ArmController();

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_Brake()
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_BRAKING);
    EXPECT_TRUE(FakeMotorDriver_IsEnabled());
    EXPECT_TRUE(FakeMotorDriver_IsBraking());

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_STATE,
        MotorController_SetDuty(10.0f)
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_STATE,
        MotorController_SetDirection(MOTOR_DIRECTION_REVERSE)
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_STATE,
        MotorController_Enable()
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_ReleaseBrake()
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_READY);
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());
    EXPECT_TRUE(!FakeMotorDriver_IsBraking());

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_STATE,
        MotorController_ReleaseBrake()
    );

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_SetDuty(20.0f)
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_Brake()
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_BRAKING);
    EXPECT_TRUE(FakeMotorDriver_IsBraking());
}

static void TestDisableAndSoftwareFault(void)
{
    ArmController();
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_SetDuty(15.0f)
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_Disable()
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_DISABLED);
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_ReportFault(
            MOTOR_CONTROLLER_FAULT_SOFTWARE
        )
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_FAULT);
    EXPECT_FAULT(MOTOR_CONTROLLER_FAULT_SOFTWARE);
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE,
        MotorController_Enable()
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE,
        MotorController_SetDuty(10.0f)
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE,
        MotorController_SetDirection(MOTOR_DIRECTION_REVERSE)
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE,
        MotorController_Brake()
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE,
        MotorController_ReleaseBrake()
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_Disable()
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_FAULT);
    EXPECT_FAULT(MOTOR_CONTROLLER_FAULT_SOFTWARE);

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_ClearFault()
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_DISABLED);
    EXPECT_FAULT(MOTOR_CONTROLLER_FAULT_NONE);
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());
}

static void TestDiagnosticFaults(void)
{
    ArmController();
    FakeMotorDriver_SetDiagnosticFault(true);

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE,
        MotorController_SetDuty(25.0f)
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_FAULT);
    EXPECT_FAULT(
        MOTOR_CONTROLLER_FAULT_DRIVER_DIAGNOSTIC
    );
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());
    EXPECT_TRUE(!FakeMotorDriver_SawUnsafeDutyPreload());

    FakeMotorDriver_SetDiagnosticFault(false);
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_ClearFault()
    );

    ArmController();
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_SetDuty(25.0f)
    );
    FakeMotorDriver_SetDiagnosticFault(true);
    MotorController_Process();

    EXPECT_STATE(MOTOR_CONTROLLER_STATE_FAULT);
    EXPECT_FAULT(
        MOTOR_CONTROLLER_FAULT_DRIVER_DIAGNOSTIC
    );
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());
}

static void TestFaultClearFailures(void)
{
    ResetController();
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_ReportFault(
            MOTOR_CONTROLLER_FAULT_SOFTWARE
        )
    );
    FakeMotorDriver_SetDiagnosticFault(true);

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_FAULT_STILL_PRESENT,
        MotorController_ClearFault()
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_FAULT);
    EXPECT_FAULT(MOTOR_CONTROLLER_FAULT_SOFTWARE);
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());

    FakeMotorDriver_SetDiagnosticFault(false);
    FakeMotorDriver_FailNext(
        FAKE_DRIVER_OPERATION_SET_DUTY,
        MOTOR_STATUS_HAL_ERROR
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_DRIVER_ERROR,
        MotorController_ClearFault()
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_FAULT);
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());

    FakeMotorDriver_FailNext(
        FAKE_DRIVER_OPERATION_ENABLE,
        MOTOR_STATUS_HAL_ERROR
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_DRIVER_ERROR,
        MotorController_ClearFault()
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_FAULT);
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_ClearFault()
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_DISABLED);
}

static void TestDriverFailures(void)
{
    ArmController();
    FakeMotorDriver_FailNext(
        FAKE_DRIVER_OPERATION_ENABLE,
        MOTOR_STATUS_HAL_ERROR
    );

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_DRIVER_ERROR,
        MotorController_SetDuty(25.0f)
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_FAULT);
    EXPECT_FAULT(MOTOR_CONTROLLER_FAULT_DRIVER_ERROR);
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());

    ArmController();
    FakeMotorDriver_FailNext(
        FAKE_DRIVER_OPERATION_SET_DUTY,
        MOTOR_STATUS_HAL_ERROR
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_DRIVER_ERROR,
        MotorController_SetDuty(25.0f)
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_FAULT);
    EXPECT_FAULT(MOTOR_CONTROLLER_FAULT_DRIVER_ERROR);
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());

    ArmController();
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_OK,
        MotorController_SetDuty(10.0f)
    );
    FakeMotorDriver_FailNext(
        FAKE_DRIVER_OPERATION_SET_DUTY,
        MOTOR_STATUS_HAL_ERROR
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_DRIVER_ERROR,
        MotorController_SetDuty(20.0f)
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_FAULT);
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());

    ArmController();
    FakeMotorDriver_FailNext(
        FAKE_DRIVER_OPERATION_SET_DIRECTION,
        MOTOR_STATUS_INVALID_ARGUMENT
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_DRIVER_ERROR,
        MotorController_SetDirection(MOTOR_DIRECTION_REVERSE)
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_FAULT);

    ArmController();
    FakeMotorDriver_FailNext(
        FAKE_DRIVER_OPERATION_BRAKE,
        MOTOR_STATUS_HAL_ERROR
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_DRIVER_ERROR,
        MotorController_Brake()
    );
    EXPECT_STATE(MOTOR_CONTROLLER_STATE_FAULT);
}

static void TestInvalidArguments(void)
{
    ArmController();

    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_ARGUMENT,
        MotorController_SetDuty(-1.0f)
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_ARGUMENT,
        MotorController_SetDuty(101.0f)
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_ARGUMENT,
        MotorController_SetDuty(NAN)
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_ARGUMENT,
        MotorController_SetDirection((MotorDirection_t)99)
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_ARGUMENT,
        MotorController_ReportFault(
            MOTOR_CONTROLLER_FAULT_NONE
        )
    );
    EXPECT_STATUS(
        MOTOR_CONTROLLER_STATUS_INVALID_ARGUMENT,
        MotorController_ReportFault(
            (MotorControllerFault_t)99
        )
    );

    EXPECT_STATE(MOTOR_CONTROLLER_STATE_READY);
    EXPECT_FAULT(MOTOR_CONTROLLER_FAULT_NONE);
    EXPECT_TRUE(!FakeMotorDriver_IsEnabled());
}

int main(void)
{
    TestPreInitialization();
    TestInitializationAndArming();
    TestRunCoastAndDirectionInterlock();
    TestBrakeAndRelease();
    TestDisableAndSoftwareFault();
    TestDiagnosticFaults();
    TestFaultClearFailures();
    TestDriverFailures();
    TestInvalidArguments();

    if (failure_count != 0U)
    {
        (void)printf(
            "%u of %u assertions failed.\n",
            failure_count,
            assertion_count
        );
        return 1;
    }

    (void)printf(
        "All %u motor-controller assertions passed.\n",
        assertion_count
    );

    return 0;
}
