# Wiring and Staged Bring-Up

## Status

The carrier and motor have not been powered by this firmware. The wiring below
is the approved plan, not a record of completed validation.

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
| UART TX | PA2 | USART2_TX | ST-LINK VCP |
| UART RX | PA3 | USART2_RX | ST-LINK VCP |

PB10 is configured with an internal pull-up only for software tests with no
carrier connected. Before logic-only carrier wiring, change it to
`GPIO_NOPULL` in CubeMX and regenerate; the carrier supplies its own DIAG
pull-up. PB10 is a 5 V-tolerant digital input.

## Equipment Gate

Do not begin powered testing until all boxes are checked:

- [ ] Regulated 12 V bench supply rated for at least 2 A with adjustable
      current limiting
- [ ] 1 A inline fuse for the initial unloaded test
- [ ] Accessible cutoff switch rated for at least 12 V and 2 A
- [ ] Multimeter
- [x] Oscilloscope with x10 probes available at home
- [ ] Suitable motor and supply wire; no Dupont wires in the current path
- [ ] Secure motor restraint
- [ ] Soldering, inspection, and insulation supplies

## Carrier Assembly

1. Solder the 1x17 logic header and both screw terminals.
2. Inspect alignment, bridges, cold joints, and terminal orientation.
3. Check continuity and confirm that VIN is not shorted to GND.
4. Verify the carrier and motor part labels before attaching wires.

## Stage 1: MCU Outputs, Carrier Disconnected

Keep the PB10 internal pull-up enabled. Flash the integrated firmware and use
the UART sequence in `test_plan.md` while probing PB6, PC7, PA8, and PA10.

Required results:

- DISABLED, READY, RELEASE, and FAULT: PWM1/PWM2 zero, EN low, ENB high
- RUNNING forward: PB6 PWM, PC7 low, EN high, ENB low
- RUNNING reverse: PB6 low, PC7 PWM, EN high, ENB low
- BRAKING: PWM1/PWM2 low, EN high, ENB low
- PWM at or below 20 kHz with correct duty scaling

If measured PWM exceeds 20.0 kHz, change TIM4 and TIM8 ARR from `4199` to
`4299` for a nominal 19.53 kHz, regenerate, rebuild, and repeat the test.

## Stage 2: Logic-Only Carrier Wiring

After changing PB10 to `GPIO_NOPULL`, disconnect all power and wire:

```text
Nucleo 5V  -> Driver VCC
Nucleo GND -> Driver GND
PB6        -> Driver PWM1
PC7        -> Driver PWM2
PA8        -> Driver EN
PA10       -> Driver ENB
PB10       -> Driver DIAG
```

Leave VIN, OUT1, OUT2, and the motor disconnected. Power the Nucleo through USB
and verify 5 V VCC, approximately 3.3 V logic highs, safe disabled EN/ENB
levels, zero PWM, and no unexpected heating. Do not issue commands that enable
the bridge while VIN is absent.

## Stage 3: Motor Supply, Motor Disconnected

With power off, connect the protected motor supply:

```text
12 V supply positive -> cutoff -> 1 A fuse -> Driver VIN
12 V supply negative -----------------------> Driver GND
```

Keep OUT1 and OUT2 disconnected. Set the current limit to 0.25 A, verify
polarity, then power the system. Exercise READY, RUNNING, BRAKING, RELEASE,
DISABLED, fault injection, and fault clearing while checking DIAG and the
control signals.

For an earth-referenced oscilloscope, connect probe grounds only to system GND.
Never place a scope ground across OUT1 and OUT2.

## Stage 4: First Unloaded Motor Spin

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
rotation, DIAG assertion, voltage collapse, heat, odor, abnormal noise, or an
unexpected waveform.
