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

### YYYY-MM-DD — Repository Initialization

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

Install STM32CubeIDE and perform LED, UART, and debugger bring-up on the
NUCLEO-G474RE.