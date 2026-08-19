# MOTOR-SPIN-001 Evidence Summary

Functional test completed 2026-08-19 using the Pololu #4866 motor, #2997
carrier, a fused 12.0 V supply, and a 0.5 A current limit.

| Check | Recorded result |
|---|---|
| DISABLED/READY idle | Stationary, CV mode, approximately 0.002 A |
| Forward startup | 10% did not start; 12% started counterclockwise at approximately 0.036 A |
| Reverse startup | 12% started clockwise at approximately 0.036 A |
| Maximum observed current | 0.070 A during a brief, unplanned 50% command |
| Software fault | Immediate coast, latched FAULT, rejected enable |
| Fault clear | Explicit clear returned to DISABLED, duty 0, fault none |
| Abnormal behavior | None observed |

The 50% command exceeded the approved 25% test ceiling and is retained as a
documented deviation, not a required test point or operating recommendation.
No permanent motor video or raw UART transcript was captured during this
session.
