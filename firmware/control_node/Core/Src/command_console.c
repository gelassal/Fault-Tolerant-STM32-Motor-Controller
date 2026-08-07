#include "command_console.h"

#include "motor_controller.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define COMMAND_BUFFER_SIZE 64U
#define UART_TX_TIMEOUT_MS  100U


static UART_HandleTypeDef *console_uart = NULL;

static uint8_t received_byte = 0U;
static char command_buffer[COMMAND_BUFFER_SIZE];

static volatile uint16_t command_length = 0U;
static volatile bool command_ready = false;
static volatile bool command_overflow = false;


/* -------------------------------------------------------------------------- */
/* Private functions                                                          */
/* -------------------------------------------------------------------------- */

static HAL_StatusTypeDef StartReceive(void)
{
    if (console_uart == NULL)
    {
        return HAL_ERROR;
    }

    return HAL_UART_Receive_IT(
        console_uart,
        &received_byte,
        1U
    );
}


static void ConsoleWrite(const char *message)
{
    if ((console_uart == NULL) || (message == NULL))
    {
        return;
    }

    (void)HAL_UART_Transmit(
        console_uart,
        (uint8_t *)message,
        (uint16_t)strlen(message),
        UART_TX_TIMEOUT_MS
    );
}


static void ConsolePrompt(void)
{
    ConsoleWrite("> ");
}


static char *TrimWhitespace(char *text)
{
    char *end;

    if (text == NULL)
    {
        return NULL;
    }

    while (isspace((unsigned char)*text))
    {
        text++;
    }

    if (*text == '\0')
    {
        return text;
    }

    end = text + strlen(text) - 1U;

    while ((end > text) &&
           isspace((unsigned char)*end))
    {
        *end = '\0';
        end--;
    }

    return text;
}


static void ConvertToLowercase(char *text)
{
    if (text == NULL)
    {
        return;
    }

    while (*text != '\0')
    {
        *text = (char)tolower((unsigned char)*text);
        text++;
    }
}


static const char *ControllerStateToString(
    MotorControllerState_t state)
{
    switch (state)
    {
        case MOTOR_CONTROLLER_STATE_UNINITIALIZED:
            return "uninitialized";

        case MOTOR_CONTROLLER_STATE_DISABLED:
            return "disabled";

        case MOTOR_CONTROLLER_STATE_READY:
            return "ready";

        case MOTOR_CONTROLLER_STATE_RUNNING:
            return "running";

        case MOTOR_CONTROLLER_STATE_BRAKING:
            return "braking";

        case MOTOR_CONTROLLER_STATE_FAULT:
            return "fault";

        default:
            return "unknown";
    }
}


static const char *ControllerFaultToString(
    MotorControllerFault_t fault)
{
    switch (fault)
    {
        case MOTOR_CONTROLLER_FAULT_NONE:
            return "none";

        case MOTOR_CONTROLLER_FAULT_DRIVER_DIAGNOSTIC:
            return "driver_diagnostic";

        case MOTOR_CONTROLLER_FAULT_DRIVER_ERROR:
            return "driver_error";

        case MOTOR_CONTROLLER_FAULT_SOFTWARE:
            return "software";

        case MOTOR_CONTROLLER_FAULT_CONTROL_TIMEOUT:
            return "control_timeout";

        case MOTOR_CONTROLLER_FAULT_ENCODER_LOSS:
            return "encoder_loss";

        case MOTOR_CONTROLLER_FAULT_OVERCURRENT:
            return "overcurrent";

        case MOTOR_CONTROLLER_FAULT_OVERSPEED:
            return "overspeed";

        default:
            return "unknown";
    }
}


static void SendMotorStatus(void)
{
    char response[160];

    const char *state_text =
        ControllerStateToString(
            MotorController_GetState()
        );

    const char *fault_text =
        ControllerFaultToString(
            MotorController_GetFault()
        );

    const char *direction_text =
        MotorController_GetDirection() ==
        MOTOR_DIRECTION_FORWARD
            ? "forward"
            : "reverse";

    int duty_percent =
        (int)(MotorController_GetDuty() + 0.5f);

    (void)snprintf(
        response,
        sizeof(response),
        "STATUS state=%s direction=%s duty=%d fault=%s\r\n",
        state_text,
        direction_text,
        duty_percent,
        fault_text
    );

    ConsoleWrite(response);
}


static void SendControllerResult(
    MotorControllerStatus_t status,
    const char *success_message)
{
    switch (status)
    {
        case MOTOR_CONTROLLER_STATUS_OK:
            ConsoleWrite(success_message);
            break;

        case MOTOR_CONTROLLER_STATUS_NOT_INITIALIZED:
            ConsoleWrite(
                "ERR controller not initialized\r\n"
            );
            break;

        case MOTOR_CONTROLLER_STATUS_INVALID_ARGUMENT:
            ConsoleWrite(
                "ERR invalid argument\r\n"
            );
            break;

        case MOTOR_CONTROLLER_STATUS_INVALID_STATE:
            ConsoleWrite(
                "ERR command not allowed in current state\r\n"
            );
            break;

        case MOTOR_CONTROLLER_STATUS_DRIVER_ERROR:
            ConsoleWrite(
                "ERR low-level motor-driver failure\r\n"
            );
            break;

        case MOTOR_CONTROLLER_STATUS_FAULT_ACTIVE:
            ConsoleWrite(
                "ERR fault is latched; clear fault first\r\n"
            );
            break;

        case MOTOR_CONTROLLER_STATUS_FAULT_STILL_PRESENT:
            ConsoleWrite(
                "ERR physical fault is still present\r\n"
            );
            break;

        default:
            ConsoleWrite(
                "ERR unknown controller failure\r\n"
            );
            break;
    }
}


static void HandleDutyCommand(char *argument)
{
    char *end_pointer = NULL;
    char response[48];

    if ((argument == NULL) ||
        (*argument == '\0'))
    {
        ConsoleWrite(
            "ERR usage: duty <0-100>\r\n"
        );
        return;
    }

    long duty_percent = strtol(
        argument,
        &end_pointer,
        10
    );

    while ((end_pointer != NULL) &&
           isspace((unsigned char)*end_pointer))
    {
        end_pointer++;
    }

    if ((end_pointer == argument) ||
        (end_pointer == NULL) ||
        (*end_pointer != '\0') ||
        (duty_percent < 0L) ||
        (duty_percent > 100L))
    {
        ConsoleWrite(
            "ERR duty must be an integer from 0 to 100\r\n"
        );
        return;
    }

    MotorControllerStatus_t status =
        MotorController_SetDuty((float)duty_percent);

    if (status != MOTOR_CONTROLLER_STATUS_OK)
    {
        SendControllerResult(status, "");
        return;
    }

    (void)snprintf(
        response,
        sizeof(response),
        "OK duty=%ld\r\n",
        duty_percent
    );

    ConsoleWrite(response);
}


static void HandleCommand(char *command)
{
    char *normalized_command =
        TrimWhitespace(command);

    if ((normalized_command == NULL) ||
        (*normalized_command == '\0'))
    {
        return;
    }

    ConvertToLowercase(normalized_command);

    if (strcmp(normalized_command, "enable") == 0)
    {
        SendControllerResult(
            MotorController_Enable(),
            "OK state=ready duty=0 output=coast\r\n"
        );
    }
    else if (strcmp(normalized_command, "disable") == 0)
    {
        SendControllerResult(
            MotorController_Disable(),
            "OK state=disabled duty=0\r\n"
        );
    }
    else if (strcmp(normalized_command, "forward") == 0)
    {
        SendControllerResult(
            MotorController_SetDirection(
                MOTOR_DIRECTION_FORWARD
            ),
            "OK direction=forward\r\n"
        );
    }
    else if (strcmp(normalized_command, "reverse") == 0)
    {
        SendControllerResult(
            MotorController_SetDirection(
                MOTOR_DIRECTION_REVERSE
            ),
            "OK direction=reverse\r\n"
        );
    }
    else if (strncmp(
                 normalized_command,
                 "duty ",
                 5U) == 0)
    {
        HandleDutyCommand(
            normalized_command + 5U
        );
    }
    else if (strcmp(normalized_command, "duty") == 0)
    {
        ConsoleWrite(
            "ERR usage: duty <0-100>\r\n"
        );
    }
    else if (strcmp(normalized_command, "brake") == 0)
    {
        SendControllerResult(
            MotorController_Brake(),
            "OK brake active\r\n"
        );
    }
    else if (strcmp(normalized_command, "release") == 0)
    {
        SendControllerResult(
            MotorController_ReleaseBrake(),
            "OK state=ready duty=0 output=coast\r\n"
        );
    }
    else if (strcmp(normalized_command, "injectfault") == 0)
    {
        SendControllerResult(
            MotorController_ReportFault(
                MOTOR_CONTROLLER_FAULT_SOFTWARE
            ),
            "OK software fault injected; state=fault\r\n"
        );
    }
    else if (strcmp(normalized_command, "clearfault") == 0)
    {
        SendControllerResult(
            MotorController_ClearFault(),
            "OK fault cleared; state=disabled\r\n"
        );
    }
    else if (strcmp(normalized_command, "status") == 0)
    {
        SendMotorStatus();
    }
    else if (strcmp(normalized_command, "help") == 0)
    {
        ConsoleWrite(
            "Commands:\r\n"
            "  enable\r\n"
            "  disable\r\n"
            "  forward\r\n"
            "  reverse\r\n"
            "  duty <0-100>\r\n"
            "  brake\r\n"
            "  release\r\n"
            "  status\r\n"
            "  injectfault\r\n"
            "  clearfault\r\n"
            "  help\r\n"
        );
    }
    else
    {
        ConsoleWrite(
            "ERR unknown command; type help\r\n"
        );
    }
}


/* -------------------------------------------------------------------------- */
/* Public functions                                                           */
/* -------------------------------------------------------------------------- */

bool CommandConsole_Init(
    UART_HandleTypeDef *uart)
{
    if (uart == NULL)
    {
        return false;
    }

    console_uart = uart;

    received_byte = 0U;
    command_length = 0U;
    command_ready = false;
    command_overflow = false;

    (void)memset(
        command_buffer,
        0,
        sizeof(command_buffer)
    );

    if (StartReceive() != HAL_OK)
    {
        console_uart = NULL;
        return false;
    }

    ConsoleWrite(
        "\r\n"
        "Command console ready. Type help.\r\n"
    );

    ConsolePrompt();

    return true;
}


void CommandConsole_Process(void)
{
    if (!command_ready)
    {
        return;
    }

    if (command_overflow)
    {
        ConsoleWrite(
            "\r\nERR command too long\r\n"
        );
    }
    else
    {
        /*
         * CubeIDE may not show typed characters through local
         * echo. Print the completed command once Enter is pressed.
         *
         * Since the prompt was already printed, this produces:
         *
         * > help
         */
        ConsoleWrite(command_buffer);
        ConsoleWrite("\r\n");

        HandleCommand(command_buffer);
    }

    command_length = 0U;
    command_ready = false;
    command_overflow = false;

    (void)memset(
        command_buffer,
        0,
        sizeof(command_buffer)
    );

    ConsolePrompt();

    if (StartReceive() != HAL_OK)
    {
        ConsoleWrite(
            "\r\n"
            "ERR failed to restart UART reception\r\n"
        );
    }
}


void CommandConsole_OnRxComplete(
    UART_HandleTypeDef *uart)
{
    if ((console_uart == NULL) ||
        (uart != console_uart) ||
        command_ready)
    {
        return;
    }

    char character = (char)received_byte;

    /*
     * Accept CR, LF, and CR+LF as Enter.
     *
     * Many serial terminals send only CR. The previous version
     * ignored CR, which prevented those terminals from submitting
     * commands.
     */
    if ((character == '\r') ||
        (character == '\n'))
    {
        /*
         * Ignore empty terminators. This also ignores the LF
         * remaining after a command that ended with CR+LF.
         */
        if ((command_length == 0U) &&
            (!command_overflow))
        {
            (void)StartReceive();
            return;
        }

        command_buffer[command_length] = '\0';
        command_ready = true;

        /*
         * Reception pauses here. The main loop processes the
         * command and then starts reception again.
         */
        return;
    }

    /*
     * Support Backspace and Delete.
     */
    if ((character == '\b') ||
        ((unsigned char)character == 127U))
    {
        if (command_length > 0U)
        {
            command_length--;
            command_buffer[command_length] = '\0';
        }

        (void)StartReceive();
        return;
    }

    if (command_length <
        (COMMAND_BUFFER_SIZE - 1U))
    {
        command_buffer[command_length] =
            character;

        command_length++;

        command_buffer[command_length] =
            '\0';

        (void)StartReceive();
        return;
    }

    command_overflow = true;
    command_ready = true;
}
