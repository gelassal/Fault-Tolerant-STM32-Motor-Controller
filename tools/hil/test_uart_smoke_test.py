#!/usr/bin/env python3
"""Unit tests for UART response parsing and the encoder-only command path."""

from __future__ import annotations

import contextlib
import io
import unittest

import uart_smoke_test


class FakeConsole:
    def __init__(self, responses: dict[str, str]) -> None:
        self.responses = responses
        self.commands: list[str] = []
        self.synchronized = False

    def synchronize(self) -> None:
        self.synchronized = True

    def command(self, command: str) -> str:
        self.commands.append(command)
        return self.responses[command]


class UARTParsingTests(unittest.TestCase):
    def test_parse_encoder_accepts_signed_values(self) -> None:
        telemetry = uart_smoke_test.parse_encoder(
            "ENCODER initialized=1 count=-1234 delta=-12 "
            "rpm_milli=-200 direction=reverse\r\n> "
        )

        self.assertEqual("1", telemetry["initialized"])
        self.assertEqual("-1234", telemetry["count"])
        self.assertEqual("-12", telemetry["delta"])
        self.assertEqual("-200", telemetry["rpm_milli"])
        self.assertEqual("reverse", telemetry["direction"])

    def test_parse_encoder_rejects_malformed_response(self) -> None:
        with self.assertRaises(AssertionError):
            uart_smoke_test.parse_encoder("ENCODER count=1\r\n> ")

    def test_encoder_only_sends_read_only_commands(self) -> None:
        console = FakeConsole(
            {
                "status": (
                    "STATUS state=disabled direction=forward duty=0 "
                    "fault=none\r\n> "
                ),
                "encoder": (
                    "ENCODER initialized=1 count=0 delta=0 rpm_milli=0 "
                    "direction=stationary\r\n> "
                ),
            }
        )

        with contextlib.redirect_stdout(io.StringIO()):
            uart_smoke_test.run_encoder_read_only(console)

        self.assertTrue(console.synchronized)
        self.assertEqual(["status", "encoder"], console.commands)

    def test_encoder_only_rejects_active_motor_state(self) -> None:
        console = FakeConsole(
            {
                "status": (
                    "STATUS state=running direction=forward duty=12 "
                    "fault=none\r\n> "
                )
            }
        )

        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaises(AssertionError):
                uart_smoke_test.run_encoder_read_only(console)

        self.assertEqual(["status"], console.commands)


if __name__ == "__main__":
    unittest.main()
