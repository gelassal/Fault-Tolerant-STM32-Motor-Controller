# Development Log

## Entry Template

### YYYY-MM-DD — Entry Title

**Goal**

Describe what was supposed to be completed.

**Work Completed**

Describe the hardware, firmware, and testing work completed.

**Observations**

Record measurements, unexpected behavior, and debugging information.

**Problems**

Record failures and unresolved issues.

**Decisions**

Record design choices and the reason behind them.

**Next Step**

State the next concrete action.

---

## Initial Entry

### 2026-07-24 — Repository Initialization

**Goal**

Establish the repository structure, initial requirements, architecture, and
validation plan.

**Work Completed**

- Created the GitHub repository.
- Added the planned directory structure.
- Documented initial safety, control, sensing, telemetry, and validation
  requirements.
- Added the initial architecture and test plan.

**Observations**

No hardware testing has been completed.

**Problems**

Hardware and STM32 pin assignments have not yet been finalized.

**Decisions**

Basic motor control will be validated without FreeRTOS before the application
is migrated into an RTOS architecture.

**Next Step**

Install STM32CubeIDE and perform initial board, UART, and debugger bring-up.

---

### 2026-07-26 — NUCLEO-F446RE Toolchain and Board Bring-Up

**Goal**

Create a reproducible STM32 development environment and verify that the
NUCLEO-F446RE can be configured, built, flashed, monitored, and debugged.

**Work Completed**

- Installed STM32CubeIDE and standalone STM32CubeMX.
- Corrected the target board from the originally planned NUCLEO-G474RE to the
  actual NUCLEO-F446RE and used the STM32CubeF4 firmware package.
- Generated a complete `control_node` project containing the CubeMX `.ioc`
  configuration, `Core`, `Drivers`, linker files, and CubeIDE build metadata.
- Imported the generated project into STM32CubeIDE and verified a clean build.
- Programmed the onboard LD2 user LED and confirmed continuous blinking using
  `GPIOA` pin 5.
- Configured USART2 on PA2/PA3 for 115200 baud, 8 data bits, no parity, and one
  stop bit.
- Printed a boot message through the ST-LINK virtual COM port identifying the
  firmware version, board, and startup state as `DISABLED`.
- Verified source-level debugging by setting and hitting a breakpoint in the
  LED toggle loop.

**Observations**

- The onboard ST-LINK connection successfully supports power, programming,
  debugging, and the USART2 virtual COM port through one USB connection.
- The LED continued blinking while UART diagnostics were active.
- The final generated project correctly exposed the F446RE peripherals and
  build files in CubeIDE.

**Problems**

- The first CubeMX/CubeIDE attempts targeted the wrong MCU family and produced
  an incomplete Eclipse project shell without `Core`, `Drivers`, or a valid
  `.ioc` file.
- The initial LED code used undefined `LD2_GPIO_Port` and `LD2_Pin` symbols and
  also introduced mismatched braces in the generated `while (1)` section.
- CubeIDE's visible Terminal view did not provide a serial-terminal option, so
  the ST-LINK virtual COM connection had to be opened through the serial
  connection tooling instead.

**Decisions**

- Retain the NUCLEO-F446RE. It provides sufficient timers, ADC/DMA capability,
  FreeRTOS support, and two classic CAN peripherals for the project.
- Use classic CAN through the F446RE bxCAN peripheral rather than CAN FD.
- Keep all manually written application code inside CubeMX `USER CODE`
  sections so regeneration does not overwrite it.

**Next Step**

Configure two PWM outputs and create a reusable motor-driver module with a
safe disabled startup state.

---

### 2026-08-04 — Dual-PWM Motor Driver Interface and Debug Validation

**Goal**

Create and validate the firmware abstraction that will eventually control the
TB9051FTG H-bridge while keeping all physical motor outputs safely disabled.

**Work Completed**

- Configured TIM4 Channel 1 on PB6 as the first PWM output.
- Configured TIM8 Channel 2 on PC7 as the second PWM output.
- Added CubeMX GPIO labels and startup configurations for:
  - `MOTOR_EN` on PA8, initialized low.
  - `MOTOR_ENB` on PA10, initialized high.
  - `MOTOR_DIAG` on PB10, configured as an input.
- Added `motor_driver.h` and `motor_driver.c` with:
  - Hardware configuration and state structures.
  - Direction and status enumerations.
  - Initialization, enable, disable, duty-cycle, direction, brake, and fault
    query functions.
  - Duty-cycle clamping and percentage-to-timer-compare conversion.
  - Safe handling that forces both PWM channels low during initialization,
    disable, and direction changes.
- Updated `main.c` so the motor-driver module owns both PWM channels.
- Preserved the UART boot message and initialized the driver through a
  `MotorDriverConfig_t` structure.
- Kept normal startup explicitly disabled with `MotorDriver_Disable()`.
- Used CubeIDE breakpoints and expressions to validate internal state and timer
  compare registers.
- Verified the following disabled-state values:
  - `motor_initialized = true`
  - `motor_enabled = false`
  - `motor_duty_percent = 0.0`
  - `TIM4->CCR1 = 0`
  - `TIM8->CCR2 = 0`
- Temporarily tested 25% software duty routing:
  - Forward: `TIM4->CCR1 = 16383`, `TIM8->CCR2 = 0`
  - Reverse: `TIM4->CCR1 = 0`, `TIM8->CCR2 = 16383`
- Removed the temporary motion commands after debugging so production startup
  remains disabled.

**Observations**

- A compare value of 16383 is approximately 25% of the current 65535 timer
  period, confirming the duty conversion logic.
- Direction selection correctly routes PWM to only one channel at a time.
- Changing direction clears the stored duty and forces both channels to zero
  before a new nonzero duty can be commanded.
- The firmware now has a reusable hardware abstraction, but the physical PWM
  frequency and voltage levels have not yet been measured.

**Problems**

- The motor, TB9051FTG carrier, current sensor, power hardware, and wiring have
  not yet been purchased.
- An oscilloscope was not available during this stage, so PB6 and PC7 have not
  yet been physically verified for frequency, duty cycle, and voltage level.
- The current timer settings are still based on a 65535 period and have not yet
  been adjusted to the final 20 kHz PWM target.

**Decisions**

- Continue software-only validation while the motor-control hardware is being
  ordered.
- Keep both PWM timers running with zero compare values after initialization,
  while using the driver's EN/ENB signals as an additional hardware-disable
  layer.
- Do not connect motor power until both PWM outputs and safe startup levels are
  verified with an oscilloscope.

**Next Step**

Calculate matching 20 kHz settings for TIM4 and TIM8, verify both PWM outputs
with an oscilloscope, and order the motor, TB9051FTG driver, power, fuse,
cutoff, and wiring components required for the first hardware bring-up.

### 2026-08-04 — Dual-PWM Configuration and Interrupt-Driven UART Console

**Goal**

Configure both motor-control PWM outputs for the same 20 kHz switching
frequency and implement a modular UART interface for controlling and inspecting
the motor driver.

**Work Completed**

- Confirmed that both the APB1 timer clock used by TIM4 and the APB2 timer
  clock used by TIM8 were configured to 84 MHz.
- Used the PWM frequency relationship:

  `PWM frequency = timer clock / ((prescaler + 1) × (period + 1))`

- Configured both TIM4 and TIM8 with:
  - Prescaler: `0`
  - Counter period: `4199`
  - Initial pulse: `0`
- Verified that these settings produce a calculated PWM frequency of:

  `84,000,000 / (1 × 4200) = 20,000 Hz`

- Configured:
  - `TIM4_CH1` on PB6 as PWM1
  - `TIM8_CH2` on PC7 as PWM2
- Used debugger register inspection to validate duty-cycle conversion and
  direction routing.
- Confirmed approximately:
  - 25% duty produced CCR = `1049`
  - 50% duty produced CCR = `2099`
  - 100% duty produced CCR = `4199`
- Confirmed duty commands below 0% were clamped to 0% and commands above 100%
  were clamped to 100%.
- Confirmed forward operation placed PWM on `TIM4->CCR1` while
  `TIM8->CCR2` remained zero.
- Confirmed reverse operation placed PWM on `TIM8->CCR2` while
  `TIM4->CCR1` remained zero.
- Confirmed that changing direction cleared the previous duty command and that
  disabling the driver returned both compare registers to zero.
- Enabled the USART2 global interrupt.
- Added `command_console.c` and `command_console.h`.
- Implemented interrupt-driven, one-byte UART reception using
  `HAL_UART_Receive_IT()`.
- Buffered received characters until a complete command was entered.
- Kept command parsing, UART transmission, and motor-control operations outside
  the UART interrupt callback.
- Added `enable`, `disable`, `forward`, `reverse`, `duty`, `brake`, `status`,
  and `help` commands.
- Added command validation and prevented nonzero duty commands while the motor
  driver was disabled.
- Added support for CR, LF, and CR+LF terminal line endings.
- Replaced the blocking LED delay with nonblocking `HAL_GetTick()` timing.

**Observations**

- Both PWM peripherals use an 84 MHz timer clock, allowing TIM4 and TIM8 to use
  identical prescaler and period settings.
- Debugger inspection confirmed the expected compare-register values for
  forward, reverse, clamped duty, direction changes, and disable operations.
- Physical 20 kHz waveform verification has not yet been performed with an
  oscilloscope.
- The CubeIDE serial console did not display characters through local echo, so
  the firmware echoes each completed command after Enter is pressed.
- Commands were successfully received and executed through the ST-LINK virtual
  COM port at 115200 baud.
- Debugger inspection confirmed that UART commands correctly updated
  `TIM4->CCR1`, `TIM8->CCR2`, driver enable state, direction, and stored duty.

**Problems**

- Physical PWM waveform validation remains pending because the oscilloscope is
  not currently available.
- Motor-driver and motor hardware testing remains pending because the external
  components have not yet arrived.

**Decisions**

- Both PWM channels will operate at 20 kHz to keep the switching frequency
  consistent across forward and reverse operation and above the most audible
  frequency range.
- PWM timers will remain running continuously while duty is controlled through
  their compare registers.
- Normal startup will keep both compare registers at zero and the motor driver
  disabled.
- UART functionality remains isolated in the command-console module. The motor
  driver has no dependency on UART, allowing the same API to be reused later by
  CAN communication and FreeRTOS tasks.

**Next Step**

Verify the TIM4_CH1 and TIM8_CH2 outputs as 20 kHz signals using an oscilloscope,
then begin TB9051FTG logic-level hardware bring-up after the required components
arrive.

---

### 2026-08-07 - Motor Safety State Machine Integration

**Goal**

Place a high-level safety policy between UART commands and the low-level motor
driver, verify it with native tests, and prepare a gated hardware procedure.

**Work Completed**

- Added `motor_controller` with disabled, ready, running, braking, and fault
  states plus explicit controller status and fault codes.
- Defined READY as armed but electrically coasting: the H-bridge remains
  disabled until nonzero duty or an explicit brake command.
- Implemented safe motion entry that requests zero duty, enables the driver,
  checks DIAG, and only then applies nonzero PWM.
- Made `duty 0` and `release` disable the H-bridge and return to READY/coast.
- Enforced direction changes only in READY, latched all reported or detected
  faults, and required explicit clear back to DISABLED.
- Routed all UART motor commands and status reporting through
  `MotorController`; added `release`, `injectfault`, and `clearfault`.
- Split HAL-free motor types and runtime driver declarations from the STM32
  configuration interface to support native tests.
- Added a MinGW C test suite with a fake driver and failure injection.
- Corrected the README, architecture, wiring, and test plan for the actual
  NUCLEO-F446RE and staged TB9051FTG bring-up.

**Verification**

- All 147 native assertions passed with `-Wall -Wextra -Werror -pedantic`.
- ARM syntax checks passed for `main.c`, `motor_driver.c`,
  `motor_controller.c`, and `command_console.c` with warnings treated as
  errors.
- The CubeIDE Debug firmware linked successfully with zero warnings.
- Linked image size was 26,952 bytes text, 100 bytes initialized data, and
  2,260 bytes BSS.

**Observations**

- The TB9051FTG two-PWM truth table makes both PWM inputs low while enabled a
  short-brake state. The earlier design's READY and release behavior would
  therefore have remained actively braking instead of coasting.
- Native tests confirmed that the controller never preloads nonzero duty while
  the driver is disabled.
- The exact Pololu #2997 carrier and #4866 motor are available, and an
  oscilloscope is available at home.

**Problems / Open Gates**

- UART behavior and STM32 register outputs have not yet been revalidated on the
  physical Nucleo with the integrated controller.
- The carrier still requires soldering.
- A suitable current-limited 12 V supply, 1 A fuse, cutoff, multimeter, wiring,
  and motor restraint must be confirmed before powered testing.
- No scope or motor measurements have been recorded yet.

**Decisions**

- Keep the current 20 kHz nominal configuration for the first scope check. If
  measured frequency exceeds the carrier's 20 kHz limit, change both timers to
  ARR 4299 for a nominal 19.53 kHz.
- Keep PB10 pulled up only for carrier-disconnected testing; change it to
  `GPIO_NOPULL` before wiring the carrier, which provides the DIAG pull-up.
- Do not connect motor power until host tests, firmware build, MCU-only scope
  checks, logic-only wiring, and motor-disconnected power checks pass in order.

**Next Step**

Flash the integrated firmware with no carrier connected, run UART-STATE-001,
and capture the PB6/PC7/EN/ENB waveforms required by SCOPE-PWM-001.

---

### 2026-08-11 — Safety-State Validation, Hardware Assembly, and TB9051 Logic Bring-Up

**Goal**

Validate the integrated motor-controller safety state machine on the physical
NUCLEO-F446RE, assemble the purchased motor-control carriers, and begin
logic-only TB9051FTG integration without applying motor power.

**Work Completed**

- Used production baseline commit `3a7cd32`, which was pushed to GitHub after
  passing all 147 host assertions and a warning-free STM32 firmware build.
- Flashed firmware version `0.1.0` to the NUCLEO-F446RE and opened the ST-LINK
  virtual COM port at 115200 baud, 8 data bits, no parity, one stop bit, and no
  flow control.
- Confirmed normal boot in `DISABLED` and exercised the safety state machine
  through the UART command console.
- Verified that:
  - A nonzero duty command is rejected in `DISABLED`.
  - `enable` transitions `DISABLED -> READY` with zero duty and coast output.
  - Nonzero duty transitions `READY -> RUNNING`.
  - Direction changes are rejected in `RUNNING`.
  - `duty 0` transitions `RUNNING -> READY` and disables the output.
  - Direction changes are accepted in `READY`.
  - `brake` transitions to `BRAKING` with zero commanded duty.
  - `release` transitions `BRAKING -> READY` and restores coast output.
  - A software-injected fault transitions `RUNNING -> FAULT`, clears duty,
    rejects re-enable, remains latched, and clears only through `clearfault`.
  - Unknown or malformed console commands are rejected.
- Performed an additional direction-interlock test starting with reverse
  selected. A `forward` command in `RUNNING` was rejected, then accepted after
  `duty 0` returned the controller to `READY`.
- Confirmed the purchased hardware as:
  - Pololu #4866 75:1 Metal Gearmotor 25Dx69L MP 12 V with 48 CPR encoder.
  - Pololu #2997 TB9051FTG single brushed DC motor-driver carrier.
  - Pololu #4041 ACS724LLCTR-05AB current-sensor carrier.
  - Pololu #2676 25D metal gearmotor bracket pair.
- Soldered the TB9051 logic header, VIN/GND terminal, and OUT1/OUT2 terminal,
  then visually inspected the assembled carrier.
- Soldered the ACS724 low-voltage logic header. Its high-current path remains
  disconnected.
- Changed PB10/MOTOR_DIAG to `GPIO_NOPULL` in `main.c` and
  `control_node.ioc` for use with the carrier's DIAG pull-up.
- Connected the TB9051 logic interface:
  - Nucleo 5 V -> TB9051 VCC
  - Nucleo GND -> TB9051 GND
  - PB6 / TIM4_CH1 -> TB9051 PWM1
  - PC7 / TIM8_CH2 -> TB9051 PWM2
  - PA8 -> TB9051 EN
  - PA10 -> TB9051 ENB
  - PB10 <- TB9051 DIAG
- Kept TB9051 VIN, OUT1/OUT2, the motor, encoder, ACS724 high-current path,
  and all 12 V motor power disconnected.
- Exercised the physical DIAG path with the carrier logic powered but VIN
  absent. After `enable`, the controller detected the carrier's diagnostic
  condition, latched `FAULT`, rejected a subsequent `duty 50`, and reported
  `ERR physical fault is still present` when `clearfault` was attempted.

**Observations**

- The UART state transitions and direction interlock matched the intended
  safety policy on the physical Nucleo.
- The VIN-absent DIAG test was the first validation of a real carrier fault
  propagating through TB9051 DIAG, STM32 PB10, controller fault monitoring, and
  the latched `FAULT` state.
- The `disable` command safely disables the output without clearing a latched
  fault, as designed. However, the console currently prints the hard-coded
  response `OK state=disabled duty=0`, which can misreport the actual retained
  `FAULT` state.
- No oscilloscope was available during this session. PWM validation remains
  limited to the 84 MHz timer configuration, ARR 4199 calculation, earlier
  debugger/register checks, and automated tests. Physical frequency, duty,
  voltage-level, braking, and fault-shutdown waveforms remain unmeasured.
- The configured PWM frequency remains nominally 20 kHz:

  `84,000,000 / 4200 = 20,000 Hz`

- A multimeter and possible battery-supported 12 V source may be available,
  but a suitable regulated/current-limited supply, fuse, and cutoff hardware
  have not yet been confirmed.

**Problems / Open Gates**

- Correct the UART `disable` success response so it reports the controller's
  actual post-command state instead of always claiming `DISABLED`.
- Physically measure PB6 and PC7 frequency, duty scaling, direction routing,
  disabled output, braking output, and fault shutdown when an oscilloscope is
  available.
- Confirm the required 12 V supply, current limiting, inline fuse, cutoff,
  multimeter, power wiring, and motor restraint before powered testing.
- The TB9051 has not received 12 V, OUT1/OUT2 remain unloaded, and the motor
  has not been spun.
- The encoder and ACS724 high-current path remain intentionally disconnected.

**Decisions**

- Use the carrier's DIAG pull-up and keep the STM32 PB10 input configured with
  no internal pull resistor while the carrier is connected.
- Treat the misleading `disable` response as a telemetry/UI defect; the
  observed output shutdown and retained fault latch behaved safely.
- Do not apply motor power until the remaining equipment gate and
  carrier-disconnected validation requirements are satisfied.
- Perform the first powered-carrier test with the motor and OUT1/OUT2 still
  disconnected before attempting an unloaded motor spin.
- Keep encoder feedback, ACS724 current sensing, closed-loop control, RTOS,
  and CAN work outside this bring-up milestone.

**Next Step**

Correct and revalidate the `disable` response, complete the oscilloscope and
power-equipment gates, then apply fused/current-limited 12 V to the TB9051 with
the motor still disconnected and verify that the undervoltage DIAG condition
clears without unexpected current draw or heating.

---

### 2026-08-12 — Encoder Foundation and Automated UART HIL Skeleton

**Goal**

Correct misleading command telemetry, implement the encoder measurement
foundation without requiring motor power, verify its arithmetic natively, and
define an objective motor-disconnected powered-carrier test.

**Work Completed**

- Reworked controller-command responses so successful commands print the
  controller's actual state, direction, duty, and fault. Rejected controller
  commands now also print the retained state after the error.
- Removed the hard-coded `disable` response that incorrectly claimed
  `DISABLED` when a fault remained latched.
- Configured TIM3 in quadrature encoder mode on PB4/TIM3_CH1 and PB5/TIM3_CH2:
  - Encoder mode TI12, counting both edges of both channels.
  - 16-bit period of 65,535.
  - Input filter value 4 on both channels.
  - No GPIO pull resistors.
- Added `encoder_driver.c/.h` with nonblocking 10 ms sampling and getters for
  continuous signed count, sample delta, gearbox-output RPM, and direction.
- Added a configurable direction inversion for later physical A/B polarity
  calibration.
- Added HAL-free `encoder_math.c/.h` for portable signed-delta, rollover,
  accumulated-position, direction, and RPM calculations.
- Used the Pololu #4866 specification of 3,591.84 counts per gearbox-output
  revolution (48 counts per motor revolution through the exact 74.83:1 ratio).
- Added native encoder tests covering positive/negative motion, zero speed,
  forward/reverse counter wrap, accumulation, high count rate, exact RPM
  conversion, direction inversion, and invalid inputs.
- Added a Python/pyserial UART HIL smoke-test skeleton for `status`, `enable`,
  `duty 25`, `injectfault`, and `clearfault`. The script parses actual state
  responses and requires explicit motor-disconnected acknowledgement.
- Added a quantitative DRIVER-POWER-001 procedure with voltage, current,
  state/output, and abort criteria for the first 12 V test with OUT1/OUT2 and
  the motor disconnected.

**Verification**

- All 57 encoder assertions passed with
  `-Wall -Wextra -Werror -pedantic`.
- All 147 existing motor-controller assertions still passed.
- The Python HIL script passed bytecode syntax validation.
- CubeIDE managed-build metadata discovered both encoder production sources.
- A complete forced STM32 rebuild compiled and linked every source with zero
  warnings.
- Linked image size was 28,388 bytes text, 100 bytes initialized data, and
  2,384 bytes BSS (30,872 bytes total).

**Observations**

- At the motor's typical 100 RPM gearbox-output speed, a 10 ms interval yields
  approximately 60 counts per sample. This is far below the 32,768-count
  modular-delta ambiguity limit.
- The current encoder direction names are a logical convention. PB4/PB5 channel
  order must be physically checked before forward/reverse sign is accepted.
- The expected-success HIL sequence cannot pass with the carrier logic powered
  but VIN absent because that real undervoltage condition correctly holds DIAG
  faulted.

**Problems / Open Gates**

- TIM3 counter movement, direction polarity, and RPM have not been validated
  with physical encoder signals.
- The updated UART response format and encoder initialization must be reflashed
  and smoke-tested on the Nucleo.
- The oscilloscope is not currently available, so PWM measurements remain
  pending.
- The protected/current-limited 12 V equipment gate must be completed before
  DRIVER-POWER-001 is executed.

**Decisions**

- Keep encoder math separate from the HAL wrapper so rollover and conversion
  behavior can be exhaustively tested on the host.
- Calculate gearbox-output RPM using the exact product count rather than the
  rounded 75:1 marketing ratio.
- Keep encoder measurement observational for now; do not add encoder-loss
  faults or closed-loop control until physical polarity and counts are verified.
- Keep FreeRTOS deferred until encoder acquisition and powered open-loop
  hardware behavior are validated synchronously.

**Next Step**

Review and flash the new firmware, verify the dynamic UART responses, then
complete the supply/fuse/cutoff gate and execute DRIVER-POWER-001 with the motor
and OUT1/OUT2 disconnected. Physical encoder wiring and direction calibration
follow only after the powered driver passes.

---

### 2026-08-14 — UART Fault-Telemetry Validation and Power-Test Readiness

**Goal**

Physically verify the corrected dynamic UART responses, confirm that fault
latching is reported accurately, and close the equipment-acquisition portion
of the motor-disconnected power-test gate.

**Work Completed**

- Flashed and ran the integrated firmware on the NUCLEO-F446RE.
- Entered `injectfault`, followed by `disable` and `status`, and physically
  verified through the ST-LINK UART console that:
  - `injectfault` transitioned the controller to `FAULT` with
    `fault=software` and duty zero.
  - `disable` safely retained and reported `state=fault` rather than
    incorrectly claiming `DISABLED`.
  - A following `status` still reported the latched `FAULT` state.
- Pressed reset and verified the safe startup report:
  `state=disabled direction=forward duty=0 fault=none`.
- Verified that malformed commands, including `inject fault`, were rejected as
  unknown commands without changing controller state.
- Acquired or confirmed the following powered-test equipment:
  - Adjustable, regulated 12 V supply rated for at least 2 A with current
    limiting.
  - 1 A inline fuse.
  - DC kill switch rated for at least 12 V and 2 A.
  - Multimeter.
  - Proper insulated power wire and secure terminations.
- Confirmed that an oscilloscope with x10 probes will be available before the
  powered waveform test.

**Observations**

- The dynamic console response now reflects the controller's real post-command
  state. `disable` makes the physical output safe but intentionally does not
  clear a latched fault.
- Reset reinitializes the RAM-held software fault latch and restores the
  defined safe startup state. Because the driver is disabled after reset, this
  does not by itself prove that a physical DIAG condition has cleared.
- No 12 V motor supply has been applied. OUT1, OUT2, the motor, encoder leads,
  and ACS724 high-current path remain disconnected.
- No physical encoder count, direction, or RPM measurement has occurred.

**Problems / Open Gates**

- The oscilloscope is not yet on the bench, so DRIVER-POWER-001 must not begin.
- The unpowered carrier inspection, VIN-to-GND continuity check, protected
  supply-harness assembly, and loose-lead polarity measurement still need to
  be performed and recorded.
- The secure motor-restraint gate remains open for MOTOR-SPIN-001; it does not
  block the current motor-disconnected milestone.

**Decisions**

- Treat a low TB9051 DIAG level as expected while the carrier is disabled or
  coasting (`EN=0` or `ENB=1`). Evaluate DIAG as a hardware fault only while
  the driver is commanded enabled (`EN=1`, `ENB=0`) with valid VIN and VCC.
- Treat 50 mA as a conservative investigation threshold for the unloaded
  carrier, not as a hard datasheet pass/fail limit. If measured current is at
  or above 50 mA, stop and investigate before continuing.
- Retain TIM4/TIM8 ARR `4199` for the intentional nominal 20.0 kHz PWM
  configuration. Change both timers to ARR `4299` only if the oscilloscope
  genuinely measures a frequency above 20.0 kHz.
- Keep the motor, encoder, and ACS724 current path disconnected throughout
  DRIVER-POWER-001, and review its records before authorizing a motor test.

**Next Step**

With all power sources off, inspect the carrier, verify that VIN is not
shorted to GND, and assemble the fused kill-switch harness. Once the
oscilloscope is present, execute DRIVER-POWER-001 at 12.0 V with a 0.25 A
current limit and record every required voltage, current, UART, DIAG, and
waveform result before proceeding.

---

### 2026-08-18 — Motor-Disconnected 12 V Bring-Up and PWM Validation

**Goal**

Assemble and validate the protected 12 V supply path, apply motor power to the
TB9051 carrier for the first time with all outputs unloaded, and measure the
forward/reverse PWM and software-fault behavior without connecting the motor.

**Work Completed**

- Kept OUT1, OUT2, the motor, encoder leads, and ACS724 high-current path
  disconnected throughout the session.
- Assembled the protected supply path using an inline blade-fuse holder and an
  illuminated three-wire kill switch:
  - Supply positive -> inline fuse -> switch red/input wire.
  - Switch yellow/switched-positive wire -> TB9051 VIN.
  - Supply negative and switch black/indicator-ground wire -> common system
    ground.
- Performed unpowered checks before applying motor power:
  - VIN-to-GND resistance was initially approximately 9 MOhm and later
    approximately 5-7 MOhm with the completed harness connected.
  - The fuse, fuse holder, switch path, and completed positive/negative wiring
    paths showed approximately 0 Ohm where continuity was required.
  - The kill switch produced an open circuit while off and continuity while
    on.
- Configured the regulated supply for 12.0 V with a 0.25 A current limit and
  measured 12.0 V at its unloaded output. The supply correctly displayed 0 A
  in constant-voltage mode with no load attached.
- Found and corrected a logic-power wiring error: the TB9051 VCC lead had been
  connected to the Nucleo `E5V` external-input pin instead of the USB-powered
  `+5V` output pin.
- After correction, measured the same 4.72 V at the Nucleo `+5V` pin and the
  TB9051 VCC pin, showing no measurable wiring drop.
- Confirmed safe UART startup before applying 12 V:
  `state=disabled direction=forward duty=0 fault=none`.
- Applied fused, current-limited 12 V to the motor-disconnected carrier:
  - Carrier VIN measured 12.0 V.
  - Supply remained in constant-voltage mode.
  - Maximum observed current was 0.002 A.
  - No heating, odor, noise, voltage collapse, or other unexpected behavior
    occurred.
- Visually confirmed that the installed inline fuse is marked 1 A.
- Completed the required two-minute powered, unloaded observation. Current
  remained stable at approximately 0.002 A and well below 0.05 A, the supply
  remained in constant-voltage mode, and no heating, odor, noise, instability,
  or voltage collapse occurred.
- Verified the kill switch removed external motor power. With USB logic power
  still present, VIN measured 0.56 V; after both sources were removed and the
  circuit discharged, VIN fell below 0.5 V.
- Scoped the forward PWM output on PB6/TIM4_CH1:
  - Amplitude: 3.44 V.
  - Duty cycle: 9.87% for a 10% command.
  - Frequency: 19.90 kHz.
  - PC7/PWM2 remained low.
- Scoped the reverse PWM output on PC7/TIM8_CH2:
  - Amplitude: 3.28 V.
  - Duty cycle: 10.0% for a 10% command.
  - Frequency: 19.90 kHz.
  - PB6/PWM1 remained low.
- Captured final clean PWM waveforms during repeated verification:
  - CH2 measured 19.92 kHz and 9.72% duty for a 10% command.
  - CH1 measured 19.95 kHz and 19.71% duty for an intentional 20% command.
  - Both channels showed clean rising and falling edges.
- Verified through the UART and both PWM probes that:
  - `brake` forced both PWM channels low.
  - `release` kept both PWM channels low while returning to READY/coast.
  - `injectfault` immediately removed PWM and reported a software FAULT.
  - `disable` retained the latched FAULT.
  - `clearfault` returned the controller to DISABLED.
- Completed the remaining multimeter checks of EN, ENB, and DIAG. Every
  measured state matched the required truth table:
  - DISABLED, READY, RELEASE, and FAULT: EN low, ENB high, and DIAG low.
  - RUNNING and BRAKING: EN high, ENB low, and DIAG deasserted/high with
    valid VIN and VCC.
- Shut down normally from a safe state using the kill switch and supply-output
  control.

**Observations**

- The earlier 0.7-3.6 V readings were taken from VIN rather than VCC while the
  12 V source was disabled or disconnected. They were not valid measurements
  of the TB9051 logic rail.
- The 0.56 V VIN reading with the kill switch off and USB connected is
  consistent with a weak, high-impedance residual or logic-side backfeed; it
  disappeared below 0.5 V when both power sources were removed.
- The measured 4.72 V VCC is slightly below the test plan's conservative
  4.75 V lower target but remains inside the TB9051FTG datasheet operating
  range of 4.5-5.5 V. Both ends of the logic-power wire measured identically.
- Both physical PWM channels were below the carrier's 20.0 kHz maximum and
  closely matched their commanded duty cycles.
- An anomalous PB6 waveform seen during repeated probing was resolved by
  correcting the measurement setup. The specific setup cause was not recorded,
  so no hardware or firmware root cause is claimed.
- Unloaded current remained far below the 50 mA investigation threshold,
  including throughout the two-minute observation.

**Problems / Open Gates**

- Scope screenshots and a saved UART transcript/artifact location still need
  to be added to the permanent test record.
- The EN/ENB/DIAG checks were reported as passing expected low/high levels,
  but exact numerical voltages were not transcribed into the test record.
- The motor, encoder, and ACS724 current path have not been powered or
  physically validated.

**Decisions**

- Retain TIM4 and TIM8 ARR `4199`; the measured 19.90 kHz PWM does not exceed
  the carrier's 20.0 kHz maximum, so the fallback ARR `4299` change is not
  required.
- Accept 4.72 V as electrically within the TB9051FTG VCC operating range while
  recording it as a deviation from the project's tighter nominal target.
- Mark DRIVER-POWER-001 as **PASS**. The permanent artifact paths and exact
  EN/ENB/DIAG voltage values remain documentation limitations, but the required
  electrical behavior and safety checks passed.
- Authorize preparation and review of MOTOR-SPIN-001. Keep the motor physically
  disconnected until that procedure and the secure-restraint gate are reviewed.

**Next Step**

Prepare and review the gated MOTOR-SPIN-001 procedure, including the secure
motor restraint, 0.5 A supply limit, lowest-duty forward/reverse sequence, and
software-fault shutdown test. Keep the motor disconnected until that review is
complete.

---

### 2026-08-19 — First Unloaded Motor Spin and Fault Shutdown

**Goal**

Complete MOTOR-SPIN-001 using the exact Pololu #4866 motor and #2997 carrier,
demonstrate controlled low-duty operation in both directions, and verify that a
software fault removes drive and remains latched.

**Work Completed**

- Secured the unloaded motor with the output shaft unobstructed and the kill
  switch within reach.
- With all power removed, connected the red motor lead to OUT1 and the black
  motor lead to OUT2. Kept the green, blue, yellow, and white encoder leads and
  the ACS724 current path disconnected and insulated.
- Completed the unpowered wiring checks:
  - OUT1-to-GND and OUT2-to-GND each measured approximately 0.8 MOhm.
  - The motor-winding continuity check passed.
- Set the supply to 12.0 V with a 0.5 A current limit. In DISABLED and READY,
  the supply remained in constant-voltage mode at approximately 0.002 A and the
  motor remained stationary.
- Tested forward rotation:
  - The motor did not start at 10% duty.
  - The lowest successful command was 12% duty.
  - Forward produced counterclockwise rotation when viewed directly at the
    output shaft and drew approximately 0.036 A.
- Returned to READY/coast before changing direction, then verified reverse at
  12% duty. Reverse produced clockwise rotation at approximately 0.036 A.
- Performed the software-fault shutdown test while running at 12% duty:
  - `injectfault` immediately removed drive and the motor coasted to a stop.
  - FAULT latched with duty 0.
  - `enable` was rejected while faulted.
  - `disable` retained FAULT.
  - `clearfault` returned the controller to DISABLED with duty 0 and no fault.
- Observed no current limiting, voltage collapse, heating, odor, abnormal
  noise, restraint movement, or other unexpected behavior.
- Finished the command sequence in DISABLED with duty 0 and no active fault.

**Observations and Deviations**

- A brief unplanned 50% duty command was exercised during forward testing. It
  drew approximately 0.070 A without abnormal behavior, but exceeded the
  approved procedure's 25% ceiling and is not adopted as a future test limit.
- The highest observed supply current during MOTOR-SPIN-001 was 0.070 A. The
  validated lowest reliable starting duty was 12% in both directions.
- The red-to-OUT1 and black-to-OUT2 wiring defines firmware `forward` as
  counterclockwise when viewed from the output shaft; `reverse` is clockwise.

**Problems / Open Gates**

- Permanent UART transcript and motor-test artifact paths have not yet been
  added to the repository record.
- The encoder remains physically unpowered and unvalidated. Its count polarity,
  measured RPM, and direction convention are still open.
- The ACS724 high-current path remains disconnected and unvalidated.

**Decisions**

- Mark MOTOR-SPIN-001 as **PASS** based on successful bidirectional low-duty
  motion, safe coast behavior, bounded current, and verified fault latching and
  clearing.
- Preserve 12% as the recorded lowest successful unloaded starting duty for
  this hardware configuration; do not treat the unplanned 50% command as a
  required or approved validation point.
- Stop further motor-power testing until this result is documented and the next
  physical encoder-validation procedure is reviewed.

**Next Step**

Save the UART and motor-test evidence, checkpoint the completed bring-up
records, then prepare the logic wiring and test sequence for physical encoder
count, RPM, rollover, and direction-polarity validation.

---

### 2026-08-19 — Encoder Telemetry and Physical-Test Preparation

**Goal**

Expose the existing encoder measurements through a stable read-only UART
interface, automate its integration check, and define the gated home-bench
procedure for physical direction and RPM validation.

**Work Completed**

- Added the read-only `encoder` console command without changing the existing
  motor `status` response format.
- Defined machine-readable telemetry containing initialization state, signed
  accumulated count, signed sample delta, signed millirpm, and encoder
  direction:

  `ENCODER initialized=1 count=-1234 delta=-12 rpm_milli=-200 direction=reverse`

- Used fixed-point millirpm in the UART response so the embedded build does not
  require floating-point `printf` support.
- Added `encoder` to console help.
- Extended the Python UART smoke-test tool with `--encoder-only`. This mode
  requires DISABLED with duty 0, queries status and encoder telemetry, and never
  sends enable, duty, direction, brake, release, or fault commands.
- Added Python unit tests for signed telemetry parsing, malformed responses,
  the read-only command sequence, and rejection of an active motor state.
- Added ENCODER-HW-001 and Stage 5 documentation covering the exact lead map,
  USB-only gate, direction calibration, quadrature scope check, RPM comparison,
  fault stop, acceptance criteria, and abort conditions.

**Verification**

- All 147 native motor-controller assertions passed.
- All 57 native encoder-math assertions passed.
- All 4 Python UART parser/read-only-mode tests passed.
- The STM32 Debug firmware compiled and linked without warnings.
- Firmware image size after encoder telemetry: 29,100 bytes text, 100 bytes
  initialized data, and 2,384 bytes BSS.

**Problems / Open Gates**

- The new firmware has not yet been flashed to the Nucleo or exercised over the
  physical UART.
- The green/blue/yellow/white encoder leads remain physically unvalidated.
- Stationary count stability, signal levels, quadrature phase, direction
  polarity, measured RPM, and scope-to-firmware agreement remain pending the
  home bench.

**Decisions**

- Preserve the motor `status` wire format so the existing HIL parser remains
  compatible; expose encoder data through a separate command.
- Standardize physical wiring as green-to-ground, blue-to-+5V, yellow-to-PB4,
  and white-to-PB5. Correct an inverted sign with the existing
  `invert_direction` configuration rather than swapping A/B wiring.
- Limit the encoder test to 12%, then 15%, 20%, and at most 25% duty as needed
  for a steady measurement. Do not repeat the earlier 50% deviation.

**Next Step**

Flash the verified firmware, complete the USB-only encoder gate, then execute
ENCODER-HW-001 at home with the fused 12 V supply, 0.5 A current limit, secure
restraint, cutoff, and oscilloscope.

---

### 2026-08-20 — USB-Only Encoder Telemetry Validation and Nano-Printf Fix

**Goal**

Flash and exercise the encoder telemetry over the physical Nucleo UART while
away from the powered motor bench, then correct any integration defect before
the physical encoder is connected.

**Work Completed**

- Flashed the encoder-telemetry firmware to the Nucleo and confirmed the
  existing motor `status` command still reported DISABLED, forward, duty 0,
  and no fault.
- Confirmed `encoder` appeared in physical UART help and was recognized by the
  command parser.
- The first physical UART query exposed malformed telemetry:

  `ENCODER initialized=1 count=ld delta=-1 rpm_milli=-1 direction=0`

- Traced the corruption to the firmware's Newlib Nano `snprintf`
  implementation. Under the current `--specs=nano.specs` link configuration,
  the 64-bit `%lld` conversion was not consumed correctly; subsequent
  variadic arguments were therefore interpreted in the wrong positions.
- Added a bounded, embedded-safe signed 64-bit decimal conversion for the
  accumulated encoder count. The final `snprintf` now receives the count as a
  string while retaining the existing signed 32-bit delta and millirpm fields.
- Rebuilt, reflashed, and repeatedly queried the Nucleo. Every query returned
  the correctly structured and stable response:

  `ENCODER initialized=1 count=-1 delta=0 rpm_milli=0 direction=stationary`

**Verification**

- All 147 native motor-controller assertions passed.
- All 57 native encoder-math assertions passed.
- All 4 Python UART parser/read-only-mode tests passed.
- The STM32 Debug firmware compiled and linked without warnings.
- Firmware image size after the formatting fix: 29,380 bytes text, 100 bytes
  initialized data, and 2,384 bytes BSS.
- Physical USB/UART validation confirmed correct encoder field names, signed
  count formatting, zero delta, zero millirpm, and the `stationary` direction
  string over repeated queries.

**Problems / Open Gates**

- The encoder was physically disconnected during this café-safe USB-only
  check. PB4/PB5 were therefore not driven by real encoder signals.
- The stable accumulated count of -1 records one earlier transition while the
  disconnected inputs were floating; the zero delta and RPM show that no new
  transitions occurred during the repeated queries.
- This check does not validate encoder supply voltage, stationary stability
  with driven inputs, quadrature phase, physical direction polarity, or RPM
  accuracy. ENCODER-HW-001 remains pending.

**Decisions**

- Retain Newlib Nano and avoid enabling a larger general-purpose formatted-I/O
  implementation solely for one signed 64-bit field.
- Treat the repeated physical UART result as a PASS for command integration
  and wire-format validation only, not as a physical encoder measurement.
- Keep the 12 V motor supply disconnected until returning to the protected
  home bench.

**Next Step**

At home with all power initially off, connect encoder green to system ground,
blue to Nucleo +5 V, yellow to PB4/TIM3_CH1, and white to PB5/TIM3_CH2. Complete
the USB logic-only gate before applying 12 V, then execute ENCODER-HW-001 for
physical direction, quadrature waveform, and RPM validation.
