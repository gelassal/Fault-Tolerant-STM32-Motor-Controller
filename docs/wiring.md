# Wiring and Staged Bring-Up

## Status

The carrier logic, fused 12 V path, motor-disconnected driver test, and first
unloaded motor spin have been completed. DRIVER-POWER-001 passed on 2026-08-18
and MOTOR-SPIN-001 passed on 2026-08-19. Physical encoder wiring and validation
are the next hardware stage.

Confirmed components:

- Pololu #2997 TB9051FTG single motor-driver carrier
- Pololu #4866 75:1 MP 12 V gearmotor with encoder
- ST NUCLEO-F446RE

## STM32 Pin Map

| Function | STM32 pin | Peripheral | Driver connection |
|---|---:|---|---|
| PWM1 | PB6 | TIM4_CH1 | PWM1 |
| PWM2 | PC7 | TIM8_CH2 | PWM2 |
| Enable | PA8 | GPIO output | EN |
| Enable-bar | PA10 | GPIO output | ENB |
| Diagnostic | PB10 | GPIO input | DIAG |
| Encoder A | PB4 | TIM3_CH1 | Yellow encoder lead (Stage 5) |
| Encoder B | PB5 | TIM3_CH2 | White encoder lead (Stage 5) |
| UART TX | PA2 | USART2_TX | ST-LINK VCP |
| UART RX | PA3 | USART2_RX | ST-LINK VCP |

PB10 is now configured as `GPIO_NOPULL` because the logic-connected carrier
supplies the DIAG pull-up. PB10 is a 5 V-tolerant digital input.

TIM3 is configured in encoder-interface mode using both edges of both channels.
For Pololu #4866, this yields 48 counts per motor-shaft revolution and 3,591.84
counts per gearbox-output revolution. PB4/PB5 direction polarity must be
confirmed during the first encoder test; firmware provides a direction-invert
option if the observed sign is opposite the commanded convention.

## Equipment Gate

For DRIVER-POWER-001, every box except the motor restraint must be checked.
The restraint is a separate mandatory gate before MOTOR-SPIN-001:

- [x] Regulated 12 V bench supply rated for at least 2 A with adjustable
      current limiting
- [x] 1 A inline fuse for the initial unloaded test
- [x] Accessible DC kill switch rated for at least 12 V and 2 A
- [x] Multimeter
- [x] Oscilloscope with x10 probes
- [x] Suitable motor and supply wire; no Dupont wires in the current path
- [x] Secure motor restraint
- [x] Soldering, inspection, and insulation supplies

## Carrier Assembly

1. Solder the 1x17 logic header and both screw terminals.
2. Inspect alignment, bridges, cold joints, and terminal orientation.
3. Check continuity and confirm that VIN is not shorted to GND.
4. Verify the carrier and motor part labels before attaching wires.

## Stage 1: MCU Outputs, Carrier Disconnected

The current firmware configures PB10 as `GPIO_NOPULL` for the carrier's 5 V
DIAG pull-up. If this carrier-disconnected test is repeated, PB10 has no valid
DIAG level unless a suitable temporary pull-up is provided or the firmware is
temporarily restored to its carrier-disconnected pull-up configuration. Flash
the integrated firmware and use the UART sequence in `test_plan.md` while
probing PB6, PC7, PA8, and PA10.

Required results:

- DISABLED, READY, RELEASE, and FAULT: PWM1/PWM2 zero, EN low, ENB high
- RUNNING forward: PB6 PWM, PC7 low, EN high, ENB low
- RUNNING reverse: PB6 low, PC7 PWM, EN high, ENB low
- BRAKING: PWM1/PWM2 low, EN high, ENB low
- PWM at or below 20 kHz with correct duty scaling

ARR `4199` is the intentional nominal 20.0 kHz configuration and must remain in
place when measured PWM is at or below 20.0 kHz. Only if the oscilloscope
genuinely measures above 20.0 kHz, change TIM4 and TIM8 ARR to `4299` for a
nominal 19.53 kHz, regenerate, rebuild, and repeat the test.

## Stage 2: Logic-Only Carrier Wiring — Complete

PB10 has been changed to `GPIO_NOPULL`. The following logic wiring was completed
on 2026-08-11:

```text
Nucleo 5V  -> Driver VCC
Nucleo GND -> Driver GND
PB6        -> Driver PWM1
PC7        -> Driver PWM2
PA8        -> Driver EN
PA10       -> Driver ENB
PB10       -> Driver DIAG
```

The VIN-absent DIAG fault path was successfully exercised. Subsequent powered
validation measured 4.72 V logic VCC and confirmed the required EN, ENB, DIAG,
and PWM behavior.

## Stage 3: Motor Supply, Motor Disconnected — Complete

### Unpowered Preparation

1. Keep the supply off, kill switch off, and Nucleo USB disconnected.
2. Confirm OUT1, OUT2, all motor and encoder leads, and the ACS724 current path
   remain disconnected.
3. Inspect carrier solder joints, terminal polarity, exposed conductors, and
   screw tightness.
4. Verify with the multimeter that there is no sustained continuity short
   between VIN and GND.
5. Assemble the protected motor supply with the fuse close to the source:

```text
Supply + -> 1 A fuse near supply -> kill switch -> Driver VIN
Supply - ------------------------------------> Driver GND
```

6. Leave the harness unpowered until the oscilloscope is physically available.

### Powered Carrier Procedure

1. With the supply disconnected from the carrier, set it to 12.0 V and a
   0.25 A current limit.
2. Measure the loose leads and require 11.8-12.2 V with correct polarity.
3. Turn the supply off, connect VIN/GND, and connect all oscilloscope probe
   grounds only to system GND.
4. Power the Nucleo through USB, reset it, and require:

   `STATUS state=disabled direction=forward duty=0 fault=none`

5. Measure logic VCC and require 4.75-5.25 V.
6. Apply 12 V through the kill switch. Require carrier VIN of 11.8-12.2 V,
   inactive current limiting, no voltage collapse, heat, odor, or noise, and
   current expected to be well below 0.05 A. If current is at or above 0.05 A,
   stop and investigate before continuing; this is a conservative
   investigation threshold, not a datasheet hard limit.
7. Turn off the kill switch and, after discharge, require VIN below 0.5 V.
   Restore VIN before continuing.
8. Run the following UART sequence while measuring EN, ENB, DIAG, PB6, and
   PC7:

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

Required output behavior:

- DISABLED, READY/coast, RELEASE, and FAULT: EN low, ENB high, PWM1/PWM2 low.
- Forward RUNNING: EN high, ENB low, PB6 approximately 10% PWM, PC7 low.
- Reverse RUNNING: EN high, ENB low, PB6 low, PC7 approximately 10% PWM.
- BRAKING: EN high, ENB low, PWM1/PWM2 low.
- Injected fault: PWM is immediately removed, the driver is disabled, and the
  controller remains fault-latched through `disable`.
- With valid VIN/VCC, `clearfault` returns to DISABLED.

The TB9051 intentionally drives DIAG low whenever it is disabled by `EN=0` or
`ENB=1`. Low DIAG is therefore expected in DISABLED, READY/coast, RELEASE, and
FAULT. Evaluate DIAG as a hardware fault only when the driver is commanded
enabled (`EN=1`, `ENB=0`) with valid VIN and VCC; it must then be deasserted.

9. Measure PWM frequency and duty. Keep ARR `4199` if frequency is at or below
   20.0 kHz. Only if it is genuinely above 20.0 kHz, change both PWM timers to
   ARR `4299` (approximately 19.53 kHz) and repeat validation.
10. Observe the unloaded carrier for two minutes. Current must remain stable
    and is expected to remain well below 0.05 A, with no heating or
    instability.

Save the UART transcript, forward/reverse scope captures, measured frequency
and duty, supply voltage, VIN, VCC, idle current, maximum current, and
kill-switch result. Cut power immediately for reversed polarity, current
limiting, voltage collapse, heat, odor, noise, incorrect waveforms, or DIAG
asserted while the driver is commanded enabled with valid VIN/VCC. Do not
connect the motor after this test; review DRIVER-POWER-001 first.

For an earth-referenced oscilloscope, connect probe grounds only to system GND.
Never place a scope ground across OUT1 and OUT2.

Recorded result: **PASS on 2026-08-18.** See `test_plan.md` and
`development_log.md` for measurements and documented limitations.

## Stage 4: First Unloaded Motor Spin — Complete

1. Power off and connect only the red/black motor leads to OUT1/OUT2.
2. Isolate the green, blue, yellow, and white encoder leads.
3. Restrain the unloaded motor and place the cutoff within reach.
4. Set 12 V and a 0.5 A current limit.
5. Reset and confirm `state=disabled`.
6. Run `enable`, then `duty 10` for no more than a few seconds.
7. If the motor does not rotate and current remains safe, increase in 5% steps
   to a maximum of 25%.
8. Run `duty 0`; confirm READY/coast before changing direction.
9. Repeat at the lowest successful reverse duty, then return to zero and
   disable.
10. At low speed, inject a software fault and confirm immediate coast, latched
    FAULT, rejected enable, and explicit clear to DISABLED.

Abort immediately for current-limit activation, rising current without
rotation, voltage collapse, heat, odor, abnormal noise, an unexpected
waveform, or DIAG asserted while the driver is commanded enabled with valid
VIN/VCC.

Recorded result: **PASS on 2026-08-19.** The lowest reliable unloaded starting
duty was 12% in both directions at approximately 0.036 A. Software-fault
injection immediately removed drive, latched FAULT, rejected re-enable, and
cleared explicitly back to DISABLED.

## Stage 5: Physical Encoder Validation — Pending Home Bench

### Unpowered Wiring

1. Enter `duty 0`, `disable`, and `status`; require DISABLED, duty 0, and no
   fault.
2. Turn the kill switch off, turn the supply output off, disconnect Nucleo USB,
   and require VIN below 0.5 V before changing wiring.
3. Keep the red/black motor leads on OUT1/OUT2 and keep the ACS724 current path
   disconnected.
4. Connect the encoder leads:

```text
Green  -> system GND
Blue   -> Nucleo +5V
Yellow -> PB4 / TIM3_CH1
White  -> PB5 / TIM3_CH2
```

The encoder requires 3.5-20 V, so do not power blue from 3.3 V. PB4 and PB5 are
5 V-tolerant inputs on the STM32F446RE. Inspect every termination, verify common
ground, and confirm that no encoder conductor can contact the rotating shaft.

### USB Logic-Only Gate

1. Keep 12 V and the kill switch off, connect Nucleo USB, reset, and require
   `state=disabled duty=0 fault=none`.
2. Measure encoder blue-to-green voltage; require 4.5-5.25 V.
3. Run `encoder` repeatedly while stationary. Require `initialized=1`, stable
   count, `delta=0`, `rpm_milli=0`, and `direction=stationary`.
4. Scope yellow and white only relative to system GND. Confirm valid stationary
   logic levels before applying motor power.

### Powered Direction and RPM Test

1. Reconfirm the restraint and cutoff, then set the supply to 12.0 V with a
   0.5 A current limit and apply VIN.
2. Require constant-voltage mode, approximately 0.002 A idle current, and no
   motion in DISABLED.
3. Run:

```text
status
encoder
enable
duty 12
encoder
duty 0
encoder
reverse
duty 12
encoder
duty 0
encoder
disable
```

4. Require nonzero RPM and changing counts while running, opposite count signs
   in forward and reverse, and stationary telemetry after coast. If 12% is not
   steady enough, use 15%, 20%, then at most 25%; do not repeat the earlier 50%
   deviation.
5. If telemetry direction is opposite the motor command, set
   `invert_direction=true` in the encoder configuration, rebuild, flash, and
   repeat without swapping the standardized A/B wiring.
6. Scope both encoder channels while running. Require clean, similar-frequency
   square waves with approximately 90-degree phase separation.
7. Compare reported output-shaft RPM with:

```text
output_rpm = encoder_A_frequency_hz * 60 / (12 * 74.83)
```

Use steady-speed measurements and require agreement within 10%.
8. At 12% duty, inject a software fault and require immediate coast, latched
   FAULT, and encoder RPM returning to zero; clear explicitly to DISABLED.

Abort for current limiting, rising current without rotation, voltage collapse,
heat, odor, abnormal noise, restraint movement, invalid encoder voltage, missing
counts while rotating, or unexpected waveforms. Save the UART transcript, A/B
scope capture, wiring photo, frequency/RPM comparison, supply settings, maximum
current, and final pass/fail result.
