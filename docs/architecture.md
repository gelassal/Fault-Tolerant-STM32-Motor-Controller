# System Architecture

## Initial Phase

The first phase uses a single STM32 without FreeRTOS.

```mermaid
flowchart LR
    PC[Python Host Tools] -->|UART Commands| STM32
    STM32 -->|UART Telemetry| PC

    Encoder[Quadrature Encoder] --> STM32
    Current[Current Sensor] --> STM32
    DriverFault[Motor Driver Fault] --> STM32

    STM32 -->|PWM / Direction / Enable| Driver[Motor Driver]
    Driver --> Motor[DC Gearmotor]
    Motor --> Encoder