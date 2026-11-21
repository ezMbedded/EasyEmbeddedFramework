/*****************************************************************************
* Filename:         unittest_ez_spi.cpp
* Author:           Hai Nguyen
* Original Date:    07.11.2026
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   unittest_ez_spi.cpp
 *  @author Hai Nguyen
 *  @date   07.11.2026
 *  @brief  Unit tests for the SPI HAL component
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
#include "ez_spi.h"
#include "fff.h"

#include <catch2/catch_test_macros.hpp>


/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwInitialize, void *);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwDeinitialize, void *);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwLock, void *, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwUnlock, void *);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTransferSync, void *, const uint8_t *, size_t, uint8_t *, size_t, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTransferAsync, void *, const uint8_t *, size_t, uint8_t *, size_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTransmitSync, void *, const uint8_t *, size_t, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTransmitAsync, void *, const uint8_t *, size_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwReceiveSync, void *, uint8_t *, size_t, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwReceiveAsync, void *, uint8_t *, size_t);
FAKE_VOID_FUNC(OnEvent, void *, uint8_t, void *, void *);


/******************************************************************************
* Module Typedefs
*******************************************************************************/
/* None */


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static struct ezSpiDriver g_drv;
static int g_hw_token = 0;              /* value of driver_h given to the HW functions */
static bool g_busy_in_callback = false; /* BUSY bit seen inside the callback */
static EZ_DRV_STATUS g_chained_status = EZ_DRV_STATUS_OK;
static ezSpiHandle_t *g_chain_handle = NULL;
static uint8_t g_buf[4] = {1, 2, 3, 4};
static uint8_t g_state_in_hw = 0U;      /* state byte seen inside a HW function */


/******************************************************************************
* Function Definitions
*******************************************************************************/
static void OnEventRecordBusy(void *ctx, uint8_t event_code, void *p1, void *p2)
{
    (void)ctx;
    (void)event_code;
    (void)p1;
    (void)p2;
    g_busy_in_callback = EZ_DRV_STATE_IS_SET(&g_drv.base, EZ_DRV_STATE_BUSY);
}

static void OnEventChain(void *ctx, uint8_t event_code, void *p1, void *p2)
{
    (void)ctx;
    (void)event_code;
    (void)p1;
    (void)p2;
    g_chained_status = ezSpi_TransmitAsync(g_chain_handle, g_buf, sizeof(g_buf));
}

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

/* A sync HW function which breaks the rule and reports events. */
static EZ_DRV_STATUS HwTransmitSyncNotifies(void *driver_h, const uint8_t *tx_data, size_t length, uint32_t timeout_millis)
{
    (void)driver_h;
    (void)tx_data;
    (void)length;
    (void)timeout_millis;
    g_state_in_hw = g_drv.base.state;
    SimulateIsrProgress(EZ_SPI_EVENT_CMPLT);
    SimulateIsrDone(EZ_SPI_EVENT_CMPLT);
    return EZ_DRV_STATUS_OK;
}

/* Record the state bits seen by the HW function. */
static EZ_DRV_STATUS HwTransmitSyncRecordState(void *driver_h, const uint8_t *tx_data, size_t length, uint32_t timeout_millis)
{
    (void)driver_h;
    (void)tx_data;
    (void)length;
    (void)timeout_millis;
    g_state_in_hw = g_drv.base.state;
    return EZ_DRV_STATUS_OK;
}

static EZ_DRV_STATUS HwTransmitAsyncRecordState(void *driver_h, const uint8_t *tx_data, size_t length)
{
    (void)driver_h;
    (void)tx_data;
    (void)length;
    g_state_in_hw = g_drv.base.state;
    return EZ_DRV_STATUS_OK;
}


/******************************************************************************
* Fixture
*******************************************************************************/
struct SpiTestFixture
{
    SpiTestFixture()
    {
        RESET_FAKE(HwInitialize);
        RESET_FAKE(HwDeinitialize);
        RESET_FAKE(HwLock);
        RESET_FAKE(HwUnlock);
        RESET_FAKE(HwTransferSync);
        RESET_FAKE(HwTransferAsync);
        RESET_FAKE(HwTransmitSync);
        RESET_FAKE(HwTransmitAsync);
        RESET_FAKE(HwReceiveSync);
        RESET_FAKE(HwReceiveAsync);
        RESET_FAKE(OnEvent);
        FFF_RESET_HISTORY();

        g_busy_in_callback = false;
        g_state_in_hw = 0U;
        g_chained_status = EZ_DRV_STATUS_OK;
        g_chain_handle = NULL;

        memset(&g_drv, 0, sizeof(g_drv));
        memset(&first, 0, sizeof(first));
        memset(&second, 0, sizeof(second));
        g_drv.base.name = "spi_mock";
        g_drv.interface.driver_h = &g_hw_token;
        g_drv.interface.initialize = HwInitialize;
        g_drv.interface.deinitialize = HwDeinitialize;
        /* lock and unlock stay NULL: the bus lock is a try-lock, tests which need the HW lock set them */
        g_drv.interface.transfer_sync = HwTransferSync;
        g_drv.interface.transfer_async = HwTransferAsync;
        g_drv.interface.transmit_sync = HwTransmitSync;
        g_drv.interface.transmit_async = HwTransmitAsync;
        g_drv.interface.receive_sync = HwReceiveSync;
        g_drv.interface.receive_async = HwReceiveAsync;

        REQUIRE(ezSpi_SystemRegisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
        driver_registered = true;
    }

    ~SpiTestFixture()
    {
        if(driver_registered)
        {
            /* Make sure a test that leaves an operation in flight does not leak.
             * The callback must not start a new operation here. */
            OnEvent_fake.custom_fake = NULL;
            g_chain_handle = NULL;
            ezDriver_NotifyEvent(&g_drv.base, EZ_DRV_EVENT_OP_DONE, 0, NULL, NULL);
            /* Handles left open by a test: the driver must be unregistrable, or it stays in the list */
            g_drv.open_count = 0U;
            g_drv.lock_owner = NULL;
            (void)ezSpi_SystemUnregisterHwDriver(&g_drv);
        }
    }

    /* Open a handle on the mock driver. */
    void OpenHandle(ezSpiHandle_t *handle, ezDrvCallback cb = OnEvent, void *ctx = NULL)
    {
        REQUIRE(ezSpi_Open(handle, "spi_mock", cb, ctx) == EZ_DRV_STATUS_OK);
    }

    ezSpiHandle_t first;
    ezSpiHandle_t second;
    bool driver_registered;
};


/******************************************************************************
* External functions
*******************************************************************************/
TEST_CASE_METHOD(SpiTestFixture, "Test HW driver registration", "[hal][spi]")
{
    WHEN("Invalid arguments are used")
    {
        struct ezSpiDriver no_name;
        memset(&no_name, 0, sizeof(no_name));

        THEN("The APIs report an argument error")
        {
            CHECK(ezSpi_SystemRegisterHwDriver(NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezSpi_SystemRegisterHwDriver(&no_name) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezSpi_SystemUnregisterHwDriver(NULL) == EZ_DRV_STATUS_ERR_ARG);
        }
    }

    WHEN("A driver with the same name is registered twice")
    {
        struct ezSpiDriver duplicate;
        memset(&duplicate, 0, sizeof(duplicate));
        duplicate.base.name = "spi_mock";

        THEN("The second registration is rejected")
        {
            CHECK(ezSpi_SystemRegisterHwDriver(&duplicate) == EZ_DRV_STATUS_ERR_ARG);
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

    WHEN("A driver sets only one of the lock and unlock functions")
    {
        struct ezSpiDriver half;
        memset(&half, 0, sizeof(half));
        half.base.name = "spi_half";

        THEN("The registration is rejected, and accepted with both or none")
        {
            half.interface.lock = HwLock;
            CHECK(ezSpi_SystemRegisterHwDriver(&half) == EZ_DRV_STATUS_ERR_ARG);
            half.interface.lock = NULL;
            half.interface.unlock = HwUnlock;
            CHECK(ezSpi_SystemRegisterHwDriver(&half) == EZ_DRV_STATUS_ERR_ARG);
            half.interface.lock = HwLock;
            CHECK(ezSpi_SystemRegisterHwDriver(&half) == EZ_DRV_STATUS_OK);
            CHECK(ezSpi_SystemUnregisterHwDriver(&half) == EZ_DRV_STATUS_OK);
        }
    }

    WHEN("A driver is registered")
    {
        THEN("It starts with no bus lock owner")
        {
            CHECK(g_drv.lock_owner == NULL);
        }
    }

    WHEN("A driver is unregistered")
    {
        THEN("It is removed from the list")
        {
            CHECK(ezSpi_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
            driver_registered = false;
            CHECK(ezSpi_Open(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("A driver is unregistered while a handle is open")
    {
        OpenHandle(&first);

        THEN("The unregistration is refused until the handle is closed")
        {
            CHECK(ezSpi_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_BUSY);
            REQUIRE(ezSpi_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(ezSpi_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
            driver_registered = false;
        }
    }

    WHEN("A driver is unregistered while an operation is in flight")
    {
        OpenHandle(&first);
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezSpi_TransmitAsync(&first, g_buf, sizeof(g_buf)) == EZ_DRV_STATUS_OK);

        THEN("The unregistration is refused")
        {
            CHECK(ezSpi_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_BUSY);
        }
    }
}


TEST_CASE_METHOD(SpiTestFixture, "Test open and close", "[hal][spi]")
{
    WHEN("Invalid arguments are used")
    {
        THEN("The APIs report an argument error")
        {
            CHECK(ezSpi_Open(NULL, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezSpi_Open(&first, NULL, OnEvent, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezSpi_Open(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(ezSpi_Open(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezSpi_Close(NULL) == EZ_DRV_STATUS_ERR_ARG);
        }
    }

    WHEN("A handle which was never opened is closed")
    {
        THEN("The call succeeds, the handle stays empty and the HW is not called")
        {
            CHECK(ezSpi_Close(&first) == EZ_DRV_STATUS_OK);
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
            CHECK(ezSpi_Open(&first, "unknown", OnEvent, NULL) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(HwInitialize_fake.call_count == 0U);
            CHECK(first.drv_handle.hw == NULL);
        }
    }

    WHEN("The HW driver has no initialize or deinitialize function (the peripheral is already configured)")
    {
        g_drv.interface.initialize = NULL;
        g_drv.interface.deinitialize = NULL;

        THEN("The open only binds the handle, and the close only unbinds it")
        {
            REQUIRE(ezSpi_Open(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == &g_drv);
            CHECK(g_drv.open_count == 1U);
            CHECK(HwInitialize_fake.call_count == 0U);
            CHECK(ezSpi_Close(&first) == EZ_DRV_STATUS_OK);
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
            REQUIRE(ezSpi_Open(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(HwInitialize_fake.call_count == 1U);
            CHECK(ezSpi_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(HwDeinitialize_fake.call_count == 0U);
        }
    }

    WHEN("The handle is opened")
    {
        int user_ctx = 0;
        REQUIRE(ezSpi_Open(&first, "spi_mock", OnEvent, &user_ctx) == EZ_DRV_STATUS_OK);

        THEN("The HW gets driver_h, once")
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
            CHECK(ezSpi_Open(&first, "spi_mock", NULL, NULL) == EZ_DRV_STATUS_ERR_ARG);
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
            CHECK(ezSpi_Open(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(g_drv.open_count == 0U);
            CHECK(IsBusy() == false);
        }

        THEN("The same handle can be opened again once the failure is gone")
        {
            CHECK(ezSpi_Open(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_GENERIC);
            HwInitialize_fake.return_val = EZ_DRV_STATUS_OK;
            CHECK(ezSpi_Open(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(HwInitialize_fake.call_count == 2U);
            CHECK(g_drv.open_count == 1U);
        }
    }

    WHEN("A second handle is opened")
    {
        OpenHandle(&first);
        REQUIRE(ezSpi_Open(&second, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);

        THEN("The bus is not initialized again")
        {
            CHECK(HwInitialize_fake.call_count == 1U);
            CHECK(g_drv.open_count == 2U);
            CHECK(second.drv_handle.hw == &g_drv);
        }
    }

    WHEN("A handle is opened while an operation is in flight")
    {
        OpenHandle(&first);
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezSpi_TransmitAsync(&first, g_buf, sizeof(g_buf)) == EZ_DRV_STATUS_OK);

        THEN("The open is refused and the operation is not disturbed")
        {
            CHECK(ezSpi_Open(&second, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_BUSY);
            CHECK(second.drv_handle.hw == NULL);
            CHECK(IsBusy() == true);
            SimulateIsrDone(EZ_SPI_EVENT_CMPLT);
            CHECK(OnEvent_fake.call_count == 1U);
        }
    }

    WHEN("The maximum number of handles is open")
    {
        g_drv.open_count = UINT8_MAX;

        THEN("The open fails and the HW is not called")
        {
            CHECK(ezSpi_Open(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(HwInitialize_fake.call_count == 0U);
            CHECK(g_drv.open_count == UINT8_MAX);
        }
    }

    WHEN("The only open handle is closed")
    {
        OpenHandle(&first);

        THEN("The bus is released and everything is cleared")
        {
            CHECK(ezSpi_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(HwDeinitialize_fake.call_count == 1U);
            CHECK(HwDeinitialize_fake.arg0_val == &g_hw_token);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(first.drv_handle.on_event == NULL);
            CHECK(first.drv_handle.ctx == NULL);
            CHECK(g_drv.open_count == 0U);
        }

        THEN("The bus can be opened again afterwards and is initialized again")
        {
            REQUIRE(ezSpi_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(ezSpi_Open(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(HwInitialize_fake.call_count == 2U);
        }
    }

    WHEN("One of two open handles is closed")
    {
        OpenHandle(&first);
        OpenHandle(&second);
        REQUIRE(ezSpi_Close(&first) == EZ_DRV_STATUS_OK);

        THEN("The bus stays up and the other handle keeps working")
        {
            CHECK(HwDeinitialize_fake.call_count == 0U);
            CHECK(g_drv.open_count == 1U);
            HwTransmitSync_fake.return_val = EZ_DRV_STATUS_OK;
            CHECK(ezSpi_TransmitSync(&second, g_buf, 1, 5) == EZ_DRV_STATUS_OK);
        }

        THEN("Closing the other one releases the bus")
        {
            CHECK(ezSpi_Close(&second) == EZ_DRV_STATUS_OK);
            CHECK(HwDeinitialize_fake.call_count == 1U);
            CHECK(g_drv.open_count == 0U);
        }
    }

    WHEN("A handle with an operation in flight is closed")
    {
        OpenHandle(&first);
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezSpi_TransmitAsync(&first, g_buf, sizeof(g_buf)) == EZ_DRV_STATUS_OK);

        THEN("The close is refused until the operation ends")
        {
            CHECK(ezSpi_Close(&first) == EZ_DRV_STATUS_BUSY);
            CHECK(first.drv_handle.hw == &g_drv);
            CHECK(HwDeinitialize_fake.call_count == 0U);
            SimulateIsrDone(EZ_SPI_EVENT_CMPLT);
            CHECK(ezSpi_Close(&first) == EZ_DRV_STATUS_OK);
        }
    }

    WHEN("Another handle is closed while a handle has an operation in flight")
    {
        OpenHandle(&first);
        OpenHandle(&second);
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezSpi_TransmitAsync(&first, g_buf, sizeof(g_buf)) == EZ_DRV_STATUS_OK);

        THEN("It is allowed and does not disturb the operation")
        {
            CHECK(ezSpi_Close(&second) == EZ_DRV_STATUS_OK);
            CHECK(IsBusy() == true);
            CHECK(HwDeinitialize_fake.call_count == 0U);
        }
    }

    WHEN("The HW driver fails to release the bus")
    {
        OpenHandle(&first);
        HwDeinitialize_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;

        THEN("The status is returned and the handle is closed")
        {
            CHECK(ezSpi_Close(&first) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(g_drv.open_count == 0U);
        }
    }

    WHEN("Two handles start async operations one after the other")
    {
        int ctx_first = 0;
        int ctx_second = 0;
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezSpi_Open(&first, "spi_mock", OnEvent, &ctx_first) == EZ_DRV_STATUS_OK);
        REQUIRE(ezSpi_Open(&second, "spi_mock", OnEvent, &ctx_second) == EZ_DRV_STATUS_OK);

        THEN("The end event goes to the callback of the handle which started the operation")
        {
            REQUIRE(ezSpi_TransmitAsync(&second, g_buf, 1) == EZ_DRV_STATUS_OK);
            SimulateIsrDone(EZ_SPI_EVENT_CMPLT);
            REQUIRE(OnEvent_fake.call_count == 1U);
            CHECK(OnEvent_fake.arg0_val == &ctx_second);
        }
    }
}


TEST_CASE_METHOD(SpiTestFixture, "Test operations require an open handle", "[hal][spi]")
{
    WHEN("The handle was never opened")
    {
        THEN("Every operation reports driver-not-found and the HW is not called")
        {
            CHECK(ezSpi_TransferSync(&first, g_buf, 1, g_buf, 1, 10) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezSpi_TransferAsync(&first, g_buf, 1, g_buf, 1) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezSpi_TransmitSync(&first, g_buf, 1, 10) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezSpi_TransmitAsync(&first, g_buf, 1) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezSpi_ReceiveSync(&first, g_buf, 1, 10) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezSpi_ReceiveAsync(&first, g_buf, 1) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(HwTransferSync_fake.call_count == 0U);
            CHECK(HwTransmitAsync_fake.call_count == 0U);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The handle is NULL")
    {
        THEN("Every operation reports driver-not-found")
        {
            CHECK(ezSpi_TransferSync(NULL, g_buf, 1, g_buf, 1, 10) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezSpi_TransmitAsync(NULL, g_buf, 1) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("The handle was closed")
    {
        OpenHandle(&first);
        REQUIRE(ezSpi_Close(&first) == EZ_DRV_STATUS_OK);

        THEN("Operations report driver-not-found again")
        {
            CHECK(ezSpi_TransmitSync(&first, g_buf, 1, 10) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(HwTransmitSync_fake.call_count == 0U);
        }
    }
}


TEST_CASE_METHOD(SpiTestFixture, "Test bus lock", "[hal][spi]")
{
    OpenHandle(&first);
    OpenHandle(&second);
    HwTransmitSync_fake.return_val = EZ_DRV_STATUS_OK;

    WHEN("The HW driver has no lock functions (no RTOS)")
    {
        THEN("The lock is a try-lock which ignores the timeout")
        {
            CHECK(ezSpi_LockBus(&first, 1000U) == EZ_DRV_STATUS_OK);
            CHECK(g_drv.lock_owner == &first.drv_handle);
            CHECK(ezSpi_LockBus(&second, 1000U) == EZ_DRV_STATUS_BUSY);
            CHECK(g_drv.lock_owner == &first.drv_handle);
            CHECK(ezSpi_UnlockBus(&first) == EZ_DRV_STATUS_OK);
            CHECK(g_drv.lock_owner == NULL);
            CHECK(ezSpi_LockBus(&second, 0U) == EZ_DRV_STATUS_OK);
            CHECK(g_drv.lock_owner == &second.drv_handle);
        }
    }

    WHEN("The HW driver has lock functions")
    {
        g_drv.interface.lock = HwLock;
        g_drv.interface.unlock = HwUnlock;

        THEN("The HW lock gets the timeout and the handle becomes the owner")
        {
            CHECK(ezSpi_LockBus(&first, 25U) == EZ_DRV_STATUS_OK);
            CHECK(HwLock_fake.call_count == 1U);
            CHECK(HwLock_fake.arg0_val == &g_hw_token);
            CHECK(HwLock_fake.arg1_val == 25U);
            CHECK(g_drv.lock_owner == &first.drv_handle);
        }

        THEN("A timeout is forwarded and nobody owns the bus")
        {
            HwLock_fake.return_val = EZ_DRV_STATUS_TIMEOUT;
            CHECK(ezSpi_LockBus(&second, 25U) == EZ_DRV_STATUS_TIMEOUT);
            CHECK(g_drv.lock_owner == NULL);
        }

        THEN("The unlock frees the bus, then calls the HW unlock")
        {
            REQUIRE(ezSpi_LockBus(&first, 25U) == EZ_DRV_STATUS_OK);
            CHECK(ezSpi_UnlockBus(&first) == EZ_DRV_STATUS_OK);
            CHECK(HwUnlock_fake.call_count == 1U);
            CHECK(HwUnlock_fake.arg0_val == &g_hw_token);
            CHECK(g_drv.lock_owner == NULL);
        }

        THEN("A failing HW unlock is forwarded, the bus is free anyway")
        {
            REQUIRE(ezSpi_LockBus(&first, 25U) == EZ_DRV_STATUS_OK);
            HwUnlock_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;
            CHECK(ezSpi_UnlockBus(&first) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(g_drv.lock_owner == NULL);
        }

        THEN("Closing the holder unlocks the bus")
        {
            REQUIRE(ezSpi_LockBus(&first, 25U) == EZ_DRV_STATUS_OK);
            CHECK(ezSpi_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(HwUnlock_fake.call_count == 1U);
            CHECK(g_drv.lock_owner == NULL);
        }

        THEN("A failing HW unlock on close is logged and the close still succeeds")
        {
            REQUIRE(ezSpi_LockBus(&first, 25U) == EZ_DRV_STATUS_OK);
            HwUnlock_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;
            CHECK(ezSpi_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == NULL);
        }
    }

    WHEN("The lock is used wrongly")
    {
        THEN("The APIs report the matching error")
        {
            CHECK(ezSpi_LockBus(NULL, 10U) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezSpi_UnlockBus(NULL) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            ezSpiHandle_t never_opened;
            memset(&never_opened, 0, sizeof(never_opened));
            CHECK(ezSpi_LockBus(&never_opened, 10U) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezSpi_UnlockBus(&never_opened) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);

            CHECK(ezSpi_UnlockBus(&first) == EZ_DRV_STATUS_ERR_ARG);
            REQUIRE(ezSpi_LockBus(&first, 10U) == EZ_DRV_STATUS_OK);
            CHECK(ezSpi_LockBus(&first, 10U) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezSpi_UnlockBus(&second) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(g_drv.lock_owner == &first.drv_handle);
        }
    }

    WHEN("A handle holds the bus")
    {
        REQUIRE(ezSpi_LockBus(&first, 10U) == EZ_DRV_STATUS_OK);

        THEN("Transfers of another handle report busy and the HW is not called")
        {
            CHECK(ezSpi_TransmitSync(&second, g_buf, 1, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(ezSpi_TransmitAsync(&second, g_buf, 1) == EZ_DRV_STATUS_BUSY);
            CHECK(ezSpi_TransferSync(&second, g_buf, 1, g_buf, 1, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(ezSpi_ReceiveSync(&second, g_buf, 1, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(HwTransmitSync_fake.call_count == 0U);
            CHECK(HwTransmitAsync_fake.call_count == 0U);
            CHECK(IsBusy() == false);
        }

        THEN("The holder can transfer as many times as it wants")
        {
            CHECK(ezSpi_TransmitSync(&first, g_buf, 1, 5) == EZ_DRV_STATUS_OK);
            CHECK(ezSpi_TransmitSync(&first, g_buf, 1, 5) == EZ_DRV_STATUS_OK);
            CHECK(HwTransmitSync_fake.call_count == 2U);
        }

        THEN("The other handle can transfer after the unlock")
        {
            REQUIRE(ezSpi_UnlockBus(&first) == EZ_DRV_STATUS_OK);
            CHECK(ezSpi_TransmitSync(&second, g_buf, 1, 5) == EZ_DRV_STATUS_OK);
        }

        THEN("The lock survives an async operation of the holder")
        {
            HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
            REQUIRE(ezSpi_TransmitAsync(&first, g_buf, 1) == EZ_DRV_STATUS_OK);
            SimulateIsrDone(EZ_SPI_EVENT_CMPLT);
            CHECK(g_drv.lock_owner == &first.drv_handle);
            CHECK(ezSpi_TransmitSync(&second, g_buf, 1, 5) == EZ_DRV_STATUS_BUSY);
        }

        THEN("Closing the holder frees the bus also without HW lock functions")
        {
            CHECK(ezSpi_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(g_drv.lock_owner == NULL);
            CHECK(ezSpi_TransmitSync(&second, g_buf, 1, 5) == EZ_DRV_STATUS_OK);
        }
    }
}


TEST_CASE_METHOD(SpiTestFixture, "Test sync operations", "[hal][spi]")
{
    OpenHandle(&first);

    WHEN("TransferSync is called")
    {
        HwTransferSync_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The arguments are forwarded and the driver is released")
        {
            uint8_t rx[2] = {0};
            CHECK(ezSpi_TransferSync(&first, g_buf, 4, rx, 2, 55) == EZ_DRV_STATUS_OK);
            CHECK(HwTransferSync_fake.call_count == 1U);
            CHECK(HwTransferSync_fake.arg0_val == &g_hw_token);
            CHECK(HwTransferSync_fake.arg1_val == g_buf);
            CHECK(HwTransferSync_fake.arg2_val == 4U);
            CHECK(HwTransferSync_fake.arg3_val == rx);
            CHECK(HwTransferSync_fake.arg4_val == 2U);
            CHECK(HwTransferSync_fake.arg5_val == 55U);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("TransmitSync and ReceiveSync are called")
    {
        HwTransmitSync_fake.return_val = EZ_DRV_STATUS_OK;
        HwReceiveSync_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The arguments are forwarded and the driver is released each time")
        {
            CHECK(ezSpi_TransmitSync(&first, g_buf, 3, 20) == EZ_DRV_STATUS_OK);
            CHECK(HwTransmitSync_fake.arg1_val == g_buf);
            CHECK(HwTransmitSync_fake.arg2_val == 3U);
            CHECK(HwTransmitSync_fake.arg3_val == 20U);
            CHECK(IsBusy() == false);

            CHECK(ezSpi_ReceiveSync(&first, g_buf, 2, 30) == EZ_DRV_STATUS_OK);
            CHECK(HwReceiveSync_fake.arg1_val == g_buf);
            CHECK(HwReceiveSync_fake.arg2_val == 2U);
            CHECK(HwReceiveSync_fake.arg3_val == 30U);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW function fails or is missing")
    {
        HwTransferSync_fake.return_val = EZ_DRV_STATUS_TIMEOUT;
        g_drv.interface.transmit_sync = NULL;

        THEN("The status is forwarded and the driver is released")
        {
            CHECK(ezSpi_TransferSync(&first, g_buf, 1, g_buf, 1, 5) == EZ_DRV_STATUS_TIMEOUT);
            CHECK(IsBusy() == false);
            CHECK(ezSpi_TransmitSync(&first, g_buf, 1, 5) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("Another handle has an operation in flight")
    {
        OpenHandle(&second);
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezSpi_TransmitAsync(&second, g_buf, 1) == EZ_DRV_STATUS_OK);

        THEN("A sync operation reports busy and the HW is not called")
        {
            CHECK(ezSpi_TransferSync(&first, g_buf, 1, g_buf, 1, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(HwTransferSync_fake.call_count == 0U);
            CHECK(IsBusy() == true);
        }
    }
}


TEST_CASE_METHOD(SpiTestFixture, "Test async operations", "[hal][spi]")
{
    int user_ctx = 0;
    OpenHandle(&first, OnEvent, &user_ctx);
    HwTransferAsync_fake.return_val = EZ_DRV_STATUS_OK;
    HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
    HwReceiveAsync_fake.return_val = EZ_DRV_STATUS_OK;

    WHEN("An async operation is started successfully")
    {
        uint8_t rx[2] = {0};
        REQUIRE(ezSpi_TransferAsync(&first, g_buf, 4, rx, 2) == EZ_DRV_STATUS_OK);

        THEN("The arguments are forwarded and the driver stays busy")
        {
            CHECK(HwTransferAsync_fake.arg0_val == &g_hw_token);
            CHECK(HwTransferAsync_fake.arg1_val == g_buf);
            CHECK(HwTransferAsync_fake.arg2_val == 4U);
            CHECK(HwTransferAsync_fake.arg3_val == rx);
            CHECK(HwTransferAsync_fake.arg4_val == 2U);
            CHECK(IsBusy() == true);
        }

        THEN("Every other operation, even from the same handle, reports busy")
        {
            CHECK(ezSpi_TransmitAsync(&first, g_buf, 1) == EZ_DRV_STATUS_BUSY);
            CHECK(ezSpi_ReceiveAsync(&first, g_buf, 1) == EZ_DRV_STATUS_BUSY);
            CHECK(ezSpi_TransferSync(&first, g_buf, 1, g_buf, 1, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(ezSpi_Open(&second, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_BUSY);
            CHECK(HwTransmitAsync_fake.call_count == 0U);
            CHECK(IsBusy() == true);
        }

        AND_WHEN("The HW driver reports the end of the operation")
        {
            SimulateIsrDone(EZ_SPI_EVENT_CMPLT);

            THEN("The driver is released and the callback gets the context and the event")
            {
                CHECK(IsBusy() == false);
                REQUIRE(OnEvent_fake.call_count == 1U);
                CHECK(OnEvent_fake.arg0_val == &user_ctx);
                CHECK(OnEvent_fake.arg1_val == EZ_SPI_EVENT_CMPLT);
            }

            THEN("A new operation can be started")
            {
                CHECK(ezSpi_ReceiveAsync(&first, g_buf, 1) == EZ_DRV_STATUS_OK);
            }
        }
    }

    WHEN("An async operation fails to start or is missing")
    {
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;
        g_drv.interface.receive_async = NULL;

        THEN("The driver is released and no callback is expected")
        {
            CHECK(ezSpi_TransmitAsync(&first, g_buf, 1) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(IsBusy() == false);
            CHECK(ezSpi_ReceiveAsync(&first, g_buf, 1) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }

    WHEN("The driver is released before the callback runs")
    {
        OnEvent_fake.custom_fake = OnEventRecordBusy;
        REQUIRE(ezSpi_TransmitAsync(&first, g_buf, 1) == EZ_DRV_STATUS_OK);
        g_busy_in_callback = true;
        SimulateIsrDone(EZ_SPI_EVENT_CMPLT);

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
        REQUIRE(ezSpi_TransmitAsync(&first, g_buf, 1) == EZ_DRV_STATUS_OK);
        SimulateIsrDone(EZ_SPI_EVENT_CMPLT);

        THEN("The chained operation is accepted and the driver is busy again")
        {
            CHECK(g_chained_status == EZ_DRV_STATUS_OK);
            CHECK(HwTransmitAsync_fake.call_count == 2U);
            CHECK(IsBusy() == true);
        }
    }

    WHEN("The end of the operation is reported with no callback registered")
    {
        ezSpiHandle_t quiet;
        memset(&quiet, 0, sizeof(quiet));
        REQUIRE(ezSpi_Open(&quiet, "spi_mock", NULL, NULL) == EZ_DRV_STATUS_OK);
        REQUIRE(ezSpi_TransmitAsync(&quiet, g_buf, 1) == EZ_DRV_STATUS_OK);
        SimulateIsrDone(EZ_SPI_EVENT_ERR);

        THEN("The driver is released and nothing crashes")
        {
            CHECK(IsBusy() == false);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }

    WHEN("An event arrives with no operation in flight")
    {
        SimulateIsrDone(EZ_SPI_EVENT_CMPLT);

        THEN("It is dropped")
        {
            CHECK(OnEvent_fake.call_count == 0U);
            CHECK(IsBusy() == false);
        }
    }
}

TEST_CASE_METHOD(SpiTestFixture, "Test events only belong to async operations", "[hal][spi]")
{
    int user_ctx = 0;
    REQUIRE(ezSpi_Open(&first, "spi_mock", OnEvent, &user_ctx) == EZ_DRV_STATUS_OK);

    WHEN("A sync HW function reports PROGRESS and DONE events")
    {
        HwTransmitSync_fake.custom_fake = HwTransmitSyncNotifies;

        THEN("The events are dropped and the status is the return value")
        {
            CHECK(ezSpi_TransmitSync(&first, g_buf, 4, 10) == EZ_DRV_STATUS_OK);
            CHECK(OnEvent_fake.call_count == 0U);
            CHECK(IsBusy() == false);
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
            CHECK(ezSpi_TransmitSync(&first, g_buf, 4, 10) == EZ_DRV_STATUS_OK);
            CHECK(OnEvent_fake.call_count == 0U);
            CHECK(HwTransmitAsync_fake.call_count == 0U);
            CHECK(g_drv.base.state == 0U);
        }
    }

    WHEN("A sync operation runs")
    {
        HwTransmitSync_fake.custom_fake = HwTransmitSyncRecordState;

        THEN("The HW function sees BUSY without ASYNC, and the driver is idle afterwards")
        {
            CHECK(ezSpi_TransmitSync(&first, g_buf, 4, 10) == EZ_DRV_STATUS_OK);
            CHECK(g_state_in_hw == EZ_DRV_STATE_BUSY);
            CHECK(g_drv.base.state == 0U);
        }
    }

    WHEN("An async operation runs")
    {
        HwTransmitAsync_fake.custom_fake = HwTransmitAsyncRecordState;
        REQUIRE(ezSpi_TransmitAsync(&first, g_buf, 4) == EZ_DRV_STATUS_OK);

        THEN("The HW function sees BUSY and ASYNC, and both stay set")
        {
            CHECK(g_state_in_hw == (EZ_DRV_STATE_BUSY | EZ_DRV_STATE_ASYNC));
            CHECK(g_drv.base.state == (EZ_DRV_STATE_BUSY | EZ_DRV_STATE_ASYNC));
        }

        AND_WHEN("The HW driver reports a PROGRESS event")
        {
            SimulateIsrProgress(EZ_SPI_EVENT_CMPLT);

            THEN("The callback is called and the operation is still in flight")
            {
                CHECK(OnEvent_fake.call_count == 1U);
                CHECK(OnEvent_fake.arg0_val == &user_ctx);
                CHECK(g_drv.base.state == (EZ_DRV_STATE_BUSY | EZ_DRV_STATE_ASYNC));
            }
        }

        AND_WHEN("The HW driver reports the DONE event")
        {
            SimulateIsrDone(EZ_SPI_EVENT_CMPLT);

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
            CHECK(ezSpi_TransmitAsync(&first, g_buf, 4) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(g_drv.base.state == 0U);
        }
    }

    WHEN("A sync operation follows an async operation")
    {
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        HwTransmitSync_fake.custom_fake = HwTransmitSyncRecordState;
        REQUIRE(ezSpi_TransmitAsync(&first, g_buf, 4) == EZ_DRV_STATUS_OK);
        SimulateIsrDone(EZ_SPI_EVENT_CMPLT);

        THEN("The sync operation does not inherit the ASYNC bit")
        {
            CHECK(ezSpi_TransmitSync(&first, g_buf, 4, 10) == EZ_DRV_STATUS_OK);
            CHECK(g_state_in_hw == EZ_DRV_STATE_BUSY);
            CHECK(OnEvent_fake.call_count == 1U);
        }
    }
}

/* End of file */
