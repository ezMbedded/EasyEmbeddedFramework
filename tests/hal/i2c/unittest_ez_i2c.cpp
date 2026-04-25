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
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwInitialize, ezDriver_t *, ezI2cConfig_t *);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTransmitSync, ezDriver_t *, uint16_t, const uint8_t *, size_t, bool, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTransmitAsync, ezDriver_t *, uint16_t, const uint8_t *, size_t, bool);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwReceiveSync, ezDriver_t *, uint16_t, uint8_t *, size_t, bool, uint32_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwReceiveAsync, ezDriver_t *, uint16_t, uint8_t *, size_t, bool);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwProbe, ezDriver_t *, uint16_t, uint32_t);
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
} ezI2cAsyncEvent_t;


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static struct ezI2cDriver g_mock_driver;
static ezI2cAsyncEvent_t g_async_event;


/******************************************************************************
* Function Definitions
*******************************************************************************/
static void ResetAsyncEvent(void);
static EZ_DRV_STATUS HwTransmitAsyncCustom(ezDriver_t *driver_h,
                                           uint16_t address,
                                           const uint8_t *data,
                                           size_t length,
                                           bool send_stop);
static EZ_DRV_STATUS HwReceiveAsyncCustom(ezDriver_t *driver_h,
                                          uint16_t address,
                                          uint8_t *data,
                                          size_t length,
                                          bool send_stop);

class I2cTestFixture
{
public:
    I2cTestFixture();
    ~I2cTestFixture();

protected:
    void RegisterDriver(void);
    void RegisterFirstInstance(void);
    void RegisterSecondInstance(void);
    void RegisterBothInstances(void);
    void ImplementAllInterfaces(void);

    ezI2cDrvInstance_t first_inst{};
    ezI2cDrvInstance_t second_inst{};
    ezI2cConfig_t config{};
    bool driver_registered;
    bool first_registered;
    bool second_registered;
};


/******************************************************************************
* External functions
*******************************************************************************/
TEST_CASE_METHOD(I2cTestFixture, "Test driver registration", "[hal][i2c]")
{
    WHEN("A NULL hardware driver is provided")
    {
        /* Verify the API rejects invalid input before touching driver state. */
        THEN("The API reports an argument error")
        {
            CHECK(ezI2c_SystemRegisterHwDriver(NULL) == STATUS_ERR_ARG);
            CHECK(ezI2c_SystemUnregisterHwDriver(NULL) == STATUS_ERR_ARG);
        }
    }

    WHEN("A valid hardware driver is registered")
    {
        RegisterDriver();

        THEN("The driver is added in a known default state")
        {
            CHECK(g_mock_driver.initialized == false);
            CHECK(g_mock_driver.common.callback != NULL);
        }

        AND_WHEN("The same driver is unregistered")
        {
            g_mock_driver.initialized = true;

            THEN("The driver is reset and removed cleanly")
            {
                CHECK(ezI2c_SystemUnregisterHwDriver(&g_mock_driver) == STATUS_OK);
                driver_registered = false;
                CHECK(g_mock_driver.initialized == false);
            }
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test instance registration lifecycle", "[hal][i2c]")
{
    WHEN("Invalid instance registration arguments are used")
    {
        /* Validate direct argument guards for register and unregister APIs. */
        THEN("The APIs report argument errors")
        {
            CHECK(ezI2c_RegisterInstance(NULL, NULL, NULL) == STATUS_ERR_ARG);
            CHECK(ezI2c_UnregisterInstance(NULL) == STATUS_ERR_ARG);
        }
    }

    WHEN("The target driver does not exist")
    {
        RegisterDriver();

        THEN("The instance registration fails with driver-not-found")
        {
            CHECK(ezI2c_RegisterInstance(&first_inst, "Unknown driver", InstanceCallback) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("A valid instance is registered to an existing driver")
    {
        RegisterDriver();

        THEN("The instance stores the driver and callback pointers")
        {
            CHECK(ezI2c_RegisterInstance(&first_inst, g_mock_driver.common.name, InstanceCallback) == STATUS_OK);
            first_registered = true;
            CHECK(first_inst.driver == &g_mock_driver);
            CHECK(first_inst.callback == InstanceCallback);
        }

        AND_WHEN("The instance is later unregistered")
        {
            REQUIRE(ezI2c_RegisterInstance(&first_inst, g_mock_driver.common.name, InstanceCallback) == STATUS_OK);
            first_registered = true;

            THEN("The driver reference is cleared")
            {
                CHECK(ezI2c_UnregisterInstance(&first_inst) == STATUS_OK);
                first_registered = false;
                CHECK(first_inst.driver == NULL);
            }
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test initialization", "[hal][i2c]")
{
    WHEN("The instance is NULL or not registered")
    {
        /* Confirm the API refuses initialization when no backing driver exists. */
        THEN("The API returns driver-not-found")
        {
            CHECK(ezI2c_Initialize(NULL, &config) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_Initialize(&first_inst, &config) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No hardware initialize function is implemented")
    {
        THEN("The default generic error is returned and the lock is released")
        {
            CHECK(ezI2c_Initialize(&first_inst, &config) == STATUS_ERR_GENERIC);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("The driver is already locked by another instance")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports a busy driver")
        {
            CHECK(ezI2c_Initialize(&second_inst, &config) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware initialize fake reports a failure")
    {
        g_mock_driver.interface.initialize = HwInitialize;
        HwInitialize_fake.return_val = STATUS_TIMEOUT;

        THEN("The failure is propagated and the driver stays uninitialized")
        {
            CHECK(ezI2c_Initialize(&first_inst, &config) == STATUS_TIMEOUT);
            CHECK(HwInitialize_fake.call_count == 1);
            CHECK(g_mock_driver.initialized == false);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("The hardware initialize fake succeeds")
    {
        g_mock_driver.interface.initialize = HwInitialize;
        HwInitialize_fake.return_val = STATUS_OK;

        THEN("The driver is initialized and the hardware call arguments are forwarded")
        {
            CHECK(ezI2c_Initialize(&first_inst, &config) == STATUS_OK);
            CHECK(HwInitialize_fake.call_count == 1);
            CHECK(HwInitialize_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwInitialize_fake.arg1_val == &config);
            CHECK(g_mock_driver.initialized == true);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test sync transmit", "[hal][i2c]")
{
    static const uint8_t tx_data[] = {0xDE, 0xAD, 0xBE, 0xEF};

    WHEN("The instance is NULL or not registered")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezI2c_TransmitSync(NULL, 0x52, tx_data, sizeof(tx_data), true, 1000U) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_TransmitSync(&first_inst, 0x52, tx_data, sizeof(tx_data), true, 1000U) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No synchronous transmit interface is implemented")
    {
        THEN("The API returns driver-not-found and keeps the driver unlocked")
        {
            CHECK(ezI2c_TransmitSync(&first_inst, 0x52, tx_data, sizeof(tx_data), true, 1000U) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("The driver is locked by another instance")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezI2c_TransmitSync(&second_inst, 0x52, tx_data, sizeof(tx_data), true, 1000U) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware synchronous transmit fake reports a timeout")
    {
        g_mock_driver.interface.transmit_sync = HwTransmitSync;
        HwTransmitSync_fake.return_val = STATUS_TIMEOUT;

        THEN("The timeout is propagated and the driver lock is released")
        {
            CHECK(ezI2c_TransmitSync(&first_inst, 0x52, tx_data, sizeof(tx_data), true, 1000U) == STATUS_TIMEOUT);
            CHECK(HwTransmitSync_fake.call_count == 1);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("The hardware synchronous transmit fake succeeds")
    {
        g_mock_driver.interface.transmit_sync = HwTransmitSync;
        HwTransmitSync_fake.return_val = STATUS_OK;

        THEN("The transmit arguments are forwarded to the hardware layer")
        {
            CHECK(ezI2c_TransmitSync(&first_inst, 0x52, tx_data, sizeof(tx_data), true, 1000U) == STATUS_OK);
            CHECK(HwTransmitSync_fake.call_count == 1);
            CHECK(HwTransmitSync_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwTransmitSync_fake.arg1_val == 0x52);
            CHECK(HwTransmitSync_fake.arg2_val == tx_data);
            CHECK(HwTransmitSync_fake.arg3_val == sizeof(tx_data));
            CHECK(HwTransmitSync_fake.arg4_val == true);
            CHECK(HwTransmitSync_fake.arg5_val == 1000U);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test async transmit", "[hal][i2c]")
{
    static const uint8_t tx_data[] = {0x11, 0x22, 0x33};

    WHEN("The instance is NULL or not registered")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezI2c_TransmitAsync(NULL, 0x62, tx_data, sizeof(tx_data), false) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_TransmitAsync(&first_inst, 0x62, tx_data, sizeof(tx_data), false) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No asynchronous transmit interface is implemented")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezI2c_TransmitAsync(&first_inst, 0x62, tx_data, sizeof(tx_data), false) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("The driver is locked by another instance")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezI2c_TransmitAsync(&second_inst, 0x62, tx_data, sizeof(tx_data), false) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware asynchronous transmit fake reports a failure")
    {
        g_mock_driver.interface.transmit_async = HwTransmitAsync;
        HwTransmitAsync_fake.custom_fake = NULL;
        HwTransmitAsync_fake.return_val = STATUS_TIMEOUT;

        THEN("The failure is returned to the caller")
        {
            CHECK(ezI2c_TransmitAsync(&first_inst, 0x62, tx_data, sizeof(tx_data), false) == STATUS_TIMEOUT);
            CHECK(HwTransmitAsync_fake.call_count == 1);
        }
    }

    WHEN("The hardware asynchronous transmit fake succeeds and triggers a callback")
    {
        g_mock_driver.interface.transmit_async = HwTransmitAsync;
        HwTransmitAsync_fake.custom_fake = HwTransmitAsyncCustom;
        HwTransmitAsync_fake.return_val = STATUS_OK;
        g_async_event.fire_async_callback = true;
        g_async_event.event_code = 0xA1U;
        g_async_event.param1 = (void*)0x1234;
        g_async_event.param2 = (void*)0x5678;

        THEN("The callback is forwarded to the registered client and the driver unlocks")
        {
            CHECK(ezI2c_TransmitAsync(&first_inst, 0x62, tx_data, sizeof(tx_data), false) == STATUS_OK);
            CHECK(HwTransmitAsync_fake.call_count == 1);
            CHECK(HwTransmitAsync_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwTransmitAsync_fake.arg1_val == 0x62);
            CHECK(HwTransmitAsync_fake.arg2_val == tx_data);
            CHECK(HwTransmitAsync_fake.arg3_val == sizeof(tx_data));
            CHECK(HwTransmitAsync_fake.arg4_val == false);
            CHECK(InstanceCallback_fake.call_count == 1);
            CHECK(InstanceCallback_fake.arg0_val == g_async_event.event_code);
            CHECK(InstanceCallback_fake.arg1_val == g_async_event.param1);
            CHECK(InstanceCallback_fake.arg2_val == g_async_event.param2);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test sync receive", "[hal][i2c]")
{
    uint8_t rx_data[4] = {0};

    WHEN("The instance is NULL or not registered")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezI2c_ReceiveSync(NULL, 0x27, rx_data, sizeof(rx_data), true, 200U) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_ReceiveSync(&first_inst, 0x27, rx_data, sizeof(rx_data), true, 200U) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No synchronous receive interface is implemented")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezI2c_ReceiveSync(&first_inst, 0x27, rx_data, sizeof(rx_data), true, 200U) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("The driver is locked by another instance")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezI2c_ReceiveSync(&second_inst, 0x27, rx_data, sizeof(rx_data), true, 200U) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware synchronous receive fake reports a timeout")
    {
        g_mock_driver.interface.receive_sync = HwReceiveSync;
        HwReceiveSync_fake.return_val = STATUS_TIMEOUT;

        THEN("The timeout is propagated and the driver unlocks")
        {
            CHECK(ezI2c_ReceiveSync(&first_inst, 0x27, rx_data, sizeof(rx_data), true, 200U) == STATUS_TIMEOUT);
            CHECK(HwReceiveSync_fake.call_count == 1);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("The hardware synchronous receive fake succeeds")
    {
        g_mock_driver.interface.receive_sync = HwReceiveSync;
        HwReceiveSync_fake.return_val = STATUS_OK;

        THEN("The receive arguments are forwarded to the hardware layer")
        {
            CHECK(ezI2c_ReceiveSync(&first_inst, 0x27, rx_data, sizeof(rx_data), true, 200U) == STATUS_OK);
            CHECK(HwReceiveSync_fake.call_count == 1);
            CHECK(HwReceiveSync_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwReceiveSync_fake.arg1_val == 0x27);
            CHECK(HwReceiveSync_fake.arg2_val == rx_data);
            CHECK(HwReceiveSync_fake.arg3_val == sizeof(rx_data));
            CHECK(HwReceiveSync_fake.arg4_val == true);
            CHECK(HwReceiveSync_fake.arg5_val == 200U);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test async receive", "[hal][i2c]")
{
    uint8_t rx_data[3] = {0};

    WHEN("The instance is NULL or not registered")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezI2c_ReceiveAsync(NULL, 0x31, rx_data, sizeof(rx_data), true) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_ReceiveAsync(&first_inst, 0x31, rx_data, sizeof(rx_data), true) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No asynchronous receive interface is implemented")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezI2c_ReceiveAsync(&first_inst, 0x31, rx_data, sizeof(rx_data), true) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("The driver is locked by another instance")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezI2c_ReceiveAsync(&second_inst, 0x31, rx_data, sizeof(rx_data), true) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware asynchronous receive fake reports a failure")
    {
        g_mock_driver.interface.receive_async = HwReceiveAsync;
        HwReceiveAsync_fake.custom_fake = NULL;
        HwReceiveAsync_fake.return_val = STATUS_TIMEOUT;

        THEN("The failure is returned to the caller")
        {
            CHECK(ezI2c_ReceiveAsync(&first_inst, 0x31, rx_data, sizeof(rx_data), true) == STATUS_TIMEOUT);
            CHECK(HwReceiveAsync_fake.call_count == 1);
        }
    }

    WHEN("The hardware asynchronous receive fake succeeds and triggers a callback")
    {
        g_mock_driver.interface.receive_async = HwReceiveAsync;
        HwReceiveAsync_fake.custom_fake = HwReceiveAsyncCustom;
        HwReceiveAsync_fake.return_val = STATUS_OK;
        g_async_event.fire_async_callback = true;
        g_async_event.event_code = 0xB2U;
        g_async_event.param1 = (void*)0xAAAA;
        g_async_event.param2 = (void*)0x5555;

        THEN("The callback is forwarded to the registered client and the driver unlocks")
        {
            CHECK(ezI2c_ReceiveAsync(&first_inst, 0x31, rx_data, sizeof(rx_data), true) == STATUS_OK);
            CHECK(HwReceiveAsync_fake.call_count == 1);
            CHECK(HwReceiveAsync_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwReceiveAsync_fake.arg1_val == 0x31);
            CHECK(HwReceiveAsync_fake.arg2_val == rx_data);
            CHECK(HwReceiveAsync_fake.arg3_val == sizeof(rx_data));
            CHECK(HwReceiveAsync_fake.arg4_val == true);
            CHECK(InstanceCallback_fake.call_count == 1);
            CHECK(InstanceCallback_fake.arg0_val == g_async_event.event_code);
            CHECK(InstanceCallback_fake.arg1_val == g_async_event.param1);
            CHECK(InstanceCallback_fake.arg2_val == g_async_event.param2);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(I2cTestFixture, "Test probe", "[hal][i2c]")
{
    WHEN("The instance is NULL or not registered")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezI2c_Probe(NULL, 0x44, 50U) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezI2c_Probe(&first_inst, 0x44, 50U) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No probe interface is implemented")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezI2c_Probe(&first_inst, 0x44, 50U) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("The driver is locked by another instance")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezI2c_Probe(&second_inst, 0x44, 50U) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware probe fake reports a timeout")
    {
        g_mock_driver.interface.probe = HwProbe;
        HwProbe_fake.return_val = STATUS_TIMEOUT;

        THEN("The timeout is propagated to the caller")
        {
            CHECK(ezI2c_Probe(&first_inst, 0x44, 50U) == STATUS_TIMEOUT);
            CHECK(HwProbe_fake.call_count == 1);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("The hardware probe fake succeeds")
    {
        g_mock_driver.interface.probe = HwProbe;
        HwProbe_fake.return_val = STATUS_OK;

        THEN("The probe arguments are forwarded to the hardware layer")
        {
            CHECK(ezI2c_Probe(&first_inst, 0x44, 50U) == STATUS_OK);
            CHECK(HwProbe_fake.call_count == 1);
            CHECK(HwProbe_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwProbe_fake.arg1_val == 0x44);
            CHECK(HwProbe_fake.arg2_val == 50U);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


/******************************************************************************
* Internal functions
*******************************************************************************/
I2cTestFixture::I2cTestFixture()
    : driver_registered(false),
      first_registered(false),
      second_registered(false)
{
    memset(&g_mock_driver, 0, sizeof(g_mock_driver));
    ResetAsyncEvent();
    RESET_FAKE(HwInitialize);
    RESET_FAKE(HwTransmitSync);
    RESET_FAKE(HwTransmitAsync);
    RESET_FAKE(HwReceiveSync);
    RESET_FAKE(HwReceiveAsync);
    RESET_FAKE(HwProbe);
    RESET_FAKE(InstanceCallback);

    g_mock_driver.common.name = "Mock I2C Driver";
    g_mock_driver.common.version[0] = 1;
    g_mock_driver.common.version[1] = 0;
    g_mock_driver.common.version[2] = 0;

    config.mode = EZ_I2C_MODE_MASTER;
    config.speed = EZ_I2C_SPEED_FAST;
    config.addressing_mode = EZ_I2C_ADDRESSING_MODE_7BIT;
}


I2cTestFixture::~I2cTestFixture()
{
    if(first_registered)
    {
        (void)ezI2c_UnregisterInstance(&first_inst);
    }

    if(second_registered)
    {
        (void)ezI2c_UnregisterInstance(&second_inst);
    }

    if(driver_registered)
    {
        (void)ezI2c_SystemUnregisterHwDriver(&g_mock_driver);
    }
}


void I2cTestFixture::RegisterDriver(void)
{
    REQUIRE(ezI2c_SystemRegisterHwDriver(&g_mock_driver) == STATUS_OK);
    driver_registered = true;
}


void I2cTestFixture::RegisterFirstInstance(void)
{
    REQUIRE(ezI2c_RegisterInstance(&first_inst, g_mock_driver.common.name, InstanceCallback) == STATUS_OK);
    first_registered = true;
}


void I2cTestFixture::RegisterSecondInstance(void)
{
    REQUIRE(ezI2c_RegisterInstance(&second_inst, g_mock_driver.common.name, InstanceCallback) == STATUS_OK);
    second_registered = true;
}


void I2cTestFixture::RegisterBothInstances(void)
{
    RegisterFirstInstance();
    RegisterSecondInstance();
}


void I2cTestFixture::ImplementAllInterfaces(void)
{
    g_mock_driver.interface.initialize = HwInitialize;
    g_mock_driver.interface.transmit_sync = HwTransmitSync;
    g_mock_driver.interface.transmit_async = HwTransmitAsync;
    g_mock_driver.interface.receive_sync = HwReceiveSync;
    g_mock_driver.interface.receive_async = HwReceiveAsync;
    g_mock_driver.interface.probe = HwProbe;
    HwTransmitAsync_fake.custom_fake = HwTransmitAsyncCustom;
    HwReceiveAsync_fake.custom_fake = HwReceiveAsyncCustom;
}


static void ResetAsyncEvent(void)
{
    memset(&g_async_event, 0, sizeof(g_async_event));
}


static EZ_DRV_STATUS HwTransmitAsyncCustom(ezDriver_t *driver_h,
                                           uint16_t address,
                                           const uint8_t *data,
                                           size_t length,
                                           bool send_stop)
{
    (void)address;
    (void)data;
    (void)length;
    (void)send_stop;
    if((g_async_event.fire_async_callback == true) && (driver_h != NULL) && (driver_h->callback != NULL) && (driver_h->curr_inst != NULL))
    {
        driver_h->callback(driver_h->curr_inst->driver,
                           g_async_event.event_code,
                           g_async_event.param1,
                           g_async_event.param2);
    }
    return HwTransmitAsync_fake.return_val;
}


static EZ_DRV_STATUS HwReceiveAsyncCustom(ezDriver_t *driver_h,
                                          uint16_t address,
                                          uint8_t *data,
                                          size_t length,
                                          bool send_stop)
{
    (void)address;
    (void)data;
    (void)length;
    (void)send_stop;
    if((g_async_event.fire_async_callback == true) && (driver_h != NULL) && (driver_h->callback != NULL) && (driver_h->curr_inst != NULL))
    {
        driver_h->callback(driver_h->curr_inst->driver,
                           g_async_event.event_code,
                           g_async_event.param1,
                           g_async_event.param2);
    }
    return HwReceiveAsync_fake.return_val;
}


/* End of file */