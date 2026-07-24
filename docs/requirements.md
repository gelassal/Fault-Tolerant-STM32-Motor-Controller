# System Requirements

## Status

These are initial design targets. They may be revised after hardware
characterization, but changes must be documented.

## Safety Requirements

### SAFE-001 — Disabled Startup

The motor output shall remain disabled during startup, programming, reset,
and initialization.

### SAFE-002 — Physical Power Cutoff

The system shall provide a physical method of removing motor power
independently of firmware.

### SAFE-003 — Fault Safe State

When a critical fault is detected, the controller shall:

1. Set PWM output to zero.
2. Disable the motor driver.
3. Clear the controller integrator.
4. Record a fault code.
5. Remain faulted until an explicit clear command is received.

### SAFE-004 — Invalid Command Rejection

The controller shall reject commands outside the configured operating range.

## Control Requirements

### CTRL-001 — Control Frequency

The closed-loop control calculation shall execute once every 1 ms.

### CTRL-002 — Speed Command Range

The initial permitted target-speed range shall be 0 to 80 RPM.

### CTRL-003 — Steady-State Error

The initial design target shall be less than 5% steady-state speed error.

### CTRL-004 — Deadline Monitoring

The controller shall count any control-loop iteration that exceeds its
1 ms execution window.

## Sensor Requirements

### SNSR-001 — Encoder Measurement

The controller shall measure motor position and direction using a quadrature
encoder.

### SNSR-002 — Current Measurement

The controller shall sample motor current through an STM32 ADC channel.

### SNSR-003 — Sensor Calibration

Current-sensor offset and scaling shall be measured and documented.

## Telemetry Requirements

### TLM-001 — UART Telemetry

During the initial phase, the controller shall transmit telemetry over UART
at approximately 100 Hz.

### TLM-002 — Telemetry Fields

Telemetry shall include:

- Timestamp
- Target speed
- Measured speed
- PWM duty cycle
- Measured current
- Controller state
- Active fault code

## Initial Fault Requirements

### FLT-001 — Invalid Command Fault

The controller shall detect invalid and out-of-range commands.

### FLT-002 — Encoder Feedback Fault

The controller shall detect missing encoder movement while meaningful motor
output is being commanded.

### FLT-003 — Motor Driver Fault

The controller shall monitor the motor driver's hardware fault output.

## Validation Requirements

### VAL-001 — Recorded Test Data

Performance tests shall save raw measurements in CSV format.

### VAL-002 — Repeatable Tests

Each major performance result shall be generated from a documented,
repeatable test procedure.

### VAL-003 — Measured Claims

No latency, error, timing, reliability, or performance value shall be
published without recorded supporting data.