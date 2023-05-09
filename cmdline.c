/**
 * @file cmdline.c
 * @brief module to parse serial command.
 *
 */

/*****************************************************************************
 *	Private Includes
 *****************************************************************************/
#include "cmdline.h"
#include <string.h>
/*****************************************************************************
 *	Private External References
 *****************************************************************************/

/*****************************************************************************
 *	Private Defines & Macros
 *****************************************************************************/
#define CHARACTER_BACKSPACE 0x08
#define CHARACTER_DELETE    127
#define CHARACTER_SPACE     0x20
#define CHARACTER_RETURN    '\r'
#define CHARACTER_NEWLINE   '\n'
/*****************************************************************************
 *	Private Typedefs & Enumerations
 *****************************************************************************/

const char* cmdline_status_msg[CMDLINE_TOTAL_STATUS] = {
    [CMDLINE_CMD_OK]          = "CMD OK",
    [CMDLINE_INVALID_ARG]     = "INVALID ARG",
    [CMDLINE_TOO_FEW_ARGS]    = "TOO FEW ARGS",
    [CMDLINE_TOO_MANY_ARGS]   = "TOO MANY ARGS",
    [CMDLINE_BAD_CMD]         = "BAD CMD",
    [CMDLINE_NOT_INITIALIZED] = "NOT INITIALIZED",
};
/*****************************************************************************
 *	Private Variables
 *****************************************************************************/

/*****************************************************************************
 *	Private Function Prototypes
 *****************************************************************************/
static cmdline_status_t cmdline_process(struct cmdline_t* cmdline);
static void             set_eol_character(struct cmdline_t* cmdline);
static bool             process_rx_msg_complete(struct cmdline_t* cmdline);
static bool             is_end_of_character_or_buffer_detected(struct cmdline_t* cmdline, char byte);
static void             terminate_receive_buffer(struct cmdline_t* cmdline);
static void             remove_last_entry_character_data(struct cmdline_t* cmdline);
static void             add_received_byte_to_serial_msg(struct cmdline_t* cmdline, char byte);
static uint8_t          get_argc_count_from_cmd_msg(struct cmdline_t* cmdline);
static cmdline_status_t execute_matching_cmd_entry_return_status(struct cmdline_t* cmdline, uint8_t argc_count);
static bool             is_argc_start_found(struct cmdline_t* cmdline, int msg_index);
static void             send_command_line_status(struct cmdline_t* cmdline, cmdline_status_t cmdline_status);
static void             clear_receive_buffer(struct cmdline_t* cmdline);
static void             echo_received_character(struct cmdline_t* cmdline, char received_byte);
/*****************************************************************************
 *	Public Functions
 *****************************************************************************/
cmdline_err_t cmdline_init(struct cmdline_t* cmdline, struct cmdline_config_t* config)
{
    if (cmdline == NULL || config == NULL)
    {
        return CMDLINE_ERROR;
    }

    if (cmdline->initialized == true)
    {
        return CMDLINE_OK;
    }

    if (config->cmdline_entry == NULL || config->receive_buffer == NULL || config->rx_char == NULL ||
        config->tx_char == NULL)
    {
        return CMDLINE_ERROR;
    }

    cmdline->config = *config;

    set_eol_character(cmdline);

    cmdline->initialized = true;
    return CMDLINE_OK;
}

cmdline_err_t cmdline_process_msg(struct cmdline_t* cmdline)
{
    if (cmdline == NULL || cmdline->initialized == false)
    {
        return CMDLINE_ERROR;
    }

    if (process_rx_msg_complete(cmdline) == false)
    {
        return CMDLINE_OK;
    }

    cmdline_status_t cmdline_status = cmdline_process(cmdline);
    send_command_line_status(cmdline, cmdline_status);

    clear_receive_buffer(cmdline);

    return CMDLINE_OK;
}
/*****************************************************************************
 *	Private Functions
 *****************************************************************************/
static bool process_rx_msg_complete(struct cmdline_t* cmdline)
{
    char received_byte;
    bool (*rx_char)(char*) = cmdline->config.rx_char;

    while (rx_char(&received_byte) == true)
    {
        echo_received_character(cmdline, received_byte);

        if (is_end_of_character_or_buffer_detected(cmdline, received_byte))
        {
            terminate_receive_buffer(cmdline);
            return true;
        }

        if (received_byte == CHARACTER_DELETE || received_byte == CHARACTER_BACKSPACE)
        {
            remove_last_entry_character_data(cmdline);
        }
        else if (received_byte != cmdline->char_ignore)
        {
            add_received_byte_to_serial_msg(cmdline, received_byte);
        }
    }

    return false;
}

static cmdline_status_t cmdline_process(struct cmdline_t* cmdline)
{
    if (cmdline->initialized == false)
    {
        return CMDLINE_NOT_INITIALIZED;
    }

    uint8_t total_argc_count = get_argc_count_from_cmd_msg(cmdline);

    if (total_argc_count == 0)
    {
        return CMDLINE_TOO_FEW_ARGS;
    }

    return execute_matching_cmd_entry_return_status(cmdline, total_argc_count);
}

static void set_eol_character(struct cmdline_t* cmdline)
{
    if (cmdline->config.eol == CMDLINE_EOL_CR)
    {
        cmdline->char_eol    = CHARACTER_RETURN;
        cmdline->char_ignore = CHARACTER_NEWLINE;
    }
    else
    {
        cmdline->char_eol    = CHARACTER_NEWLINE;
        cmdline->char_ignore = CHARACTER_RETURN;
    }
    return;
}

static bool is_end_of_character_or_buffer_detected(struct cmdline_t* cmdline, char byte)
{
    uint16_t receive_char_index  = cmdline->receive_char_index;
    uint16_t receive_buffer_size = cmdline->config.receive_buffer_size;
    if (receive_char_index >= receive_buffer_size - 1 || byte == cmdline->char_eol)
    {
        return true;
    }

    return false;
}

static void terminate_receive_buffer(struct cmdline_t* cmdline)
{
    uint16_t receive_char_index  = cmdline->receive_char_index;
    uint16_t receive_buffer_size = cmdline->config.receive_buffer_size;
    char*    receive_buffer      = cmdline->config.receive_buffer;

    int i;
    for (i = receive_char_index; i < receive_buffer_size; i++)
    {
        receive_buffer[i] = 0;
    }

    return;
}

static void remove_last_entry_character_data(struct cmdline_t* cmdline)
{
    char* receive_buffer  = cmdline->config.receive_buffer;

    if (cmdline->receive_char_index == 0)
    {
        return;
    }

    cmdline->receive_char_index--;
    receive_buffer[cmdline->receive_char_index] = 0;
}

static void add_received_byte_to_serial_msg(struct cmdline_t* cmdline, char byte)
{
    char* receive_buffer  = cmdline->config.receive_buffer;

    receive_buffer[cmdline->receive_char_index] = byte;
    cmdline->receive_char_index++;
}

static uint8_t get_argc_count_from_cmd_msg(struct cmdline_t* cmdline)
{
    char*   receive_buffer = cmdline->config.receive_buffer;
    uint8_t argc_counter   = 0;
    int i;
    for (i = 0; i < cmdline->receive_char_index; i++)
    {
        if (is_argc_start_found(cmdline, i))
        {
            if (argc_counter >= CMDLINE_MAX_ARGS)
            {
                break;
            }

            cmdline->ppcArgv[argc_counter++] = &receive_buffer[i];
        }
    }

    return argc_counter;
}

static cmdline_status_t execute_matching_cmd_entry_return_status(struct cmdline_t* cmdline, uint8_t argc_count)
{
    struct cmdline_entry_t* psCmdEntry = &cmdline->config.cmdline_entry[0];

    while (psCmdEntry->pcCmd)
    {
        if (strcmp(cmdline->ppcArgv[0], psCmdEntry->pcCmd) == 0)
        {
            return psCmdEntry->pfnCmd(argc_count, (uint8_t**)cmdline->ppcArgv);
        }

        psCmdEntry++;
    }

    return CMDLINE_BAD_CMD;
}

static bool is_argc_start_found(struct cmdline_t* cmdline, int msg_index)
{
    if (msg_index == 0)
    {
        return true;
    }

    char* msg = cmdline->config.receive_buffer;

    if (msg[msg_index] == ' ')
    {
        msg[msg_index] = 0;
        return false;
    }

    if (msg[msg_index - 1] == 0)
    {
        return true;
    }

    return false;
}

static void send_command_line_status(struct cmdline_t* cmdline, cmdline_status_t cmdline_status)
{
    if (cmdline_status >= CMDLINE_TOTAL_STATUS)
    {
        cmdline_status = CMDLINE_BAD_CMD;
    }

    const char* msg       = cmdline_status_msg[cmdline_status];
    bool (*tx_char)(char) = cmdline->config.tx_char;

    while (*msg)
    {
        tx_char(*msg);
        msg++;
    }

    tx_char(CHARACTER_RETURN);
    tx_char(CHARACTER_NEWLINE);

    return;
}

static void clear_receive_buffer(struct cmdline_t* cmdline)
{
    uint16_t receive_buffer_size = cmdline->config.receive_buffer_size;
    char*    receive_buffer      = cmdline->config.receive_buffer;

    memset(receive_buffer, 0, receive_buffer_size);
    cmdline->receive_char_index = 0;
}

static void echo_received_character(struct cmdline_t* cmdline, char received_byte)
{
    bool (*tx_char)(char) = cmdline->config.tx_char;
    tx_char(received_byte);
}
