# UART HIL Smoke Test

This script automates the first safety-state sequence over the Nucleo's
ST-LINK virtual COM port. It is a UART precursor to the later CAN HIL suite.

## Safety Preconditions

- OUT1, OUT2, and the motor are physically disconnected.
- Either the TB9051 is disconnected with a valid DIAG test bias, or it is
  logic-connected and powered from the protected 12 V setup so DIAG can clear.
- Do not run the expected-success profile with the present logic-connected,
  VIN-absent setup; the real undervoltage fault should correctly make it fail.
- The controller starts in `DISABLED` with no latched fault.

## Usage

```powershell
python -m pip install -r tools\hil\requirements.txt
python tools\hil\uart_smoke_test.py --port COM5 --motor-disconnected
```

The script checks this sequence:

```text
status       -> DISABLED
enable       -> READY
duty 25      -> RUNNING
injectfault  -> FAULT
clearfault   -> DISABLED
```

Every response is parsed from the controller's actual state report. On a test
failure, the script attempts a final `disable` command before closing the port.

## Read-Only Encoder Check

The encoder-only mode never sends enable, duty, direction, brake, release, or
fault commands:

```powershell
python tools\hil\uart_smoke_test.py --port COM5 --encoder-only
```

It requires the controller to already report `state=disabled duty=0`, then
parses the machine-readable response:

```text
ENCODER initialized=1 count=-1234 delta=-12 rpm_milli=-200 direction=reverse
```

With PB4/PB5 physically disconnected, the timer inputs can float, so this mode
validates integration and response formatting but must not be used to claim
stationary-count stability. Physical encoder wiring is required for that test.
