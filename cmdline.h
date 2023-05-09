/*****************************************************************************
 *  Copyright (C) RJ Brands 2021. All rights reserved.
 *****************************************************************************/

/**
 * @file  cmdline.h
 *
 */

#ifndef __CMDLINE_H__
#define __CMDLINE_H__

/*****************************************************************************
 *	Public Includes
 *****************************************************************************/
#include <stdbool.h>
#include <stdint.h>
/*****************************************************************************
 *	Public External References
 *****************************************************************************/

/*****************************************************************************
 *	Public Defines & Macros
 *****************************************************************************/
#define CMDLINE_MAX_ARGS 8 // maximum number of arguments

/*****************************************************************************
 *	Public Typedefs & Enumerations
 *****************************************************************************/
typedef enum
{
    CMDLINE_OK = 0,
    CMDLINE_ERROR,
} cmdline_err_t;

typedef enum
{
    CMDLINE_EOL_CR = 0,
    CMDLINE_EOL_LF,
} cmdline_eol_t;

typedef enum
{
    CMDLINE_CMD_OK = 0,
    CMDLINE_INVALID_ARG,
    CMDLINE_TOO_FEW_ARGS,
    CMDLINE_TOO_MANY_ARGS,
    CMDLINE_BAD_CMD,
    CMDLINE_NOT_INITIALIZED,
    CMDLINE_TOTAL_STATUS,
} cmdline_status_t;

struct cmdline_config_t
{
    struct cmdline_entry_t* cmdline_entry;
    char*                   receive_buffer;
    uint16_t                receive_buffer_size;
    cmdline_eol_t           eol;
    bool (*tx_char)(char);
    bool (*rx_char)(char*);
};

typedef cmdline_status_t (*pfnCmdLine)(uint8_t argc, uint8_t* argv[]);

struct cmdline_entry_t
{
    const char* pcCmd;       // string containing the name of the command.
    pfnCmdLine  pfnCmd;      // function pointer to the implementation of the command.
    const char* pcHelp;      // string of brief help text for the command.
    void (*pfCmdHelp)(void); // Detailed help on command
};

struct cmdline_t
{
    char*                   ppcArgv[CMDLINE_MAX_ARGS + 1];
    uint8_t                 total_argc_count;
    bool                    initialized;
    uint16_t                receive_char_index;
    char                    char_eol;
    char                    char_ignore;
    struct cmdline_config_t config;
};

/*****************************************************************************
 *	Public Function Prototypes
 *****************************************************************************/
cmdline_err_t cmdline_init(struct cmdline_t* cmdline, struct cmdline_config_t* config);
cmdline_err_t cmdline_process_msg(struct cmdline_t* cmdline);
#endif // __CMDLINE_H__
