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
#include <stddef.h>
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
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwInitialize, void *);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwDeinitialize, void *);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTransmitAsync, void *, const uint8_t *, uint16_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwReceiveAsync, void *, uint8_t *, uint16_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTransmitSync, void *, const uint8_t *, uint16_t, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwReceiveSync, void *, uint8_t *, uint16_t, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwUpdateConfig, void *);
FAKE_VOID_FUNC(OnEvent, void *, uint8_t, void *, void *);


/******************************************************************************
* Module Typedefs
*******************************************************************************/
/* None */


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static struct ezUartDriver g_drv;
static int g_hw_token = 0;              /* value of driver_h given to the HW functions */
static bool g_busy_in_callback = false; /* BUSY bit seen inside the callback */
static uint8_t g_state_in_hw = 0U;      /* state byte seen inside a HW function */
static EZ_DRV_STATUS g_chained_status = EZ_DRV_STATUS_OK;
static ezUartHandle_t *g_chain_handle = NULL;
static uint8_t g_buf[4] = {1, 2, 3, 4};


/******************************************************************************
* Function Definitions
*******************************************************************************/
static bool IsBusy(void)
{
    return EZ_DRV_STATE_IS_SET(&g_drv.base, EZ_DRV_STATE_BUSY);
}

static void SimulateIsrDone(uint8_t event_code)
{
    ezDriver_NotifyEvent(&g_drv.base, EZ_DRV_EVENT_OP_DONE, event_code, NULL, NULL);
}

static void SimulateIsrProgress(uint8_t event_code)
{
    ezDriver_NotifyEvent(&g_drv.base, EZ_DRV_EVENT_OP_PROGRESS, event_code, NULL, NULL);
}

static void OnEventRecordBusy(void *ctx, uint8_t event_code, void *p1, void *p2)
{
    (void)ctx;
    (void)event_code;
    (void)p1;
    (void)p2;
    g_busy_in_callback = IsBusy();
}

static void OnEventChain(void *ctx, uint8_t event_code, void *p1, void *p2)
{
    (void)ctx;
    (void)event_code;
    (void)p1;
    (void)p2;
    g_chained_status = ezUart_TransmitAsync(g_chain_handle, g_buf, sizeof(g_buf));
}

/* A sync HW function which breaks the rule and reports events. */
static EZ_DRV_STATUS HwTransmitSyncNotifies(void *driver_h, const uint8_t *tx_buff, uint16_t buff_size, uint32_t timeout_millis)
{
    (void)driver_h;
    (void)tx_buff;
    (void)buff_size;
    (void)timeout_millis;
    g_state_in_hw = g_drv.base.state;
    SimulateIsrProgress(UART_EVENT_TX_CMPLT);
    SimulateIsrDone(UART_EVENT_TX_CMPLT);
    return EZ_DRV_STATUS_OK;
}

/* Record the state bits seen by the HW function. */
static EZ_DRV_STATUS HwReceiveSyncRecordState(void *driver_h, uint8_t *rx_buff, uint16_t buff_size, uint32_t timeout_millis)
{
    (void)driver_h;
    (void)rx_buff;
    (void)buff_size;
    (void)timeout_millis;
    g_state_in_hw = g_drv.base.state;
    return EZ_DRV_STATUS_OK;
}

static EZ_DRV_STATUS HwTransmitAsyncRecordState(void *driver_h, const uint8_t *tx_buff, uint16_t buff_size)
{
    (void)driver_h;
    (void)tx_buff;
    (void)buff_size;
    g_state_in_hw = g_drv.base.state;
    return EZ_DRV_STATUS_OK;
}


/******************************************************************************
* Fixture
*******************************************************************************/
struct UartTestFixture
{
    UartTestFixture()
    {
        RESET_FAKE(HwInitialize);
        RESET_FAKE(HwDeinitialize);
        RESET_FAKE(HwTransmitAsync);
        RESET_FAKE(HwReceiveAsync);
        RESET_FAKE(HwTransmitSync);
        RESET_FAKE(HwReceiveSync);
        RESET_FAKE(HwUpdateConfig);
        RESET_FAKE(OnEvent);
        FFF_RESET_HISTORY();

        g_busy_in_callback = false;
        g_state_in_hw = 0U;
        g_chained_status = EZ_DRV_STATUS_OK;
        g_chain_handle = NULL;

        memset(&g_drv, 0, sizeof(g_drv));
        memset(&first, 0, sizeof(first));
        memset(&second, 0, sizeof(second));
        g_drv.base.name = "uart_mock";
        g_drv.interface.driver_h = &g_hw_token;
        g_drv.interface.initialize = HwInitialize;
        g_drv.interface.deinitialize = HwDeinitialize;
        g_drv.interface.transmit_async = HwTransmitAsync;
        g_drv.interface.receive_async = HwReceiveAsync;
        g_drv.interface.transmit_sync = HwTransmitSync;
        g_drv.interface.receive_sync = HwReceiveSync;
        g_drv.interface.update_conf = HwUpdateConfig;

        REQUIRE(ezUart_SystemRegisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
        driver_registered = true;
    }

    ~UartTestFixture()
    {
        if(driver_registered)
        {
            /* Make sure a test that leaves an operation in flight does not leak.
             * The callback must not start a new operation here. */
            OnEvent_fake.custom_fake = NULL;
            g_chain_handle = NULL;
            SimulateIsrDone(0);
            /* Handles left open by a test: the driver must be unregistrable, or it stays in the list */
            g_drv.open_count = 0U;
            CHECK(ezUart_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
        }
    }

    /* Open a handle on the mock driver. */
    void OpenHandle(ezUartHandle_t *handle, ezDrvCallback cb = OnEvent, void *ctx = NULL)
    {
        REQUIRE(ezUart_Open(handle, "uart_mock", cb, ctx) == EZ_DRV_STATUS_OK);
    }

    /* Start an async transmit which is accepted by the HW driver. */
    void StartAsyncTransmit(ezUartHandle_t *handle)
    {
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezUart_TransmitAsync(handle, g_buf, sizeof(g_buf)) == EZ_DRV_STATUS_OK);
    }

    ezUartHandle_t first;
    ezUartHandle_t second;
    bool driver_registered;
};


/******************************************************************************
* External functions
*******************************************************************************/
TEST_CASE_METHOD(UartTestFixture, "Test driver registration", "[hal][uart]")
{
    WHEN("Invalid arguments are used")
    {
        struct ezUartDriver no_name;
        memset(&no_name, 0, sizeof(no_name));

        THEN("The APIs report an argument error")
        {
            CHECK(ezUart_SystemRegisterHwDriver(NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezUart_SystemRegisterHwDriver(&no_name) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezUart_SystemUnregisterHwDriver(NULL) == EZ_DRV_STATUS_ERR_ARG);
        }
    }

    WHEN("A driver with the same name is registered twice")
    {
        struct ezUartDriver duplicate;
        memset(&duplicate, 0, sizeof(duplicate));
        duplicate.base.name = "uart_mock";

        THEN("The second registration is rejected")
        {
            CHECK(ezUart_SystemRegisterHwDriver(&duplicate) == EZ_DRV_STATUS_ERR_ARG);
        }
    }

    WHEN("A second driver with another name is registered")
    {
        struct ezUartDriver other;
        memset(&other, 0, sizeof(other));
        other.base.name = "uart_other";

        THEN("Both can be found by name")
        {
            REQUIRE(ezUart_SystemRegisterHwDriver(&other) == EZ_DRV_STATUS_OK);
            CHECK(ezUart_Open(&first, "uart_other", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == &other);
            CHECK(ezUart_Open(&second, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(second.drv_handle.hw == &g_drv);
            CHECK(ezUart_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(ezUart_SystemUnregisterHwDriver(&other) == EZ_DRV_STATUS_OK);
        }
    }

    WHEN("A driver is registered")
    {
        THEN("It starts idle with no open handle")
        {
            CHECK(g_drv.open_count == 0U);
            CHECK(g_drv.base.state == 0U);
            CHECK(g_drv.base.user == NULL);
        }
    }

    WHEN("A driver is unregistered")
    {
        THEN("It is removed from the list and cannot be found anymore")
        {
            CHECK(ezUart_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
            driver_registered = false;
            CHECK(ezUart_Open(&first, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("A driver is unregistered while a handle is open")
    {
        OpenHandle(&first);

        THEN("The unregistration is refused until the handle is closed")
        {
            CHECK(ezUart_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_BUSY);
            REQUIRE(ezUart_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(ezUart_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
            driver_registered = false;
        }
    }

    WHEN("A driver is unregistered while an operation is in flight")
    {
        OpenHandle(&first);
        StartAsyncTransmit(&first);

        THEN("The unregistration is refused")
        {
            CHECK(ezUart_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_BUSY);
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test open and close", "[hal][uart]")
{
    WHEN("Invalid arguments are used")
    {
        THEN("The APIs report an argument error")
        {
            CHECK(ezUart_Open(NULL, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezUart_Open(&first, NULL, OnEvent, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezUart_Close(NULL) == EZ_DRV_STATUS_ERR_ARG);
        }
    }

    WHEN("A handle which was never opened is closed")
    {
        THEN("The call succeeds, the handle stays empty and the HW is not called")
        {
            CHECK(ezUart_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(first.drv_handle.on_event == NULL);
            CHECK(first.drv_handle.ctx == NULL);
            CHECK(HwDeinitialize_fake.call_count == 0U);
        }
    }

    WHEN("The target driver does not exist")
    {
        THEN("The open fails with driver-not-found and the HW is not called")
        {
            CHECK(ezUart_Open(&first, "unknown", OnEvent, NULL) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(HwInitialize_fake.call_count == 0U);
            CHECK(first.drv_handle.hw == NULL);
        }
    }

    WHEN("The HW driver has no initialize or deinitialize function (the UART is already configured)")
    {
        g_drv.interface.initialize = NULL;
        g_drv.interface.deinitialize = NULL;

        THEN("The open only binds the handle, and the close only unbinds it")
        {
            REQUIRE(ezUart_Open(&first, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == &g_drv);
            CHECK(g_drv.open_count == 1U);
            CHECK(HwInitialize_fake.call_count == 0U);
            CHECK(ezUart_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(g_drv.open_count == 0U);
            CHECK(HwDeinitialize_fake.call_count == 0U);
        }
    }

    WHEN("The HW driver has an initialize function but no deinitialize function")
    {
        g_drv.interface.deinitialize = NULL;

        THEN("The close succeeds without a HW call")
        {
            REQUIRE(ezUart_Open(&first, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(HwInitialize_fake.call_count == 1U);
            CHECK(ezUart_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(HwDeinitialize_fake.call_count == 0U);
        }
    }

    WHEN("The handle is opened")
    {
        int user_ctx = 0;
        REQUIRE(ezUart_Open(&first, "uart_mock", OnEvent, &user_ctx) == EZ_DRV_STATUS_OK);

        THEN("The HW gets its token, once")
        {
            CHECK(HwInitialize_fake.call_count == 1U);
            CHECK(HwInitialize_fake.arg0_val == &g_hw_token);
        }

        THEN("The handle stores the driver, the callback and the context, and the driver is idle")
        {
            CHECK(first.drv_handle.hw == &g_drv);
            CHECK(first.drv_handle.on_event == OnEvent);
            CHECK(first.drv_handle.ctx == &user_ctx);
            CHECK(g_drv.open_count == 1U);
            CHECK(IsBusy() == false);
        }

        THEN("Opening it again is refused and the binding is unchanged")
        {
            CHECK(ezUart_Open(&first, "uart_mock", NULL, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(first.drv_handle.on_event == OnEvent);
            CHECK(first.drv_handle.ctx == &user_ctx);
            CHECK(HwInitialize_fake.call_count == 1U);
            CHECK(g_drv.open_count == 1U);
        }
    }

    WHEN("The HW initialization fails")
    {
        HwInitialize_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;

        THEN("The status is forwarded and nothing is bound")
        {
            CHECK(ezUart_Open(&first, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(g_drv.open_count == 0U);
            CHECK(IsBusy() == false);
        }

        THEN("The same handle can be opened again once the failure is gone")
        {
            CHECK(ezUart_Open(&first, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_GENERIC);
            HwInitialize_fake.return_val = EZ_DRV_STATUS_OK;
            CHECK(ezUart_Open(&first, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(HwInitialize_fake.call_count == 2U);
            CHECK(g_drv.open_count == 1U);
        }
    }

    WHEN("A second handle is opened")
    {
        OpenHandle(&first);
        REQUIRE(ezUart_Open(&second, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);

        THEN("The UART is not initialized again")
        {
            CHECK(HwInitialize_fake.call_count == 1U);
            CHECK(g_drv.open_count == 2U);
            CHECK(second.drv_handle.hw == &g_drv);
        }
    }

    WHEN("A handle is opened while an operation is in flight")
    {
        OpenHandle(&first);
        StartAsyncTransmit(&first);

        THEN("The open is refused and the operation is not disturbed")
        {
            CHECK(ezUart_Open(&second, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_BUSY);
            CHECK(second.drv_handle.hw == NULL);
            CHECK(IsBusy() == true);
            SimulateIsrDone(UART_EVENT_TX_CMPLT);
            CHECK(OnEvent_fake.call_count == 1U);
        }
    }

    WHEN("The maximum number of handles is open")
    {
        g_drv.open_count = UINT8_MAX;

        THEN("The open fails and the HW is not called")
        {
            CHECK(ezUart_Open(&first, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(HwInitialize_fake.call_count == 0U);
            CHECK(g_drv.open_count == UINT8_MAX);
        }
    }

    WHEN("The only open handle is closed")
    {
        OpenHandle(&first);

        THEN("The UART is released and everything is cleared")
        {
            CHECK(ezUart_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(HwDeinitialize_fake.call_count == 1U);
            CHECK(HwDeinitialize_fake.arg0_val == &g_hw_token);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(first.drv_handle.on_event == NULL);
            CHECK(first.drv_handle.ctx == NULL);
            CHECK(g_drv.open_count == 0U);
        }

        THEN("It can be opened again afterwards and the UART is initialized again")
        {
            REQUIRE(ezUart_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(ezUart_Open(&first, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(HwInitialize_fake.call_count == 2U);
        }
    }

    WHEN("One of two open handles is closed")
    {
        OpenHandle(&first);
        OpenHandle(&second);
        REQUIRE(ezUart_Close(&first) == EZ_DRV_STATUS_OK);

        THEN("The UART stays up and the other handle keeps working")
        {
            CHECK(HwDeinitialize_fake.call_count == 0U);
            CHECK(g_drv.open_count == 1U);
            HwTransmitSync_fake.return_val = EZ_DRV_STATUS_OK;
            CHECK(ezUart_TransmitSync(&second, g_buf, 1, 5) == EZ_DRV_STATUS_OK);
        }

        THEN("Closing the other one releases the UART")
        {
            CHECK(ezUart_Close(&second) == EZ_DRV_STATUS_OK);
            CHECK(HwDeinitialize_fake.call_count == 1U);
            CHECK(g_drv.open_count == 0U);
        }
    }

    WHEN("A handle with an operation in flight is closed")
    {
        OpenHandle(&first);
        StartAsyncTransmit(&first);

        THEN("The close is refused until the operation ends")
        {
            CHECK(ezUart_Close(&first) == EZ_DRV_STATUS_BUSY);
            CHECK(first.drv_handle.hw == &g_drv);
            CHECK(HwDeinitialize_fake.call_count == 0U);
            SimulateIsrDone(UART_EVENT_TX_CMPLT);
            CHECK(ezUart_Close(&first) == EZ_DRV_STATUS_OK);
        }
    }

    WHEN("Another handle is closed while a handle has an operation in flight")
    {
        OpenHandle(&first);
        OpenHandle(&second);
        StartAsyncTransmit(&first);

        THEN("It is allowed and does not disturb the operation")
        {
            CHECK(ezUart_Close(&second) == EZ_DRV_STATUS_OK);
            CHECK(IsBusy() == true);
            CHECK(HwDeinitialize_fake.call_count == 0U);
        }
    }

    WHEN("The HW driver fails to release the UART")
    {
        OpenHandle(&first);
        HwDeinitialize_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;

        THEN("The status is returned and the handle is closed")
        {
            CHECK(ezUart_Close(&first) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(g_drv.open_count == 0U);
        }
    }

    WHEN("Two handles start async operations one after the other")
    {
        int ctx_first = 0;
        int ctx_second = 0;
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezUart_Open(&first, "uart_mock", OnEvent, &ctx_first) == EZ_DRV_STATUS_OK);
        REQUIRE(ezUart_Open(&second, "uart_mock", OnEvent, &ctx_second) == EZ_DRV_STATUS_OK);

        THEN("The end event goes to the callback of the handle which started the operation")
        {
            REQUIRE(ezUart_TransmitAsync(&second, g_buf, 1) == EZ_DRV_STATUS_OK);
            SimulateIsrDone(UART_EVENT_TX_CMPLT);
            REQUIRE(OnEvent_fake.call_count == 1U);
            CHECK(OnEvent_fake.arg0_val == &ctx_second);
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test operations on an unusable handle", "[hal][uart]")
{
    uint8_t rx[2] = {0};
    struct ezUartConfiguration *cfg = NULL;

    WHEN("The handle is NULL or not open")
    {
        THEN("Every API reports driver-not-found")
        {
            CHECK(ezUart_TransmitAsync(&first, g_buf, 1) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_ReceiveAsync(&first, rx, 1) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_TransmitSync(&first, g_buf, 1, 5) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_ReceiveSync(&first, rx, 1, 5) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_TransmitSync(NULL, g_buf, 1, 5) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_UpdateConfig(&first) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_GetConfig(&first, &cfg) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezUart_GetConfig(NULL, &cfg) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(HwTransmitAsync_fake.call_count == 0U);
            CHECK(HwReceiveAsync_fake.call_count == 0U);
            CHECK(HwTransmitSync_fake.call_count == 0U);
            CHECK(HwReceiveSync_fake.call_count == 0U);
            CHECK(IsBusy() == false);
            CHECK(cfg == NULL);
        }
    }

    WHEN("The handle was closed")
    {
        OpenHandle(&first);
        REQUIRE(ezUart_Close(&first) == EZ_DRV_STATUS_OK);

        THEN("Operations report driver-not-found again")
        {
            CHECK(ezUart_TransmitSync(&first, g_buf, 1, 5) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(HwTransmitSync_fake.call_count == 0U);
        }
    }

    WHEN("The handle is open")
    {
        OpenHandle(&first);

        THEN("The configuration can be read and applied")
        {
            HwUpdateConfig_fake.return_val = EZ_DRV_STATUS_OK;
            CHECK(ezUart_GetConfig(&first, &cfg) == EZ_DRV_STATUS_OK);
            CHECK(ezUart_UpdateConfig(&first) == EZ_DRV_STATUS_OK);
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test configuration", "[hal][uart]")
{
    struct ezUartConfiguration *cfg = NULL;
    REQUIRE(ezUart_Open(&first, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);

    WHEN("The configuration is requested with a NULL pointer")
    {
        THEN("The API reports an argument error")
        {
            CHECK(ezUart_GetConfig(&first, NULL) == EZ_DRV_STATUS_ERR_ARG);
        }
    }

    WHEN("The configuration is requested")
    {
        THEN("The pointer refers to the configuration of the driver")
        {
            REQUIRE(ezUart_GetConfig(&first, &cfg) == EZ_DRV_STATUS_OK);
            CHECK(cfg == &g_drv.config);
            cfg->baudrate = 115200U;
            CHECK(g_drv.config.baudrate == 115200U);
        }
    }

    WHEN("The configuration is applied")
    {
        HwUpdateConfig_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The HW is called with its token and the driver is idle")
        {
            CHECK(ezUart_UpdateConfig(&first) == EZ_DRV_STATUS_OK);
            CHECK(HwUpdateConfig_fake.call_count == 1U);
            CHECK(HwUpdateConfig_fake.arg0_val == &g_hw_token);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW fails to apply the configuration")
    {
        HwUpdateConfig_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;

        THEN("The failure is returned and the driver is idle")
        {
            CHECK(ezUart_UpdateConfig(&first) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW driver has no update function")
    {
        g_drv.interface.update_conf = NULL;

        THEN("The API reports interface-not-exist")
        {
            CHECK(ezUart_UpdateConfig(&first) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("An operation is in flight")
    {
        OpenHandle(&second);
        StartAsyncTransmit(&second);

        THEN("UpdateConfig reports busy, GetConfig still works")
        {
            CHECK(ezUart_UpdateConfig(&first) == EZ_DRV_STATUS_BUSY);
            CHECK(HwUpdateConfig_fake.call_count == 0U);
            CHECK(ezUart_GetConfig(&first, &cfg) == EZ_DRV_STATUS_OK);
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test sync transmit and receive", "[hal][uart]")
{
    uint8_t rx[3] = {0};
    OpenHandle(&first);

    WHEN("A sync transmit succeeds")
    {
        HwTransmitSync_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The arguments are forwarded and the driver is idle afterwards")
        {
            CHECK(ezUart_TransmitSync(&first, g_buf, 4, 25) == EZ_DRV_STATUS_OK);
            CHECK(HwTransmitSync_fake.arg0_val == &g_hw_token);
            CHECK(HwTransmitSync_fake.arg1_val == g_buf);
            CHECK(HwTransmitSync_fake.arg2_val == 4U);
            CHECK(HwTransmitSync_fake.arg3_val == 25U);
            CHECK(IsBusy() == false);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }

    WHEN("A sync receive succeeds")
    {
        HwReceiveSync_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The arguments are forwarded and the driver is idle afterwards")
        {
            CHECK(ezUart_ReceiveSync(&first, rx, 3, 30) == EZ_DRV_STATUS_OK);
            CHECK(HwReceiveSync_fake.arg0_val == &g_hw_token);
            CHECK(HwReceiveSync_fake.arg1_val == rx);
            CHECK(HwReceiveSync_fake.arg2_val == 3U);
            CHECK(HwReceiveSync_fake.arg3_val == 30U);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW reports an error or a timeout")
    {
        THEN("The status is returned unchanged and the driver is idle")
        {
            HwTransmitSync_fake.return_val = EZ_DRV_STATUS_TIMEOUT;
            CHECK(ezUart_TransmitSync(&first, g_buf, 4, 5) == EZ_DRV_STATUS_TIMEOUT);
            CHECK(IsBusy() == false);
            HwReceiveSync_fake.return_val = EZ_DRV_STATUS_ERR_ARG;
            CHECK(ezUart_ReceiveSync(&first, rx, 3, 5) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW driver has no sync functions")
    {
        g_drv.interface.transmit_sync = NULL;
        g_drv.interface.receive_sync = NULL;

        THEN("The APIs report interface-not-exist and release the driver")
        {
            CHECK(ezUart_TransmitSync(&first, g_buf, 4, 5) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
            CHECK(ezUart_ReceiveSync(&first, rx, 3, 5) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("An async operation is in flight")
    {
        StartAsyncTransmit(&first);

        THEN("The sync APIs report busy and the HW is not called")
        {
            CHECK(ezUart_TransmitSync(&first, g_buf, 4, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(ezUart_ReceiveSync(&first, rx, 3, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(HwTransmitSync_fake.call_count == 0U);
            CHECK(HwReceiveSync_fake.call_count == 0U);
            CHECK(IsBusy() == true);
        }
    }

    WHEN("A sync operation runs")
    {
        HwReceiveSync_fake.custom_fake = HwReceiveSyncRecordState;

        THEN("The HW function sees BUSY without ASYNC, and the driver is idle afterwards")
        {
            CHECK(ezUart_ReceiveSync(&first, rx, 3, 5) == EZ_DRV_STATUS_OK);
            CHECK(g_state_in_hw == EZ_DRV_STATE_BUSY);
            CHECK(g_drv.base.state == 0U);
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test async transmit and receive", "[hal][uart]")
{
    int user_ctx = 0;
    uint8_t rx[2] = {0};
    OpenHandle(&first, OnEvent, &user_ctx);
    HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
    HwReceiveAsync_fake.return_val = EZ_DRV_STATUS_OK;

    WHEN("A transmit is started successfully")
    {
        REQUIRE(ezUart_TransmitAsync(&first, g_buf, 4) == EZ_DRV_STATUS_OK);

        THEN("The arguments are forwarded and the driver stays busy")
        {
            CHECK(HwTransmitAsync_fake.arg0_val == &g_hw_token);
            CHECK(HwTransmitAsync_fake.arg1_val == g_buf);
            CHECK(HwTransmitAsync_fake.arg2_val == 4U);
            CHECK(IsBusy() == true);
        }

        THEN("Every other operation, even from the same handle, reports busy")
        {
            CHECK(ezUart_TransmitAsync(&first, g_buf, 1) == EZ_DRV_STATUS_BUSY);
            CHECK(ezUart_ReceiveAsync(&first, rx, 1) == EZ_DRV_STATUS_BUSY);
            CHECK(ezUart_TransmitSync(&first, g_buf, 1, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(ezUart_ReceiveSync(&first, rx, 1, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(ezUart_Open(&second, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_BUSY);
            CHECK(HwTransmitAsync_fake.call_count == 1U);
            CHECK(IsBusy() == true);
        }

        AND_WHEN("The HW driver reports the end of the operation")
        {
            SimulateIsrDone(UART_EVENT_TX_CMPLT);

            THEN("The driver is released and the callback gets the context and the event")
            {
                CHECK(IsBusy() == false);
                REQUIRE(OnEvent_fake.call_count == 1U);
                CHECK(OnEvent_fake.arg0_val == &user_ctx);
                CHECK(OnEvent_fake.arg1_val == UART_EVENT_TX_CMPLT);
            }

            THEN("A new operation can be started")
            {
                CHECK(ezUart_ReceiveAsync(&first, rx, 1) == EZ_DRV_STATUS_OK);
            }
        }
    }

    WHEN("A receive is started successfully")
    {
        REQUIRE(ezUart_ReceiveAsync(&first, rx, 2) == EZ_DRV_STATUS_OK);

        THEN("The arguments are forwarded and the driver stays busy until the end event")
        {
            CHECK(HwReceiveAsync_fake.arg0_val == &g_hw_token);
            CHECK(HwReceiveAsync_fake.arg1_val == rx);
            CHECK(HwReceiveAsync_fake.arg2_val == 2U);
            CHECK(IsBusy() == true);
            SimulateIsrDone(UART_EVENT_RX_CMPLT);
            CHECK(IsBusy() == false);
            REQUIRE(OnEvent_fake.call_count == 1U);
            CHECK(OnEvent_fake.arg1_val == UART_EVENT_RX_CMPLT);
        }
    }

    WHEN("An async operation fails to start or is missing")
    {
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;
        g_drv.interface.receive_async = NULL;

        THEN("The driver is released and no callback is expected")
        {
            CHECK(ezUart_TransmitAsync(&first, g_buf, 1) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(IsBusy() == false);
            CHECK(ezUart_ReceiveAsync(&first, rx, 1) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }

    WHEN("The transmit function is missing")
    {
        g_drv.interface.transmit_async = NULL;

        THEN("The API reports interface-not-exist and releases the driver")
        {
            CHECK(ezUart_TransmitAsync(&first, g_buf, 1) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("Another handle has an operation in flight")
    {
        REQUIRE(ezUart_Open(&second, "uart_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
        StartAsyncTransmit(&second);

        THEN("The API reports busy and the HW is not called again")
        {
            CHECK(ezUart_ReceiveAsync(&first, rx, 1) == EZ_DRV_STATUS_BUSY);
            CHECK(HwReceiveAsync_fake.call_count == 0U);
        }
    }

    WHEN("The driver is released before the callback runs")
    {
        OnEvent_fake.custom_fake = OnEventRecordBusy;
        REQUIRE(ezUart_TransmitAsync(&first, g_buf, 1) == EZ_DRV_STATUS_OK);
        g_busy_in_callback = true;
        SimulateIsrDone(UART_EVENT_TX_CMPLT);

        THEN("The callback sees the driver as idle")
        {
            CHECK(OnEvent_fake.call_count == 1U);
            CHECK(g_busy_in_callback == false);
        }
    }

    WHEN("The callback starts the next operation")
    {
        OnEvent_fake.custom_fake = OnEventChain;
        g_chain_handle = &first;
        REQUIRE(ezUart_TransmitAsync(&first, g_buf, 1) == EZ_DRV_STATUS_OK);
        SimulateIsrDone(UART_EVENT_TX_CMPLT);

        THEN("The chained operation is accepted and the driver is busy again")
        {
            CHECK(g_chained_status == EZ_DRV_STATUS_OK);
            CHECK(HwTransmitAsync_fake.call_count == 2U);
            CHECK(IsBusy() == true);
        }
    }

    WHEN("The end of the operation is reported with no callback registered")
    {
        ezUartHandle_t quiet;
        memset(&quiet, 0, sizeof(quiet));
        REQUIRE(ezUart_Open(&quiet, "uart_mock", NULL, NULL) == EZ_DRV_STATUS_OK);
        REQUIRE(ezUart_TransmitAsync(&quiet, g_buf, 1) == EZ_DRV_STATUS_OK);
        SimulateIsrDone(UART_EVENT_TX_ERR);

        THEN("The driver is released and nothing crashes")
        {
            CHECK(IsBusy() == false);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }

    WHEN("An event arrives with no operation in flight")
    {
        SimulateIsrDone(UART_EVENT_TX_CMPLT);

        THEN("It is dropped")
        {
            CHECK(OnEvent_fake.call_count == 0U);
            CHECK(IsBusy() == false);
        }
    }
}


TEST_CASE_METHOD(UartTestFixture, "Test events only belong to async operations", "[hal][uart]")
{
    int user_ctx = 0;
    OpenHandle(&first, OnEvent, &user_ctx);

    WHEN("A sync HW function reports PROGRESS and DONE events")
    {
        HwTransmitSync_fake.custom_fake = HwTransmitSyncNotifies;

        THEN("The events are dropped and the status is the return value")
        {
            CHECK(ezUart_TransmitSync(&first, g_buf, 4, 10) == EZ_DRV_STATUS_OK);
            CHECK(g_state_in_hw == EZ_DRV_STATE_BUSY);
            CHECK(OnEvent_fake.call_count == 0U);
            CHECK(g_drv.base.state == 0U);
        }
    }

    WHEN("A sync HW function reports DONE and the callback would start an async operation")
    {
        HwTransmitSync_fake.custom_fake = HwTransmitSyncNotifies;
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        OnEvent_fake.custom_fake = OnEventChain;
        g_chain_handle = &first;

        THEN("The callback is not called, so no operation is started and the driver ends idle")
        {
            CHECK(ezUart_TransmitSync(&first, g_buf, 4, 10) == EZ_DRV_STATUS_OK);
            CHECK(OnEvent_fake.call_count == 0U);
            CHECK(HwTransmitAsync_fake.call_count == 0U);
            CHECK(g_drv.base.state == 0U);
        }
    }

    WHEN("An async operation runs")
    {
        HwTransmitAsync_fake.custom_fake = HwTransmitAsyncRecordState;
        REQUIRE(ezUart_TransmitAsync(&first, g_buf, 4) == EZ_DRV_STATUS_OK);

        THEN("The HW function sees BUSY and ASYNC, and both stay set")
        {
            CHECK(g_state_in_hw == (EZ_DRV_STATE_BUSY | EZ_DRV_STATE_ASYNC));
            CHECK(g_drv.base.state == (EZ_DRV_STATE_BUSY | EZ_DRV_STATE_ASYNC));
        }

        AND_WHEN("The HW driver reports a PROGRESS event")
        {
            SimulateIsrProgress(UART_EVENT_TX_CMPLT);

            THEN("The callback is called and the operation is still in flight")
            {
                CHECK(OnEvent_fake.call_count == 1U);
                CHECK(OnEvent_fake.arg0_val == &user_ctx);
                CHECK(g_drv.base.state == (EZ_DRV_STATE_BUSY | EZ_DRV_STATE_ASYNC));
            }
        }

        AND_WHEN("The HW driver reports the DONE event")
        {
            SimulateIsrDone(UART_EVENT_TX_CMPLT);

            THEN("BUSY and ASYNC are both cleared")
            {
                CHECK(OnEvent_fake.call_count == 1U);
                CHECK(g_drv.base.state == 0U);
            }
        }
    }

    WHEN("An async operation fails to start")
    {
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;

        THEN("BUSY and ASYNC are both cleared")
        {
            CHECK(ezUart_TransmitAsync(&first, g_buf, 4) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(g_drv.base.state == 0U);
        }
    }

    WHEN("A sync operation follows an async operation")
    {
        HwReceiveSync_fake.custom_fake = HwReceiveSyncRecordState;
        StartAsyncTransmit(&first);
        SimulateIsrDone(UART_EVENT_TX_CMPLT);

        THEN("The sync operation does not inherit the ASYNC bit")
        {
            uint8_t rx[1] = {0};
            CHECK(ezUart_ReceiveSync(&first, rx, 1, 10) == EZ_DRV_STATUS_OK);
            CHECK(g_state_in_hw == EZ_DRV_STATE_BUSY);
            CHECK(OnEvent_fake.call_count == 1U);
        }
    }
}

/* End of file */
