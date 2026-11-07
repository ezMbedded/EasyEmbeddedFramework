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
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwInitialize, void *, ezSpiConfig_t *);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTransferSync, void *, const uint8_t *, size_t, uint8_t *, size_t, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTransferAsync, void *, const uint8_t *, size_t, uint8_t *, size_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTransmitSync, void *, const uint8_t *, size_t, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTransmitAsync, void *, const uint8_t *, size_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwReceiveSync, void *, uint8_t *, size_t, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwReceiveAsync, void *, uint8_t *, size_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwChipSelect, void *, bool);
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


/******************************************************************************
* Fixture
*******************************************************************************/
struct SpiTestFixture
{
    SpiTestFixture()
    {
        RESET_FAKE(HwInitialize);
        RESET_FAKE(HwTransferSync);
        RESET_FAKE(HwTransferAsync);
        RESET_FAKE(HwTransmitSync);
        RESET_FAKE(HwTransmitAsync);
        RESET_FAKE(HwReceiveSync);
        RESET_FAKE(HwReceiveAsync);
        RESET_FAKE(HwChipSelect);
        RESET_FAKE(OnEvent);
        FFF_RESET_HISTORY();

        g_busy_in_callback = false;
        g_chained_status = EZ_DRV_STATUS_OK;
        g_chain_handle = NULL;

        memset(&g_drv, 0, sizeof(g_drv));
        memset(&first, 0, sizeof(first));
        memset(&second, 0, sizeof(second));
        memset(&config, 0, sizeof(config));
        g_drv.base.name = "spi_mock";
        g_drv.interface.driver_h = &g_hw_token;
        g_drv.interface.initialize = HwInitialize;
        g_drv.interface.transfer_sync = HwTransferSync;
        g_drv.interface.transfer_async = HwTransferAsync;
        g_drv.interface.transmit_sync = HwTransmitSync;
        g_drv.interface.transmit_async = HwTransmitAsync;
        g_drv.interface.receive_sync = HwReceiveSync;
        g_drv.interface.receive_async = HwReceiveAsync;
        g_drv.interface.chip_select = HwChipSelect;

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
            (void)ezSpi_SystemUnregisterHwDriver(&g_drv);
        }
    }

    /* Register a handle and initialize the driver. */
    void BindAndInit(ezSpiHandle_t *handle, ezDrvCallback cb = OnEvent, void *ctx = NULL)
    {
        HwInitialize_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezSpi_RegisterInstance(handle, "spi_mock", cb, ctx) == EZ_DRV_STATUS_OK);
        REQUIRE(ezSpi_Initialize(handle, &config) == EZ_DRV_STATUS_OK);
    }

    ezSpiHandle_t first;
    ezSpiHandle_t second;
    ezSpiConfig_t config;
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
        THEN("It starts idle and not initialized")
        {
            CHECK(g_drv.initialized == false);
            CHECK(g_drv.base.state == 0U);
            CHECK(g_drv.base.user == NULL);
        }
    }

    WHEN("A driver is unregistered")
    {
        g_drv.initialized = true;

        THEN("It is reset")
        {
            CHECK(ezSpi_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
            driver_registered = false;
            CHECK(g_drv.initialized == false);
            CHECK(ezSpi_RegisterInstance(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("A driver is unregistered while an operation is in flight")
    {
        BindAndInit(&first);
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezSpi_TransmitAsync(&first, g_buf, sizeof(g_buf)) == EZ_DRV_STATUS_OK);

        THEN("The unregistration is refused")
        {
            CHECK(ezSpi_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_BUSY);
        }
    }
}


TEST_CASE_METHOD(SpiTestFixture, "Test instance registration lifecycle", "[hal][spi]")
{
    WHEN("Invalid arguments are used")
    {
        THEN("The APIs report an argument error")
        {
            CHECK(ezSpi_RegisterInstance(NULL, "spi_mock", NULL, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezSpi_RegisterInstance(&first, NULL, NULL, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezSpi_UnregisterInstance(NULL) == EZ_DRV_STATUS_ERR_ARG);
        }
    }

    WHEN("The target driver does not exist")
    {
        THEN("The registration fails with driver-not-found")
        {
            CHECK(ezSpi_RegisterInstance(&first, "unknown", OnEvent, NULL) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("A handle which is already bound is registered again")
    {
        int first_ctx = 0;
        REQUIRE(ezSpi_RegisterInstance(&first, "spi_mock", OnEvent, &first_ctx) == EZ_DRV_STATUS_OK);

        THEN("The registration is refused and the binding is unchanged")
        {
            CHECK(ezSpi_RegisterInstance(&first, "spi_mock", NULL, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(first.drv_handle.hw == &g_drv);
            CHECK(first.drv_handle.on_event == OnEvent);
            CHECK(first.drv_handle.ctx == &first_ctx);
        }

        THEN("It can be registered again after it is unregistered")
        {
            REQUIRE(ezSpi_UnregisterInstance(&first) == EZ_DRV_STATUS_OK);
            CHECK(ezSpi_RegisterInstance(&first, "spi_mock", NULL, NULL) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.on_event == NULL);
        }
    }

    WHEN("A handle with an operation in flight is registered again")
    {
        BindAndInit(&first);
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezSpi_TransmitAsync(&first, g_buf, sizeof(g_buf)) == EZ_DRV_STATUS_OK);

        THEN("The registration is refused and the operation is not disturbed")
        {
            CHECK(ezSpi_RegisterInstance(&first, "spi_mock", NULL, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(IsBusy() == true);
            SimulateIsrDone(EZ_SPI_EVENT_CMPLT);
            CHECK(OnEvent_fake.call_count == 1U);
        }
    }

    WHEN("A handle is registered")
    {
        int user_ctx = 0;
        REQUIRE(ezSpi_RegisterInstance(&first, "spi_mock", OnEvent, &user_ctx) == EZ_DRV_STATUS_OK);

        THEN("It stores the driver, the callback and the context")
        {
            CHECK(first.drv_handle.hw == &g_drv);
            CHECK(first.drv_handle.on_event == OnEvent);
            CHECK(first.drv_handle.ctx == &user_ctx);
        }

        AND_WHEN("It is unregistered")
        {
            THEN("Everything is cleared")
            {
                CHECK(ezSpi_UnregisterInstance(&first) == EZ_DRV_STATUS_OK);
                CHECK(first.drv_handle.hw == NULL);
                CHECK(first.drv_handle.on_event == NULL);
                CHECK(first.drv_handle.ctx == NULL);
            }
        }
    }

    WHEN("A handle with an operation in flight is unregistered")
    {
        BindAndInit(&first);
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezSpi_TransmitAsync(&first, g_buf, sizeof(g_buf)) == EZ_DRV_STATUS_OK);

        THEN("The unregistration is refused until the operation ends")
        {
            CHECK(ezSpi_UnregisterInstance(&first) == EZ_DRV_STATUS_BUSY);
            CHECK(first.drv_handle.hw == &g_drv);
            SimulateIsrDone(EZ_SPI_EVENT_CMPLT);
            CHECK(ezSpi_UnregisterInstance(&first) == EZ_DRV_STATUS_OK);
        }
    }

    WHEN("Another handle is unregistered while a handle has an operation in flight")
    {
        BindAndInit(&first);
        REQUIRE(ezSpi_RegisterInstance(&second, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
        HwTransmitAsync_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezSpi_TransmitAsync(&first, g_buf, sizeof(g_buf)) == EZ_DRV_STATUS_OK);

        THEN("It is allowed and does not disturb the operation")
        {
            CHECK(ezSpi_UnregisterInstance(&second) == EZ_DRV_STATUS_OK);
            CHECK(IsBusy() == true);
        }
    }
}


TEST_CASE_METHOD(SpiTestFixture, "Test initialization", "[hal][spi]")
{
    WHEN("The handle is NULL or not registered")
    {
        THEN("The API reports driver-not-found")
        {
            CHECK(ezSpi_Initialize(NULL, &config) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezSpi_Initialize(&first, &config) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("The config is NULL")
    {
        REQUIRE(ezSpi_RegisterInstance(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);

        THEN("The API reports an argument error and the HW is not called")
        {
            CHECK(ezSpi_Initialize(&first, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(HwInitialize_fake.call_count == 0U);
        }
    }

    WHEN("The HW driver has no initialize function")
    {
        g_drv.interface.initialize = NULL;
        REQUIRE(ezSpi_RegisterInstance(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);

        THEN("The API reports interface-not-exist and releases the driver")
        {
            CHECK(ezSpi_Initialize(&first, &config) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(g_drv.initialized == false);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW initialization fails")
    {
        REQUIRE(ezSpi_RegisterInstance(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
        HwInitialize_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;

        THEN("The status is forwarded, the driver stays not initialized and is released")
        {
            CHECK(ezSpi_Initialize(&first, &config) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(g_drv.initialized == false);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW initialization succeeds")
    {
        REQUIRE(ezSpi_RegisterInstance(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
        HwInitialize_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The HW gets driver_h and the config, and the driver is initialized and idle")
        {
            CHECK(ezSpi_Initialize(&first, &config) == EZ_DRV_STATUS_OK);
            CHECK(HwInitialize_fake.arg0_val == &g_hw_token);
            CHECK(HwInitialize_fake.arg1_val == &config);
            CHECK(g_drv.initialized == true);
            CHECK(IsBusy() == false);
        }
    }
}


TEST_CASE_METHOD(SpiTestFixture, "Test operations require initialization", "[hal][spi]")
{
    REQUIRE(ezSpi_RegisterInstance(&first, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);

    WHEN("The driver is not initialized")
    {
        THEN("Every operation reports not-initialized and the HW is not called")
        {
            CHECK(ezSpi_TransferSync(&first, g_buf, 1, g_buf, 1, 10) == EZ_DRV_STATUS_ERR_NOT_INITIALIZED);
            CHECK(ezSpi_TransferAsync(&first, g_buf, 1, g_buf, 1) == EZ_DRV_STATUS_ERR_NOT_INITIALIZED);
            CHECK(ezSpi_TransmitSync(&first, g_buf, 1, 10) == EZ_DRV_STATUS_ERR_NOT_INITIALIZED);
            CHECK(ezSpi_TransmitAsync(&first, g_buf, 1) == EZ_DRV_STATUS_ERR_NOT_INITIALIZED);
            CHECK(ezSpi_ReceiveSync(&first, g_buf, 1, 10) == EZ_DRV_STATUS_ERR_NOT_INITIALIZED);
            CHECK(ezSpi_ReceiveAsync(&first, g_buf, 1) == EZ_DRV_STATUS_ERR_NOT_INITIALIZED);
            CHECK(ezSpi_ChipSelect(&first, true) == EZ_DRV_STATUS_ERR_NOT_INITIALIZED);
            CHECK(HwTransferSync_fake.call_count == 0U);
            CHECK(HwTransmitAsync_fake.call_count == 0U);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The handle is NULL or not registered")
    {
        THEN("Every operation reports driver-not-found")
        {
            CHECK(ezSpi_TransferSync(NULL, g_buf, 1, g_buf, 1, 10) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezSpi_ChipSelect(&second, true) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
        }
    }
}


TEST_CASE_METHOD(SpiTestFixture, "Test sync operations", "[hal][spi]")
{
    BindAndInit(&first);

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

    WHEN("TransmitSync, ReceiveSync and ChipSelect are called")
    {
        HwTransmitSync_fake.return_val = EZ_DRV_STATUS_OK;
        HwReceiveSync_fake.return_val = EZ_DRV_STATUS_OK;
        HwChipSelect_fake.return_val = EZ_DRV_STATUS_OK;

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

            CHECK(ezSpi_ChipSelect(&first, true) == EZ_DRV_STATUS_OK);
            CHECK(HwChipSelect_fake.arg1_val == true);
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
        REQUIRE(ezSpi_RegisterInstance(&second, "spi_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
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
    BindAndInit(&first, OnEvent, &user_ctx);
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
            CHECK(ezSpi_ChipSelect(&first, true) == EZ_DRV_STATUS_BUSY);
            CHECK(ezSpi_Initialize(&first, &config) == EZ_DRV_STATUS_BUSY);
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
        REQUIRE(ezSpi_RegisterInstance(&quiet, "spi_mock", NULL, NULL) == EZ_DRV_STATUS_OK);
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

/* End of file */
