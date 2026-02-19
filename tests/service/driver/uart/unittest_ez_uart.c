/*****************************************************************************
* Filename:         unittest_ez_uart.c
* Author:           Hai Nguyen
* Original Date:    15.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   unittest_ez_uart.c
 *  @author Hai Nguyen
 *  @date   15.03.2024
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
#include "ez_uart.h"
#include "ez_driver_def.h"

TEST_GROUP(ez_uart);

/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
#define DRV_NAME        "MOCK DRIVER"
#define PORT_NAME       "COM1"

/******************************************************************************
* Module Typedefs
*******************************************************************************/
/* None */

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static struct ezUartDriver mock_hw;
static ezUartDrvInstance_t first_inst;
static ezUartDrvInstance_t second_inst;
static EZ_UART_EVENT uart_event = UART_NUM_EVENTS;

/******************************************************************************
* Function Definitions
*******************************************************************************/
static void RunAllTests(void);
static EZ_DRV_STATUS ezMockUart_Initialize(uint8_t index);
static EZ_DRV_STATUS ezMockUart_Deinitialize(uint8_t index);
static EZ_DRV_STATUS ezMockUart_AsyncTransmit(uint8_t index, const uint8_t *tx_buff, uint16_t buff_size);
static EZ_DRV_STATUS ezMockUart_AsyncReceive(uint8_t index, uint8_t *rx_buff, uint16_t buff_size);
static EZ_DRV_STATUS ezMockUart_SyncTransmit(uint8_t index, const uint8_t *tx_buff, uint16_t buff_size, uint32_t timeout_millis);
static EZ_DRV_STATUS ezMockUart_SyncReceive(uint8_t index, uint8_t *rx_buff, uint16_t buff_size, uint32_t timeout_millis);
static EZ_DRV_STATUS ezMockUart_UpdateConfig(uint8_t index);
static void ezMockUart_SetImplementedInterface(struct ezHwUartInterface *interface);
static void ezMockUart_Callback(uint8_t event_code, void *param1, void *param2);


/******************************************************************************
* External functions
*******************************************************************************/
int main(int argc, const char *argv[])
{
    return UnityMain(argc, argv, RunAllTests);
}


TEST_SETUP(ez_uart)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;
    uart_event = UART_NUM_EVENTS;
    mock_hw.common.version[0] = 1;
    mock_hw.common.version[1] = 2;
    mock_hw.common.version[2] = 3;
    mock_hw.common.name = DRV_NAME;
    mock_hw.config.baudrate = 115200;
    mock_hw.config.byte_size = 8;
    mock_hw.config.parity = PARITY_ODD;
    mock_hw.config.port_name = PORT_NAME;
    mock_hw.config.stop_bit = ONE_BIT;

    status = ezUart_SystemRegisterHwDriver(&mock_hw);
    TEST_ASSERT_EQUAL(STATUS_OK, status);
}


TEST_TEAR_DOWN(ez_uart)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;

    status = ezUart_UnregisterInstance(&first_inst);
    TEST_ASSERT_EQUAL(STATUS_OK, status);

    status = ezUart_UnregisterInstance(&second_inst);
    TEST_ASSERT_EQUAL(STATUS_OK, status);

    status = ezUart_SystemUnregisterHwDriver(&mock_hw);
    TEST_ASSERT_EQUAL(STATUS_OK, status);

    memset(&mock_hw, 0, sizeof(mock_hw));
}


TEST_GROUP_RUNNER(ez_uart)
{
    RUN_TEST_CASE(ez_uart, ezUart_SystemRegisterHwDriver);
    RUN_TEST_CASE(ez_uart, ezUart_SystemUnregisterHwDriver);
    RUN_TEST_CASE(ez_uart, ezUart_RegisterInstance);
    RUN_TEST_CASE(ez_uart, ezUart_UnregisterInstance);
    RUN_TEST_CASE(ez_uart, ezUart_Initialize);
    RUN_TEST_CASE(ez_uart, ezUart_Deinitialize);
    RUN_TEST_CASE(ez_uart, ezUart_AsyncTransmit);
    RUN_TEST_CASE(ez_uart, ezUart_AsyncReceive);
    RUN_TEST_CASE(ez_uart, ezUart_SyncTransmit);
    RUN_TEST_CASE(ez_uart, ezUart_SyncReceive);
    RUN_TEST_CASE(ez_uart, ezUart_GetConfig);
    RUN_TEST_CASE(ez_uart, ezUart_UpdateConfig);
}


TEST(ez_uart, ezUart_SystemRegisterHwDriver)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;

    TEST_MESSAGE("Test NULL argument");
    status = ezUart_SystemRegisterHwDriver(NULL);
    TEST_ASSERT_EQUAL(STATUS_ERR_ARG, status);
}


TEST(ez_uart, ezUart_SystemUnregisterHwDriver)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;

    TEST_MESSAGE("Test NULL argument");
    status = ezUart_SystemUnregisterHwDriver(NULL);
    TEST_ASSERT_EQUAL(STATUS_ERR_ARG, status);
}


TEST(ez_uart, ezUart_RegisterInstance)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;

    TEST_MESSAGE("Test null pointer");
    status = ezUart_RegisterInstance(NULL, NULL, ezMockUart_Callback);
    TEST_ASSERT_EQUAL(STATUS_ERR_ARG, status);

    TEST_MESSAGE("Test driver does not exist");
    status = ezUart_RegisterInstance(&first_inst, "Random driver name", ezMockUart_Callback);
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    TEST_MESSAGE("Test happy path");
    status = ezUart_RegisterInstance(&first_inst, DRV_NAME, ezMockUart_Callback);
    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_NOT_EQUAL(NULL, first_inst.driver);
}


TEST(ez_uart, ezUart_UnregisterInstance)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;

    TEST_MESSAGE("Test null pointer");
    status = ezUart_UnregisterInstance(NULL);
    TEST_ASSERT_EQUAL(STATUS_ERR_ARG, status);

    TEST_MESSAGE("Test happy path");
    status = ezUart_UnregisterInstance(&first_inst);
    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_EQUAL(NULL, first_inst.driver);
}


TEST(ez_uart, ezUart_Initialize)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;

    TEST_MESSAGE("Test null pointer");
    status = ezUart_Initialize(NULL);
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    TEST_MESSAGE("Test unregistered instance");
    status = ezUart_Initialize(&second_inst);
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    /* Now we register the instance */
    status = ezUart_RegisterInstance(&first_inst, DRV_NAME, ezMockUart_Callback);
    status = ezUart_RegisterInstance(&second_inst, DRV_NAME, ezMockUart_Callback);

    TEST_MESSAGE("Test unimplmented interface ");
    status = ezUart_Initialize(&first_inst);
    TEST_ASSERT_EQUAL(STATUS_ERR_INF_NOT_EXIST, status);

    TEST_MESSAGE("Test interface is locked");
    /* manually set lock */
    mock_hw.common.curr_inst = &first_inst;
    status = ezUart_Initialize(&second_inst);
    TEST_ASSERT_EQUAL(STATUS_BUSY, status);
    /* release the lock */
    mock_hw.common.curr_inst = NULL;

    TEST_MESSAGE("Test happy path");
    ezMockUart_SetImplementedInterface(&mock_hw.interface);
    status = ezUart_Initialize(&first_inst);
    TEST_ASSERT_EQUAL(STATUS_OK, status);
}


TEST(ez_uart, ezUart_Deinitialize)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;

    TEST_MESSAGE("Test null pointer");
    status = ezUart_Deinitialize(NULL);
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    TEST_MESSAGE("Test unregistered instance");
    status = ezUart_Deinitialize(&second_inst);
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    /* Now we register the instance */
    status = ezUart_RegisterInstance(&first_inst, DRV_NAME, ezMockUart_Callback);
    status = ezUart_RegisterInstance(&second_inst, DRV_NAME, ezMockUart_Callback);

    TEST_MESSAGE("Test unimplmented interface ");
    status = ezUart_Deinitialize(&first_inst);
    TEST_ASSERT_EQUAL(STATUS_ERR_INF_NOT_EXIST, status);

    TEST_MESSAGE("Test interface is locked");
    /* manually set lock */
    mock_hw.common.curr_inst = &first_inst;
    status = ezUart_Deinitialize(&second_inst);
    TEST_ASSERT_EQUAL(STATUS_BUSY, status);
    /* release the lock */
    mock_hw.common.curr_inst = NULL;

    TEST_MESSAGE("Test happy path");
    ezMockUart_SetImplementedInterface(&mock_hw.interface);
    status = ezUart_Deinitialize(&first_inst);
    TEST_ASSERT_EQUAL(STATUS_OK, status);
}


TEST(ez_uart, ezUart_AsyncTransmit)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;
    uint8_t tx_buff[] = {0xde, 0xad, 0xca, 0xfe};

    TEST_MESSAGE("Test null pointer");
    status = ezUart_AsyncTransmit(NULL, tx_buff, sizeof(tx_buff));
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    TEST_MESSAGE("Test unregistered instance");
    status = ezUart_AsyncTransmit(&second_inst, tx_buff, sizeof(tx_buff));
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    /* Now we register the instance */
    status = ezUart_RegisterInstance(&first_inst, DRV_NAME, ezMockUart_Callback);
    status = ezUart_RegisterInstance(&second_inst, DRV_NAME, ezMockUart_Callback);

    TEST_MESSAGE("Test unimplmented interface ");
    status = ezUart_AsyncTransmit(&first_inst, tx_buff, sizeof(tx_buff));
    TEST_ASSERT_EQUAL(STATUS_ERR_INF_NOT_EXIST, status);

    TEST_MESSAGE("Test interface is locked");
    /* manually set lock */
    mock_hw.common.curr_inst = &first_inst;
    status = ezUart_AsyncTransmit(&second_inst, tx_buff, sizeof(tx_buff));
    TEST_ASSERT_EQUAL(STATUS_BUSY, status);
    /* release the lock */
    mock_hw.common.curr_inst = NULL;

    TEST_MESSAGE("Test happy path");
    ezMockUart_SetImplementedInterface(&mock_hw.interface);
    status = ezUart_AsyncTransmit(&first_inst, tx_buff, sizeof(tx_buff));
    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_EQUAL(NULL, mock_hw.common.curr_inst);
    TEST_ASSERT_EQUAL(UART_EVENT_TX_CMPLT, uart_event);
}


TEST(ez_uart, ezUart_AsyncReceive)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;
    uint8_t rx_buff[4];

    TEST_MESSAGE("Test null pointer");
    status = ezUart_AsyncReceive(NULL, rx_buff, sizeof(rx_buff));
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    TEST_MESSAGE("Test unregistered instance");
    status = ezUart_AsyncReceive(&second_inst, rx_buff, sizeof(rx_buff));
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    /* Now we register the instance */
    status = ezUart_RegisterInstance(&first_inst, DRV_NAME, ezMockUart_Callback);
    status = ezUart_RegisterInstance(&second_inst, DRV_NAME, ezMockUart_Callback);

    TEST_MESSAGE("Test unimplmented interface ");
    status = ezUart_AsyncReceive(&first_inst, rx_buff, sizeof(rx_buff));
    TEST_ASSERT_EQUAL(STATUS_ERR_INF_NOT_EXIST, status);

    TEST_MESSAGE("Test interface is locked");
    /* manually set lock */
    mock_hw.common.curr_inst = &first_inst;
    status = ezUart_AsyncReceive(&second_inst, rx_buff, sizeof(rx_buff));
    TEST_ASSERT_EQUAL(STATUS_BUSY, status);
    /* release the lock */
    mock_hw.common.curr_inst = NULL;

    TEST_MESSAGE("Test happy path");
    ezMockUart_SetImplementedInterface(&mock_hw.interface);
    status = ezUart_AsyncReceive(&first_inst, rx_buff, sizeof(rx_buff));
    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_EQUAL(NULL, mock_hw.common.curr_inst);
    TEST_ASSERT_EQUAL(UART_EVENT_RX_CMPLT, uart_event);
}


TEST(ez_uart, ezUart_SyncTransmit)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;
    uint8_t tx_buff[] = {0xde, 0xad, 0xca, 0xfe};

    TEST_MESSAGE("Test null pointer");
    status = ezUart_SyncTransmit(NULL, tx_buff, sizeof(tx_buff), 1000);
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    TEST_MESSAGE("Test unregistered instance");
    status = ezUart_SyncTransmit(&second_inst, tx_buff, sizeof(tx_buff), 1000);
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    /* Now we register the instance */
    status = ezUart_RegisterInstance(&first_inst, DRV_NAME, ezMockUart_Callback);
    status = ezUart_RegisterInstance(&second_inst, DRV_NAME, ezMockUart_Callback);

    TEST_MESSAGE("Test unimplmented interface ");
    status = ezUart_SyncTransmit(&first_inst, tx_buff, sizeof(tx_buff), 1000);
    TEST_ASSERT_EQUAL(STATUS_ERR_INF_NOT_EXIST, status);

    TEST_MESSAGE("Test interface is locked");
    /* manually set lock */
    mock_hw.common.curr_inst = &first_inst;
    status = ezUart_SyncTransmit(&second_inst, tx_buff, sizeof(tx_buff), 1000);
    TEST_ASSERT_EQUAL(STATUS_BUSY, status);
    /* release the lock */
    mock_hw.common.curr_inst = NULL;

    TEST_MESSAGE("Test happy path");
    ezMockUart_SetImplementedInterface(&mock_hw.interface);
    status = ezUart_SyncTransmit(&first_inst, tx_buff, sizeof(tx_buff), 1000);
    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_EQUAL(NULL, mock_hw.common.curr_inst);
    TEST_ASSERT_EQUAL(UART_NUM_EVENTS, uart_event);
}


TEST(ez_uart, ezUart_SyncReceive)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;
    uint8_t rx_buff[4];

    TEST_MESSAGE("Test null pointer");
    status = ezUart_SyncReceive(NULL, rx_buff, sizeof(rx_buff), 1000);
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    TEST_MESSAGE("Test unregistered instance");
    status = ezUart_SyncReceive(&second_inst, rx_buff, sizeof(rx_buff), 1000);
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    /* Now we register the instance */
    status = ezUart_RegisterInstance(&first_inst, DRV_NAME, ezMockUart_Callback);
    status = ezUart_RegisterInstance(&second_inst, DRV_NAME, ezMockUart_Callback);

    TEST_MESSAGE("Test unimplmented interface ");
    status = ezUart_SyncReceive(&first_inst, rx_buff, sizeof(rx_buff), 1000);
    TEST_ASSERT_EQUAL(STATUS_ERR_INF_NOT_EXIST, status);

    TEST_MESSAGE("Test interface is locked");
    /* manually set lock */
    mock_hw.common.curr_inst = &first_inst;
    status = ezUart_SyncReceive(&second_inst, rx_buff, sizeof(rx_buff), 1000);
    TEST_ASSERT_EQUAL(STATUS_BUSY, status);
    /* release the lock */
    mock_hw.common.curr_inst = NULL;

    TEST_MESSAGE("Test happy path");
    ezMockUart_SetImplementedInterface(&mock_hw.interface);
    status = ezUart_SyncReceive(&first_inst, rx_buff, sizeof(rx_buff), 1000);
    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_EQUAL(NULL, mock_hw.common.curr_inst);
    TEST_ASSERT_EQUAL(UART_NUM_EVENTS, uart_event);
}


TEST(ez_uart, ezUart_GetConfig)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;
    struct ezUartConfiguration *config = NULL;

    TEST_MESSAGE("Test null pointer");
    status = ezUart_GetConfig(NULL, &config);
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    TEST_MESSAGE("Test unregistered instance");
    status = ezUart_GetConfig(&second_inst, &config);
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    /* Now we register the instance */
    status = ezUart_RegisterInstance(&first_inst, DRV_NAME, ezMockUart_Callback);
    status = ezUart_RegisterInstance(&second_inst, DRV_NAME, ezMockUart_Callback);

    TEST_MESSAGE("Test null pointer ");
    status = ezUart_GetConfig(&first_inst, NULL);
    TEST_ASSERT_EQUAL(STATUS_ERR_ARG, status);

    TEST_MESSAGE("Test interface is locked");
    /* manually set lock */
    mock_hw.common.curr_inst = &first_inst;
    status = ezUart_GetConfig(&second_inst, &config);
    TEST_ASSERT_EQUAL(STATUS_BUSY, status);
    /* release the lock */
    mock_hw.common.curr_inst = NULL;

    TEST_MESSAGE("Test happy path");
    ezMockUart_SetImplementedInterface(&mock_hw.interface);
    status = ezUart_GetConfig(&first_inst, &config);
    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_EQUAL(&mock_hw.config, config);
}


TEST(ez_uart, ezUart_UpdateConfig)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;
    struct ezUartConfiguration *config = NULL;

    TEST_MESSAGE("Test null pointer");
    status = ezUart_UpdateConfig(NULL);
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    TEST_MESSAGE("Test unregistered instance");
    status = ezUart_UpdateConfig(&second_inst);
    TEST_ASSERT_EQUAL(STATUS_ERR_DRV_NOT_FOUND, status);

    /* Now we register the instance */
    status = ezUart_RegisterInstance(&first_inst, DRV_NAME, ezMockUart_Callback);
    status = ezUart_RegisterInstance(&second_inst, DRV_NAME, ezMockUart_Callback);

    TEST_MESSAGE("Test interface is locked");
    /* manually set lock */
    mock_hw.common.curr_inst = &first_inst;
    status = ezUart_UpdateConfig(&second_inst);
    TEST_ASSERT_EQUAL(STATUS_BUSY, status);
    /* release the lock */
    mock_hw.common.curr_inst = NULL;

    TEST_MESSAGE("Test happy path");
    ezMockUart_SetImplementedInterface(&mock_hw.interface);
    status = ezUart_GetConfig(&first_inst, &config);
    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_EQUAL(&mock_hw.config, config);

    config->baudrate = 9600;
    config->stop_bit = TWO_BITS;

    status = ezUart_UpdateConfig(&first_inst);
    TEST_ASSERT_EQUAL(STATUS_OK, status);
    TEST_ASSERT_EQUAL_MEMORY(&mock_hw.config, config, sizeof(mock_hw.config));
}



/******************************************************************************
* Internal functions
*******************************************************************************/
static void RunAllTests(void)
{
    RUN_TEST_GROUP(ez_uart);
}

static EZ_DRV_STATUS ezMockUart_Initialize(uint8_t index)
{
    return STATUS_OK;
}


static EZ_DRV_STATUS ezMockUart_Deinitialize(uint8_t index)
{
    return STATUS_OK;
}


static EZ_DRV_STATUS ezMockUart_AsyncTransmit(uint8_t index, const uint8_t *tx_buff, uint16_t buff_size)
{
    /* In reality, the HW driver declare the ezUartDriver, so it has access to this struct */
    if(mock_hw.common.curr_inst != NULL && mock_hw.common.curr_inst->calback != NULL)
    {
        mock_hw.common.curr_inst->calback(UART_EVENT_TX_CMPLT, NULL, NULL);
    }

    /* Job done, unlock driver */
    mock_hw.common.curr_inst = NULL;
    return STATUS_OK;
}


static EZ_DRV_STATUS ezMockUart_AsyncReceive(uint8_t index, uint8_t *rx_buff, uint16_t buff_size)
{
    /* In reality, the HW driver declare the ezUartDriver, so it has access to this struct */
    if(mock_hw.common.curr_inst != NULL && mock_hw.common.curr_inst->calback != NULL)
    {
        mock_hw.common.curr_inst->calback(UART_EVENT_RX_CMPLT, NULL, NULL);
    }

    /* Job done, unlock driver */
    mock_hw.common.curr_inst = NULL;
    return STATUS_OK;
}


static EZ_DRV_STATUS ezMockUart_SyncTransmit(uint8_t index, const uint8_t *tx_buff, uint16_t buff_size, uint32_t timeout_millis)
{
    return STATUS_OK;
}


static EZ_DRV_STATUS ezMockUart_SyncReceive(uint8_t index, uint8_t *rx_buff, uint16_t buff_size, uint32_t timeout_millis)
{
    return STATUS_OK;
}


static EZ_DRV_STATUS ezMockUart_UpdateConfig(uint8_t index)
{
    return STATUS_OK;
}


static void ezMockUart_SetImplementedInterface(struct ezHwUartInterface *interface)
{
    if(interface != NULL)
    {
        interface->index = 0;
        interface->async_receive = ezMockUart_AsyncReceive;
        interface->async_transmit = ezMockUart_AsyncTransmit;
        interface->deinitialize = ezMockUart_Deinitialize;
        interface->initialize = ezMockUart_Initialize;
        interface->sync_receive = ezMockUart_SyncReceive;
        interface->sync_transmit = ezMockUart_SyncTransmit;
        interface->update_conf = ezMockUart_UpdateConfig;
    }
}

static void ezMockUart_Callback(uint8_t event_code, void *param1, void *param2)
{
    uart_event = (EZ_UART_EVENT)event_code;
}


/* End of file */
