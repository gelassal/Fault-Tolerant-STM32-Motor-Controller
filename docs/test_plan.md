# Validation Test Plan

## Test Record Format

Each completed test records the date, firmware commit or worktree state,
hardware revision, procedure, expected result, actual result, pass/fail status,
and supporting artifact location. Pending physical tests must not be presented
as completed measurements.

## HOST-CTRL-001 - Motor Safety State Machine

Requirements: SAFE-001, SAFE-003, SAFE-004

Procedure:

1. From `tests/motor_controller`, run `mingw32-make run`.
2. Compile the production controller against the fake driver using `-Wall
   -Wextra -Werror -pedantic`.
3. Exercise all legal and illegal transitions, invalid arguments, coast/brake
   behavior, diagnostic and software faults, driver failures, latching, and
   fault clearing.

Expected result: all assertions pass and no nonzero duty is preloaded while the
driver is disabled.

Recorded result, 2026-08-07: **PASS - 147 of 147 assertions passed.**

## BUILD-001 - STM32 Debug Firmware

Procedure: refresh the CubeIDE managed build and build the Debug configuration.

Expected result: `control_node.elf` links with `motor_controller.c` and no
compiler warnings.

Recorded result, 2026-08-07: **PASS - zero warnings.** Image size was 26,952
bytes text, 100 bytes initialized data, and 2,260 bytes BSS. Result was produced
from the uncommitted integration worktree.

## UART-STATE-001 - Controller Transition Sequence

Status: **PENDING BOARD TEST**

Run with no external carrier and PB10 internally pulled up:

```text
status
duty 25
enable
status
duty 25
status
reverse
duty 0
reverse
status
brake
status
release
disable
injectfault
status
enable
clearfault
status
```

Expected behavior:

| Action | Expected result |
|---|---|
| Initial status | disabled, duty 0, fault none |
| Duty while disabled | rejected |
| Enable | READY/coast; EN low, ENB high |
| Duty 25 | RUNNING; selected PWM channel active |
| Reverse while running | rejected without output change |
| Duty 0 | READY/coast; both PWM zero and driver disabled |
| Reverse in READY | accepted |
| Brake | BRAKING; both PWM low, EN high, ENB low |
| Release | READY/coast; driver disabled |
| Inject fault | FAULT; zero PWM and driver disabled |
| Enable while faulted | rejected |
| Clear fault | DISABLED with no fault |

Record the UART transcript plus `controller_state`, `latched_fault`,
`commanded_duty_percent`, `commanded_direction`, `TIM4->CCR1`, and
`TIM8->CCR2`.

## SCOPE-PWM-001 - MCU PWM Outputs

Status: **PENDING EQUIPMENT-GATED TEST**

Measure PB6 and PC7 with the carrier disconnected. Record frequency, 10%, 25%,
and 50% duty, forward/reverse routing, coast, braking, disable, and fault
shutdown. The measured frequency must not exceed 20.0 kHz.

## DRIVER-LOGIC-001 - Logic-Only Carrier

Status: **PENDING SOLDERING AND EQUIPMENT GATE**

Verify carrier assembly, 5 V VCC, common ground, safe startup levels, DIAG, and
absence of shorts with VIN and the motor disconnected.

## DRIVER-POWER-001 - Powered Carrier Without Motor

Status: **PENDING DRIVER-LOGIC-001**

Use fused, current-limited 12 V power with OUT1/OUT2 disconnected. Validate all
controller states, DIAG, PWM routing, and temperature before connecting a motor.

## MOTOR-SPIN-001 - First Unloaded Motor Spin

Status: **PENDING DRIVER-POWER-001**

Use a 0.5 A supply limit and the staged procedure in `wiring.md`. Demonstrate
the lowest successful forward and reverse duty, READY/coast, disable, and
software-fault shutdown. Record maximum observed current, supply settings,
measured PWM, UART transcript, scope captures, and pass/fail result.
