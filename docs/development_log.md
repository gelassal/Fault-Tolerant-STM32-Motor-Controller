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