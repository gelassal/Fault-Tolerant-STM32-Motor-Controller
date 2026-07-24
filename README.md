# Fault-Tolerant STM32 Motor-Control Platform

A real-time motor-control platform built around an STM32 microcontroller,
quadrature encoder feedback, current sensing, CAN communication, fault
detection, and automated Python hardware-in-the-loop testing.

## Project Status

Current phase: Hardware selection and controller bring-up.

The initial milestone is a single STM32 controlling an encoded DC motor with:

- PWM and direction control
- Quadrature encoder measurement
- Motor-current measurement
- Closed-loop speed control
- UART telemetry
- Safe startup and fault handling

FreeRTOS, CAN communication, and hardware-in-the-loop automation will be
introduced after the basic motor-control hardware has been validated.

## Planned Hardware

- STM32 NUCLEO-G474RE
- 12 V brushed DC gearmotor with quadrature encoder
- TB9051FTG motor-driver carrier
- ACS724 current-sensor carrier
- USB-to-CAN interface
- Current-limited 12 V power supply

## Planned Capabilities

- 1 kHz closed-loop speed controller
- FreeRTOS task-based firmware architecture
- Interrupt-driven quadrature encoder measurement
- ADC and DMA current sampling
- CAN command and telemetry protocol
- Overcurrent, stall, encoder, command, and communication fault detection
- Hardware watchdog supervision
- Python hardware-in-the-loop tests
- Automated build, unit-test, formatting, and static-analysis checks

## Repository Structure

- `firmware/control_node/` — STM32 motor-controller firmware
- `host_tools/` — Python telemetry, plotting, and command utilities
- `tests/` — Firmware and host-side automated tests
- `docs/` — Requirements, architecture, wiring, and test documentation
- `results/` — Curated measurements, plots, and validation reports
- `.github/workflows/` — Continuous-integration workflows

## Development Phases

1. Basic motor and sensor bring-up
2. Closed-loop speed control
3. FreeRTOS migration and timing analysis
4. CAN communication
5. Fault-tolerance validation
6. Python hardware-in-the-loop automation
7. Optional custom PCB and bootloader

## Safety

The motor must start in a disabled state after reset. Motor power will pass
through a fuse and physical cutoff switch. High-current motor wiring will not
be routed through a solderless breadboard.

## Results

Measured performance results will be added after validation. No performance
numbers will be reported until they have been reproduced and documented.