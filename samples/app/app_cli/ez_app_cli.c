/*****************************************************************************
* Filename:         ez_app_cli.c
* Author:           Hai Nguyen
* Original Date:    16.02.2026
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_app_cli.c
 *  @author Hai Nguyen
 *  @date   16.02.2026
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_app_cli.h"

#define DEBUG_LVL   LVL_TRACE   /**< logging level */
#define MOD_NAME    "ez_app_cli"       /**< module name */
#include "ez_logging.h"

#include "ez_cli.h"
#include "ez_osal.h"
#include "ez_osal_threadx.h"
#include "ez_osal_freertos.h"
#include "ez_ring_buffer.h"
#include "ez_app_osal.h"
#include "ez_app_task_worker.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define CMD_BUFF_SIZE    1024
#define STACK_SIZE       512
#define RING_BUFF_SIZE   256

/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* None */

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
#if (EZ_FREERTOS_PORT == 1)
static ezOsal_Stack_t cli_task_stack[STACK_SIZE];
static ezOsal_Stack_t get_input_task_stack[STACK_SIZE];

static ezOsal_TaskResource_t cli_task_resource = {
    .stack = cli_task_stack,
};
static ezOsal_TaskResource_t get_input_task_resource = {
    .stack = get_input_task_stack,
};
#elif (EZ_THREADX_PORT == 1)
static ezOsal_TaskResource_t cli_task_resource;
static ezOsal_TaskResource_t get_input_task_resource;
#else
#endif

static ezCli_t cli;


static char command_buffer[CMD_BUFF_SIZE];
static char ring_buffer[RING_BUFF_SIZE];
static ezCliBuff_t cli_buffer = {
    .buff = command_buffer,
    .size = CMD_BUFF_SIZE,
    .curr_index = 0
};

RingBuffer input_buffer;


/*****************************************************************************
* Function Definitions
*****************************************************************************/
static void CliTaskFunction(void *argument);
static void GetInputTaskFunction(void *argument);
static char GetCharCallback(void);
static void SendCharsCallback(char *tx_buff, uint16_t size);
static CLI_NOTIFY_CODE HandleHelloCallback(
    char * tx_rx_buff,
    const void **arg_list,
    const void **value_list);

static CLI_NOTIFY_CODE HandlEchoCmdCallback(
    char * tx_rx_buff,
    const void **arg_list,
    const void **value_list);

static CLI_NOTIFY_CODE HandleStartOsalDemo(
    char * tx_rx_buff,
    const void **arg_list,
    const void **value_list);

static CLI_NOTIFY_CODE HandleStartTaskWorkerDemo(
    char * tx_rx_buff,
    const void **arg_list,
    const void **value_list);

static ezCliCommand_t commands[] = {
    {
        .command = "hello",
        .cmd_description = "say hello world",
        .callback = HandleHelloCallback,
        .long_arg_list = {NULL, NULL},
        .short_arg_list = {NULL, NULL},
        .arg_description = {NULL, NULL},
    },
    {
        .command = "echo",
        .cmd_description = "echo string value",
        .callback = HandlEchoCmdCallback,
        .long_arg_list = {"--string", NULL},
        .short_arg_list = {"-s", NULL},
        .arg_description = {"input string", NULL},
    },
    {
        .command = "start_osal_demo",
        .cmd_description = "start osal component demo",
        .callback = HandleStartOsalDemo,
        .long_arg_list = {NULL, NULL},
        .short_arg_list = {NULL, NULL},
        .arg_description = {NULL, NULL},
    },
    {
        .command = "start_task_worker_demo",
        .cmd_description = "start task worker demo",
        .callback = HandleStartTaskWorkerDemo,
        .long_arg_list = {NULL, NULL},
        .short_arg_list = {NULL, NULL},
        .arg_description = {NULL, NULL},
    }
};

static ezCliInterface_t cli_interface = {
    .GetCharCallback = GetCharCallback,
    .SendCharsCallback = SendCharsCallback
};

static ezCliCmdList_t cmd_list = {
    .commands = commands,
    .num_of_cmd = sizeof(commands) / sizeof(ezCliCommand_t)
};

EZ_OSAL_DEFINE_TASK_HANDLE(cli_task, STACK_SIZE, 1, CliTaskFunction, NULL, &cli_task_resource);
EZ_OSAL_DEFINE_TASK_HANDLE(get_input_task, STACK_SIZE, 2, GetInputTaskFunction, NULL, &get_input_task_resource);

/*********************************************************************
* Public functions
*****************************************************************************/
void AppCli_Init(void)
{
    ezSTATUS init_result = ezCli_Init(&cli, &cli_buffer, &cli_interface, &cmd_list);
    if(init_result != ezSUCCESS)
    {
        EZERROR("Failed to initialize CLI, error code: %d", init_result);
        return;
    }

    if(ezRingBuffer_Init(&input_buffer, (uint8_t*)ring_buffer, RING_BUFF_SIZE) != true)
    {
        EZERROR("Failed to initialize input buffer");
        return;
    }

    (void) ezOsal_TaskCreate(&cli_task);
    (void) ezOsal_TaskCreate(&get_input_task);
}

/*****************************************************************************
* Local functions
*****************************************************************************/
static char GetCharCallback(void)
{
    if(ezRingBuffer_IsEmpty(&input_buffer) == false)
    {
        uint8_t c;
        ezRingBuffer_Pop(&input_buffer, &c, sizeof(c));
        return (char)c;
    }
    return '\0';
}

static void SendCharsCallback(char *tx_buff, uint16_t size)
{
    printf("%s", tx_buff);
}

static void CliTaskFunction(void *argument)
{
    while (1)
    {
        ezCli_Run(&cli);
        ezOsal_TaskDelay(1);
    }
}

static void GetInputTaskFunction(void *argument)
{
    while(1)
    {
        char c = (char)getchar();
        if(c != EOF && c != '\0')
        {
            ezRingBuffer_Push(&input_buffer, (uint8_t*)&c, sizeof(c));
        }
        ezOsal_TaskDelay(1);
    }
}

static CLI_NOTIFY_CODE HandleHelloCallback(
    char * tx_rx_buff,
    const void **arg_list,
    const void **value_list)
{
    if(tx_rx_buff == NULL || arg_list == NULL || value_list == NULL)
    {
        return CLI_NC_ERR;
    }

    if(arg_list[0] != NULL || value_list[0] != NULL)
    {
        return CLI_NC_BAD_ARG;
    }

    sprintf(tx_rx_buff, "%s", "Hello World!\r\n");
    return CLI_NC_OK;
}

static CLI_NOTIFY_CODE HandlEchoCmdCallback(
    char * tx_rx_buff,
    const void **arg_list,
    const void **value_list)
{
    if(tx_rx_buff == NULL || arg_list == NULL || value_list == NULL)
    {
        return CLI_NC_ERR;
    }

    if(arg_list[0] == NULL || value_list[0] == NULL)
    {
        EZERROR("Argument is missing");
        return CLI_NC_ERR;
    }

    if(strcasecmp(arg_list[0], "--string") != 0 && strcasecmp(arg_list[0], "-s") != 0)
    {
        EZERROR("Unsupported argument: [arg = %s]", (char*)arg_list[0]);
        return CLI_NC_ERR;
    }

    sprintf(tx_rx_buff, "%s\r\n", (char*)value_list[0]);
    return CLI_NC_OK;
}

static CLI_NOTIFY_CODE HandleStartOsalDemo(
    char * tx_rx_buff,
    const void **arg_list,
    const void **value_list)
{
    if(tx_rx_buff == NULL || arg_list == NULL || value_list == NULL)
    {
        return CLI_NC_ERR;
    }

    if(arg_list[0] != NULL || value_list[0] != NULL)
    {
        return CLI_NC_BAD_ARG;
    }

    sprintf(tx_rx_buff, "%s", "Start OSAL demo application. This application runs only once!");
    ezApp_StartOsalApp();
    return CLI_NC_OK;
}


static CLI_NOTIFY_CODE HandleStartTaskWorkerDemo(
    char * tx_rx_buff,
    const void **arg_list,
    const void **value_list)
{
    if(tx_rx_buff == NULL || arg_list == NULL || value_list == NULL)
    {
        return CLI_NC_ERR;
    }

    if(arg_list[0] != NULL || value_list[0] != NULL)
    {
        return CLI_NC_BAD_ARG;
    }

    sprintf(tx_rx_buff, "%s", "Start task worker demo application.");
    ezApp_TaskWorkerInit(NULL);
    return CLI_NC_OK;
}

/* End of file*/
