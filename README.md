# Fault-Tolerant STM32 Motor-Control Platform

A safety-oriented brushed-DC motor controller built on the STM32F446RE. The
project is intended to demonstrate professional embedded-firmware architecture,
hardware bring-up, real-time control, fault handling, communication, and
automated validation using measured results.

## Current Status

The current milestone integrates and validates the open-loop safety layer before
external motor hardware is powered.

Completed in firmware:

- NUCLEO-F446RE board, debugger, LED, and USART2 bring-up
- Interrupt-driven UART command console at 115200 baud
- TIM4_CH1 and TIM8_CH2 dual-PWM outputs configured for a calculated 20 kHz
- Low-level TB9051FTG motor-driver abstraction
- High-level disabled, ready, running, braking, and fault state machine
- READY and brake-release semantics that leave the H-bridge disabled to coast
- Direction interlock, latched faults, explicit fault clearing, and software
  fault injection
- Native state-machine regression suite with a mocked motor driver

Verified on 2026-08-07:

- All 147 native motor-controller assertions passed with `-Wall -Wextra
  -Werror -pedantic`.
- The STM32 Debug firmware built and linked with zero warnings.
- Image size: 26,952 bytes text, 100 bytes initialized data, and 2,260 bytes BSS.

Not yet physically verified:

- PWM frequency and duty cycle with an oscilloscope
- Soldered TB9051FTG carrier and logic wiring
- Powered driver behavior
- First motor spin
- Encoder feedback, current sensing, closed-loop control, FreeRTOS, CAN, and HIL

## Hardware

- ST NUCLEO-F446RE
- Pololu #2997 TB9051FTG single brushed-DC motor-driver carrier
- Pololu #4866 75:1 MP 12 V 25D gearmotor with 48 CPR encoder
- Protected, current-limited 12 V bench supply and physical cutoff (required
  before powered testing)

See [docs/wiring.md](docs/wiring.md) for the exact pin map, equipment gate, and
staged bring-up procedure.

## Architecture

UART commands pass through `motor_controller`, which enforces application
policy before using the low-level `motor_driver`. The console cannot directly
enable the H-bridge or change PWM.

```text
USART2 interrupt -> command buffer -> command parser
                                      |
                                      v
                               motor_controller
                                      |
                                      v
                                 motor_driver
                                      |
                                      v
                         GPIO + PWM + TB9051FTG
```

The current synchronous main loop is intentional. Encoder feedback and
closed-loop behavior will be validated before the application is migrated to
FreeRTOS tasks.

## Native Tests

From `tests/motor_controller`:

```powershell
mingw32-make run
```

The tests compile the production controller against a fake driver and verify
legal and illegal transitions, coast/brake behavior, safe duty sequencing,
diagnostic and software faults, low-level failures, fault latching, and fault
clearing.

## Long-Term Target

The resume-ready target is an encoder-based 1 kHz closed-loop controller with
FreeRTOS timing measurements, ADC/DMA current monitoring, CAN commands and
telemetry, multiple detected fault conditions, Python HIL automation, CI, and
recorded performance data. A custom carrier PCB and bootloader remain optional
stretch goals after the development-board system is validated.

## Safety and Measurement Policy

- Reset, disable, and fault states must force zero PWM and disable the driver.
- READY means armed but electrically coasting; BRAKING is the deliberate
  short-brake state.
- Never connect 12 V to a Nucleo logic or 5 V pin.
- Do not use Dupont wires or a solderless breadboard for motor current.
- Do not power the motor until the equipment checklist and motor-disconnected
  tests pass.
- Do not publish performance claims without saved test records.

Measured scope captures and motor results will be added only after the physical
tests are completed.
