/*****************************************************************************
* Filename:         unittest_ez_i2c.cpp
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

/** @file   unittest_ez_i2c.cpp
 *  @author Hai Nguyen
 *  @date   25.04.2026
 *  @brief  Unit tests for the I2C HAL component
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
#include "ez_i2c.h"
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
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTransmitSync, void *, uint16_t, const uint8_t *, size_t, bool, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTransmitAsync, void *, uint16_t, const uint8_t *, size_t, bool);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwReceiveSync, void *, uint16_t, uint8_t *, size_t, bool, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwReceiveAsync, void *, uint16_t, uint8_t *, size_t, bool);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwProbe, void *, uint16_t, uint32_t);
FAKE_VOID_FUNC(OnEvent, void *, uint8_t, void *, void *);

#define DEVICE_ADDR     (0x50U)


/******************************************************************************
* Module Typedefs
*******************************************************************************/
/* None */


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static struct ezI2cDriver g_drv;
static int g_hw_token = 0;              /* value of driver_h given to the HW functions */
static bool g_busy_in_callback = false; /* BUSY bit seen inside the callback */
static EZ_DRV_STATUS g_chained_status = EZ_DRV_STATUS_OK;
static ezI2cHandle_t *g_chain_handle = NULL;
static uint8_t g_buf[4] = {1, 2, 3, 4};
static uint8_t g_state_in_hw = 0U;      /* state byte seen inside a HW function */


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
    g_chained_status = ezI2c_TransmitAsync(g_chain_handle, DEVICE_ADDR, g_buf, sizeof(g_buf), true);
}

/* A sync HW function which breaks the rule and reports events. */
static EZ_DRV_STATUS HwProbeEndsItself(void *driver_h, uint16_t address, uint32_t timeout_millis)
{
    (void)driver_h;
    (void)address;
    (void)timeout_millis;
    g_state_in_hw = g_drv.base.state;
    SimulateIsrProgress(EZ_I2C_EVENT_CMPLT);
    SimulateIsrDone(EZ_I2C_EVENT_CMPLT);
    return EZ_DRV_STATUS_OK;
}

/* Record the state bits seen by the HW function. */
static EZ_DRV_STATUS HwTransmitSyncRecordState(void *driver_h, uint16_t address, const uint8_t *data, size_t length, bool send_stop, uint32_t timeout_millis)
{
    (void)driver_h;
    (void)address;
    (void)data;
    (void)length;
    (void)send_stop;
    (void)timeout_millis;
    g_state_in_hw = g_drv.base.state;
    return EZ_DRV_STATUS_OK;
}

static EZ_DRV_STATUS HwTransmitAsyncRecordState(void *driver_h, uint16_t address, const uint8_t *data, size_t length, bool send_stop)
{
    (void)driver_h;
    (void)address;
    (void)data;
    (void)length;
    (void)send_stop;
    g_state_in_hw = g_drv.base.state;
    return EZ_DRV_STATUS_OK;
}


/******************************************************************************
* Fixture
*******************************************************************************/
struct I2cTestFixture
{
    I2cTestFixture()
    {
        RESET_FAKE(HwInitialize);
        RESET_FAKE(HwDeinitialize);
        RESET_FAKE(HwLock);
        RESET_FAKE(HwUnlock);
        RESET_FAKE(HwTransmitSync);
        RESET_FAKE(HwTransmitAsync);
        RESET_FAKE(HwReceiveSync);
        RESET_FAKE(HwReceiveAsync);
        RESET_FAKE(HwProbe);
        RESET_FAKE(OnEvent);
        FFF_RESET_HISTORY();

        g_busy_in_callback = false;
        g_state_in_hw = 0U;
        g_chained_status = EZ_DRV_STATUS_OK;
        g_chain_handle = NULL;

        memset(&g_drv, 0, sizeof(g_drv));
        memset(&first, 0, sizeof(first));
        memset(&second, 0, sizeof(second));
        g_drv.base.name = "i2c_mock";
        g_drv.interface.driver_h = &g_hw_token;
        g_drv.interface.initialize = HwInitialize;
        g_drv.interface.deinitialize = HwDeinitialize;
        /* lock and unlock stay NULL: the bus lock is a try-lock, tests which need the HW lock set them */
        g_drv.interface.transmit_sync = HwTransmitSync;
        g_drv.interface.transmit_async = HwTransmitAsync;
        g_drv.interface.receive_sync = HwReceiveSync;
        g_drv.interface.receive_async = HwReceiveAsync;
        g_drv.interface.probe = HwProbe;

        REQUIRE(ezI2c_SystemRegisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
        driver_registered = true;
    }

    ~I2cTestFixture()
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
            (void)ezI2c_SystemUnregisterHwDriver(&g_drv);
        }
    }

    /* Open a handle on the mock driver. */
    void OpenHandle(ezI2cHandle_t *handle, ezDrvCallback cb = OnEvent, void *ctx = NULL)
    {
        REQUIRE(ezI2c_Open(handle, "i2c_mock", cb, ctx) == EZ_DRV_STATUS_OK);
    }

    /* Start an async transmit which is accepted by the HW driver. */
    void StartAsyncTransmit(ezI2cHandle_t *handle)
    {
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezI2c_TransmitAsync(handle, DEVICE_ADDR, g_buf, sizeof(g_buf), true) == EZ_DRV_STATUS_OK);
    }

    ezI2cHandle_t first;
    ezI2cHandle_t second;
    bool driver_registered;
};


/******************************************************************************
* External functions
*******************************************************************************/
TEST_CASE_METHOD(I2cTestFixture, "Test driver registration", "[hal][i2c]")
{
    WHEN("Invalid arguments are used")
    {
        struct ezI2cDriver no_name;
        memset(&no_name, 0, sizeof(no_name));

        THEN("The APIs report an argument error")
        {
            CHECK(ezI2c_SystemRegisterHwDriver(NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezI2c_SystemRegisterHwDriver(&no_name) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezI2c_SystemUnregisterHwDriver(NULL) == EZ_DRV_STATUS_ERR_ARG);
        }
    }

    WHEN("A driver with the same name is registered twice")
    {
        struct ezI2cDriver duplicate;
        memset(&duplicate, 0, sizeof(duplicate));
        duplicate.base.name = "i2c_mock";

        THEN("The second registration is rejected")
        {
            CHECK(ezI2c_SystemRegisterHwDriver(&duplicate) == EZ_DRV_STATUS_ERR_ARG);
        }
    }

    WHEN("A second driver with another name is registered")
    {
        struct ezI2cDriver other;
        memset(&other, 0, sizeof(other));
        other.base.name = "i2c_other";

        THEN("Both can be found by name")
        {
            other.interface.initialize = HwInitialize;
            other.interface.deinitialize = HwDeinitialize;
            REQUIRE(ezI2c_SystemRegisterHwDriver(&other) == EZ_DRV_STATUS_OK);
            CHECK(ezI2c_Open(&first, "i2c_other", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == &other);
            CHECK(ezI2c_Open(&second, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(second.drv_handle.hw == &g_drv);
            CHECK(ezI2c_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(ezI2c_SystemUnregisterHwDriver(&other) == EZ_DRV_STATUS_OK);
        }
    }

    WHEN("A driver is registered")
    {
        THEN("It starts idle with no open handle")
        {
            CHECK(g_drv.open_count == 0U);
            CHECK(g_drv.lock_owner == NULL);
            CHECK(g_drv.base.state == 0U);
            CHECK(g_drv.base.user == NULL);
        }
    }

    WHEN("A driver sets only one of the lock and unlock functions")
    {
        struct ezI2cDriver half;
        memset(&half, 0, sizeof(half));
        half.base.name = "i2c_half";

        THEN("The registration is rejected, and accepted with both or none")
        {
            half.interface.lock = HwLock;
            CHECK(ezI2c_SystemRegisterHwDriver(&half) == EZ_DRV_STATUS_ERR_ARG);
            half.interface.lock = NULL;
            half.interface.unlock = HwUnlock;
            CHECK(ezI2c_SystemRegisterHwDriver(&half) == EZ_DRV_STATUS_ERR_ARG);
            half.interface.lock = HwLock;
            CHECK(ezI2c_SystemRegisterHwDriver(&half) == EZ_DRV_STATUS_OK);
            CHECK(ezI2c_SystemUnregisterHwDriver(&half) == EZ_DRV_STATUS_OK);
        }
    }

    WHEN("A driver is unregistered")
    {
        THEN("It is removed from the list and cannot be found anymore")
        {
            CHECK(ezI2c_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
            driver_registered = false;
            CHECK(ezI2c_Open(&first, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("A driver is unregistered while a handle is open")
    {
        OpenHandle(&first);

        THEN("The unregistration is refused until the handle is closed")
        {
            CHECK(ezI2c_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_BUSY);
            REQUIRE(ezI2c_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(ezI2c_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
            driver_registered = false;
        }
    }

    WHEN("A driver is unregistered while an operation is in flight")
    {
        OpenHandle(&first);
        StartAsyncTransmit(&first);

        THEN("The unregistration is refused")
        {
            CHECK(ezI2c_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_BUSY);
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test open and close", "[hal][i2c]")
{
    WHEN("Invalid arguments are used")
    {
        THEN("The APIs report an argument error")
        {
            CHECK(ezI2c_Open(NULL, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezI2c_Open(&first, NULL, OnEvent, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezI2c_Open(&first, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(ezI2c_Open(&first, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezI2c_Close(NULL) == EZ_DRV_STATUS_ERR_ARG);
        }
    }

    WHEN("A handle which was never opened is closed")
    {
        THEN("The call succeeds, the handle stays empty and the HW is not called")
        {
            CHECK(ezI2c_Close(&first) == EZ_DRV_STATUS_OK);
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
            CHECK(ezI2c_Open(&first, "unknown", OnEvent, NULL) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
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
            REQUIRE(ezI2c_Open(&first, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == &g_drv);
            CHECK(g_drv.open_count == 1U);
            CHECK(HwInitialize_fake.call_count == 0U);
            CHECK(ezI2c_Close(&first) == EZ_DRV_STATUS_OK);
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
            REQUIRE(ezI2c_Open(&first, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(HwInitialize_fake.call_count == 1U);
            CHECK(ezI2c_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(HwDeinitialize_fake.call_count == 0U);
        }
    }

    WHEN("The handle is opened")
    {
        int user_ctx = 0;
        REQUIRE(ezI2c_Open(&first, "i2c_mock", OnEvent, &user_ctx) == EZ_DRV_STATUS_OK);

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
            CHECK(ezI2c_Open(&first, "i2c_mock", NULL, NULL) == EZ_DRV_STATUS_ERR_ARG);
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
            CHECK(ezI2c_Open(&first, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(g_drv.open_count == 0U);
            CHECK(IsBusy() == false);
        }

        THEN("The same handle can be opened again once the failure is gone")
        {
            CHECK(ezI2c_Open(&first, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_GENERIC);
            HwInitialize_fake.return_val = EZ_DRV_STATUS_OK;
            CHECK(ezI2c_Open(&first, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(HwInitialize_fake.call_count == 2U);
            CHECK(g_drv.open_count == 1U);
        }
    }

    WHEN("A second handle is opened")
    {
        OpenHandle(&first);
        REQUIRE(ezI2c_Open(&second, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);

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
        REQUIRE(ezI2c_TransmitAsync(&first, DEVICE_ADDR, g_buf, 4U, true) == EZ_DRV_STATUS_OK);

        THEN("The open is refused and the operation is not disturbed")
        {
            CHECK(ezI2c_Open(&second, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_BUSY);
            CHECK(second.drv_handle.hw == NULL);
            CHECK(IsBusy() == true);
            SimulateIsrDone(EZ_I2C_EVENT_CMPLT);
            CHECK(OnEvent_fake.call_count == 1U);
        }
    }

    WHEN("The maximum number of handles is open")
    {
        g_drv.open_count = UINT8_MAX;

        THEN("The open fails and the HW is not called")
        {
            CHECK(ezI2c_Open(&first, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(HwInitialize_fake.call_count == 0U);
            CHECK(g_drv.open_count == UINT8_MAX);
        }
    }

    WHEN("The only open handle is closed")
    {
        OpenHandle(&first);

        THEN("The bus is released and everything is cleared")
        {
            CHECK(ezI2c_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(HwDeinitialize_fake.call_count == 1U);
            CHECK(HwDeinitialize_fake.arg0_val == &g_hw_token);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(first.drv_handle.on_event == NULL);
            CHECK(first.drv_handle.ctx == NULL);
            CHECK(g_drv.open_count == 0U);
        }

        THEN("The bus can be opened again afterwards and is initialized again")
        {
            REQUIRE(ezI2c_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(ezI2c_Open(&first, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(HwInitialize_fake.call_count == 2U);
        }
    }

    WHEN("One of two open handles is closed")
    {
        OpenHandle(&first);
        OpenHandle(&second);
        REQUIRE(ezI2c_Close(&first) == EZ_DRV_STATUS_OK);

        THEN("The bus stays up and the other handle keeps working")
        {
            CHECK(HwDeinitialize_fake.call_count == 0U);
            CHECK(g_drv.open_count == 1U);
            HwTransmitSync_fake.return_val = EZ_DRV_STATUS_OK;
            CHECK(ezI2c_TransmitSync(&second, DEVICE_ADDR, g_buf, 1, true, 5) == EZ_DRV_STATUS_OK);
        }

        THEN("Closing the other one releases the bus")
        {
            CHECK(ezI2c_Close(&second) == EZ_DRV_STATUS_OK);
            CHECK(HwDeinitialize_fake.call_count == 1U);
            CHECK(g_drv.open_count == 0U);
        }
    }

    WHEN("A handle with an operation in flight is closed")
    {
        OpenHandle(&first);
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezI2c_TransmitAsync(&first, DEVICE_ADDR, g_buf, 4U, true) == EZ_DRV_STATUS_OK);

        THEN("The close is refused until the operation ends")
        {
            CHECK(ezI2c_Close(&first) == EZ_DRV_STATUS_BUSY);
            CHECK(first.drv_handle.hw == &g_drv);
            CHECK(HwDeinitialize_fake.call_count == 0U);
            SimulateIsrDone(EZ_I2C_EVENT_CMPLT);
            CHECK(ezI2c_Close(&first) == EZ_DRV_STATUS_OK);
        }
    }

    WHEN("Another handle is closed while a handle has an operation in flight")
    {
        OpenHandle(&first);
        OpenHandle(&second);
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezI2c_TransmitAsync(&first, DEVICE_ADDR, g_buf, 4U, true) == EZ_DRV_STATUS_OK);

        THEN("It is allowed and does not disturb the operation")
        {
            CHECK(ezI2c_Close(&second) == EZ_DRV_STATUS_OK);
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
            CHECK(ezI2c_Close(&first) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(g_drv.open_count == 0U);
        }
    }

    WHEN("Two handles start async operations one after the other")
    {
        int ctx_first = 0;
        int ctx_second = 0;
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezI2c_Open(&first, "i2c_mock", OnEvent, &ctx_first) == EZ_DRV_STATUS_OK);
        REQUIRE(ezI2c_Open(&second, "i2c_mock", OnEvent, &ctx_second) == EZ_DRV_STATUS_OK);

        THEN("The end event goes to the callback of the handle which started the operation")
        {
            REQUIRE(ezI2c_TransmitAsync(&second, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_OK);
            SimulateIsrDone(EZ_I2C_EVENT_CMPLT);
            REQUIRE(OnEvent_fake.call_count == 1U);
            CHECK(OnEvent_fake.arg0_val == &ctx_second);
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test operations require an open handle", "[hal][i2c]")
{
    WHEN("The handle was never opened")
    {
        THEN("Every operation reports driver-not-found and the HW is not called")
        {
            CHECK(ezI2c_ReceiveSync(&first, DEVICE_ADDR, g_buf, 1, true, 10) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_ReceiveAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_TransmitSync(&first, DEVICE_ADDR, g_buf, 1, true, 10) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_TransmitAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_ReceiveSync(&first, DEVICE_ADDR, g_buf, 1, true, 10) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_ReceiveAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_Probe(&first, DEVICE_ADDR, 10) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(HwReceiveSync_fake.call_count == 0U);
            CHECK(HwTransmitAsync_fake.call_count == 0U);
            CHECK(HwReceiveSync_fake.call_count == 0U);
            CHECK(HwProbe_fake.call_count == 0U);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The handle is NULL")
    {
        THEN("Every operation reports driver-not-found")
        {
            CHECK(ezI2c_ReceiveSync(NULL, DEVICE_ADDR, g_buf, 1, true, 10) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_TransmitAsync(NULL, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("The handle was closed")
    {
        OpenHandle(&first);
        REQUIRE(ezI2c_Close(&first) == EZ_DRV_STATUS_OK);

        THEN("Operations report driver-not-found again")
        {
            CHECK(ezI2c_TransmitSync(&first, DEVICE_ADDR, g_buf, 1, true, 10) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(HwTransmitSync_fake.call_count == 0U);
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test bus lock", "[hal][i2c]")
{
    OpenHandle(&first);
    OpenHandle(&second);
    HwTransmitSync_fake.return_val = EZ_DRV_STATUS_OK;

    WHEN("The HW driver has no lock functions (no RTOS)")
    {
        THEN("The lock is a try-lock which ignores the timeout")
        {
            CHECK(ezI2c_LockBus(&first, 1000U) == EZ_DRV_STATUS_OK);
            CHECK(g_drv.lock_owner == &first.drv_handle);
            CHECK(ezI2c_LockBus(&second, 1000U) == EZ_DRV_STATUS_BUSY);
            CHECK(g_drv.lock_owner == &first.drv_handle);
            CHECK(ezI2c_UnlockBus(&first) == EZ_DRV_STATUS_OK);
            CHECK(g_drv.lock_owner == NULL);
            CHECK(ezI2c_LockBus(&second, 0U) == EZ_DRV_STATUS_OK);
            CHECK(g_drv.lock_owner == &second.drv_handle);
        }
    }

    WHEN("The HW driver has lock functions")
    {
        g_drv.interface.lock = HwLock;
        g_drv.interface.unlock = HwUnlock;

        THEN("The HW lock gets the timeout and the handle becomes the owner")
        {
            CHECK(ezI2c_LockBus(&first, 25U) == EZ_DRV_STATUS_OK);
            CHECK(HwLock_fake.call_count == 1U);
            CHECK(HwLock_fake.arg0_val == &g_hw_token);
            CHECK(HwLock_fake.arg1_val == 25U);
            CHECK(g_drv.lock_owner == &first.drv_handle);
        }

        THEN("A timeout is forwarded and nobody owns the bus")
        {
            HwLock_fake.return_val = EZ_DRV_STATUS_TIMEOUT;
            CHECK(ezI2c_LockBus(&second, 25U) == EZ_DRV_STATUS_TIMEOUT);
            CHECK(g_drv.lock_owner == NULL);
        }

        THEN("The unlock frees the bus, then calls the HW unlock")
        {
            REQUIRE(ezI2c_LockBus(&first, 25U) == EZ_DRV_STATUS_OK);
            CHECK(ezI2c_UnlockBus(&first) == EZ_DRV_STATUS_OK);
            CHECK(HwUnlock_fake.call_count == 1U);
            CHECK(HwUnlock_fake.arg0_val == &g_hw_token);
            CHECK(g_drv.lock_owner == NULL);
        }

        THEN("A failing HW unlock is forwarded, the bus is free anyway")
        {
            REQUIRE(ezI2c_LockBus(&first, 25U) == EZ_DRV_STATUS_OK);
            HwUnlock_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;
            CHECK(ezI2c_UnlockBus(&first) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(g_drv.lock_owner == NULL);
        }

        THEN("Closing the holder unlocks the bus")
        {
            REQUIRE(ezI2c_LockBus(&first, 25U) == EZ_DRV_STATUS_OK);
            CHECK(ezI2c_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(HwUnlock_fake.call_count == 1U);
            CHECK(g_drv.lock_owner == NULL);
        }

        THEN("A failing HW unlock on close is logged and the close still succeeds")
        {
            REQUIRE(ezI2c_LockBus(&first, 25U) == EZ_DRV_STATUS_OK);
            HwUnlock_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;
            CHECK(ezI2c_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == NULL);
        }
    }

    WHEN("The lock is used wrongly")
    {
        THEN("The APIs report the matching error")
        {
            CHECK(ezI2c_LockBus(NULL, 10U) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_UnlockBus(NULL) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            ezI2cHandle_t never_opened;
            memset(&never_opened, 0, sizeof(never_opened));
            CHECK(ezI2c_LockBus(&never_opened, 10U) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_UnlockBus(&never_opened) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);

            CHECK(ezI2c_UnlockBus(&first) == EZ_DRV_STATUS_ERR_ARG);
            REQUIRE(ezI2c_LockBus(&first, 10U) == EZ_DRV_STATUS_OK);
            CHECK(ezI2c_LockBus(&first, 10U) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezI2c_UnlockBus(&second) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(g_drv.lock_owner == &first.drv_handle);
        }
    }

    WHEN("A handle holds the bus")
    {
        REQUIRE(ezI2c_LockBus(&first, 10U) == EZ_DRV_STATUS_OK);

        THEN("Transfers of another handle report busy and the HW is not called")
        {
            CHECK(ezI2c_TransmitSync(&second, DEVICE_ADDR, g_buf, 1, true, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(ezI2c_TransmitAsync(&second, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_BUSY);
            CHECK(ezI2c_ReceiveSync(&second, DEVICE_ADDR, g_buf, 1, true, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(ezI2c_ReceiveSync(&second, DEVICE_ADDR, g_buf, 1, true, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(ezI2c_Probe(&second, DEVICE_ADDR, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(HwTransmitSync_fake.call_count == 0U);
            CHECK(HwTransmitAsync_fake.call_count == 0U);
            CHECK(HwReceiveSync_fake.call_count == 0U);
            CHECK(HwProbe_fake.call_count == 0U);
            CHECK(IsBusy() == false);
        }

        THEN("The holder can transfer as many times as it wants")
        {
            CHECK(ezI2c_TransmitSync(&first, DEVICE_ADDR, g_buf, 1, true, 5) == EZ_DRV_STATUS_OK);
            CHECK(ezI2c_TransmitSync(&first, DEVICE_ADDR, g_buf, 1, true, 5) == EZ_DRV_STATUS_OK);
            CHECK(HwTransmitSync_fake.call_count == 2U);
        }

        THEN("The other handle can transfer after the unlock")
        {
            REQUIRE(ezI2c_UnlockBus(&first) == EZ_DRV_STATUS_OK);
            CHECK(ezI2c_TransmitSync(&second, DEVICE_ADDR, g_buf, 1, true, 5) == EZ_DRV_STATUS_OK);
        }

        THEN("The lock survives an async operation of the holder")
        {
            HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
            REQUIRE(ezI2c_TransmitAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_OK);
            SimulateIsrDone(EZ_I2C_EVENT_CMPLT);
            CHECK(g_drv.lock_owner == &first.drv_handle);
            CHECK(ezI2c_TransmitSync(&second, DEVICE_ADDR, g_buf, 1, true, 5) == EZ_DRV_STATUS_BUSY);
        }

        THEN("Closing the holder frees the bus also without HW lock functions")
        {
            CHECK(ezI2c_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(g_drv.lock_owner == NULL);
            CHECK(ezI2c_TransmitSync(&second, DEVICE_ADDR, g_buf, 1, true, 5) == EZ_DRV_STATUS_OK);
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test sync transmit", "[hal][i2c]")
{
    OpenHandle(&first);

    WHEN("The HW transmit succeeds")
    {
        HwTransmitSync_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The arguments are forwarded and the driver is released")
        {
            CHECK(ezI2c_TransmitSync(&first, DEVICE_ADDR, g_buf, 3, true, 55) == EZ_DRV_STATUS_OK);
            CHECK(HwTransmitSync_fake.call_count == 1U);
            CHECK(HwTransmitSync_fake.arg0_val == &g_hw_token);
            CHECK(HwTransmitSync_fake.arg1_val == DEVICE_ADDR);
            CHECK(HwTransmitSync_fake.arg2_val == g_buf);
            CHECK(HwTransmitSync_fake.arg3_val == 3U);
            CHECK(HwTransmitSync_fake.arg4_val == true);
            CHECK(HwTransmitSync_fake.arg5_val == 55U);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW transmit reports a timeout")
    {
        HwTransmitSync_fake.return_val = EZ_DRV_STATUS_TIMEOUT;

        THEN("The timeout is propagated and the driver is released")
        {
            CHECK(ezI2c_TransmitSync(&first, DEVICE_ADDR, g_buf, 3, false, 5) == EZ_DRV_STATUS_TIMEOUT);
            CHECK(HwTransmitSync_fake.arg4_val == false);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW driver has no transmit function")
    {
        g_drv.interface.transmit_sync = NULL;

        THEN("The API reports interface-not-exist and releases the driver")
        {
            CHECK(ezI2c_TransmitSync(&first, DEVICE_ADDR, g_buf, 3, true, 5) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("Another handle has an operation in flight")
    {
        REQUIRE(ezI2c_Open(&second, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
        StartAsyncTransmit(&second);

        THEN("The API reports busy and the HW is not called")
        {
            CHECK(ezI2c_TransmitSync(&first, DEVICE_ADDR, g_buf, 3, true, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(HwTransmitSync_fake.call_count == 0U);
            CHECK(IsBusy() == true);
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test sync receive", "[hal][i2c]")
{
    OpenHandle(&first);

    WHEN("The HW receive succeeds")
    {
        HwReceiveSync_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The arguments are forwarded and the driver is released")
        {
            uint8_t rx[2] = {0};
            CHECK(ezI2c_ReceiveSync(&first, DEVICE_ADDR, rx, 2, false, 30) == EZ_DRV_STATUS_OK);
            CHECK(HwReceiveSync_fake.arg0_val == &g_hw_token);
            CHECK(HwReceiveSync_fake.arg1_val == DEVICE_ADDR);
            CHECK(HwReceiveSync_fake.arg2_val == rx);
            CHECK(HwReceiveSync_fake.arg3_val == 2U);
            CHECK(HwReceiveSync_fake.arg4_val == false);
            CHECK(HwReceiveSync_fake.arg5_val == 30U);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW receive reports a timeout")
    {
        HwReceiveSync_fake.return_val = EZ_DRV_STATUS_TIMEOUT;

        THEN("The timeout is propagated and the driver is released")
        {
            CHECK(ezI2c_ReceiveSync(&first, DEVICE_ADDR, g_buf, 2, true, 5) == EZ_DRV_STATUS_TIMEOUT);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW driver has no receive function")
    {
        g_drv.interface.receive_sync = NULL;

        THEN("The API reports interface-not-exist and releases the driver")
        {
            CHECK(ezI2c_ReceiveSync(&first, DEVICE_ADDR, g_buf, 2, true, 5) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("Another handle has an operation in flight")
    {
        REQUIRE(ezI2c_Open(&second, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
        StartAsyncTransmit(&second);

        THEN("The API reports busy and the HW is not called")
        {
            CHECK(ezI2c_ReceiveSync(&first, DEVICE_ADDR, g_buf, 2, true, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(HwReceiveSync_fake.call_count == 0U);
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test probe", "[hal][i2c]")
{
    OpenHandle(&first);

    WHEN("The HW probe succeeds")
    {
        HwProbe_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The arguments are forwarded and the driver is released")
        {
            CHECK(ezI2c_Probe(&first, DEVICE_ADDR, 20) == EZ_DRV_STATUS_OK);
            CHECK(HwProbe_fake.arg0_val == &g_hw_token);
            CHECK(HwProbe_fake.arg1_val == DEVICE_ADDR);
            CHECK(HwProbe_fake.arg2_val == 20U);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW probe reports a timeout")
    {
        HwProbe_fake.return_val = EZ_DRV_STATUS_TIMEOUT;

        THEN("The timeout is propagated and the driver is released")
        {
            CHECK(ezI2c_Probe(&first, DEVICE_ADDR, 20) == EZ_DRV_STATUS_TIMEOUT);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW driver has no probe function")
    {
        g_drv.interface.probe = NULL;

        THEN("The API reports interface-not-exist and releases the driver")
        {
            CHECK(ezI2c_Probe(&first, DEVICE_ADDR, 20) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("Another handle has an operation in flight")
    {
        REQUIRE(ezI2c_Open(&second, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
        StartAsyncTransmit(&second);

        THEN("The API reports busy and the HW is not called")
        {
            CHECK(ezI2c_Probe(&first, DEVICE_ADDR, 20) == EZ_DRV_STATUS_BUSY);
            CHECK(HwProbe_fake.call_count == 0U);
        }
    }

    WHEN("The sync HW function reports PROGRESS and DONE events")
    {
        g_drv.interface.probe = HwProbeEndsItself;

        THEN("The events are dropped, the status is returned and the driver is idle")
        {
            CHECK(ezI2c_Probe(&first, DEVICE_ADDR, 20) == EZ_DRV_STATUS_OK);
            CHECK(g_state_in_hw == EZ_DRV_STATE_BUSY);
            CHECK(IsBusy() == false);
            CHECK(g_drv.base.state == 0U);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test async transmit", "[hal][i2c]")
{
    int user_ctx = 0;
    OpenHandle(&first, OnEvent, &user_ctx);
    HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;

    WHEN("The transmit is started successfully")
    {
        REQUIRE(ezI2c_TransmitAsync(&first, DEVICE_ADDR, g_buf, 4, false) == EZ_DRV_STATUS_OK);

        THEN("The arguments are forwarded and the driver stays busy")
        {
            CHECK(HwTransmitAsync_fake.arg0_val == &g_hw_token);
            CHECK(HwTransmitAsync_fake.arg1_val == DEVICE_ADDR);
            CHECK(HwTransmitAsync_fake.arg2_val == g_buf);
            CHECK(HwTransmitAsync_fake.arg3_val == 4U);
            CHECK(HwTransmitAsync_fake.arg4_val == false);
            CHECK(IsBusy() == true);
        }

        THEN("Every other operation, even from the same handle, reports busy")
        {
            CHECK(ezI2c_TransmitAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_BUSY);
            CHECK(ezI2c_ReceiveAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_BUSY);
            CHECK(ezI2c_TransmitSync(&first, DEVICE_ADDR, g_buf, 1, true, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(ezI2c_ReceiveSync(&first, DEVICE_ADDR, g_buf, 1, true, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(ezI2c_Probe(&first, DEVICE_ADDR, 5) == EZ_DRV_STATUS_BUSY);
            CHECK(HwTransmitAsync_fake.call_count == 1U);
            CHECK(IsBusy() == true);
        }

        AND_WHEN("The HW driver reports the end of the operation")
        {
            SimulateIsrDone(EZ_I2C_EVENT_CMPLT);

            THEN("The driver is released and the callback gets the context and the event")
            {
                CHECK(IsBusy() == false);
                REQUIRE(OnEvent_fake.call_count == 1U);
                CHECK(OnEvent_fake.arg0_val == &user_ctx);
                CHECK(OnEvent_fake.arg1_val == EZ_I2C_EVENT_CMPLT);
            }

            THEN("A new operation can be started")
            {
                HwReceiveAsync_fake.return_val = EZ_DRV_STATUS_OK;
                CHECK(ezI2c_ReceiveAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_OK);
            }
        }
    }

    WHEN("The HW fails to start the transmit")
    {
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;

        THEN("The failure is returned, the driver is released and no callback is expected")
        {
            CHECK(ezI2c_TransmitAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(IsBusy() == false);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }

    WHEN("The HW driver has no async transmit function")
    {
        g_drv.interface.transmit_async = NULL;

        THEN("The API reports interface-not-exist and releases the driver")
        {
            CHECK(ezI2c_TransmitAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("Another handle has an operation in flight")
    {
        REQUIRE(ezI2c_Open(&second, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
        StartAsyncTransmit(&second);

        THEN("The API reports busy and the HW is not called again")
        {
            CHECK(ezI2c_TransmitAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_BUSY);
            CHECK(HwTransmitAsync_fake.call_count == 1U);
        }
    }

    WHEN("The driver is released before the callback runs")
    {
        OnEvent_fake.custom_fake = OnEventRecordBusy;
        REQUIRE(ezI2c_TransmitAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_OK);
        g_busy_in_callback = true;
        SimulateIsrDone(EZ_I2C_EVENT_CMPLT);

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
        REQUIRE(ezI2c_TransmitAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_OK);
        SimulateIsrDone(EZ_I2C_EVENT_CMPLT);

        THEN("The chained operation is accepted and the driver is busy again")
        {
            CHECK(g_chained_status == EZ_DRV_STATUS_OK);
            CHECK(HwTransmitAsync_fake.call_count == 2U);
            CHECK(IsBusy() == true);
        }
    }

    WHEN("The end of the operation is reported with no callback registered")
    {
        ezI2cHandle_t quiet;
        memset(&quiet, 0, sizeof(quiet));
        REQUIRE(ezI2c_Open(&quiet, "i2c_mock", NULL, NULL) == EZ_DRV_STATUS_OK);
        REQUIRE(ezI2c_TransmitAsync(&quiet, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_OK);
        SimulateIsrDone(EZ_I2C_EVENT_ERR);

        THEN("The driver is released and nothing crashes")
        {
            CHECK(IsBusy() == false);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }

    WHEN("An event arrives with no operation in flight")
    {
        SimulateIsrDone(EZ_I2C_EVENT_CMPLT);

        THEN("It is dropped")
        {
            CHECK(OnEvent_fake.call_count == 0U);
            CHECK(IsBusy() == false);
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test async receive", "[hal][i2c]")
{
    int user_ctx = 0;
    OpenHandle(&first, OnEvent, &user_ctx);
    HwReceiveAsync_fake.return_val = EZ_DRV_STATUS_OK;

    WHEN("The receive is started successfully")
    {
        uint8_t rx[3] = {0};
        REQUIRE(ezI2c_ReceiveAsync(&first, DEVICE_ADDR, rx, 3, false) == EZ_DRV_STATUS_OK);

        THEN("The arguments are forwarded and the driver stays busy")
        {
            CHECK(HwReceiveAsync_fake.arg0_val == &g_hw_token);
            CHECK(HwReceiveAsync_fake.arg1_val == DEVICE_ADDR);
            CHECK(HwReceiveAsync_fake.arg2_val == rx);
            CHECK(HwReceiveAsync_fake.arg3_val == 3U);
            CHECK(HwReceiveAsync_fake.arg4_val == false);
            CHECK(IsBusy() == true);
        }

        AND_WHEN("The HW driver reports the end of the operation with a timeout")
        {
            SimulateIsrDone(EZ_I2C_EVENT_TIMEOUT);

            THEN("The driver is released and the callback gets the context and the event")
            {
                CHECK(IsBusy() == false);
                REQUIRE(OnEvent_fake.call_count == 1U);
                CHECK(OnEvent_fake.arg0_val == &user_ctx);
                CHECK(OnEvent_fake.arg1_val == EZ_I2C_EVENT_TIMEOUT);
            }
        }
    }

    WHEN("The HW fails to start the receive")
    {
        HwReceiveAsync_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;

        THEN("The failure is returned and the driver is released")
        {
            CHECK(ezI2c_ReceiveAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW driver has no async receive function")
    {
        g_drv.interface.receive_async = NULL;

        THEN("The API reports interface-not-exist and releases the driver")
        {
            CHECK(ezI2c_ReceiveAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("Another handle has an operation in flight")
    {
        REQUIRE(ezI2c_Open(&second, "i2c_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
        StartAsyncTransmit(&second);

        THEN("The API reports busy and the HW is not called")
        {
            CHECK(ezI2c_ReceiveAsync(&first, DEVICE_ADDR, g_buf, 1, true) == EZ_DRV_STATUS_BUSY);
            CHECK(HwReceiveAsync_fake.call_count == 0U);
        }
    }
}

TEST_CASE_METHOD(I2cTestFixture, "Test events only belong to async operations", "[hal][i2c]")
{
    int user_ctx = 0;
    OpenHandle(&first, OnEvent, &user_ctx);

    WHEN("A sync HW function reports DONE and the callback would start an async operation")
    {
        g_drv.interface.probe = HwProbeEndsItself;
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        OnEvent_fake.custom_fake = OnEventChain;
        g_chain_handle = &first;

        THEN("The callback is not called, so no operation is started and the driver ends idle")
        {
            CHECK(ezI2c_Probe(&first, DEVICE_ADDR, 20) == EZ_DRV_STATUS_OK);
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
            CHECK(ezI2c_TransmitSync(&first, DEVICE_ADDR, g_buf, 4, true, 10) == EZ_DRV_STATUS_OK);
            CHECK(g_state_in_hw == EZ_DRV_STATE_BUSY);
            CHECK(g_drv.base.state == 0U);
        }
    }

    WHEN("An async operation runs")
    {
        HwTransmitAsync_fake.custom_fake = HwTransmitAsyncRecordState;
        REQUIRE(ezI2c_TransmitAsync(&first, DEVICE_ADDR, g_buf, 4, true) == EZ_DRV_STATUS_OK);

        THEN("The HW function sees BUSY and ASYNC, and both stay set")
        {
            CHECK(g_state_in_hw == (EZ_DRV_STATE_BUSY | EZ_DRV_STATE_ASYNC));
            CHECK(g_drv.base.state == (EZ_DRV_STATE_BUSY | EZ_DRV_STATE_ASYNC));
        }

        AND_WHEN("The HW driver reports a PROGRESS event")
        {
            SimulateIsrProgress(EZ_I2C_EVENT_CMPLT);

            THEN("The callback is called and the operation is still in flight")
            {
                CHECK(OnEvent_fake.call_count == 1U);
                CHECK(OnEvent_fake.arg0_val == &user_ctx);
                CHECK(g_drv.base.state == (EZ_DRV_STATE_BUSY | EZ_DRV_STATE_ASYNC));
            }
        }

        AND_WHEN("The HW driver reports the DONE event")
        {
            SimulateIsrDone(EZ_I2C_EVENT_CMPLT);

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
            CHECK(ezI2c_TransmitAsync(&first, DEVICE_ADDR, g_buf, 4, true) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(g_drv.base.state == 0U);
        }
    }

    WHEN("A sync operation follows an async operation")
    {
        HwTransmitSync_fake.custom_fake = HwTransmitSyncRecordState;
        StartAsyncTransmit(&first);
        SimulateIsrDone(EZ_I2C_EVENT_CMPLT);

        THEN("The sync operation does not inherit the ASYNC bit")
        {
            CHECK(ezI2c_TransmitSync(&first, DEVICE_ADDR, g_buf, 4, true, 10) == EZ_DRV_STATUS_OK);
            CHECK(g_state_in_hw == EZ_DRV_STATE_BUSY);
            CHECK(OnEvent_fake.call_count == 1U);
        }
    }
}

/* End of file */
