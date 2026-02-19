/*****************************************************************************
* Filename:         ez_cli.h
* Author:           Hai Nguyen
* Original Date:    10.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_cli.h
 *  @author Hai Nguyen
 *  @date   18.02.2025
 *  @brief  Public API for Command Line Interface application
 *
 *  @details 
 */

#ifndef _EZ_CLI_H
#define _EZ_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_CLI == 1)
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "ez_linked_list.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#ifndef CONFIG_NUM_OF_ARGUMENT
#define CONFIG_NUM_OF_ARGUMENT      2
#endif

#ifndef CONFIG_NUM_OF_CMD
#define CONFIG_NUM_OF_CMD      4
#endif


/*****************************************************************************
* Component Typedefs
*****************************************************************************/

/** @brief CLI notification code
 */
typedef enum
{
    CLI_NC_OK,      /**< Ok code */
    CLI_NC_ERR,     /**< Generic error code */
    CLI_NC_BAD_ARG, /**< Received bad arguments */
}CLI_NOTIFY_CODE;


/** @brief state of the CLI command parser
 */
typedef enum 
{
    STATE_COMMAND,  /**< State parsing the command */
    STATE_ARGUMENT, /**< State parsing the argument */
    STATE_VALUE,    /**< State parsing the value */
    STATE_ERROR,    /**< Handle error and clean up */
}CLI_STATE;

/** @brief command callback function
 * @param[out] tx_rx_buff: buffer to store the response after the command is handled
 * @param[in] arg_list:  list of arguments. Each element is pointer to argument (char*). Number of elements in a list is defined by CONFIG_NUM_OF_ARGUMENT
 * @param[in] value_list: list of values. Each element is pointer to value (char*). Number of elements in a list matches the number of arguments
 * @return CLI_NOTIFY_CODE
 */
typedef CLI_NOTIFY_CODE (*EXEC_CMD_CALLBACK)(char * tx_rx_buff, const void **arg_list, const void **value_list);


/** @brief Callback to get 1 char
 * @param: None
 * @return one character (char)
 */
typedef const char (*GET_CHAR_CALLBACK)(void);

/** @brief Callback to send response
 * @param[in] tx_buff:  data to be transmitted
 * @param[in] size: size of tx_buff
 * @return None
 */
typedef void (*SEND_CHARS_CALLBACK)(char * tx_buff, uint16_t size);


/** @brief Structure holding information of one command
 */
typedef struct{
    const char          *command;           /**< Pointer to the command */
    const char          *cmd_description;   /**< Pointer to the command description */
    EXEC_CMD_CALLBACK   callback;           /**< Pointer to the command callback function */
    const char          *long_arg_list[CONFIG_NUM_OF_ARGUMENT];     /**< Pointer to the list of argument in long form */
    const char          *short_arg_list[CONFIG_NUM_OF_ARGUMENT];    /**< Pointer to the list of argument in short form */
    const char          *arg_description[CONFIG_NUM_OF_ARGUMENT];   /**< Pointer to the command description */
}ezCliCommand_t;


/** @brief Structure holding the interface for getting and receiving character
 */
typedef struct{
    GET_CHAR_CALLBACK GetCharCallback;      /**< Callback function to get 1 character */
    SEND_CHARS_CALLBACK SendCharsCallback;  /**< Callback function to send response */
}ezCliInterface_t;


/** @brief Structure holding the buffer and its informatiom
 */
typedef struct{
    char* buff; /**< pointer to the buffer storing to command */ 
    size_t size; /**< size of the command */
    size_t curr_index;  /**< index of the buffer where next char will be stored */
}ezCliBuff_t;


/** @brief Structure holding all of supported commands
 */
typedef struct{
    ezCliCommand_t *commands;               /**< List of commands */
    size_t num_of_cmd;                      /**< Num of supported commands */
}ezCliCmdList_t;


/** @brief Structure holding the command line interface instance
 */
typedef struct{
    ezCliCmdList_t *cmd_list;               /**< List of commands */
    CLI_STATE       state;                  /**< CLI state machine */
    ezCliBuff_t *cli_buffer;                /**< Pointer to the buffer storing the command */
    ezCliInterface_t *interface;            /**< Interface to get and send characters */
    uint8_t curr_cmd_index;                 /**< Current index of the command being parsed */
    uint8_t curr_arg_index;                 /**< Index of the current argument */
    const void *arg_list[CONFIG_NUM_OF_ARGUMENT];   /**< Storing received arguments */
    const void *value_list[CONFIG_NUM_OF_ARGUMENT]; /**< Storing received values */
    bool is_arg_found;  /**< flag indicate that an argument is found */
    bool is_value_found;    /**< flag indicate that a value is found */
    uint8_t quotation_mark_count;   /**< counter for char " when parsing string value */
}ezCli_t;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/** @brief Initialize a command line interface instance
 * @param[in] self:  command line interface instance
 * @param[in] cli_buffer: buffer for storing data
 * @param[in] interface: interface to send/receive data
 * @param[in] cmd_list: list of supported commands
 * @return ezSTATUS
 */
ezSTATUS ezCli_Init(
    ezCli_t *self,
    ezCliBuff_t *cli_buffer,
    ezCliInterface_t *interface,
    ezCliCmdList_t *cmd_list);


/** @brief Call this function to periodically to process the commands
 * @param[in] self:  command line interface instance
 * @return None
 */
void ezCli_Run(ezCli_t *self);

#ifdef __cplusplus
}
#endif

#endif /* EZ_CLI == 1 */
#endif /* _EZ_CLI_H */


/* End of file */
