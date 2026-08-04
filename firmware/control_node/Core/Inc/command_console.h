#ifndef COMMAND_CONSOLE_H
#define COMMAND_CONSOLE_H

#include "main.h"

#include <stdbool.h>

/**
 * Initializes the UART command console and starts interrupt-driven reception.
 *
 * Returns true if reception started successfully.
 */
bool CommandConsole_Init(UART_HandleTypeDef *uart);

/**
 * Parses and executes a completed command.
 *
 * Call repeatedly from the main loop.
 */
void CommandConsole_Process(void);

/**
 * Passes the HAL UART receive-complete callback into the console module.
 */
void CommandConsole_OnRxComplete(UART_HandleTypeDef *uart);

#endif
