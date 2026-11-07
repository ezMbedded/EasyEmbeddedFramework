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
#include <stddef.h>
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
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwInitPin, void *, uint16_t, ezHwGpioConfig_t *);
FAKE_VALUE_FUNC(EZ_GPIO_PIN_STATE, HwReadPin, void *, uint16_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwWritePin, void *, uint16_t, EZ_GPIO_PIN_STATE);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwTogglePin, void *, uint16_t);
FAKE_VOID_FUNC(OnEvent, void *, uint8_t, void *, void *);

#define PIN     (7U)


/******************************************************************************
* Module Typedefs
*******************************************************************************/
/* None */


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static struct ezGpioDriver g_drv;
static int g_hw_token = 0;              /* value of driver_h given to the HW functions */
static ezGpioHandle_t *g_reentrant_handle = NULL;
static EZ_DRV_STATUS g_reentrant_status = EZ_DRV_STATUS_OK;
static uint8_t g_state_in_hw = 0U;      /* state byte seen inside a HW function */


/******************************************************************************
* Function Definitions
*******************************************************************************/
static bool IsBusy(void)
{
    return EZ_DRV_STATE_IS_SET(&g_drv.base, EZ_DRV_STATE_BUSY);
}

static bool IsListening(void)
{
    return EZ_DRV_STATE_IS_SET(&g_drv.base, EZ_DRV_STATE_LISTENING);
}

/* What the ISR of the HW driver does when a pin interrupt fires. */
static void SimulatePinInterrupt(uint16_t *pin)
{
    ezDriver_NotifyEvent(&g_drv.base, EZ_DRV_EVENT_ASYNC, EZ_GPIO_EVENT_PIN_INTERRUPT, pin, NULL);
}

/* A HW read function which calls back into the API while the driver is held. */
static EZ_GPIO_PIN_STATE HwReadPinReentrant(void *driver_h, uint16_t pin_index)
{
    (void)driver_h;
    (void)pin_index;
    g_reentrant_status = ezGpio_UnregisterInstance(g_reentrant_handle);
    return EZ_GPIO_PIN_HIGH;
}

/* A HW read function which sees an interrupt while it runs. */
static EZ_GPIO_PIN_STATE HwReadPinWithInterrupt(void *driver_h, uint16_t pin_index)
{
    (void)driver_h;
    uint16_t pin = pin_index;
    SimulatePinInterrupt(&pin);
    return EZ_GPIO_PIN_LOW;
}


/* A HW init function which arms the interrupt and sees an edge at once. */
static EZ_DRV_STATUS HwInitPinWithInterrupt(void *driver_h, uint16_t pin_index, ezHwGpioConfig_t *config)
{
    (void)driver_h;
    (void)config;
    g_state_in_hw = g_drv.base.state;
    uint16_t pin = pin_index;
    SimulatePinInterrupt(&pin);
    return EZ_DRV_STATUS_OK;
}


/******************************************************************************
* Fixture
*******************************************************************************/
struct GpioTestFixture
{
    GpioTestFixture()
    {
        RESET_FAKE(HwInitPin);
        RESET_FAKE(HwReadPin);
        RESET_FAKE(HwWritePin);
        RESET_FAKE(HwTogglePin);
        RESET_FAKE(OnEvent);
        FFF_RESET_HISTORY();

        g_reentrant_handle = NULL;
        g_state_in_hw = 0U;
        g_reentrant_status = EZ_DRV_STATUS_OK;

        memset(&g_drv, 0, sizeof(g_drv));
        memset(&first, 0, sizeof(first));
        memset(&second, 0, sizeof(second));
        memset(&config, 0, sizeof(config));
        config.intr_mode = EZ_INTERRUPT_NONE;
        g_drv.base.name = "gpio_mock";
        g_drv.interface.driver_h = &g_hw_token;
        g_drv.interface.init_pin = HwInitPin;
        g_drv.interface.read_pin = HwReadPin;
        g_drv.interface.write_pin = HwWritePin;
        g_drv.interface.toggle_pin = HwTogglePin;

        REQUIRE(ezGpio_SystemRegisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
        driver_registered = true;
    }

    ~GpioTestFixture()
    {
        if(driver_registered)
        {
            CHECK(ezGpio_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
        }
    }

    /* Register a handle and initialize a pin without interrupt. */
    void BindAndInit(ezGpioHandle_t *handle, ezDrvCallback cb = OnEvent, void *ctx = NULL)
    {
        HwInitPin_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezGpio_RegisterInstance(handle, "gpio_mock", cb, ctx) == EZ_DRV_STATUS_OK);
        REQUIRE(ezGpio_Initialize(handle, PIN, &config) == EZ_DRV_STATUS_OK);
    }

    ezGpioHandle_t first;
    ezGpioHandle_t second;
    ezHwGpioConfig_t config;
    bool driver_registered;
};


/******************************************************************************
* External functions
*******************************************************************************/
TEST_CASE_METHOD(GpioTestFixture, "Test driver registration", "[hal][gpio]")
{
    WHEN("Invalid arguments are used")
    {
        struct ezGpioDriver no_name;
        memset(&no_name, 0, sizeof(no_name));

        THEN("The APIs report an argument error")
        {
            CHECK(ezGpio_SystemRegisterHwDriver(NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezGpio_SystemRegisterHwDriver(&no_name) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezGpio_SystemUnregisterHwDriver(NULL) == EZ_DRV_STATUS_ERR_ARG);
        }
    }

    WHEN("A driver with the same name is registered twice")
    {
        struct ezGpioDriver duplicate;
        memset(&duplicate, 0, sizeof(duplicate));
        duplicate.base.name = "gpio_mock";

        THEN("The second registration is rejected")
        {
            CHECK(ezGpio_SystemRegisterHwDriver(&duplicate) == EZ_DRV_STATUS_ERR_ARG);
        }
    }

    WHEN("A second driver with another name is registered")
    {
        struct ezGpioDriver other;
        memset(&other, 0, sizeof(other));
        other.base.name = "gpio_other";

        THEN("Both can be found by name")
        {
            REQUIRE(ezGpio_SystemRegisterHwDriver(&other) == EZ_DRV_STATUS_OK);
            CHECK(ezGpio_RegisterInstance(&first, "gpio_other", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == &other);
            CHECK(ezGpio_RegisterInstance(&second, "gpio_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(second.drv_handle.hw == &g_drv);
            CHECK(ezGpio_UnregisterInstance(&first) == EZ_DRV_STATUS_OK);
            CHECK(ezGpio_SystemUnregisterHwDriver(&other) == EZ_DRV_STATUS_OK);
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

    WHEN("A driver which listens to events is unregistered")
    {
        config.intr_mode = EZ_INTERRUPT_RISING;
        BindAndInit(&first);
        REQUIRE(IsListening() == true);

        THEN("It is reset and cannot be found anymore")
        {
            CHECK(ezGpio_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
            driver_registered = false;
            CHECK(g_drv.initialized == false);
            CHECK(g_drv.base.state == 0U);
            CHECK(ezGpio_RegisterInstance(&second, "gpio_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("A driver is unregistered while an operation is in flight")
    {
        EZ_DRV_STATE_SET(&g_drv.base, EZ_DRV_STATE_BUSY);

        THEN("The unregistration is refused")
        {
            CHECK(ezGpio_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_BUSY);
            EZ_DRV_STATE_CLEAR(&g_drv.base, EZ_DRV_STATE_BUSY);
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test instance registration lifecycle", "[hal][gpio]")
{
    WHEN("Invalid arguments are used")
    {
        THEN("The APIs report an argument error")
        {
            CHECK(ezGpio_RegisterInstance(NULL, "gpio_mock", NULL, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezGpio_RegisterInstance(&first, NULL, NULL, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezGpio_UnregisterInstance(NULL) == EZ_DRV_STATUS_ERR_ARG);
        }
    }

    WHEN("The target driver does not exist")
    {
        THEN("The registration fails with driver-not-found")
        {
            CHECK(ezGpio_RegisterInstance(&first, "unknown", OnEvent, NULL) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("A handle which was never bound is unregistered")
    {
        THEN("The call succeeds and the handle stays empty")
        {
            CHECK(ezGpio_UnregisterInstance(&first) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(first.drv_handle.on_event == NULL);
            CHECK(first.drv_handle.ctx == NULL);
        }
    }

    WHEN("A handle which is already bound is registered again")
    {
        int first_ctx = 0;
        REQUIRE(ezGpio_RegisterInstance(&first, "gpio_mock", OnEvent, &first_ctx) == EZ_DRV_STATUS_OK);

        THEN("The registration is refused and the first binding is kept")
        {
            CHECK(ezGpio_RegisterInstance(&first, "gpio_mock", NULL, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(first.drv_handle.hw == &g_drv);
            CHECK(first.drv_handle.on_event == OnEvent);
            CHECK(first.drv_handle.ctx == &first_ctx);
        }
    }

    WHEN("A handle is registered")
    {
        int user_ctx = 0;

        THEN("The driver, callback and context are stored, and unregister clears them")
        {
            REQUIRE(ezGpio_RegisterInstance(&first, "gpio_mock", OnEvent, &user_ctx) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == &g_drv);
            CHECK(first.drv_handle.on_event == OnEvent);
            CHECK(first.drv_handle.ctx == &user_ctx);
            REQUIRE(ezGpio_UnregisterInstance(&first) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(first.drv_handle.on_event == NULL);
            CHECK(first.drv_handle.ctx == NULL);
            CHECK(ezGpio_RegisterInstance(&first, "gpio_mock", NULL, NULL) == EZ_DRV_STATUS_OK);
        }
    }

    WHEN("The handle listens to pin events")
    {
        config.intr_mode = EZ_INTERRUPT_BOTH;
        BindAndInit(&first);
        REQUIRE(ezGpio_RegisterInstance(&second, "gpio_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
        REQUIRE(IsListening() == true);

        THEN("Another handle is refused, and after the unregistration it can listen")
        {
            CHECK(ezGpio_Initialize(&second, PIN, &config) == EZ_DRV_STATUS_BUSY);
            CHECK(ezGpio_UnregisterInstance(&first) == EZ_DRV_STATUS_OK);
            CHECK(IsListening() == false);
            CHECK(ezGpio_Initialize(&second, PIN, &config) == EZ_DRV_STATUS_OK);
            CHECK(g_drv.base.user == &second.drv_handle);
            CHECK(IsListening() == true);
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test unregister while the handle holds the driver", "[hal][gpio]")
{
    BindAndInit(&first);
    REQUIRE(ezGpio_RegisterInstance(&second, "gpio_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);

    WHEN("A HW function calls ezGpio_UnregisterInstance of the same handle")
    {
        g_reentrant_handle = &first;
        HwReadPin_fake.custom_fake = HwReadPinReentrant;

        THEN("The unregistration is refused with busy")
        {
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_HIGH);
            CHECK(g_reentrant_status == EZ_DRV_STATUS_BUSY);
            CHECK(first.drv_handle.hw == &g_drv);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("A HW function calls ezGpio_UnregisterInstance of another handle")
    {
        g_reentrant_handle = &second;
        HwReadPin_fake.custom_fake = HwReadPinReentrant;

        THEN("The other handle is unregistered")
        {
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_HIGH);
            CHECK(g_reentrant_status == EZ_DRV_STATUS_OK);
            CHECK(second.drv_handle.hw == NULL);
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test initialization", "[hal][gpio]")
{
    REQUIRE(ezGpio_RegisterInstance(&first, "gpio_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);

    WHEN("The arguments are invalid")
    {
        THEN("The API reports the matching error")
        {
            CHECK(ezGpio_Initialize(&first, PIN, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezGpio_Initialize(NULL, PIN, &config) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezGpio_Initialize(&second, PIN, &config) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(HwInitPin_fake.call_count == 0U);
        }
    }

    WHEN("The HW initialization succeeds without interrupt")
    {
        HwInitPin_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The HW gets its token, the pin and the config, and the driver does not listen")
        {
            CHECK(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_OK);
            CHECK(HwInitPin_fake.arg0_val == &g_hw_token);
            CHECK(HwInitPin_fake.arg1_val == PIN);
            CHECK(HwInitPin_fake.arg2_val == &config);
            CHECK(g_drv.initialized == true);
            CHECK(IsBusy() == false);
            CHECK(IsListening() == false);
        }
    }

    WHEN("The HW initialization succeeds with an interrupt mode")
    {
        HwInitPin_fake.return_val = EZ_DRV_STATUS_OK;
        config.intr_mode = EZ_INTERRUPT_FALLING;

        THEN("The handle listens to the pin events")
        {
            CHECK(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_OK);
            CHECK(IsListening() == true);
            CHECK(g_drv.base.user == &first.drv_handle);
            CHECK(IsBusy() == false);
        }

        AND_WHEN("Another pin is initialized by the same handle")
        {
            REQUIRE(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_OK);

            THEN("It is accepted and the handle still listens")
            {
                CHECK(ezGpio_Initialize(&first, PIN + 1U, &config) == EZ_DRV_STATUS_OK);
                CHECK(IsListening() == true);
            }
        }

        AND_WHEN("Another handle tries to use the driver")
        {
            REQUIRE(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_OK);
            REQUIRE(ezGpio_RegisterInstance(&second, "gpio_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);

            THEN("It is refused with busy and the HW is not called")
            {
                CHECK(ezGpio_Initialize(&second, PIN + 1U, &config) == EZ_DRV_STATUS_BUSY);
                CHECK(ezGpio_WritePin(&second, PIN, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_BUSY);
                CHECK(ezGpio_ReadPin(&second, PIN) == EZ_GPIO_PIN_ERROR);
                CHECK(HwInitPin_fake.call_count == 1U);
                CHECK(HwWritePin_fake.call_count == 0U);
                CHECK(HwReadPin_fake.call_count == 0U);
            }
        }
    }

    WHEN("The HW arms the interrupt inside init_pin and an edge comes at once")
    {
        int user_ctx = 0;
        ezGpioHandle_t listener;
        memset(&listener, 0, sizeof(listener));
        REQUIRE(ezGpio_RegisterInstance(&listener, "gpio_mock", OnEvent, &user_ctx) == EZ_DRV_STATUS_OK);
        HwInitPin_fake.custom_fake = HwInitPinWithInterrupt;
        config.intr_mode = EZ_INTERRUPT_RISING;

        THEN("The handle already listens, so the event is delivered during Initialize")
        {
            CHECK(ezGpio_Initialize(&listener, PIN, &config) == EZ_DRV_STATUS_OK);
            CHECK(g_state_in_hw == (EZ_DRV_STATE_BUSY | EZ_DRV_STATE_LISTENING));
            REQUIRE(OnEvent_fake.call_count == 1U);
            CHECK(OnEvent_fake.arg0_val == &user_ctx);
            CHECK(OnEvent_fake.arg1_val == EZ_GPIO_EVENT_PIN_INTERRUPT);
            CHECK(IsBusy() == false);
            CHECK(IsListening() == true);
        }
    }

    WHEN("The handle already listens and a later initialization fails")
    {
        config.intr_mode = EZ_INTERRUPT_BOTH;
        HwInitPin_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_OK);
        HwInitPin_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;

        THEN("The existing subscription is kept")
        {
            CHECK(ezGpio_Initialize(&first, PIN + 1U, &config) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(IsListening() == true);
            CHECK(g_drv.base.user == &first.drv_handle);
        }
    }

    WHEN("Another handle listens")
    {
        ezGpioHandle_t other;
        memset(&other, 0, sizeof(other));
        config.intr_mode = EZ_INTERRUPT_BOTH;
        HwInitPin_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezGpio_RegisterInstance(&other, "gpio_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
        REQUIRE(ezGpio_Initialize(&other, PIN, &config) == EZ_DRV_STATUS_OK);

        THEN("The initialization is refused before the HW is touched, even without interrupt")
        {
            CHECK(ezGpio_Initialize(&first, PIN + 1U, &config) == EZ_DRV_STATUS_BUSY);
            config.intr_mode = EZ_INTERRUPT_NONE;
            CHECK(ezGpio_Initialize(&first, PIN + 1U, &config) == EZ_DRV_STATUS_BUSY);
            CHECK(HwInitPin_fake.call_count == 1U);
            CHECK(g_drv.base.user == &other.drv_handle);
        }
    }

    WHEN("An operation is in flight and no interrupt is requested")
    {
        EZ_DRV_STATE_SET(&g_drv.base, EZ_DRV_STATE_BUSY);

        THEN("The initialization reports busy and the HW is not called")
        {
            CHECK(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_BUSY);
            CHECK(HwInitPin_fake.call_count == 0U);
            EZ_DRV_STATE_CLEAR(&g_drv.base, EZ_DRV_STATE_BUSY);
        }
    }

    WHEN("An operation is in flight and an interrupt is requested")
    {
        config.intr_mode = EZ_INTERRUPT_FALLING;
        EZ_DRV_STATE_SET(&g_drv.base, EZ_DRV_STATE_BUSY);

        THEN("The initialization reports busy, the HW is not called and nothing is subscribed")
        {
            CHECK(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_BUSY);
            CHECK(HwInitPin_fake.call_count == 0U);
            CHECK(IsListening() == false);
            EZ_DRV_STATE_CLEAR(&g_drv.base, EZ_DRV_STATE_BUSY);
        }
    }

    WHEN("The HW initialization fails with an interrupt mode")
    {
        HwInitPin_fake.return_val = EZ_DRV_STATUS_TIMEOUT;
        config.intr_mode = EZ_INTERRUPT_RISING;

        THEN("The failure is returned, the driver is not initialized and does not listen")
        {
            CHECK(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_TIMEOUT);
            CHECK(g_drv.initialized == false);
            CHECK(IsListening() == false);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW driver has no init function")
    {
        g_drv.interface.init_pin = NULL;

        THEN("The API reports interface-not-exist")
        {
            CHECK(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(g_drv.initialized == false);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The driver is initialized and a second initialization fails")
    {
        HwInitPin_fake.return_val = EZ_DRV_STATUS_OK;
        REQUIRE(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_OK);
        HwInitPin_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;

        THEN("The driver is not initialized any more")
        {
            CHECK(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(g_drv.initialized == false);
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_ERROR);
            CHECK(HwReadPin_fake.call_count == 0U);
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test read write and toggle", "[hal][gpio]")
{
    WHEN("The handle is NULL or not bound")
    {
        THEN("The APIs report driver-not-found")
        {
            CHECK(ezGpio_ReadPin(NULL, PIN) == EZ_GPIO_PIN_ERROR);
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_ERROR);
            CHECK(ezGpio_WritePin(NULL, PIN, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezGpio_WritePin(&first, PIN, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezGpio_TogglePin(NULL, PIN) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezGpio_TogglePin(&first, PIN) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("The driver is bound but not initialized")
    {
        REQUIRE(ezGpio_RegisterInstance(&first, "gpio_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);

        THEN("The APIs report not-initialized and the HW is not called")
        {
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_ERROR);
            CHECK(ezGpio_WritePin(&first, PIN, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_ERR_NOT_INITIALIZED);
            CHECK(ezGpio_TogglePin(&first, PIN) == EZ_DRV_STATUS_ERR_NOT_INITIALIZED);
            CHECK(HwReadPin_fake.call_count == 0U);
            CHECK(HwWritePin_fake.call_count == 0U);
            CHECK(HwTogglePin_fake.call_count == 0U);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW functions succeed")
    {
        BindAndInit(&first);
        HwReadPin_fake.return_val = EZ_GPIO_PIN_HIGH;
        HwWritePin_fake.return_val = EZ_DRV_STATUS_OK;
        HwTogglePin_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The arguments are forwarded, the result is returned and the driver is idle")
        {
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_HIGH);
            CHECK(HwReadPin_fake.arg0_val == &g_hw_token);
            CHECK(HwReadPin_fake.arg1_val == PIN);
            CHECK(IsBusy() == false);

            CHECK(ezGpio_WritePin(&first, PIN, EZ_GPIO_PIN_LOW) == EZ_DRV_STATUS_OK);
            CHECK(HwWritePin_fake.arg0_val == &g_hw_token);
            CHECK(HwWritePin_fake.arg1_val == PIN);
            CHECK(HwWritePin_fake.arg2_val == EZ_GPIO_PIN_LOW);
            CHECK(IsBusy() == false);

            CHECK(ezGpio_TogglePin(&first, PIN) == EZ_DRV_STATUS_OK);
            CHECK(HwTogglePin_fake.arg0_val == &g_hw_token);
            CHECK(HwTogglePin_fake.arg1_val == PIN);
            CHECK(IsBusy() == false);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }

    WHEN("The HW reports an error")
    {
        BindAndInit(&first);
        HwReadPin_fake.return_val = EZ_GPIO_PIN_ERROR;
        HwWritePin_fake.return_val = EZ_DRV_STATUS_TIMEOUT;
        HwTogglePin_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;

        THEN("The status is returned unchanged and the driver is idle")
        {
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_ERROR);
            CHECK(ezGpio_WritePin(&first, PIN, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_TIMEOUT);
            CHECK(ezGpio_TogglePin(&first, PIN) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW driver has no read, write or toggle function")
    {
        BindAndInit(&first);
        g_drv.interface.read_pin = NULL;
        g_drv.interface.write_pin = NULL;
        g_drv.interface.toggle_pin = NULL;

        THEN("The APIs report an error and release the driver")
        {
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_ERROR);
            CHECK(IsBusy() == false);
            CHECK(ezGpio_WritePin(&first, PIN, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
            CHECK(ezGpio_TogglePin(&first, PIN) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("An operation is in flight")
    {
        BindAndInit(&first);
        EZ_DRV_STATE_SET(&g_drv.base, EZ_DRV_STATE_BUSY);

        THEN("The APIs report busy and the HW is not called")
        {
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_ERROR);
            CHECK(ezGpio_WritePin(&first, PIN, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_BUSY);
            CHECK(ezGpio_TogglePin(&first, PIN) == EZ_DRV_STATUS_BUSY);
            CHECK(HwReadPin_fake.call_count == 0U);
            CHECK(HwWritePin_fake.call_count == 0U);
            CHECK(HwTogglePin_fake.call_count == 0U);
            EZ_DRV_STATE_CLEAR(&g_drv.base, EZ_DRV_STATE_BUSY);
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test pin events", "[hal][gpio]")
{
    int user_ctx = 0;
    uint16_t pin = PIN;

    WHEN("The handle did not ask for interrupts")
    {
        BindAndInit(&first, OnEvent, &user_ctx);
        SimulatePinInterrupt(&pin);

        THEN("The event is dropped")
        {
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }

    WHEN("The handle listens to pin events")
    {
        config.intr_mode = EZ_INTERRUPT_BOTH;
        BindAndInit(&first, OnEvent, &user_ctx);
        SimulatePinInterrupt(&pin);

        THEN("The callback gets the context, the event code and the pin")
        {
            REQUIRE(OnEvent_fake.call_count == 1U);
            CHECK(OnEvent_fake.arg0_val == &user_ctx);
            CHECK(OnEvent_fake.arg1_val == EZ_GPIO_EVENT_PIN_INTERRUPT);
            CHECK(OnEvent_fake.arg2_val == &pin);
            CHECK(OnEvent_fake.arg3_val == NULL);
        }

        THEN("The subscription survives pin operations and events keep coming")
        {
            HwReadPin_fake.return_val = EZ_GPIO_PIN_HIGH;
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_HIGH);
            CHECK(IsListening() == true);
            SimulatePinInterrupt(&pin);
            CHECK(OnEvent_fake.call_count == 2U);
        }

        THEN("An interrupt during a pin operation is delivered")
        {
            HwReadPin_fake.custom_fake = HwReadPinWithInterrupt;
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_LOW);
            CHECK(OnEvent_fake.call_count == 2U);
            CHECK(IsBusy() == false);
        }

        THEN("Operation events are dropped, there is no async operation")
        {
            ezDriver_NotifyEvent(&g_drv.base, EZ_DRV_EVENT_OP_DONE, 0, NULL, NULL);
            ezDriver_NotifyEvent(&g_drv.base, EZ_DRV_EVENT_OP_PROGRESS, 0, NULL, NULL);
            CHECK(OnEvent_fake.call_count == 1U);   /* only the event of the WHEN block */
        }

        AND_WHEN("The handle is unregistered")
        {
            REQUIRE(ezGpio_UnregisterInstance(&first) == EZ_DRV_STATUS_OK);

            THEN("The events are dropped")
            {
                OnEvent_fake.call_count = 0U;
                SimulatePinInterrupt(&pin);
                CHECK(OnEvent_fake.call_count == 0U);
            }
        }
    }

    WHEN("The handle listens and has no callback")
    {
        config.intr_mode = EZ_INTERRUPT_RISING;
        BindAndInit(&first, NULL, NULL);
        SimulatePinInterrupt(&pin);

        THEN("Nothing crashes and no callback is called")
        {
            CHECK(OnEvent_fake.call_count == 0U);
            CHECK(IsListening() == true);
        }
    }
}

/* End of file */
