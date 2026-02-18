/*****************************************************************************
* Filename:         unittest_ez_event_notifier.c
* Author:           Hai Nguyen
* Original Date:    06.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   unittest_ez_event_notifier.c
 *  @author Hai Nguyen
 *  @date   06.03.2024
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 * 
 */

/******************************************************************************
* Includes
*******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "unity.h"
#include "unity_fixture.h"
#include "ez_cli.h"

TEST_GROUP(ez_cli);

/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
#define BUFF_SIZE    256

/******************************************************************************
* Module Typedefs
*******************************************************************************/
static CLI_NOTIFY_CODE ExecuteOneArgumentCallback(char * tx_rx_buff, const void **arg_list, const void **value_list);
static CLI_NOTIFY_CODE ExecuteTwoArgumentsCallback(char * tx_rx_buff, const void **arg_list, const void **value_list);
static CLI_NOTIFY_CODE ExecutesStringArgumentsCallback(char * tx_rx_buff, const void **arg_list, const void **value_list);
static CLI_NOTIFY_CODE ExecuteTwoArgumentsButFirstOnlyCallback(char * tx_rx_buff, const void **arg_list, const void **value_list);
static CLI_NOTIFY_CODE ExecuteTwoArgumentsButSecondOnlyCallback(char * tx_rx_buff, const void **arg_list, const void **value_list);


static ezCliCommand_t commands[] = {
    {
        .command = "hello",
        .cmd_description = "show hello world",
        .callback = NULL,
        .long_arg_list = {NULL, NULL},
        .short_arg_list = {NULL, NULL},
        .arg_description = {NULL, NULL},
    },
    {
        .command = "one_arg_cmd",
        .cmd_description = "show one argument command",
        .callback = ExecuteOneArgumentCallback,
        .long_arg_list = {"--one", NULL},
        .short_arg_list = {"-o", NULL},
        .arg_description = {"firt argument", NULL},
    },
    {
        .command = "two_arg_cmd",
        .cmd_description = "show two argument command",
        .callback = ExecuteTwoArgumentsCallback,
        .long_arg_list = {"--one", "--two"},
        .short_arg_list = {"-o", "-t"},
        .arg_description = {"first argument", "second argument"},
    },
    {
        .command = "echo",
        .cmd_description = "echo string value",
        .callback = ExecutesStringArgumentsCallback,
        .long_arg_list = {"--string", NULL},
        .short_arg_list = {"-s", NULL},
        .arg_description = {"string argument", NULL},
    }
};

static char command_buffer[BUFF_SIZE];
static ezCliBuff_t cli_buffer = {
    .buff = command_buffer,
    .size = BUFF_SIZE,
    .curr_index = 0
};

static ezCliInterface_t cli_interface = {
    .GetCharCallback = NULL,
    .SendCharsCallback = NULL
};

static ezCliCmdList_t cmd_list = {
    .commands = commands,
    .num_of_cmd = sizeof(commands) / sizeof(ezCliCommand_t)
};

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static ezCli_t cli;
static const char one_arg_short[] = "one_arg_cmd -o value1\n";
static const char one_arg_invalid_arg1[] = "one_arg_cmd value\n";
static const char one_arg_invalid_arg2[] = "one_arg_cmd $value\n";
static const char one_arg_short_wrong_arg[] = "one_arg_cmd -n value1\n";
static const char one_arg_short_spaces[] = "   one_arg_cmd    -o   value1  \n";
static const char one_arg_long[] = "one_arg_cmd --one value1\n";
static const char two_arg[] = "two_arg_cmd -o value1 -t value2\n";
static const char two_arg_but_first_only[] = "two_arg_cmd -o value1\n";
static const char two_arg_but_second_only[] = "two_arg_cmd -t value2\n";
static const char string[] = "echo -s \"Hello world\"\n";
static const char string_err1[] = "echo -s \"Hello world\n";
static const char string_err2[] = "echo -s Hello world\"\n";
static const char input_string[] = "Hello world";
static char rx_buff[BUFF_SIZE];
static uint16_t buff_index = 0;
static bool is_one_arg_cmd_executed = false;
static bool is_two_arg_cmd_executed = false;
static bool is_string_executed = false;

/******************************************************************************
* Function Definitions
*******************************************************************************/
static void RunAllTests(void);
static char GetCharCallback(void);
static void SendCharsCallback(char *tx_buff, uint16_t size);

/******************************************************************************
* External functions
*******************************************************************************/
int main(int argc, const char *argv[])
{
    return UnityMain(argc, argv, RunAllTests);
}


TEST_SETUP(ez_cli)
{
    cli_interface.GetCharCallback = GetCharCallback;
    cli_interface.SendCharsCallback = SendCharsCallback;
    ezSTATUS init_result = ezCli_Init(&cli, &cli_buffer, &cli_interface, &cmd_list);
    TEST_ASSERT_EQUAL(ezSUCCESS, init_result);
    buff_index = 0;
    is_one_arg_cmd_executed = false;
    is_two_arg_cmd_executed = false;
    is_string_executed = false;
}


TEST_TEAR_DOWN(ez_cli)
{
}


TEST_GROUP_RUNNER(ez_cli)
{
    RUN_TEST_CASE(ez_cli, TestInitCli);
    RUN_TEST_CASE(ez_cli, TestParseOneArgumentShortForm);
    RUN_TEST_CASE(ez_cli, TestParseOneArgumentLongForm);
    RUN_TEST_CASE(ez_cli, TestParseOneArgumentShortFormWithSpaces);
    RUN_TEST_CASE(ez_cli, TestParseTwoArguments);
    RUN_TEST_CASE(ez_cli, TestParseTwoArgumentsButFirstOnly);
    RUN_TEST_CASE(ez_cli, TestParseTwoArgumentsButSecondOnly);
    RUN_TEST_CASE(ez_cli, TestParseOneArgumentShortFormWrongArg);
    RUN_TEST_CASE(ez_cli, TestParseStringOK);
    RUN_TEST_CASE(ez_cli, TestParseStringNOK1);
    RUN_TEST_CASE(ez_cli, TestParseStringNOK2);
    RUN_TEST_CASE(ez_cli, TestInvalidArg1);
    RUN_TEST_CASE(ez_cli, TestInvalidArg2);
}


TEST(ez_cli, TestInitCli)
{
    ezSTATUS init_result = ezCli_Init(NULL, &cli_buffer, &cli_interface, &cmd_list);
    TEST_ASSERT_EQUAL(ezSTATUS_ARG_INVALID, init_result);

    init_result = ezCli_Init(&cli, NULL, &cli_interface, &cmd_list);
    TEST_ASSERT_EQUAL(ezSTATUS_ARG_INVALID, init_result);

    init_result = ezCli_Init(&cli, &cli_buffer, NULL, &cmd_list);
    TEST_ASSERT_EQUAL(ezSTATUS_ARG_INVALID, init_result);

    init_result = ezCli_Init(&cli, &cli_buffer, &cli_interface, NULL);
    TEST_ASSERT_EQUAL(ezSTATUS_ARG_INVALID, init_result);

    ezCliBuff_t invalid_buffer = {
        .buff = NULL,
        .size = 0,
        .curr_index = 0
    };

    init_result = ezCli_Init(&cli, &invalid_buffer, &cli_interface, &cmd_list);
    TEST_ASSERT_EQUAL(ezSTATUS_ARG_INVALID, init_result);
    
    ezCliInterface_t invalid_interface = {
        .GetCharCallback = NULL,
        .SendCharsCallback = NULL
    };

    init_result = ezCli_Init(&cli, &cli_buffer, &invalid_interface, &cmd_list);
    TEST_ASSERT_EQUAL(ezSTATUS_ARG_INVALID, init_result);

    ezCliCmdList_t invalid_cmd_list = {
        .commands = NULL,
        .num_of_cmd = 0
    };

    init_result = ezCli_Init(&cli, &cli_buffer, &cli_interface, &invalid_cmd_list);
    TEST_ASSERT_EQUAL(ezSTATUS_ARG_INVALID, init_result);

    init_result = ezCli_Init(&cli, &cli_buffer, &cli_interface, &cmd_list);
    TEST_ASSERT_EQUAL(ezSUCCESS, init_result);
}


TEST(ez_cli, TestParseOneArgumentShortForm)
{
    strcpy(rx_buff, one_arg_short);
    cli_buffer.curr_index = 0;
    for(uint32_t i = 0; i <= strlen(rx_buff); i++) /* Need one extra to execute the command */
    {
        ezCli_Run(&cli);
    }

    TEST_ASSERT_TRUE(is_one_arg_cmd_executed);
}


TEST(ez_cli, TestParseOneArgumentShortFormWrongArg)
{
    strcpy(rx_buff, one_arg_short_wrong_arg);
    cli_buffer.curr_index = 0;
    for(uint32_t i = 0; i <= strlen(rx_buff); i++) /* Need one extra to execute the command */
    {
        ezCli_Run(&cli);
    }

    TEST_ASSERT_FALSE(is_one_arg_cmd_executed);
}

TEST(ez_cli, TestParseOneArgumentLongForm)
{
    strcpy(rx_buff, one_arg_long);
    cli_buffer.curr_index = 0;
    for(uint32_t i = 0; i <= strlen(rx_buff); i++) /* Need one extra to execute the command */
    {
        ezCli_Run(&cli);
    }

    TEST_ASSERT_TRUE(is_one_arg_cmd_executed);
}


TEST(ez_cli, TestParseOneArgumentShortFormWithSpaces)
{
    strcpy(rx_buff, one_arg_short_spaces);
    cli_buffer.curr_index = 0;
    for(uint32_t i = 0; i <= strlen(rx_buff); i++) /* Need one extra to execute the command */
    {
        ezCli_Run(&cli);
    }

    TEST_ASSERT_TRUE(is_one_arg_cmd_executed);
}


TEST(ez_cli, TestParseTwoArguments)
{
    strcpy(rx_buff, two_arg);
    cli_buffer.curr_index = 0;
    for(uint32_t i = 0; i <= strlen(rx_buff); i++) /* Need one extra to execute the command */
    {
        ezCli_Run(&cli);
    }

    TEST_ASSERT_TRUE(is_two_arg_cmd_executed);
}


TEST(ez_cli, TestParseTwoArgumentsButFirstOnly)
{
    commands[2].callback = ExecuteTwoArgumentsButFirstOnlyCallback;
    strcpy(rx_buff, two_arg_but_first_only);
    cli_buffer.curr_index = 0;
    for(uint32_t i = 0; i <= strlen(rx_buff); i++) /* Need one extra to execute the command */
    {
        ezCli_Run(&cli);
    }

    TEST_ASSERT_TRUE(is_two_arg_cmd_executed);
}


TEST(ez_cli, TestParseTwoArgumentsButSecondOnly)
{
    commands[2].callback = ExecuteTwoArgumentsButSecondOnlyCallback;
    strcpy(rx_buff, two_arg_but_second_only);
    cli_buffer.curr_index = 0;
    for(uint32_t i = 0; i <= strlen(rx_buff); i++) /* Need one extra to execute the command */
    {
        ezCli_Run(&cli);
    }

    TEST_ASSERT_TRUE(is_two_arg_cmd_executed);
}


TEST(ez_cli, TestParseStringOK)
{
    strcpy(rx_buff, string);
    cli_buffer.curr_index = 0;
    for(uint32_t i = 0; i <= strlen(rx_buff); i++) /* Need one extra to execute the command */
    {
        ezCli_Run(&cli);
    }

    TEST_ASSERT_TRUE(is_string_executed);
}

TEST(ez_cli, TestParseStringNOK1)
{
    strcpy(rx_buff, string_err1);
    cli_buffer.curr_index = 0;
    for(uint32_t i = 0; i <= strlen(rx_buff); i++) /* Need one extra to execute the command */
    {
        ezCli_Run(&cli);
    }

    TEST_ASSERT_FALSE(is_string_executed);
}


TEST(ez_cli, TestParseStringNOK2)
{
    strcpy(rx_buff, string_err2);
    cli_buffer.curr_index = 0;
    for(uint32_t i = 0; i <= strlen(rx_buff); i++) /* Need one extra to execute the command */
    {
        ezCli_Run(&cli);
    }

    TEST_ASSERT_FALSE(is_string_executed);
}


TEST(ez_cli, TestInvalidArg1)
{
    strcpy(rx_buff, one_arg_invalid_arg1);
    cli_buffer.curr_index = 0;
    for(uint32_t i = 0; i <= strlen(rx_buff); i++) /* Need one extra to execute the command */
    {
        ezCli_Run(&cli);
    }

    TEST_ASSERT_FALSE(is_string_executed);
}


TEST(ez_cli, TestInvalidArg2)
{
    strcpy(rx_buff, one_arg_invalid_arg2);
    cli_buffer.curr_index = 0;
    for(uint32_t i = 0; i <= strlen(rx_buff); i++) /* Need one extra to execute the command */
    {
        ezCli_Run(&cli);
    }

    TEST_ASSERT_FALSE(is_string_executed);
}

/******************************************************************************
* Internal functions
*******************************************************************************/
static void RunAllTests(void)
{
    RUN_TEST_GROUP(ez_cli);
}


static char GetCharCallback(void)
{
    if(buff_index < strlen(rx_buff))
    {
        return rx_buff[buff_index++];
    }
    else
    {
        return '\0';
    }
}


static void SendCharsCallback(char *tx_buff, uint16_t size)
{
    printf("\n%.*s\n", size, tx_buff);
}


static CLI_NOTIFY_CODE ExecuteOneArgumentCallback(char * tx_rx_buff, const void **arg_list, const void **value_list)
{
    is_one_arg_cmd_executed = true;
    TEST_ASSERT_EQUAL_STRING("value1", value_list[0]);
    sprintf(tx_rx_buff, "test completed\n");
    return CLI_NC_OK;
}

static CLI_NOTIFY_CODE ExecuteTwoArgumentsCallback(char * tx_rx_buff, const void **arg_list, const void **value_list)
{
    is_two_arg_cmd_executed = true;
    TEST_ASSERT_EQUAL_STRING("value1", value_list[0]);
    TEST_ASSERT_EQUAL_STRING("value2", value_list[1]);
    sprintf(tx_rx_buff, "test completed\n");
    return CLI_NC_OK;
}

static CLI_NOTIFY_CODE ExecuteTwoArgumentsButFirstOnlyCallback(char * tx_rx_buff, const void **arg_list, const void **value_list)
{
    is_two_arg_cmd_executed = true;
    TEST_ASSERT_EQUAL_STRING("value1", value_list[0]);
    TEST_ASSERT_NULL(value_list[1]);
    sprintf(tx_rx_buff, "test completed\n");
    return CLI_NC_OK;
}


static CLI_NOTIFY_CODE ExecuteTwoArgumentsButSecondOnlyCallback(char * tx_rx_buff, const void **arg_list, const void **value_list)
{
    TEST_ASSERT_EQUAL_STRING("value2", value_list[0]);
    is_two_arg_cmd_executed = true;
    TEST_ASSERT_NULL(value_list[1]);
    sprintf(tx_rx_buff, "test completed\n");
    return CLI_NC_OK;
}

static CLI_NOTIFY_CODE ExecutesStringArgumentsCallback(char * tx_rx_buff, const void **arg_list, const void **value_list)
{
    TEST_ASSERT_EQUAL_STRING(input_string, value_list[0]);
    is_string_executed = true;
    TEST_ASSERT_NULL(value_list[1]);
    sprintf(tx_rx_buff, "%s\n", value_list[0]);
    return CLI_NC_OK;
}


/* End of file */
