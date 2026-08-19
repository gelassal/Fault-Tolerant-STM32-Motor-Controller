# Curated Test Evidence

This directory contains small, reviewable artifacts that support the recorded
hardware results. High-volume or temporary captures belong under `results/raw/`
and remain ignored by Git.

| Milestone | Result | Evidence |
|---|---|---|
| UART fault validation | PASS | [`uart-fault-validation.png`](uart-fault-validation.png) |
| DRIVER-POWER-001 | PASS | [`driver-power-001/`](driver-power-001/) |
| MOTOR-SPIN-001 | Functional PASS | [`motor-spin-001/`](motor-spin-001/) |

Permanent raw UART transcripts were not captured during the original sessions.
This limitation is retained in the development log and test plan rather than
reconstructing data that was not saved.
