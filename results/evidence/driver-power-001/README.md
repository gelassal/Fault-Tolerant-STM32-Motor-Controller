# DRIVER-POWER-001 Evidence

Recorded 2026-08-18 through 2026-08-19 with the motor and OUT1/OUT2 initially
disconnected.

| Capture | Commanded output | Measured result |
|---|---|---|
| [`reverse-pwm-10-percent.jpg`](reverse-pwm-10-percent.jpg) | CH2/reverse, 10% | 19.92 kHz, 9.72%, clean edges |
| [`forward-pwm-20-percent.jpg`](forward-pwm-20-percent.jpg) | CH1/forward, 20% | 19.95 kHz, 19.71%, clean edges |

Additional recorded results:

- 12.0 V carrier supply with a 0.25 A current limit
- Approximately 0.002 A maximum unloaded carrier current
- 4.72 V logic VCC
- EN, ENB, DIAG, coast, brake, direction routing, kill-switch, and injected
  fault behavior passed
- Two-minute powered observation completed without heating or instability

The terminal's automated statistics include measurements from both enabled
channels and cursor/statistics history; the table above records the active PWM
channel values used for acceptance.
