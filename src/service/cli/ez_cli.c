/*****************************************************************************
* Filename:         ez_cli.c
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

/** @file   ez_cli.c
 *  @author Hai Nguyen
 *  @date   18.02.2025
 *  @brief  Implementation of the Command Line Interface application
 *
 *  @details
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_cli.h"

#if (EZ_CLI == 1)
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_CLI_LOGGING_LEVEL     /**< logging level */
#define MOD_NAME    "ez_cli"        /**< module name */
#include "ez_logging.h"


/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define STR_TERMINATE                   '\0'
#define SPACE                           ' '
#define ARG_INVALID                     CONFIG_NUM_OF_ARGUMENT


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* None */

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Definitions
*****************************************************************************/
static bool ezCli_IsCommandExisting(
    ezCliCmdList_t *cmd_list,
    const char * command,
    uint8_t * index);

static void ezCli_ResetStateMachine(ezCli_t *cli);
static bool ezCli_IsArgumentExisting(
    ezCliCommand_t *cmd,
    const char * arg);
static void ezCli_Printf (ezCli_t *self, char* fmt, ...);
static void ezCli_ProcessCommand(ezCli_t *cli);
static void ezCli_PrintMenu(ezCli_t *self, ezCliCmdList_t *cmd_list);
static void ezCli_PrintCommandHelper(ezCli_t *self, ezCliCommand_t *cmd);
static void ezCli_HandleStateCommand(ezCli_t *self, const char c);
static void ezCli_HandleStateArgument(ezCli_t *self, const char c);
static void ezCli_HandleStateValue(ezCli_t *self, const char c);
static void ezCli_HandleStateError(ezCli_t *self, const char c);

/*****************************************************************************
* Public functions
*****************************************************************************/
ezSTATUS ezCli_Init(
    ezCli_t *self,
    ezCliBuff_t *cli_buffer,
    ezCliInterface_t *interface,
    ezCliCmdList_t *cmd_list)
{
    if(self == NULL || cli_buffer == NULL || interface == NULL || cmd_list == NULL)
    {
        return ezSTATUS_ARG_INVALID;
    }

    if(cli_buffer->size == 0 || cli_buffer->buff == NULL)
    {
        return ezSTATUS_ARG_INVALID;
    }

    if(interface->GetCharCallback == NULL || interface->SendCharsCallback == NULL)
    {
        return ezSTATUS_ARG_INVALID;
    }

    if(cmd_list->num_of_cmd == 0 || cmd_list->commands == NULL)
    {
        return ezSTATUS_ARG_INVALID;
    }

    self->cmd_list = cmd_list;
    self->interface = interface;
    self->cli_buffer = cli_buffer;
    self->state = STATE_COMMAND;
    self->quotation_mark_count = 0;

    ezCli_ResetStateMachine(self);
    EZINFO("Init CLI ok");
    ezCli_PrintMenu(self, self->cmd_list);
    return ezSUCCESS;
}

void ezCli_Run(ezCli_t *self)
{
    if(self == NULL)
    {
        return;
    }

    if(self->cli_buffer->curr_index >= self->cli_buffer->size)
    {
        /* buffer overflow, reset the state machine */
        EZERROR("CLI buffer overflow");
        self->state = STATE_ERROR;
    }

    char c = self->interface->GetCharCallback();
    if((c == SPACE || c == STR_TERMINATE || c == '\n' || c == '\r')
        && self->cli_buffer->curr_index == 0)
    {
        /* skip space if it is the first character */
        return;
    }

    self->cli_buffer->buff[self->cli_buffer->curr_index] = c;
    
    switch (self->state)
    {
    case STATE_COMMAND:
    {
        ezCli_HandleStateCommand(self, c);
        break;
    }
    case STATE_ARGUMENT:
    {
        ezCli_HandleStateArgument(self, c);
        break;
    }
    case STATE_VALUE:
    {
        ezCli_HandleStateValue(self, c);
        break;
    }
    case STATE_ERROR:
        ezCli_HandleStateError(self, c);
        break;
    default:
        break;
    }
}


/*****************************************************************************
* Local functions
*****************************************************************************/
static void ezCli_HandleStateCommand(ezCli_t *self, const char c)
{
    if(self == NULL)
    {
        return;
    }

    if(c == SPACE)
    {
        self->cli_buffer->buff[self->cli_buffer->curr_index] = STR_TERMINATE;
        self->cli_buffer->curr_index++;
        EZDEBUG("receive command: [command = %s]", self->cli_buffer->buff);
        if(ezCli_IsCommandExisting(self->cmd_list, self->cli_buffer->buff, &self->curr_cmd_index) == true)
        {
            memset(self->arg_list, 0, sizeof(self->arg_list));
            self->state = STATE_ARGUMENT;
        }
        else
        {
            self->state = STATE_ERROR;
        }
    }
    else if (c == '\n' || c == '\r')
    {
        self->cli_buffer->buff[self->cli_buffer->curr_index] = STR_TERMINATE;
        self->cli_buffer->curr_index++;
        EZDEBUG("received command [cmd = %s]. check if command requires argument",
            self->cli_buffer->buff);

        if(ezCli_IsCommandExisting(self->cmd_list, self->cli_buffer->buff, &self->curr_cmd_index) == true
            && self->cmd_list->commands[self->curr_cmd_index].short_arg_list[0] == NULL)
        {
            ezCli_ProcessCommand(self);
            self->state = STATE_COMMAND;
        }
        else
        {
            ezCli_PrintMenu(self, self->cmd_list);
            self->state = STATE_ERROR;
        }
    }
    else
    {
        /* do nothing, wait for next character */
        self->cli_buffer->curr_index++;
    }
}

static void ezCli_HandleStateArgument(ezCli_t *self, const char c)
{
    if(self == NULL)
    {
        return;
    }

    if(c == SPACE)
    {
        /* argument is found and we hit a space -> complete parsing the argument */
        if(self->is_arg_found == true)
        {
            self->cli_buffer->buff[self->cli_buffer->curr_index] = STR_TERMINATE;
            self->cli_buffer->curr_index++;
            
            if(ezCli_IsArgumentExisting(
                &self->cmd_list->commands[self->curr_cmd_index],
                self->arg_list[self->curr_arg_index]) == false)
            {
                /* argument is not existing, return error */
                EZDEBUG("[arg = %s] is not supported", self->arg_list[self->curr_arg_index]);
                ezCli_PrintCommandHelper(self, &self->cmd_list->commands[self->curr_cmd_index]);
                self->state = STATE_ERROR;
                return;
            }
            else
            {
                EZDEBUG("complete parse the argument: [arg = %s]", self->arg_list[self->curr_arg_index]);
                self->is_arg_found = false;
                self->state = STATE_VALUE;
            }
        }
        /* spaces before the argument, do nothing */
        else
        {
            /* do nothing, skip the spaces */
            return;
        }
    }
    else if (c == '\n' || c == '\r')
    {
        self->cli_buffer->buff[self->cli_buffer->curr_index] = STR_TERMINATE;
        self->cli_buffer->curr_index++;
        ezCli_ProcessCommand(self);
        self->state = STATE_COMMAND;
    }
    else
    {
        /* do nothing, wait for next character */
        if(self->is_arg_found == false)
        {
            /* Detect the argument */
            if(c == '-')
            {
                self->is_arg_found = true;
                self->arg_list[self->curr_arg_index] = (void*)&self->cli_buffer->buff[self->cli_buffer->curr_index];
            }
            /* invalid argument */
            else
            {
                self->state = STATE_ERROR;
            }
        }
        self->cli_buffer->curr_index++;
    }
}


static void ezCli_HandleStateValue(ezCli_t *self, const char c)
{
    if(self == NULL)
    {
        return;
    }

    if(c == SPACE)
    {
        /* value is found and we hit a space -> complete parsing the value */
        if(self->is_value_found == true)
        {
            /* get a string or non-string value */
            if(self->quotation_mark_count == 0 || self->quotation_mark_count == 2)
            {
                /* complete parse the argument*/
                self->cli_buffer->buff[self->cli_buffer->curr_index] = STR_TERMINATE;
                self->cli_buffer->curr_index++;
                EZDEBUG("complete parse the value: [value = %s]", self->value_list[self->curr_arg_index]);
                self->quotation_mark_count = 0;
                self->is_value_found = false;
                self->curr_arg_index++;
                if(self->curr_arg_index < CONFIG_NUM_OF_ARGUMENT)
                {
                    self->state = STATE_ARGUMENT;
                }
                else
                {
                    ezCli_PrintCommandHelper(self, &self->cmd_list->commands[self->curr_cmd_index]);
                    self->state = STATE_ERROR;
                }
            }
            else if (self->quotation_mark_count == 1)
            {
                /* continue to store char since wa have string value */
                self->cli_buffer->curr_index++;
            }
            
        }
        else
        {
            /* do nothing, skip the spaces */
            return;
        }
    }
    else if (c == '"')
    {
        self->quotation_mark_count++;
        if(self->quotation_mark_count == 1)
        {
            EZDEBUG("String starts");
            self->is_value_found == true;

            /* Automatically increase index by 1 to bypass the " and store the next char */
            self->value_list[self->curr_arg_index] = (void*)&self->cli_buffer->buff[self->cli_buffer->curr_index + 1];
        }
        else if (self->quotation_mark_count == 2)
        {
            /* not increase the index becase we do not take " char */
            EZDEBUG("String ends");
        }
        else
        {
            self->state = STATE_ERROR;
        }
    }
    else if (c == '\n' || c == '\r')
    {
        /* complete parse the argument*/
        if(self->quotation_mark_count == 0 || self->quotation_mark_count == 2)
        {
            self->cli_buffer->buff[self->cli_buffer->curr_index] = STR_TERMINATE;
            self->cli_buffer->curr_index++;
            self->quotation_mark_count = 0;
            self->is_value_found = false;
            self->curr_arg_index++;
            ezCli_ProcessCommand(self);
            self->state = STATE_COMMAND;
            EZDEBUG("complete parse the value: [value = %s]", self->value_list[self->curr_arg_index]);
        }
        else
        {
            self->state = STATE_ERROR;
        }
    }
    else
    {
        if(self->is_value_found == false)
        {
            self->value_list[self->curr_arg_index] = (void*)&self->cli_buffer->buff[self->cli_buffer->curr_index];
            self->is_value_found = true;
        }
        /* do nothing, wait for next character */
        self->cli_buffer->curr_index++;
    }
}

static void ezCli_HandleStateError(ezCli_t *self, const char c)
{
    if(self == NULL)
    {
        return;
    }

    if(c == '\n' || c == '\r')
    {
        ezCli_ResetStateMachine(self);
        self->state = STATE_COMMAND;
    }
}



/** @brief Process the received command
 * @param[in] self:  command line interface instance
 * @return None
 */
static void ezCli_ProcessCommand(ezCli_t *self)
{
    if(self == NULL)
    {
        ezCli_ResetStateMachine(self);
        return;
    }

    if(self->cmd_list->commands[self->curr_cmd_index].callback == NULL)
    {
        ezCli_ResetStateMachine(self);
        return;
    }

    CLI_NOTIFY_CODE ret = self->cmd_list->commands[self->curr_cmd_index].callback(
        self->cli_buffer->buff,
        self->arg_list,
        self->value_list);

    if(ret == CLI_NC_OK)
    {
        EZDEBUG("execute command successfully");
        self->interface->SendCharsCallback(self->cli_buffer->buff, strlen(self->cli_buffer->buff));
    }
    else if(ret == CLI_NC_BAD_ARG)
    {
        ezCli_Printf(self, "error: bad argument\n");
        ezCli_PrintCommandHelper(self, &self->cmd_list->commands[self->curr_cmd_index]);
        EZDEBUG("execute command failed due to bad argument");
    }
    else
    {
        ezCli_Printf(self, "error: generic error\n");
        ezCli_PrintCommandHelper(self, &self->cmd_list->commands[self->curr_cmd_index]);
        EZDEBUG("execute command failed due to error");
    }
    ezCli_ResetStateMachine(self);
}


/** @brief Print the menu
 * @param[in] self:  command line interface instance
 * @param[in] cmd_list: pointer to command list
 * @return None
 */
static void ezCli_PrintMenu(ezCli_t *self, ezCliCmdList_t *cmd_list)
{
    if(self == NULL || cmd_list == NULL)
    {
        return;
    }

    ezCli_Printf(self, "Available commands:\n");
    for(uint8_t i = 0; i < cmd_list->num_of_cmd; i++)
    {
        ezCli_PrintCommandHelper(self, &cmd_list->commands[i]);
    }
}


/** @brief Print the help of a specific command
 * @param[in] self:  command line interface instance
 * @param[in] cmd: pointer to command to be porinted
 * @return None
 */
static void ezCli_PrintCommandHelper(ezCli_t *self, ezCliCommand_t *cmd)
{
    if(self == NULL || cmd == NULL)
    {
        return;
    }

    ezCli_Printf(self, "%s: %s\n", cmd->command, cmd->cmd_description);
    for(uint8_t i = 0; i < CONFIG_NUM_OF_ARGUMENT; i++)
    {
        if(cmd->long_arg_list[i] != NULL || cmd->short_arg_list[i] != NULL)
        {
            ezCli_Printf(self, "    %s %s: %s\n",
                (char*)cmd->long_arg_list[i],
                (char*)cmd->short_arg_list[i],
                (char*)cmd->arg_description[i]);
        }
    }
}


/** @brief Check if the received command exists
 * @param[in] self:  command line interface instance
 * @param[in] command: command to be checked
 * @param[out] index: index of the command in command list
 * @return true if command exist, otherwise false
 */
static bool ezCli_IsCommandExisting(
    ezCliCmdList_t *cmd_list,
    const char * command,
    uint8_t * index)
{
    if(cmd_list == NULL || command == NULL || cmd_list->commands == NULL || index == NULL)
    {
        return false;
    }

    for(uint8_t i = 0; i < cmd_list->num_of_cmd; i++)
    {
        if(cmd_list->commands[i].command != NULL && 
            strcmp(cmd_list->commands[i].command, command) == 0)
        {
            *index = i;
            EZDEBUG("Command is existing: [command = %s]", command);
            return true;
        }
    }

    return false;
}


/** @brief Reset the state machine
 * @param[in] self: Command line interface instance
 * @return None
 */
static void ezCli_ResetStateMachine(ezCli_t *cli)
{
    if(cli == NULL)
    {
        return;
    }
    memset(cli->cli_buffer->buff, 0, cli->cli_buffer->size);
    cli->cli_buffer->curr_index = 0;
    cli->curr_cmd_index = 0;
    cli->is_arg_found = false;
    cli->is_value_found = false;
    cli->curr_arg_index = 0;
    
    for(uint8_t i = 0; i < CONFIG_NUM_OF_ARGUMENT; i++)
    {
        cli->arg_list[i] = NULL;
        cli->value_list[i] = NULL;
    }
}


/** @brief Check if an argument exists
 * @param[in] cmd: command list
 * @param[in] arg: argument to be checked
 * @return True if argument exist, otherwise false
 */
static bool ezCli_IsArgumentExisting(
    ezCliCommand_t *cmd,
    const char * arg)
{
    uint8_t index = ARG_INVALID;
    for(uint8_t i = 0; i < CONFIG_NUM_OF_ARGUMENT; i++)
    {
        if(cmd->long_arg_list[i] != NULL &&
            strcmp((char *)cmd->long_arg_list[i], arg) == 0)
        {
            return true;
        }
        else if (cmd->short_arg_list[i] != NULL &&
            strcmp((char *)cmd->short_arg_list[i], arg) == 0)
        {
            return true;
        }
    }
    return false;
}


/** @brief Custom printf function
 * @param[in] self: command line interface instance
 * @param[in] fmt: formated string
 * @return None
 */
static void ezCli_Printf (ezCli_t *self, char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vsnprintf(self->cli_buffer->buff, self->cli_buffer->size, fmt, args);
    va_end(args);
    self->interface->SendCharsCallback(self->cli_buffer->buff, self->cli_buffer->size);
}

#endif /* EZ_CLI == 1 */
/* End of file*/
