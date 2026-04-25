/*****************************************************************************
* Filename:         unittest_ez_gpio.cpp
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

/** @file   unittest_ez_gpio.cpp
 *  @author Hai Nguyen
 *  @date   25.04.2026
 *  @brief  Unit tests for the GPIO HAL component
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
#include "ez_gpio.h"
#include "fff.h"

#include <catch2/catch_test_macros.hpp>


/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwInitPin, ezDriver_t *, uint16_t, ezHwGpioConfig_t *);
FAKE_VALUE_FUNC(EZ_GPIO_PIN_STATE, HwReadPin, ezDriver_t *, uint16_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwWritePin, ezDriver_t *, uint16_t, EZ_GPIO_PIN_STATE);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTogglePin, ezDriver_t *, uint16_t);
FAKE_VOID_FUNC(InstanceCallback, uint8_t, void *, void *);


/******************************************************************************
* Module Typedefs
*******************************************************************************/
typedef struct
{
    uint8_t event_code;
    void *param1;
    void *param2;
} ezGpioEvent_t;


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static struct ezGpioDriver g_mock_driver;
static ezGpioEvent_t g_event;


/******************************************************************************
* Function Definitions
*******************************************************************************/
static void ResetEvent(void);

class GpioTestFixture
{
public:
    GpioTestFixture();
    ~GpioTestFixture();

protected:
    void RegisterDriver(void);
    void RegisterFirstInstance(void);
    void RegisterSecondInstance(void);
    void RegisterBothInstances(void);

    ezGpioDrvInstance_t first_inst{};
    ezGpioDrvInstance_t second_inst{};
    ezHwGpioConfig_t config{};
    bool driver_registered;
    bool first_registered;
    bool second_registered;
};


/******************************************************************************
* External functions
*******************************************************************************/
TEST_CASE_METHOD(GpioTestFixture, "Test driver registration", "[hal][gpio]")
{
    WHEN("A NULL hardware driver is provided")
    {
        /* Verify registration entry points reject invalid arguments. */
        THEN("The APIs report an argument error")
        {
            CHECK(ezGpio_SystemRegisterHwDriver(NULL) == STATUS_ERR_ARG);
            CHECK(ezGpio_SystemUnregisterHwDriver(NULL) == STATUS_ERR_ARG);
        }
    }

    WHEN("A valid GPIO driver is registered")
    {
        RegisterDriver();

        THEN("The driver is reset and the receive callback is installed")
        {
            CHECK(g_mock_driver.initialized == false);
            CHECK(g_mock_driver.common.callback != NULL);
        }

        AND_WHEN("The same driver is unregistered")
        {
            g_mock_driver.initialized = true;

            THEN("The driver is removed cleanly")
            {
                CHECK(ezGpio_SystemUnregisterHwDriver(&g_mock_driver) == STATUS_OK);
                driver_registered = false;
                CHECK(g_mock_driver.initialized == false);
            }
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test instance registration lifecycle", "[hal][gpio]")
{
    WHEN("Invalid instance arguments are used")
    {
        /* Confirm register and unregister perform basic argument validation. */
        THEN("The APIs report argument errors")
        {
            CHECK(ezGpio_RegisterInstance(NULL, NULL, NULL) == STATUS_ERR_ARG);
            CHECK(ezGpio_UnregisterInstance(NULL) == STATUS_ERR_ARG);
        }
    }

    WHEN("The target driver name is unknown")
    {
        RegisterDriver();

        THEN("The instance registration fails with driver-not-found")
        {
            CHECK(ezGpio_RegisterInstance(&first_inst, "Unknown driver", InstanceCallback) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("A valid instance is registered")
    {
        RegisterDriver();

        THEN("The instance stores the selected driver and callback")
        {
            CHECK(ezGpio_RegisterInstance(&first_inst, g_mock_driver.common.name, InstanceCallback) == STATUS_OK);
            first_registered = true;
            CHECK(first_inst.driver == &g_mock_driver);
            CHECK(first_inst.callback == InstanceCallback);
        }

        AND_WHEN("The instance is unregistered")
        {
            REQUIRE(ezGpio_RegisterInstance(&first_inst, g_mock_driver.common.name, InstanceCallback) == STATUS_OK);
            first_registered = true;

            THEN("The driver reference is cleared")
            {
                CHECK(ezGpio_UnregisterInstance(&first_inst) == STATUS_OK);
                first_registered = false;
                CHECK(first_inst.driver == NULL);
            }
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test initialization", "[hal][gpio]")
{
    const uint16_t pin_index = 0x01U;

    WHEN("The instance is NULL or not registered")
    {
        /* The API cannot resolve a backing driver in this state. */
        THEN("The API returns driver-not-found")
        {
            CHECK(ezGpio_Initialize(NULL, pin_index, &config) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezGpio_Initialize(&first_inst, pin_index, &config) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No GPIO init interface is implemented")
    {
        THEN("The API reports a missing interface and unlocks the driver")
        {
            CHECK(ezGpio_Initialize(&first_inst, pin_index, &config) == STATUS_ERR_INF_NOT_EXIST);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("Another instance already owns the driver")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezGpio_Initialize(&second_inst, pin_index, &config) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware init fake reports a timeout")
    {
        g_mock_driver.interface.init_pin = HwInitPin;
        HwInitPin_fake.return_val = STATUS_TIMEOUT;

        THEN("The failure is propagated")
        {
            CHECK(ezGpio_Initialize(&first_inst, pin_index, &config) == STATUS_TIMEOUT);
            CHECK(HwInitPin_fake.call_count == 1);
            CHECK(HwInitPin_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwInitPin_fake.arg1_val == pin_index);
            CHECK(HwInitPin_fake.arg2_val == &config);
            CHECK(g_mock_driver.common.curr_inst == NULL);
            CHECK(g_mock_driver.initialized == false);
        }
    }

    WHEN("The hardware init fake succeeds")
    {
        g_mock_driver.interface.init_pin = HwInitPin;
        HwInitPin_fake.return_val = STATUS_OK;

        THEN("The driver is marked initialized and arguments are forwarded")
        {
            CHECK(ezGpio_Initialize(&first_inst, pin_index, &config) == STATUS_OK);
            CHECK(HwInitPin_fake.call_count == 1);
            CHECK(HwInitPin_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwInitPin_fake.arg1_val == pin_index);
            CHECK(HwInitPin_fake.arg2_val == &config);
            CHECK(g_mock_driver.initialized == true);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test read pin", "[hal][gpio]")
{
    const uint16_t pin_index = 0x02U;

    WHEN("The instance is NULL or not registered")
    {
        THEN("The API reports a pin error")
        {
            CHECK(ezGpio_ReadPin(NULL, pin_index) == EZ_GPIO_PIN_ERROR);
            CHECK(ezGpio_ReadPin(&first_inst, pin_index) == EZ_GPIO_PIN_ERROR);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No read interface is implemented")
    {
        THEN("The API reports a pin error and unlocks the driver")
        {
            CHECK(ezGpio_ReadPin(&first_inst, pin_index) == EZ_GPIO_PIN_ERROR);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("Another instance already owns the driver")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports a pin error")
        {
            CHECK(ezGpio_ReadPin(&second_inst, pin_index) == EZ_GPIO_PIN_ERROR);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware read fake returns HIGH")
    {
        g_mock_driver.interface.read_pin = HwReadPin;
        HwReadPin_fake.return_val = EZ_GPIO_PIN_HIGH;

        THEN("The pin state is forwarded to the caller")
        {
            CHECK(ezGpio_ReadPin(&first_inst, pin_index) == EZ_GPIO_PIN_HIGH);
            CHECK(HwReadPin_fake.call_count == 1);
            CHECK(HwReadPin_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwReadPin_fake.arg1_val == pin_index);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test write pin", "[hal][gpio]")
{
    const uint16_t pin_index = 0x03U;

    WHEN("The instance is NULL or not registered")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezGpio_WritePin(NULL, pin_index, EZ_GPIO_PIN_HIGH) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezGpio_WritePin(&first_inst, pin_index, EZ_GPIO_PIN_HIGH) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No write interface is implemented")
    {
        THEN("The API reports a missing interface")
        {
            CHECK(ezGpio_WritePin(&first_inst, pin_index, EZ_GPIO_PIN_HIGH) == STATUS_ERR_INF_NOT_EXIST);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("Another instance already owns the driver")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezGpio_WritePin(&second_inst, pin_index, EZ_GPIO_PIN_HIGH) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware write fake reports a timeout")
    {
        g_mock_driver.interface.write_pin = HwWritePin;
        HwWritePin_fake.return_val = STATUS_TIMEOUT;

        THEN("The failure is propagated")
        {
            CHECK(ezGpio_WritePin(&first_inst, pin_index, EZ_GPIO_PIN_HIGH) == STATUS_TIMEOUT);
            CHECK(HwWritePin_fake.call_count == 1);
            CHECK(HwWritePin_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwWritePin_fake.arg1_val == pin_index);
            CHECK(HwWritePin_fake.arg2_val == EZ_GPIO_PIN_HIGH);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("The hardware write fake succeeds")
    {
        g_mock_driver.interface.write_pin = HwWritePin;
        HwWritePin_fake.return_val = STATUS_OK;

        THEN("The state and pin index are forwarded to the hardware layer")
        {
            CHECK(ezGpio_WritePin(&first_inst, pin_index, EZ_GPIO_PIN_HIGH) == STATUS_OK);
            CHECK(HwWritePin_fake.call_count == 1);
            CHECK(HwWritePin_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwWritePin_fake.arg1_val == pin_index);
            CHECK(HwWritePin_fake.arg2_val == EZ_GPIO_PIN_HIGH);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test toggle pin", "[hal][gpio]")
{
    const uint16_t pin_index = 0x04U;

    WHEN("The instance is NULL or not registered")
    {
        THEN("The API returns driver-not-found")
        {
            CHECK(ezGpio_TogglePin(NULL, pin_index) == STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezGpio_TogglePin(&first_inst, pin_index) == STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    RegisterDriver();
    RegisterBothInstances();

    WHEN("No toggle interface is implemented")
    {
        THEN("The API reports a missing interface")
        {
            CHECK(ezGpio_TogglePin(&first_inst, pin_index) == STATUS_ERR_INF_NOT_EXIST);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("Another instance already owns the driver")
    {
        g_mock_driver.common.curr_inst = &first_inst;

        THEN("The API reports busy")
        {
            CHECK(ezGpio_TogglePin(&second_inst, pin_index) == STATUS_BUSY);
        }

        g_mock_driver.common.curr_inst = NULL;
    }

    WHEN("The hardware toggle fake reports a timeout")
    {
        g_mock_driver.interface.toggle_pin = HwTogglePin;
        HwTogglePin_fake.return_val = STATUS_TIMEOUT;

        THEN("The failure is propagated")
        {
            CHECK(ezGpio_TogglePin(&first_inst, pin_index) == STATUS_TIMEOUT);
            CHECK(HwTogglePin_fake.call_count == 1);
            CHECK(HwTogglePin_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwTogglePin_fake.arg1_val == pin_index);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }

    WHEN("The hardware toggle fake succeeds")
    {
        g_mock_driver.interface.toggle_pin = HwTogglePin;
        HwTogglePin_fake.return_val = STATUS_OK;

        THEN("The pin index is forwarded to the hardware layer")
        {
            CHECK(ezGpio_TogglePin(&first_inst, pin_index) == STATUS_OK);
            CHECK(HwTogglePin_fake.call_count == 1);
            CHECK(HwTogglePin_fake.arg0_val == &g_mock_driver.common);
            CHECK(HwTogglePin_fake.arg1_val == pin_index);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test callback forwarding", "[hal][gpio]")
{
    WHEN("A GPIO driver receives an event while an instance owns the driver")
    {
        RegisterDriver();
        RegisterFirstInstance();

        g_mock_driver.common.curr_inst = &first_inst;
        g_event.event_code = 0x10U;
        g_event.param1 = (void*)0x1234;
        g_event.param2 = (void*)0x5678;

        THEN("The registered instance callback is invoked and the driver unlocks")
        {
            REQUIRE(g_mock_driver.common.callback != NULL);

            g_mock_driver.common.callback(&g_mock_driver,
                                          g_event.event_code,
                                          g_event.param1,
                                          g_event.param2);

            CHECK(InstanceCallback_fake.call_count == 1);
            CHECK(InstanceCallback_fake.arg0_val == g_event.event_code);
            CHECK(InstanceCallback_fake.arg1_val == g_event.param1);
            CHECK(InstanceCallback_fake.arg2_val == g_event.param2);
            CHECK(g_mock_driver.common.curr_inst == NULL);
        }
    }
}


/******************************************************************************
* Internal functions
*******************************************************************************/
GpioTestFixture::GpioTestFixture()
    : driver_registered(false),
      first_registered(false),
      second_registered(false)
{
    memset(&g_mock_driver, 0, sizeof(g_mock_driver));
    ResetEvent();

    RESET_FAKE(HwInitPin);
    RESET_FAKE(HwReadPin);
    RESET_FAKE(HwWritePin);
    RESET_FAKE(HwTogglePin);
    RESET_FAKE(InstanceCallback);

    g_mock_driver.common.name = "Mock GPIO Driver";
    g_mock_driver.common.version[0] = 1;
    g_mock_driver.common.version[1] = 0;
    g_mock_driver.common.version[2] = 0;

    config.mode = EZ_GPIO_MODE_OUTPUT;
    config.pull = EZ_GPIO_NO_PULL;
    config.intr_mode = EZ_INTTERTUP_NONE;
}


GpioTestFixture::~GpioTestFixture()
{
    if(first_registered)
    {
        (void)ezGpio_UnregisterInstance(&first_inst);
    }

    if(second_registered)
    {
        (void)ezGpio_UnregisterInstance(&second_inst);
    }

    if(driver_registered)
    {
        (void)ezGpio_SystemUnregisterHwDriver(&g_mock_driver);
    }
}


void GpioTestFixture::RegisterDriver(void)
{
    REQUIRE(ezGpio_SystemRegisterHwDriver(&g_mock_driver) == STATUS_OK);
    driver_registered = true;
}


void GpioTestFixture::RegisterFirstInstance(void)
{
    REQUIRE(ezGpio_RegisterInstance(&first_inst, g_mock_driver.common.name, InstanceCallback) == STATUS_OK);
    first_registered = true;
}


void GpioTestFixture::RegisterSecondInstance(void)
{
    REQUIRE(ezGpio_RegisterInstance(&second_inst, g_mock_driver.common.name, InstanceCallback) == STATUS_OK);
    second_registered = true;
}


void GpioTestFixture::RegisterBothInstances(void)
{
    RegisterFirstInstance();
    RegisterSecondInstance();
}


static void ResetEvent(void)
{
    memset(&g_event, 0, sizeof(g_event));
}


/* End of file */
