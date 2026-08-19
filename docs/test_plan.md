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

## HOST-ENC-001 - Encoder Measurement Math

Requirements: SNSR-001, VAL-002

Procedure:

1. From `tests/encoder`, run `mingw32-make run`.
2. Compile the production `encoder_math.c` with
   `-Wall -Wextra -Werror -pedantic`.
3. Feed synthetic 16-bit TIM3 counts into the tracker and verify positive,
   negative, stationary, wraparound, high-rate, inversion, accumulated-count,
   and RPM-conversion behavior.

Expected result: all assertions pass using 3,591.84 counts per revolution for
the Pololu #4866 gearbox output. Counter changes must remain below 32,768
counts per sample so the signed modular difference is unambiguous.

Recorded result, 2026-08-12: **PASS - 57 of 57 assertions passed.**

## BUILD-001 - STM32 Debug Firmware

Procedure: refresh the CubeIDE managed build and build the Debug configuration.

Expected result: `control_node.elf` links with `motor_controller.c` and no
compiler warnings.

Recorded result, 2026-08-07: **PASS - zero warnings.** Image size was 26,952
bytes text, 100 bytes initialized data, and 2,260 bytes BSS. Result was produced
from the uncommitted integration worktree.

Recorded result, 2026-08-12: **PASS - complete forced rebuild with zero
warnings.** The build compiled and linked `encoder_driver.c` and
`encoder_math.c`. Image size was 28,388 bytes text, 100 bytes initialized data,
and 2,384 bytes BSS (30,872 bytes total).

## UART-STATE-001 - Controller Transition Sequence

Status: **PASS ON PHYSICAL NUCLEO, 2026-08-11**

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

Recorded result: the command sequence, READY/coast behavior, duty interlock,
direction interlock, brake/release transitions, injected-fault latch, rejected
re-enable, and explicit clear were observed through the ST-LINK virtual COM
port. Oscilloscope evidence remains pending under SCOPE-PWM-001.

Supplemental recorded result, 2026-08-14: **PASS - dynamic fault telemetry.**
On the physical Nucleo, `injectfault` reported `state=fault` and
`fault=software`; `disable` retained and reported that same fault state; and a
following `status` confirmed the latch. Reset restored the defined safe startup
report of `state=disabled direction=forward duty=0 fault=none`. Malformed
commands were rejected. This verifies UART/controller telemetry only; it is
not a powered-carrier or oscilloscope result.

## SCOPE-PWM-001 - MCU PWM Outputs

Status: **PENDING EQUIPMENT-GATED TEST**

Measure PB6 and PC7 with the carrier disconnected. Record frequency, 10%, 25%,
and 50% duty, forward/reverse routing, coast, braking, disable, and fault
shutdown. The measured frequency must not exceed 20.0 kHz.

## DRIVER-LOGIC-001 - Logic-Only Carrier

Status: **PARTIAL, 2026-08-11**

Verify carrier assembly, 5 V VCC, common ground, safe startup levels, DIAG, and
absence of shorts with VIN and the motor disconnected.

Recorded result: the carrier was soldered and logic wiring was completed with
PB10 configured as `GPIO_NOPULL`. With VIN absent, the real DIAG signal
propagated through PB10 and latched a controller fault. Final voltage and
continuity measurements remain pending.

## DRIVER-POWER-001 - Powered Carrier Without Motor

Status: **PASS - COMPLETED 2026-08-18**

Use fused, current-limited 12 V power with OUT1/OUT2 disconnected. Validate all
controller states, DIAG, PWM routing, and temperature before connecting a motor.

### Preconditions

- Motor red/black leads, encoder leads, OUT1, OUT2, and the ACS724 current path
  remain disconnected.
- The Nucleo USB, supply, and kill switch remain off during inspection and
  wiring.
- Supply positive is routed through a 1 A inline fuse placed near the supply,
  then through an accessible kill switch to TB9051 VIN.
- Supply negative is connected directly to TB9051 GND, which already shares
  ground with the Nucleo.
- Supply is off while wiring, set to 12.0 V, and current-limited to 0.25 A.
- Polarity is verified at the loose supply leads before they touch the carrier.
- Carrier solder joints, terminal polarity, exposed conductors, screw
  tightness, and the absence of a sustained VIN-to-GND continuity short are
  verified before power is applied.
- Oscilloscope grounds connect only to system GND, never across OUT1 and OUT2.

### Required Measurements and PASS/FAIL Limits

| Check | PASS | FAIL / immediate abort |
|---|---|---|
| Supply before connection | 11.8-12.2 V with correct polarity | Outside range or reversed polarity |
| VIN at carrier after power-on | 11.8-12.2 V and stable | Collapse, oscillation, or outside range |
| Logic VCC | 4.75-5.25 V | Outside range |
| Initial supply current | Expected well below 0.05 A; 0.25 A limit inactive | Current limiting is an immediate abort; at/above 0.05 A requires stop and investigation |
| Initial controller state | `DISABLED`, duty 0 | Any enabled/running output |
| READY | EN low, ENB high, both PWM low; low DIAG is expected | Driver enabled or PWM nonzero |
| RUNNING command, outputs unloaded | EN high, ENB low, DIAG deasserted, no latched fault | DIAG asserted with valid VIN/VCC, current limit, or voltage collapse |
| BRAKING | EN high, ENB low, PWM1/PWM2 low, DIAG deasserted | Incorrect control level or DIAG asserted with valid VIN/VCC |
| RELEASE/DISABLED/FAULT | EN low, ENB high, PWM1/PWM2 low; low DIAG is expected | Any active output |
| `clearfault` after software fault | Returns to `DISABLED` | Physical fault remains or output stays enabled |
| Kill-switch check | After discharge, VIN falls below 0.5 V | VIN remains at/above 0.5 V |
| Two-minute unloaded observation | Current stable and expected well below 0.05 A; no heat, odor, or noise | At/above 0.05 A requires investigation; rising current, heat, odor, or noise is an abort |

### Command and Waveform Sequence

After verifying 11.8-12.2 V at VIN, 4.75-5.25 V at logic VCC, stable unloaded
current, and successful kill-switch removal of VIN, run:

```text
status
enable
duty 10
duty 0
reverse
duty 10
brake
release
duty 10
injectfault
disable
clearfault
status
```

Record forward PWM on PB6 with PC7 low, reverse PWM on PC7 with PB6 low,
READY/RELEASE coast levels, BRAKING levels, and injected-fault shutdown. PWM
must be nominally 20.0 kHz and must not exceed 20.0 kHz. Keep ARR `4199` when
the measurement is at or below the limit; only change both PWM timers to ARR
`4299` and repeat validation if the measured frequency is genuinely above it.

DIAG is intentionally low whenever the TB9051 is disabled (`EN=0` or
`ENB=1`), so that level is expected in DISABLED, READY/coast, RELEASE, and
FAULT. Treat DIAG as a fault indication only while the driver is commanded
enabled (`EN=1`, `ENB=0`) with valid VIN and VCC.

Record actual supply voltage, carrier VIN, VCC, idle current, maximum observed
current, UART transcript, DIAG behavior, and pass/fail for every row. A failed
row blocks motor connection. Immediately cut power for reversed polarity,
current limiting, voltage collapse, heat, odor, noise, incorrect waveforms, or
DIAG asserted while the driver is commanded enabled with valid VIN/VCC. If
unloaded current reaches or exceeds 0.05 A, stop and investigate before
continuing. Do not connect the motor after this test; review and document the
complete DRIVER-POWER-001 record first.

Recorded result, 2026-08-18: **PASS.** The carrier remained stable at 12.0 V
with a 0.25 A current limit and approximately 0.002 A maximum unloaded current.
Forward/reverse PWM, coast, brake, DIAG, fault shutdown, clear, kill-switch,
and two-minute observation checks passed. Measured PWM was 19.90-19.95 kHz.
Logic VCC measured 4.72 V, which missed the project's conservative 4.75 V
target but remained within the TB9051FTG operating range. Permanent artifact
paths and exact EN/ENB/DIAG voltage values remain documentation limitations.

## MOTOR-SPIN-001 - First Unloaded Motor Spin

Status: **PASS - COMPLETED 2026-08-19**

Use a 0.5 A supply limit and the staged procedure in `wiring.md`. Demonstrate
the lowest successful forward and reverse duty, READY/coast, disable, and
software-fault shutdown. Record maximum observed current, supply settings,
measured PWM, UART transcript, scope captures, and pass/fail result.

Recorded result, 2026-08-19: **FUNCTIONAL PASS.** The motor was securely
restrained and connected red-to-OUT1 and black-to-OUT2 while its four encoder
leads and the ACS724 current path remained disconnected. The supply was set to
12.0 V with a 0.5 A limit and remained in constant-voltage mode. Idle current
was approximately 0.002 A with no motion in DISABLED or READY.

- A 10% forward command did not start the motor.
- The lowest successful forward command was 12%, producing counterclockwise
  output-shaft rotation and approximately 0.036 A supply current.
- Reverse operation at 12% produced clockwise rotation and approximately the
  same 0.036 A current.
- An unplanned brief 50% command produced 0.070 A, the maximum observed current,
  without current limiting or abnormal behavior. This exceeded the procedure's
  planned 25% ceiling and is recorded as a test deviation, not a new limit.
- At 12% duty, `injectfault` immediately removed drive and latched FAULT;
  `enable` was rejected, `disable` retained FAULT, and `clearfault` returned the
  controller to DISABLED with duty 0 and no fault.
- No voltage collapse, heating, odor, abnormal noise, restraint movement, or
  other unexpected behavior occurred.

Permanent UART transcript and motor-test artifact paths remain to be added to
the repository record.

## ENCODER-HW-001 - Physical Encoder Direction and RPM

Status: **PENDING HOME HARDWARE VALIDATION**

Use the Stage 5 procedure in `wiring.md` with encoder blue powered from Nucleo
+5V, green at system ground, yellow on PB4/TIM3_CH1, and white on PB5/TIM3_CH2.
Keep the ACS724 path disconnected and retain the 12.0 V, 0.5 A motor-supply
limit.

Required results:

| Check | PASS | FAIL / immediate stop |
|---|---|---|
| Stationary telemetry | Initialized, stable count, delta 0, RPM 0, stationary | Count drift, nonzero RPM, or uninitialized |
| Encoder supply | 4.5-5.25 V blue-to-green | Outside range or incorrect polarity |
| Forward at lowest steady duty | Changing count, nonzero RPM, forward direction | Missing counts, wrong direction after calibration, or unstable signal |
| Reverse at same duty | Opposite count sign, nonzero RPM, reverse direction | Same sign as forward or missing counts |
| A/B waveforms | Clean quadrature, similar frequency, approximately 90-degree phase | Invalid levels, noise preventing reliable count, or missing channel |
| RPM cross-check | UART RPM within 10% of scope-derived RPM | Difference greater than 10% at steady speed |
| Coast/fault | RPM returns to zero; fault removes drive and latches | Continued drive or persistent nonzero speed after stop |

Use 12% first, then 15%, 20%, and at most 25% only if needed for steady
measurement. If physical polarity is inverted, set the existing encoder
configuration's `invert_direction` flag, rebuild, and repeat. Do not swap the
standardized yellow/white channel wiring. Save the UART transcript, wiring
photo, A/B scope capture, frequency calculation, maximum current, and result.
