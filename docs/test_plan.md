
Do not guess the exact STM32 pins yet. Select them in CubeMX later, then update the table.

---

# 7. Add the initial test plan

```markdown
# Validation Test Plan

## Test Record Format

Each completed test shall record:

- Test identifier
- Date
- Firmware commit
- Hardware revision
- Test procedure
- Expected result
- Actual result
- Pass or fail
- Supporting log or image location

## BRINGUP-001 — STM32 LED Test

Requirement: Basic board bring-up

Procedure:

1. Program the STM32.
2. Toggle the onboard LED every 500 ms.
3. Reset the board.
4. Confirm normal operation resumes.

Expected result:

- Firmware programs successfully.
- LED toggles at the expected rate.
- Reset does not cause unexpected behavior.

## BRINGUP-002 — UART Boot Message

Requirements: TLM-001

Procedure:

1. Open the ST-LINK virtual COM port.
2. Reset the STM32.
3. Capture the boot message.

Expected result:

- A complete boot message is received once per reset.
- Motor state is reported as disabled.

## MOTOR-001 — Open-Loop PWM

Requirements: SAFE-001

Procedure:

1. Start with the driver disabled.
2. Set the requested duty cycle to 10%.
3. Enable the driver.
4. Increase duty cycle in controlled increments.
5. Return duty cycle to zero.
6. Disable the driver.

Expected result:

- Motor remains off before enable.
- Motor responds to PWM commands.
- Reset leaves the motor disabled.

## ENC-001 — Encoder Direction

Requirement: SNSR-001

Procedure:

1. Rotate the shaft manually in one direction.
2. Record the encoder count.
3. Rotate the shaft in the opposite direction.
4. Record the encoder count.

Expected result:

- Count increases in one direction.
- Count decreases in the opposite direction.

## CONTROL-001 — Speed Step Response

Requirements: CTRL-001, CTRL-003, VAL-001

Procedure:

1. Command 20 RPM for five seconds.
2. Command 60 RPM for ten seconds.
3. Command 40 RPM for five seconds.
4. Command zero.
5. Save telemetry to CSV.
6. Repeat five times.

Measurements:

- Rise time
- Settling time
- Overshoot
- Steady-state error
- Peak current
- Missed deadlines