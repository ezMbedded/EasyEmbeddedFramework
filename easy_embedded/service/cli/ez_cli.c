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
 *  @date   10.03.2024
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

#define DEBUG_LVL   EZ_CLI_LOGGING_LEVEL       /**< logging level */
#define MOD_NAME    "ez_cli"        /**< module name */
#include "ez_logging.h"

/*the rest of include go here*/

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define STR_TERMINATE                   '\0'
#define SPACE                           ' '
#define ARG_INVALID                     CONFIG_NUM_OF_ARGUMENT
#define SKIP_NULL_SPACE(pointer, end)   while((*pointer == ' ' || *pointer == '\0')\
                                                && pointer != end)\
                                        {pointer++;}
#define BEGIN                           "$ "
#define BUFF_SIZE                       256
#define BUFF_PRINTF_SIZE                256


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/** @brief Metadata of a command
 */
typedef struct 
{
    const char      *command;                        /**< Pointer to the command */
    const char      *description;                    /**< Pointer to the command description */
    CLI_CALLBACK    callback;                       /**< Pointer to the command callback function */
    uint32_t        *long_arg_list[CONFIG_NUM_OF_ARGUMENT];      /**< Pointer to the list of argument in long form */
    uint32_t        *short_arg_list[CONFIG_NUM_OF_ARGUMENT];     /**< Pointer to the list of argument in short form */
    uint32_t        *description_list[CONFIG_NUM_OF_ARGUMENT];   /**< Pointer to the command description */
    char            *value_list[CONFIG_NUM_OF_ARGUMENT];         /**< Pointer to the list of values */
}CommandMetadata;


/** @brief state of the CLI command parser
 */
typedef enum 
{
    STATE_COMMAND,  /**< State parsing the command */
    STATE_ARGUMENT, /**< State parsing the argument */
    STATE_VALUE,    /**< State parsing the value */
    STATE_EXECUTE,  /**< State execute the callback function*/
    STATE_ERROR,    /**< Handle error and clean up */
}ENUM_CLI_STATE;


/**@brief
 */
typedef enum
{
    GET_BYTE,   /**< */
    WAIT,       /**< */
    PROC_CMD,   /**< */
}CLI_RUN_STATE;


/**@brief
 */
struct CliInstance
{ 
    uint8_t         cli_buffer[BUFF_SIZE];  /**< */
    CommandHandle   cli_inst_num;           /**< */
    uint8_t         one_byte;               /**< */
    uint16_t        buff_index;             /**< */
    CLI_RUN_STATE   state;                  /**< */
    #if 0  /* TODO */
    UartDrvApi*     uart_driver;            /**< */
    #endif
};


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
static CommandMetadata  astMetaData[CONFIG_NUM_OF_CMD] = { 0 }; /**< holding commands metadata */
static ENUM_CLI_STATE   eState = STATE_COMMAND;                 /**< Holding the current state of the parser */
static const char       cmd_help[] = "help";
static const char       cmd_help_desc[] = "show help";
static uint8_t          welcome[] = "\n$ CLI has been activated, type help for the list of command\n";
static char             buff_printf[BUFF_PRINTF_SIZE] = { 0 };
static struct CliInstance  cli_inst;

/*****************************************************************************
* Function Definitions
*****************************************************************************/
static uint8_t ezCli_GetFreeInstance(void);
static bool ezCli_ResetMetaData(uint8_t index);
static bool ezCli_IsCommandExist(const char * command,
                                 uint8_t * index);
static uint8_t ezCli_IsLongFormArgumentExist(uint8_t command_index,
                                              const char * long_arg);
static uint8_t ezCli_IsShortFormArgumentExist(uint8_t command_index,
                                               const char * short_arg);
static void ezCli_PrintCommandHelp(uint8_t index);
static bool ezCli_IsArgumentShortForm(char * short_arg);
static bool ezCli_IsArgumentLongForm(char * long_arg);
static void ezCli_ResetValueList(uint8_t index);

static uint8_t UartCallbackHandle(uint8_t notify_code,
                                  void *param1);

static CLI_NOTIFY_CODE HandleHelpCommand(const char* command, void* value_list);


/*****************************************************************************
* Public functions
*****************************************************************************/
#if 0  /* TODO */
bool ezCli_Init(UartDrvApi* uart_driver)
{
    bool success = true;

    for (uint8_t i = 0; i < CONFIG_NUM_OF_CMD; i++)
    {
        if (!ezCli_ResetMetaData(i))
        {
            ERROR("Cannot reset metadata: [index = %d]", i);
        }
    }

    if (uart_driver == NULL)
    {
        success = false;
    }

    if (success)
    {
        cli_inst.uart_driver = uart_driver;
        cli_inst.uart_driver->ezmUart_RegisterCallback(UartCallbackHandle);
        cli_inst.buff_index = 0;
        cli_inst.state = GET_BYTE;
        memset(cli_inst.cli_buffer, 0, sizeof(cli_inst.cli_buffer));

        cli_inst.cli_inst_num = ezCli_RegisterCommand(cmd_help, 
                                                      cmd_help_desc, 
                                                      HandleHelpCommand);

        if (cli_inst.cli_inst_num == 0xFF)
        {
            ERROR("add command error");
            success = false;
        }
        else
        {
            INFO("CLI has been activated, type help for the list of command");
            cli_inst.uart_driver->ezmUart_Send(welcome, sizeof(welcome));
            cli_inst.uart_driver->ezmUart_Send((uint8_t*)"$ ", sizeof("$ "));
        }
    }

    return success;
}
#endif


#if 0  /* TODO */
void ezCli_Run(void)
{
#if (CONFIG_WIN == 1U)
    switch (cli_inst.state)
    {
    case GET_BYTE:
        (void)cli_inst.uart_driver->ezmUart_Receive(&cli_inst.one_byte, 1U);
        break;
    case WAIT:
        break;
    case PROC_CMD:
        (void)ezmCli_CommandReceivedCallback(0,
                                            (char*)cli_inst.cli_buffer,
                                            sizeof(cli_inst.cli_buffer));

        memset(cli_inst.cli_buffer, 0, sizeof(cli_inst.cli_buffer));
        cli_inst.buff_index = 0U;
        cli_inst.uart_driver->ezmUart_Send((uint8_t*)BEGIN, sizeof(BEGIN));
        cli_inst.state = GET_BYTE;
        break;
    default:
        cli_inst.state = GET_BYTE;
        break;
    }
#else
    switch (cli_inst.state)
    {
    case GET_BYTE:
        /* Handle by low level, just move to the next state and wait for data */
        cli_inst.state = WAIT;
        break;
    case WAIT:
        break;
    case PROC_CMD:
        (void)ezmCli_CommandReceivedCallback(0,
                                            (char*)cli_inst.cli_buffer,
                                            sizeof(cli_inst.cli_buffer));

        memset(cli_inst.cli_buffer, 0, sizeof(cli_inst.cli_buffer));
        cli_inst.buff_index = 0U;
        cli_inst.uart_driver->ezmUart_Send((uint8_t*)BEGIN, sizeof(BEGIN));
        cli_inst.state = GET_BYTE;
        break;
    default:
        cli_inst.state = GET_BYTE;
        break;
    }
#endif
}
#endif


CommandHandle ezCli_RegisterCommand(const char *command,
                                    const char *description,
                                    CLI_CALLBACK callback)
{
    CommandHandle handle = CLI_HANDLE_INVALID;
    
    if (command != NULL
        && callback != NULL
        && description != NULL
        && ezCli_IsCommandExist(command, NULL) == false)
    {
        handle = ezCli_GetFreeInstance();
        if (handle < CLI_HANDLE_INVALID)
        {
            astMetaData[handle].callback = callback;
            astMetaData[handle].command = command;
            astMetaData[handle].description = description;

            EZDEBUG("Add new command: [command = %s] [description = %s]",
                        command,
                        description);
        }
        else
        {
            EZERROR("ezCli_RegisterCommand() FAILED");
        }
    }
    return handle;
}


bool ezCli_AddArgument(CommandHandle cmd_handle,
                       const char * long_arg,
                       const char * short_arg,
                       const char * description)
{
    bool bResult = false;

    if (long_arg != NULL
        && short_arg != NULL
        && description != NULL
        && (uint32_t)cmd_handle < CLI_HANDLE_INVALID)
    {
        if (ezCli_IsLongFormArgumentExist(cmd_handle, long_arg) == ARG_INVALID
            && ezCli_IsShortFormArgumentExist(cmd_handle, short_arg) == ARG_INVALID)
        {
            for (uint8_t i = 0; i < CONFIG_NUM_OF_ARGUMENT; i++)
            {
                if (astMetaData[cmd_handle].long_arg_list[i] == NULL)
                {
                    astMetaData[cmd_handle].long_arg_list[i] = (uint32_t*)long_arg;
                    astMetaData[cmd_handle].short_arg_list[i] = (uint32_t*)short_arg;
                    astMetaData[cmd_handle].description_list[i] = (uint32_t*)description;

                    EZDEBUG("Add new argument: [long = %s] [shord = %s] [description = %s]",
                        (char*)astMetaData[cmd_handle].long_arg_list[i],
                        (char*)astMetaData[cmd_handle].short_arg_list[i],
                        (char*)astMetaData[cmd_handle].description_list[i]);
                    bResult = true;
                    break;
                }
            }
        }
    }
    return bResult;
}


bool ezCli_CommandReceivedCallback(uint8_t notify_code,
                                   char * command_buffer,
                                   uint16_t command_buff_size)
{
    bool bResult = false;
    bool process_is_done = false;
    uint8_t u8CommandIndex = CLI_HANDLE_INVALID;
    uint8_t u8ValueIndex = CLI_HANDLE_INVALID;
    char* tail;
    eState = STATE_COMMAND;

    char* head = command_buffer;
    SKIP_NULL_SPACE(head, command_buffer + command_buff_size);
    tail = head;

    if (*head == '\n' || *head == '\r')
    {
        process_is_done = true;
    }

    while (head != command_buffer + command_buff_size &&
        process_is_done == false)
    {
        switch (eState)
        {
        case STATE_COMMAND:
            if (*head == SPACE)
            {
                *head = STR_TERMINATE;
                EZDEBUG("Receive command: [command = %s]", tail);

                if (ezCli_IsCommandExist((char*)tail, &u8CommandIndex))
                {
                    eState = STATE_ARGUMENT;

                    SKIP_NULL_SPACE(head, command_buffer + command_buff_size);
                    tail = head;
                }
                else
                {
                    eState = STATE_ERROR;
                }
            }
            
            if (*head == '\r' || *head == '\n')
            {
                *head = STR_TERMINATE;
                if(ezCli_IsCommandExist((char*)tail, &u8CommandIndex) 
                    && NULL == astMetaData[u8CommandIndex].long_arg_list[0])
                {
                    eState = STATE_EXECUTE;
                }
                else
                {
                    eState = STATE_ERROR;
                }
            }

            break;
        case STATE_ARGUMENT:
            if (*head == SPACE)
            {
                *head = STR_TERMINATE;

                EZDEBUG("Receive argument: [argument = %s]", tail);

                u8ValueIndex = ezCli_IsShortFormArgumentExist(u8CommandIndex, tail);
                if (u8ValueIndex == ARG_INVALID)
                {
                    u8ValueIndex = ezCli_IsLongFormArgumentExist(u8CommandIndex, tail);
                }

                if (ARG_INVALID != u8ValueIndex)
                {
                    eState = STATE_VALUE;
                    EZDEBUG("Value index: [idx = %d]", u8ValueIndex);

                    SKIP_NULL_SPACE(head, command_buffer + command_buff_size);
                    tail = head;
                }
                else
                {
                    eState = STATE_ERROR;
                }
            }
            
            if (*head == '\r' || *head == '\n')
            {
                eState = STATE_ERROR;
            }
            break;

        case STATE_VALUE:
            if (*head == SPACE && *(head + 1) == '-')
            {
                *head = STR_TERMINATE;
                EZDEBUG("Receive value: [value = %s]", tail);

                astMetaData[u8CommandIndex].value_list[u8ValueIndex] = tail;
                eState = STATE_ARGUMENT;

                SKIP_NULL_SPACE(head, command_buffer + command_buff_size);
                tail = head;

            }
            
            if (*head == '\r' || *head == '\n')
            {
                *head = STR_TERMINATE;
                EZDEBUG("Receive value: [value = %s]", tail);

                astMetaData[u8CommandIndex].value_list[u8ValueIndex] = tail;
                eState = STATE_EXECUTE;
            }

            break;

        case STATE_EXECUTE:
            /* Execute the callback */
            if (CLI_NC_OK == astMetaData[u8CommandIndex].callback(astMetaData[u8CommandIndex].command,
                                                                    astMetaData[u8CommandIndex].value_list))
            {
                bResult = true;
            }

            ezCli_ResetValueList(u8CommandIndex);
            process_is_done = true;
            break;

        case STATE_ERROR:
            EZWARNING("Error, wrong command or argument");
            ezCli_PrintCommandHelp(u8CommandIndex);
            process_is_done = true;
            break;
        }
        head++;
    }

    (void)(notify_code);

    return bResult;
}



void ezCli_Printf (char* fmt, ...)
{
#if 0  /* TODO */
    va_list args;
    va_start(args, fmt);
    vsnprintf(buff_printf, BUFF_PRINTF_SIZE, fmt, args);
    va_end(args);

    cli_inst.uart_driver->ezmUart_Send((uint8_t *)buff_printf,
                                        (uint16_t)strlen(buff_printf));
    memset(buff_printf, 0, BUFF_PRINTF_SIZE);
#endif
}



void ezCli_PrintMenu(void)
{
#if 0  /* TODO */
    ezCli_Printf("\nList of commands:\n");
#endif
    for (uint8_t i = 0; i < CONFIG_NUM_OF_CMD; i++)
    {
        ezCli_PrintCommandHelp(i);
    }
#if 0  /* TODO */
    ezCli_Printf("\n\n");
#endif
}


/*****************************************************************************
* Local functions
*****************************************************************************/

/******************************************************************************
* Function : ezCli_ResetMetaData
*//** 
* \b Description:
*
* This function resets the contents of the metadata at a specific index
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
* 
* @param    index:    (IN) Index of the command metadata
* @return   True:       Metadata is reset
*           False:      Cannot find the index of metadata
*
* \b Example Example:
* @code
* ezCli_ResetMetaData(0U);
* @endcode
*
*******************************************************************************/
static bool ezCli_ResetMetaData (uint8_t index)
{
    bool bResult = false;

    if(index < CONFIG_NUM_OF_CMD)
    {
        astMetaData[index].callback = NULL;
        astMetaData[index].command = NULL;
        astMetaData[index].description = NULL;
        for(uint8_t i = 0; i < CONFIG_NUM_OF_ARGUMENT; i++)
        {
            astMetaData[index].description_list[i] = NULL;
            astMetaData[index].long_arg_list[i] = NULL;
            astMetaData[index].short_arg_list[i] = NULL;
            astMetaData[index].value_list[i] =NULL;
        }
        bResult = true; 
    }

    return bResult;
}

/******************************************************************************
* Function : ezCli_GetFreeInstance
*//** 
* \b Description:
*
* This function gets a free instance of the command data
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
* 
* @param    None
* @return   CLI_HANDLE_INVALID if there is no free instance, otherwise the index
*           of the intsance
*
* \b Example Example:
* @code
* uint8_t u8FreeInstance = ezCli_GetFreeInstance()
* @endcode
*
*******************************************************************************/
static uint8_t ezCli_GetFreeInstance (void)
{
    uint8_t u8FreeInstanceIndex = CLI_HANDLE_INVALID;

    for(uint8_t i = 0; i < CONFIG_NUM_OF_CMD; i++)
    {
        if(astMetaData[i].command == NULL)
        {
            u8FreeInstanceIndex = i;
            EZDEBUG("Found free instance: [index = %d]", u8FreeInstanceIndex);
            break;
        }
    }

    return u8FreeInstanceIndex;
}

/******************************************************************************
* Function : ezCli_IsCommandExist
*//** 
* \b Description:
*
* This function checks if the the command exists
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
* 
* @param    *command:(IN) pointer to the command string
* @param    *index:(OUT) index of the checked command
* @return   True:       Command is existing
*           False:      Command is not existing
*
* \b Example Example:
* @code
* uint8_t index = CLI_HANDLE_INVALID;
* if(ezCli_IsCommandExist("TurnOnLED", &index))
* {
*     printf("command exists, index %d", index);
* }
* @endcode
*
*******************************************************************************/
static bool ezCli_IsCommandExist   (const char *command, uint8_t *index)
{
    bool cmd_exist = false;

    for(uint8_t i = 0; i < CONFIG_NUM_OF_CMD; i++)
    {
        if(astMetaData[i].command != NULL && 
            strcmp(astMetaData[i].command, command) == 0)
        {
            cmd_exist = true;

            if(index != NULL)
            {
                *index = i;
            }

            EZDEBUG("Command is existing: [command = %s]", command);
            break;
        }
    }
    return cmd_exist;
}


/******************************************************************************
* Function : ezCli_PrintCommandHelp
*//**
* \b Description:
*
* This function prints the help of a specific command
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
*
* @param    index:(IN) index of the command
* @return   None
*
*******************************************************************************/
static void ezCli_PrintCommandHelp (uint8_t index)
{
    if(index < CONFIG_NUM_OF_CMD && astMetaData[index].command != NULL)
    {
        ezCli_Printf("%s\n", "-----------------------------------------");
        ezCli_Printf("%s", "usage: ");
        ezCli_Printf("%s ", astMetaData[index].command);

        for(uint8_t i = 0; i < CONFIG_NUM_OF_ARGUMENT; i++)
        {
            if(astMetaData[index].short_arg_list[i] != NULL)
            {
                ezCli_Printf("[%s] ", (char *)astMetaData[index].short_arg_list[i]);
                ezCli_Printf("%s\n", "<VALUE>");
            }
        }

        ezCli_Printf("\n");
        ezCli_Printf("%s", "description: ");
        ezCli_Printf("\n\t%s", astMetaData[index].description);
        ezCli_Printf("\n");
        ezCli_Printf("Argument options:\n");
        for(uint8_t i = 0; i < CONFIG_NUM_OF_ARGUMENT; i++)
        {
            if(astMetaData[index].short_arg_list[i] != NULL)
            {
                ezCli_Printf("\t%s, ", (char *)astMetaData[index].short_arg_list[i]);
                ezCli_Printf("%s ", (char *)astMetaData[index].long_arg_list[i]);
                ezCli_Printf("\t%s \n", (char *)astMetaData[index].description_list[i]);
            }
        }
        ezCli_Printf("");
    }
}


/******************************************************************************
* Function : ezCli_IsArgumentShortForm
*//**
* \b Description:
*
* This function checks if the arguments are in short form
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
*
* @param    *short_arg:    (IN) pointer to the argument
*
* @return   True:       Argument is in short form
*           False:      Argument is not in short form 
*
*******************************************************************************/
static bool     ezCli_IsArgumentShortForm(char* short_arg)
{
    bool is_exist = false;
    if(*short_arg == '-')
    {
        if(( *(short_arg + 1) >= 'a' && *(short_arg + 1) <= 'z')
            || ( *(short_arg + 1) >= 'A' && *(short_arg + 1) <= 'Z'))
        {
            is_exist = true;
        }
    } 
    return  is_exist;
}

/******************************************************************************
* Function : ezCli_IsArgumentLongForm
*//**
* \b Description:
*
* This function checks if the arguments are in long form
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
*
* @param    *long_arg:    (IN) pointer to the argument
*
* @return   True:       Argument is in long form
*           False:      Argument is not in long form
*
*******************************************************************************/
static bool ezCli_IsArgumentLongForm(char * long_arg)
{
    return (*long_arg == '-' && ezCli_IsArgumentShortForm(long_arg + 1));
}

/******************************************************************************
* Function : ezCli_IsLongFormArgumentExist
*//**
* \b Description:
*
* This function checks if the long form argument is existing and return its index
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
*
* @param    u8CommandIndex: (IN) Index of the command metadata
* @param    *pu8LongForm:   (IN) Pointer to the buffer storing the argument
*
* @return   Index of the argument
*
*******************************************************************************/
static uint8_t  ezCli_IsLongFormArgumentExist(uint8_t command_index, 
                                                const char* long_arg)
{
    uint8_t index = ARG_INVALID;
    for(uint8_t i = 0; i < CONFIG_NUM_OF_ARGUMENT; i++)
    {
        if(astMetaData[command_index].long_arg_list[i] != NULL &&
                strcmp((char *)astMetaData[command_index].long_arg_list[i], long_arg) == 0)
        {
            index = i;
            EZDEBUG("argument exists: [argument = %s]", long_arg);
            break;
        }
    }
    return index;
}

/******************************************************************************
* Function : ezCli_IsShortFormArgumentExist
*//**
* \b Description:
*
* This function checks if the short form argument is existing and return its index
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
*
* @param    command_index:  (IN) Index of the command metadata
* @param    *short_arg:     (IN) Pointer to the buffer storing the argument
*
* @return   Index of the argument
*
*******************************************************************************/
static uint8_t  ezCli_IsShortFormArgumentExist(uint8_t command_index,
                                               const char* short_arg)
{
    uint8_t index = ARG_INVALID;
    for(uint8_t i = 0; i < CONFIG_NUM_OF_ARGUMENT; i++)
    {
        if(astMetaData[command_index].short_arg_list[i] != NULL &&
            strcmp((char *)astMetaData[command_index].short_arg_list[i], short_arg) == 0)
        {
            index = i;
            EZDEBUG("argument exists: [argument = %s]", short_arg);
            break;
        }
    }
    return index;
}

/******************************************************************************
* Function : ezCli_ResetValueList
*//**
* \b Description:
*
* This function resets the list storing the pointer the the values.
* It is called after the callback function is executed
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
*
* @param    index:    (IN) Index of the command metadata
* @return   None
*
*******************************************************************************/
static void ezCli_ResetValueList(uint8_t index)
{
    if(index < CONFIG_NUM_OF_CMD)
    {
        for(uint8_t i = 0; i < CONFIG_NUM_OF_ARGUMENT; i++)
        {
            astMetaData[index].value_list[i] = NULL;
        }
        EZDEBUG("[index = %d]", index);
    }
}

/******************************************************************************
* Function : UartCallbackHandle
*//**
* \b Description:
*
* Callback from the UART peripheral to handle the data. Here we check:
*   * buffer does not have enough size --> termnate
*   * if we get the \n or \r --> terminate and process the command
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
*
* @param    notify_code: notification code
* @param    *param1
* @param    *param2
* @return   None
*
*******************************************************************************/
#if 0  /* TODO */
static uint8_t UartCallbackHandle(uint8_t notify_code, void* param1)
{
    
    switch ((UART_NOTIFY_CODE)notify_code)
    {
    case UART_TX_COMPLT:
        break;
    case UART_RX_COMPLT:
    {
        uint16_t size = *(uint16_t*)param1;
#if(CONFIG_WIN == 1U)
        cli_inst.cli_buffer[cli_inst.buff_index] = cli_inst.one_byte;

        TRACE("[rx = %c], [size = %x]",
            cli_inst.cli_buffer[cli_inst.buff_index],
            size);

        if (cli_inst.cli_buffer[cli_inst.buff_index] == '\n' ||
            cli_inst.buff_index == sizeof(cli_inst.cli_buffer) ||
            cli_inst.cli_buffer[cli_inst.buff_index] == '\r')
        {
            cli_inst.state = PROC_CMD;
        }
#else
        if (size < (sizeof(cli_inst.cli_buffer) - cli_inst.buff_index - 1))
        {
            (void)cli_inst.uart_driver->ezmUart_Receive(&cli_inst.cli_buffer[cli_inst.buff_index],
                size);
            for (uint16_t i = cli_inst.buff_index;
                i < cli_inst.buff_index + size;
                i++)
            {
                if (cli_inst.cli_buffer[i] == '\n' ||
                    cli_inst.cli_buffer[i] == '\r')
                {
                    TRACE("command receive");
                    cli_inst.state = PROC_CMD;
                    break;
                }
            }
        }
        else
        {
            (void)cli_inst.uart_driver->ezmUart_Receive(&cli_inst.cli_buffer[cli_inst.buff_index],
                (sizeof(cli_inst.cli_buffer) - cli_inst.buff_index - 1));
            /* buffer overflow, trigger the process command to reset the cli*/
            cli_inst.state = PROC_CMD;
        }

        TRACE("[test = %s ], [size = %d]",
            &cli_inst.cli_buffer[cli_inst.buff_index],
            *(uint16_t*)param1);
#endif
        cli_inst.buff_index = cli_inst.buff_index + size;
        break;
    }

    case UART_BUFF_FULL:
        break;
    case UART_UNSUPPORTED:
        break;
    default:
        break;
    }

    return 0U;
}
#endif

/******************************************************************************
* Function : HandleHelpCommand
*//**
* \b Description:
*
* Callback to handle the "help" command, which is a default command to print
* the list of commands
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
*
* @param    *command: pointer to the help command, to cross check if it is correct
* @param    *value_list: values of the args
* @return   return code
*
*******************************************************************************/
static CLI_NOTIFY_CODE  HandleHelpCommand(const char *command, void *value_list)
{
    if (strcmp(command, cmd_help) == 0U)
    {
        ezCli_PrintMenu();
    }
    else
    {
        EZWARNING("Unknown command");
    }

    (void)value_list;
    return CLI_NC_OK;
}

#endif /* EZ_CLI == 1 */
/* End of file*/
