#!/usr/bin/env python3
"""Run the motor safety-state smoke test over the STM32 UART console."""

from __future__ import annotations

import argparse
import re
import sys
import time
from dataclasses import dataclass

try:
    import serial
except ImportError:  # pragma: no cover - depends on local installation
    serial = None


PROMPT = b"> "
STATE_PATTERN = re.compile(
    r"^(?:OK|STATUS) state=(?P<state>[a-z_]+) "
    r"direction=(?P<direction>[a-z_]+) "
    r"duty=(?P<duty>\d+) fault=(?P<fault>[a-z_]+)\r?$",
    re.MULTILINE,
)

ENCODER_PATTERN = re.compile(
    r"^ENCODER initialized=(?P<initialized>[01]) "
    r"count=(?P<count>-?\d+) delta=(?P<delta>-?\d+) "
    r"rpm_milli=(?P<rpm_milli>-?\d+) "
    r"direction=(?P<direction>[a-z_]+)\r?$",
    re.MULTILINE,
)


@dataclass(frozen=True)
class ExpectedState:
    command: str
    state: str
    duty: int
    fault: str


SEQUENCE = (
    ExpectedState("status", "disabled", 0, "none"),
    ExpectedState("enable", "ready", 0, "none"),
    ExpectedState("duty 25", "running", 25, "none"),
    ExpectedState("injectfault", "fault", 0, "software"),
    ExpectedState("clearfault", "disabled", 0, "none"),
)


class Console:
    def __init__(self, port: str, baud: int, command_timeout: float) -> None:
        self._command_timeout = command_timeout
        self._serial = serial.Serial(
            port=port,
            baudrate=baud,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.05,
            write_timeout=1.0,
        )

    def close(self) -> None:
        self._serial.close()

    def synchronize(self) -> None:
        time.sleep(0.25)
        self._serial.reset_input_buffer()
        self.command("status")

    def command(self, command: str) -> str:
        self._serial.write((command + "\r").encode("ascii"))
        self._serial.flush()

        deadline = time.monotonic() + self._command_timeout
        response = bytearray()

        while time.monotonic() < deadline:
            waiting = self._serial.in_waiting
            chunk = self._serial.read(waiting if waiting > 0 else 1)

            if chunk:
                response.extend(chunk)

                if response.endswith(PROMPT):
                    return response.decode("ascii", errors="replace")

        raise TimeoutError(
            f"timed out waiting for prompt after command {command!r}; "
            f"received {response!r}"
        )


def parse_state(response: str) -> dict[str, str]:
    matches = list(STATE_PATTERN.finditer(response))

    if not matches:
        raise AssertionError(f"no controller state found in response:\n{response}")

    return matches[-1].groupdict()


def parse_encoder(response: str) -> dict[str, str]:
    matches = list(ENCODER_PATTERN.finditer(response))

    if not matches:
        raise AssertionError(
            f"no encoder telemetry found in response:\n{response}"
        )

    return matches[-1].groupdict()


def run_test(console: Console) -> None:
    console.synchronize()

    for step in SEQUENCE:
        response = console.command(step.command)
        print(f"\n$ {step.command}\n{response.rstrip()}")

        if "ERR " in response:
            raise AssertionError(
                f"{step.command!r} returned an error:\n{response}"
            )

        actual = parse_state(response)
        expected = {
            "state": step.state,
            "duty": str(step.duty),
            "fault": step.fault,
        }

        for field, expected_value in expected.items():
            if actual[field] != expected_value:
                raise AssertionError(
                    f"after {step.command!r}, expected {field}={expected_value}, "
                    f"got {field}={actual[field]}"
                )


def run_encoder_read_only(console: Console) -> None:
    console.synchronize()

    status_response = console.command("status")
    print(f"\n$ status\n{status_response.rstrip()}")
    state = parse_state(status_response)

    if (state["state"] != "disabled" or state["duty"] != "0"):
        raise AssertionError(
            "encoder-only mode requires state=disabled and duty=0; "
            f"received state={state['state']} duty={state['duty']}"
        )

    encoder_response = console.command("encoder")
    print(f"\n$ encoder\n{encoder_response.rstrip()}")

    if "ERR " in encoder_response:
        raise AssertionError(
            f"'encoder' returned an error:\n{encoder_response}"
        )

    telemetry = parse_encoder(encoder_response)

    if telemetry["initialized"] != "1":
        raise AssertionError("encoder telemetry reports initialized=0")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run the STM32 UART motor safety-state smoke test."
    )
    parser.add_argument("--port", required=True, help="ST-LINK VCP, for example COM5")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=2.0)
    parser.add_argument(
        "--motor-disconnected",
        action="store_true",
        help="Required acknowledgement that OUT1/OUT2 and the motor are disconnected.",
    )
    parser.add_argument(
        "--encoder-only",
        action="store_true",
        help="Read status and encoder telemetry without sending motor commands.",
    )

    args = parser.parse_args()

    if args.motor_disconnected and args.encoder_only:
        parser.error("choose either --motor-disconnected or --encoder-only")

    if not args.motor_disconnected and not args.encoder_only:
        parser.error(
            "one safety mode is required: --motor-disconnected or --encoder-only"
        )

    return args


def main() -> int:
    args = parse_args()

    if serial is None:
        print(
            "pyserial is required; run: python -m pip install -r "
            "tools/hil/requirements.txt",
            file=sys.stderr,
        )
        return 2

    console = Console(args.port, args.baud, args.timeout)
    passed = False

    try:
        if args.encoder_only:
            run_encoder_read_only(console)
            pass_message = "PASS: read-only encoder telemetry check completed."
        else:
            run_test(console)
            pass_message = "PASS: UART safety-state smoke test completed."

        passed = True
        print(f"\n{pass_message}")
        return 0
    finally:
        if not passed and not args.encoder_only:
            try:
                response = console.command("disable")
                print(f"\nSafety cleanup response:\n{response.rstrip()}")
            except Exception as cleanup_error:  # pragma: no cover - hardware path
                print(
                    f"WARNING: automatic disable failed: {cleanup_error}",
                    file=sys.stderr,
                )

        console.close()


if __name__ == "__main__":
    raise SystemExit(main())
