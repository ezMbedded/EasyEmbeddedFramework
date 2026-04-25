/*****************************************************************************
* Filename:         unittest_ez_uart.cpp
* Author:           Hai Nguyen
* Original Date:    25.04.2026
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   unittest_ez_uart.cpp
 *  @author Hai Nguyen
 *  @date   25.04.2026
 *  @brief  Unit tests for the UART HAL component
 *
 *  @details -
 */

/******************************************************************************
* Includes
*******************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ez_driver_def.h"
#include "ez_uart.h"
#include "fff.h"

#include <catch2/catch_test_macros.hpp>


/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwInitialize, ezDriver_t *);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwDeinitialize, ezDriver_t *);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwAsyncTransmit, ezDriver_t *, const uint8_t *, uint16_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwAsyncReceive, ezDriver_t *, uint8_t *, uint16_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwSyncTransmit, ezDriver_t *, const uint8_t *, uint16_t, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwSyncReceive, ezDriver_t *, uint8_t *, uint16_t, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwUpdateConfig, ezDriver_t *);
FAKE_VOID_FUNC(InstanceCallback, uint8_t, void *, void *);


/******************************************************************************
* Module Typedefs
*******************************************************************************/
typedef struct
{
    uint8_t event_code;
    void *param1;
    void *param2;
    bool fire_async_callback;
} ezUartAsyncEvent_t;


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static struct ezUartDriver g_mock_driver;
static ezUartAsyncEvent_t g_async_event;


/******************************************************************************
* Function Definitions
*******************************************************************************/
static void ResetAsyncEvent(void);
static EZ_DRV_STATUS HwAsyncTransmitCustom(ezDriver_t *driver_h, const uint8_t *tx_buff, uint16_t buff_size);
static EZ_DRV_STATUS HwAsyncReceiveCustom(ezDriver_t *driver_h, uint8_t *rx_buff, uint16_t buff_size);

class UartTestFixture
{
public:
    UartTestFixture();
    ~UartTestFixture();

protected:
    void RegisterDriver(void);
    void RegisterFirstInstance(void);
    void RegisterSecondInstance(void);
    void RegisterBothInstances(void);

    ezUartDrvInstance_t first_inst{};
    ezUartDrvInstance_t second_inst{};
    bool driver_registered;
    bool first_registered;
    bool second_registered;
};


/******************************************************************************
* External functions
*******************************************************************************/
TEST_CASE_METHOD(UartTestFixture, "Test driver registration", "[hal][uart]")
{
    WHEN("A NULL hardware driver is provided")
    {
        /* Verify the registration entry points reject invalid arguments. */
        THEN("The APIs report an argument error")
        {
            CHECK(ezUart_SystemRegisterHwDriver(NULL) == STATUS_ERR_ARG);
            CHECK(ezUart_SystemUnregisterHwDriver(NULL) == STATUS_ERR_ARG);
        }
    }

    WHEN("A valid UART driver is registered")
    {
        RegisterDriver();

        THEN("The receive-event callback is installed for async completion handling")
        {
            CHECK(g_mock_driver.common.callback != NULL);
        }

        AND_WHEN("The same driver is unregistered")
        {
            THEN("The driver is removed cleanly")
            {
                CHECK(ezUart_SystemUnregisterHwDriver(&g_mock_driver) == STATUS_OK);
                driver_registered = false;
            }
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test instance registration lifecycle", "[hal][uart]")
{
    WHEN("Invalid instance arguments are used")
    {
        /* Confirm basic argument validation for register and unregister APIs. */
        THEN("The APIs report argument errors")
        {
            CHECK(ezUart_RegisterInstance(NULL, NULL, NULL) == STATUS_ERR_ARG);
            CHECK(ezUart_UnregisterInstance(NULL) == STATUS_ERR_ARG);
        }
    }

    WHEN("The target driver name is unknown")
    {
        RegisterDriver();

        THEN("The instance registration fails with driver-not-found")
        {
            CHECK(ezUart_RegisterInstance(&first_inst, "Unknown driver", InstanceCallback) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("A valid instance is registered")
    {
        RegisterDriver();

        THEN("The instance stores the selected driver and callback")
        {
            CHECK(ezUart_RegisterInstance(&first_inst, g_mock_driver.common.name, InstanceCallback) == STATUS_OK);
            first_registered = true;
            CHECK(first_inst.driver == &g_mock_driver);
            CHECK(first_inst.callback == InstanceCallback);
        }

        AND_WHEN("The instance is unregistered")
        {
            REQUIRE(ezUart_RegisterInstance(&first_inst, g_mock_driver.common.name, InstanceCallback) == STATUS_OK);
            first_registered = true;

            THEN("The driver reference is cleared")
            {
                CHECK(ezUart_UnregisterInstance(&first_inst) == STATUS_OK);
                first_registered = false;
                CHECK(first_inst.driver == NULL);
            }
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test initialization", "[hal][uart]")
{
    WHEN("The instance is NULL or not registered")
    {
        /* The API cannot resolve a backing hardware driver in this state. */
        THEN("The API returns driver-not-found")
        {
            CHECK(ezUart_Initialize(NULL) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_Initialize(&first_inst) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No hardware initialize function is implemented")
    {
        THEN("The API reports a missing interface and unlocks the driver")
        {
            CHECK(ezUart_Initialize(&first_inst) == STATUS_ERR_INF_NOT_EXIST);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("Another instance is already using the driver")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports a busy driver")
        {
            CHECK(ezUart_Initialize(&second_inst) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware initialize fake reports a timeout")
    {
        g_mock_driver.interface.initialize = HwInitialize;
        HwInitialize_fake.return_val = STATUS_TIMEOUT;

        THEN("The failure is propagated to the caller")
        {
            CHECK(ezUart_Initialize(&first_inst) == STATUS_TIMEOUT);
            CHECK(HwInitialize_fake.call_count == 1);
            CHECK(HwInitialize_fake.arg0_val == &g_mock_driver.common);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("The hardware initialize fake succeeds")
    {
        g_mock_driver.interface.initialize = HwInitialize;
        HwInitialize_fake.return_val = STATUS_OK;

        THEN("The driver forwards the handle and reports success")
        {
            CHECK(ezUart_Initialize(&first_inst) == STATUS_OK);
            CHECK(HwInitialize_fake.call_count == 1);
            CHECK(HwInitialize_fake.arg0_val == &g_mock_driver.common);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test deinitialization", "[hal][uart]")
{
    WHEN("The instance is NULL or not registered")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezUart_Deinitialize(NULL) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_Deinitialize(&first_inst) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No hardware deinitialize function is implemented")
    {
        THEN("The API reports a missing interface")
        {
            CHECK(ezUart_Deinitialize(&first_inst) == STATUS_ERR_INF_NOT_EXIST);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("Another instance already owns the driver")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezUart_Deinitialize(&second_inst) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware deinitialize fake reports a timeout")
    {
        g_mock_driver.interface.deinitialize = HwDeinitialize;
        HwDeinitialize_fake.return_val = STATUS_TIMEOUT;

        THEN("The failure is propagated")
        {
            CHECK(ezUart_Deinitialize(&first_inst) == STATUS_TIMEOUT);
            CHECK(HwDeinitialize_fake.call_count == 1);
            CHECK(HwDeinitialize_fake.arg0_val == &g_mock_driver.common);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("The hardware deinitialize fake succeeds")
    {
        g_mock_driver.interface.deinitialize = HwDeinitialize;
        HwDeinitialize_fake.return_val = STATUS_OK;

        THEN("The driver forwards the handle and reports success")
        {
            CHECK(ezUart_Deinitialize(&first_inst) == STATUS_OK);
            CHECK(HwDeinitialize_fake.call_count == 1);
            CHECK(HwDeinitialize_fake.arg0_val == &g_mock_driver.common);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test async transmit", "[hal][uart]")
{
    static const uint8_t tx_buff[] = {0x10, 0x20, 0x30};

    WHEN("The instance is NULL or not registered")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezUart_AsyncTransmit(NULL, tx_buff, sizeof(tx_buff)) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_AsyncTransmit(&first_inst, tx_buff, sizeof(tx_buff)) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No asynchronous transmit interface is implemented")
    {
        THEN("The API reports a missing interface")
        {
            CHECK(ezUart_AsyncTransmit(&first_inst, tx_buff, sizeof(tx_buff)) == STATUS_ERR_INF_NOT_EXIST);
        }
    }

    WHEN("Another instance already owns the driver")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezUart_AsyncTransmit(&second_inst, tx_buff, sizeof(tx_buff)) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware async transmit fake reports a timeout")
    {
        g_mock_driver.interface.async_transmit = HwAsyncTransmit;
        HwAsyncTransmit_fake.custom_fake = NULL;
        HwAsyncTransmit_fake.return_val = STATUS_TIMEOUT;

        THEN("The timeout is propagated to the caller")
        {
            CHECK(ezUart_AsyncTransmit(&first_inst, tx_buff, sizeof(tx_buff)) == STATUS_TIMEOUT);
            CHECK(HwAsyncTransmit_fake.call_count == 1);
            CHECK(HwAsyncTransmit_fake.arg0_val == &g_mock_driver.common);
        }
    }

    WHEN("The hardware async transmit fake succeeds and raises a callback")
    {
        g_mock_driver.interface.async_transmit = HwAsyncTransmit;
        HwAsyncTransmit_fake.custom_fake = HwAsyncTransmitCustom;
        HwAsyncTransmit_fake.return_val = STATUS_OK;
        g_async_event.fire_async_callback = true;
        g_async_event.event_code = UART_EVENT_TX_CMPLT;
        g_async_event.param1 = (void*)0x1111;
        g_async_event.param2 = (void*)0x2222;

        THEN("The callback is forwarded and the driver is unlocked")
        {
            CHECK(ezUart_AsyncTransmit(&first_inst, tx_buff, sizeof(tx_buff)) == STATUS_OK);
            CHECK(HwAsyncTransmit_fake.call_count == 1);
            CHECK(HwAsyncTransmit_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwAsyncTransmit_fake.arg1_val == tx_buff);
            CHECK(HwAsyncTransmit_fake.arg2_val == sizeof(tx_buff));
            CHECK(InstanceCallback_fake.call_count == 1);
            CHECK(InstanceCallback_fake.arg0_val == g_async_event.event_code);
            CHECK(InstanceCallback_fake.arg1_val == g_async_event.param1);
            CHECK(InstanceCallback_fake.arg2_val == g_async_event.param2);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test async receive", "[hal][uart]")
{
    uint8_t rx_buff[4] = {0};

    WHEN("The instance is NULL or not registered")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezUart_AsyncReceive(NULL, rx_buff, sizeof(rx_buff)) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_AsyncReceive(&first_inst, rx_buff, sizeof(rx_buff)) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No asynchronous receive interface is implemented")
    {
        THEN("The API reports a missing interface")
        {
            CHECK(ezUart_AsyncReceive(&first_inst, rx_buff, sizeof(rx_buff)) == STATUS_ERR_INF_NOT_EXIST);
        }
    }

    WHEN("Another instance already owns the driver")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezUart_AsyncReceive(&second_inst, rx_buff, sizeof(rx_buff)) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware async receive fake reports a timeout")
    {
        g_mock_driver.interface.async_receive = HwAsyncReceive;
        HwAsyncReceive_fake.custom_fake = NULL;
        HwAsyncReceive_fake.return_val = STATUS_TIMEOUT;

        THEN("The timeout is propagated to the caller")
        {
            CHECK(ezUart_AsyncReceive(&first_inst, rx_buff, sizeof(rx_buff)) == STATUS_TIMEOUT);
            CHECK(HwAsyncReceive_fake.call_count == 1);
            CHECK(HwAsyncReceive_fake.arg0_val == &g_mock_driver.common);
        }
    }

    WHEN("The hardware async receive fake succeeds and raises a callback")
    {
        g_mock_driver.interface.async_receive = HwAsyncReceive;
        HwAsyncReceive_fake.custom_fake = HwAsyncReceiveCustom;
        HwAsyncReceive_fake.return_val = STATUS_OK;
        g_async_event.fire_async_callback = true;
        g_async_event.event_code = UART_EVENT_RX_CMPLT;
        g_async_event.param1 = (void*)0x3333;
        g_async_event.param2 = (void*)0x4444;

        THEN("The callback is forwarded and the driver is unlocked")
        {
            CHECK(ezUart_AsyncReceive(&first_inst, rx_buff, sizeof(rx_buff)) == STATUS_OK);
            CHECK(HwAsyncReceive_fake.call_count == 1);
            CHECK(HwAsyncReceive_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwAsyncReceive_fake.arg1_val == rx_buff);
            CHECK(HwAsyncReceive_fake.arg2_val == sizeof(rx_buff));
            CHECK(InstanceCallback_fake.call_count == 1);
            CHECK(InstanceCallback_fake.arg0_val == g_async_event.event_code);
            CHECK(InstanceCallback_fake.arg1_val == g_async_event.param1);
            CHECK(InstanceCallback_fake.arg2_val == g_async_event.param2);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test sync transmit", "[hal][uart]")
{
    static const uint8_t tx_buff[] = {0xAB, 0xCD, 0xEF};

    WHEN("The instance is NULL or not registered")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezUart_SyncTransmit(NULL, tx_buff, sizeof(tx_buff), 1000U) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_SyncTransmit(&first_inst, tx_buff, sizeof(tx_buff), 1000U) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No synchronous transmit interface is implemented")
    {
        THEN("The API reports a missing interface")
        {
            CHECK(ezUart_SyncTransmit(&first_inst, tx_buff, sizeof(tx_buff), 1000U) == STATUS_ERR_INF_NOT_EXIST);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("Another instance already owns the driver")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezUart_SyncTransmit(&second_inst, tx_buff, sizeof(tx_buff), 1000U) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware sync transmit fake reports a timeout")
    {
        g_mock_driver.interface.sync_transmit = HwSyncTransmit;
        HwSyncTransmit_fake.return_val = STATUS_TIMEOUT;

        THEN("The timeout is propagated and the driver unlocks")
        {
            CHECK(ezUart_SyncTransmit(&first_inst, tx_buff, sizeof(tx_buff), 1000U) == STATUS_TIMEOUT);
            CHECK(HwSyncTransmit_fake.call_count == 1);
            CHECK(HwSyncTransmit_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwSyncTransmit_fake.arg1_val == tx_buff);
            CHECK(HwSyncTransmit_fake.arg2_val == sizeof(tx_buff));
            CHECK(HwSyncTransmit_fake.arg3_val == 1000U);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("The hardware sync transmit fake succeeds")
    {
        g_mock_driver.interface.sync_transmit = HwSyncTransmit;
        HwSyncTransmit_fake.return_val = STATUS_OK;

        THEN("The transmit arguments are forwarded to the hardware layer")
        {
            CHECK(ezUart_SyncTransmit(&first_inst, tx_buff, sizeof(tx_buff), 1000U) == STATUS_OK);
            CHECK(HwSyncTransmit_fake.call_count == 1);
            CHECK(HwSyncTransmit_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwSyncTransmit_fake.arg1_val == tx_buff);
            CHECK(HwSyncTransmit_fake.arg2_val == sizeof(tx_buff));
            CHECK(HwSyncTransmit_fake.arg3_val == 1000U);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test sync receive", "[hal][uart]")
{
    uint8_t rx_buff[4] = {0};

    WHEN("The instance is NULL or not registered")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezUart_SyncReceive(NULL, rx_buff, sizeof(rx_buff), 1000U) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_SyncReceive(&first_inst, rx_buff, sizeof(rx_buff), 1000U) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No synchronous receive interface is implemented")
    {
        THEN("The API reports a missing interface")
        {
            CHECK(ezUart_SyncReceive(&first_inst, rx_buff, sizeof(rx_buff), 1000U) == STATUS_ERR_INF_NOT_EXIST);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("Another instance already owns the driver")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezUart_SyncReceive(&second_inst, rx_buff, sizeof(rx_buff), 1000U) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware sync receive fake reports a timeout")
    {
        g_mock_driver.interface.sync_receive = HwSyncReceive;
        HwSyncReceive_fake.return_val = STATUS_TIMEOUT;

        THEN("The timeout is propagated and the driver unlocks")
        {
            CHECK(ezUart_SyncReceive(&first_inst, rx_buff, sizeof(rx_buff), 1000U) == STATUS_TIMEOUT);
            CHECK(HwSyncReceive_fake.call_count == 1);
            CHECK(HwSyncReceive_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwSyncReceive_fake.arg1_val == rx_buff);
            CHECK(HwSyncReceive_fake.arg2_val == sizeof(rx_buff));
            CHECK(HwSyncReceive_fake.arg3_val == 1000U);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("The hardware sync receive fake succeeds")
    {
        g_mock_driver.interface.sync_receive = HwSyncReceive;
        HwSyncReceive_fake.return_val = STATUS_OK;

        THEN("The receive arguments are forwarded to the hardware layer")
        {
            CHECK(ezUart_SyncReceive(&first_inst, rx_buff, sizeof(rx_buff), 1000U) == STATUS_OK);
            CHECK(HwSyncReceive_fake.call_count == 1);
            CHECK(HwSyncReceive_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwSyncReceive_fake.arg1_val == rx_buff);
            CHECK(HwSyncReceive_fake.arg2_val == sizeof(rx_buff));
            CHECK(HwSyncReceive_fake.arg3_val == 1000U);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test get configuration", "[hal][uart]")
{
    struct ezUartConfiguration *config = NULL;

    WHEN("The instance is NULL or not registered")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezUart_GetConfig(NULL, &config) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_GetConfig(&first_inst, &config) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("The output configuration pointer is NULL")
    {
        THEN("The API reports an argument error")
        {
            CHECK(ezUart_GetConfig(&first_inst, NULL) == STATUS_ERR_ARG);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("Another instance already owns the driver")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezUart_GetConfig(&second_inst, &config) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("A valid output pointer is provided")
    {
        THEN("The API exposes the driver's live configuration structure")
        {
            CHECK(ezUart_GetConfig(&first_inst, &config) == STATUS_OK);
            REQUIRE(config != NULL);
            CHECK(config == &g_mock_driver.config);
            CHECK(config->baudrate == 115200U);
            CHECK(config->port_name == g_mock_driver.config.port_name);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test update configuration", "[hal][uart]")
{
    WHEN("The instance is NULL or not registered")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezUart_UpdateConfig(NULL) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_UpdateConfig(&first_inst) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No update-config interface is implemented")
    {
        /* The source leaves the default driver-not-found status unchanged here. */
        THEN("The API returns the current default status")
        {
            CHECK(ezUart_UpdateConfig(&first_inst) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("Another instance already owns the driver")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezUart_UpdateConfig(&second_inst) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware update-config fake reports a timeout")
    {
        g_mock_driver.interface.update_conf = HwUpdateConfig;
        HwUpdateConfig_fake.return_val = STATUS_TIMEOUT;

        THEN("The timeout is propagated to the caller")
        {
            CHECK(ezUart_UpdateConfig(&first_inst) == STATUS_TIMEOUT);
            CHECK(HwUpdateConfig_fake.call_count == 1);
            CHECK(HwUpdateConfig_fake.arg0_val == &g_mock_driver.common);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("The hardware update-config fake succeeds")
    {
        g_mock_driver.interface.update_conf = HwUpdateConfig;
        HwUpdateConfig_fake.return_val = STATUS_OK;

        THEN("The driver forwards the handle and reports success")
        {
            CHECK(ezUart_UpdateConfig(&first_inst) == STATUS_OK);
            CHECK(HwUpdateConfig_fake.call_count == 1);
            CHECK(HwUpdateConfig_fake.arg0_val == &g_mock_driver.common);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


/******************************************************************************
* Internal functions
*******************************************************************************/
UartTestFixture::UartTestFixture()
    : driver_registered(false),
      first_registered(false),
      second_registered(false)
{
    memset(&g_mock_driver, 0, sizeof(g_mock_driver));
    ResetAsyncEvent();

    RESET_FAKE(HwInitialize);
    RESET_FAKE(HwDeinitialize);
    RESET_FAKE(HwAsyncTransmit);
    RESET_FAKE(HwAsyncReceive);
    RESET_FAKE(HwSyncTransmit);
    RESET_FAKE(HwSyncReceive);
    RESET_FAKE(HwUpdateConfig);
    RESET_FAKE(InstanceCallback);

    g_mock_driver.common.name = "Mock UART Driver";
    g_mock_driver.common.version[0] = 1;
    g_mock_driver.common.version[1] = 0;
    g_mock_driver.common.version[2] = 0;

    g_mock_driver.config.port_name = "COM1";
    g_mock_driver.config.baudrate = 115200U;
    g_mock_driver.config.parity = PARITY_ODD;
    g_mock_driver.config.stop_bit = ONE_BIT;
    g_mock_driver.config.byte_size = 8U;
}


UartTestFixture::~UartTestFixture()
{
    if(first_registered)
    {
        (void)ezUart_UnregisterInstance(&first_inst);
    }

    if(second_registered)
    {
        (void)ezUart_UnregisterInstance(&second_inst);
    }

    if(driver_registered)
    {
        (void)ezUart_SystemUnregisterHwDriver(&g_mock_driver);
    }
}


void UartTestFixture::RegisterDriver(void)
{
    REQUIRE(ezUart_SystemRegisterHwDriver(&g_mock_driver) == STATUS_OK);
    driver_registered = true;
}


void UartTestFixture::RegisterFirstInstance(void)
{
    REQUIRE(ezUart_RegisterInstance(&first_inst, g_mock_driver.common.name, InstanceCallback) == STATUS_OK);
    first_registered = true;
}


void UartTestFixture::RegisterSecondInstance(void)
{
    REQUIRE(ezUart_RegisterInstance(&second_inst, g_mock_driver.common.name, InstanceCallback) == STATUS_OK);
    second_registered = true;
}


void UartTestFixture::RegisterBothInstances(void)
{
    RegisterFirstInstance();
    RegisterSecondInstance();
}


static void ResetAsyncEvent(void)
{
    memset(&g_async_event, 0, sizeof(g_async_event));
}


static EZ_DRV_STATUS HwAsyncTransmitCustom(ezDriver_t *driver_h, const uint8_t *tx_buff, uint16_t buff_size)
{
    (void)tx_buff;
    (void)buff_size;

    if((g_async_event.fire_async_callback == true) && (driver_h != NULL) && (driver_h->callback != NULL) && (driver_h->curr_inst != NULL))
    {
        driver_h->callback(driver_h->curr_inst->driver,
                           g_async_event.event_code,
                           g_async_event.param1,
                           g_async_event.param2);
    }

    return HwAsyncTransmit_fake.return_val;
}


static EZ_DRV_STATUS HwAsyncReceiveCustom(ezDriver_t *driver_h, uint8_t *rx_buff, uint16_t buff_size)
{
    (void)rx_buff;
    (void)buff_size;

    if((g_async_event.fire_async_callback == true) && (driver_h != NULL) && (driver_h->callback != NULL) && (driver_h->curr_inst != NULL))
    {
        driver_h->callback(driver_h->curr_inst->driver,
                           g_async_event.event_code,
                           g_async_event.param1,
                           g_async_event.param2);
    }

    return HwAsyncReceive_fake.return_val;
}


/* End of file */