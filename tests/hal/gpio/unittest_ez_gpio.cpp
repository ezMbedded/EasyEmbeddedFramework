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
FAKE_VOID_FUNC(OnEventB, void *, uint8_t, void *, void *);

#define PIN     (7U)
#define PIN_B   (9U)


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
static EZ_DRV_STATUS g_reentrant_status2 = EZ_DRV_STATUS_OK;
static ezHwGpioConfig_t g_reentrant_config;


/******************************************************************************
* Function Definitions
*******************************************************************************/
static bool IsBusy(void)
{
    return EZ_DRV_STATE_IS_SET(&g_drv.base, EZ_DRV_STATE_BUSY);
}

/* Number of pin subscriptions in use. */
static unsigned ListenerCount(void)
{
    unsigned count = 0U;
    for(unsigned i = 0U; i < EZ_GPIO_MAX_LISTENERS; i++)
    {
        if(g_drv.listeners[i].handle != NULL)
        {
            count++;
        }
    }
    return count;
}

/* What the ISR of the HW driver does when a pin interrupt fires. */
static void SimulatePinInterrupt(uint16_t pin, EZ_GPIO_PIN_STATE level = EZ_GPIO_PIN_LOW)
{
    ezGpio_NotifyPinEvent(&g_drv, pin, level);
}

/* A HW read function which closes a handle while the driver is held. */
static EZ_GPIO_PIN_STATE HwReadPinReentrant(void *driver_h, uint16_t pin_index)
{
    (void)driver_h;
    (void)pin_index;
    g_reentrant_status = ezGpio_Close(g_reentrant_handle);
    return EZ_GPIO_PIN_HIGH;
}

/* A HW read function which sees an interrupt while it runs. */
static EZ_GPIO_PIN_STATE HwReadPinWithInterrupt(void *driver_h, uint16_t pin_index)
{
    (void)driver_h;
    SimulatePinInterrupt(pin_index, EZ_GPIO_PIN_HIGH);
    return EZ_GPIO_PIN_LOW;
}

/* A HW init function which arms the interrupt and sees an edge at once. */
static EZ_DRV_STATUS HwInitPinWithInterrupt(void *driver_h, uint16_t pin_index, ezHwGpioConfig_t *config)
{
    (void)driver_h;
    (void)config;
    SimulatePinInterrupt(pin_index, EZ_GPIO_PIN_HIGH);
    return EZ_DRV_STATUS_OK;
}

/* A HW init function which calls other GPIO functions of the same handle while the driver is held. */
static EZ_DRV_STATUS HwInitPinReentrant(void *driver_h, uint16_t pin_index, ezHwGpioConfig_t *config)
{
    (void)driver_h;
    (void)pin_index;
    (void)config;
    g_reentrant_status = ezGpio_Initialize(g_reentrant_handle, PIN_B, &g_reentrant_config);
    g_reentrant_status2 = ezGpio_WritePin(g_reentrant_handle, PIN_B, EZ_GPIO_PIN_HIGH);
    return EZ_DRV_STATUS_OK;
}

/* A HW write function which tries to unregister the driver while it is held. */
static EZ_DRV_STATUS HwWritePinUnregisters(void *driver_h, uint16_t pin_index, EZ_GPIO_PIN_STATE state)
{
    (void)driver_h;
    (void)pin_index;
    (void)state;
    g_reentrant_status = ezGpio_SystemUnregisterHwDriver(&g_drv);
    return EZ_DRV_STATUS_OK;
}

/* A HW write function which tries to use the API of the other handle while the driver is held. */
static EZ_DRV_STATUS HwWritePinReentrant(void *driver_h, uint16_t pin_index, EZ_GPIO_PIN_STATE state)
{
    (void)driver_h;
    (void)pin_index;
    (void)state;
    g_reentrant_status = ezGpio_WritePin(g_reentrant_handle, PIN, EZ_GPIO_PIN_HIGH);
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
        RESET_FAKE(OnEventB);
        FFF_RESET_HISTORY();

        g_reentrant_handle = NULL;
        g_reentrant_status = EZ_DRV_STATUS_OK;
        g_reentrant_status2 = EZ_DRV_STATUS_OK;

        memset(&g_drv, 0, sizeof(g_drv));
        memset(&first, 0, sizeof(first));
        memset(&second, 0, sizeof(second));
        memset(&config, 0, sizeof(config));
        memset(&irq_config, 0, sizeof(irq_config));
        memset(&g_reentrant_config, 0, sizeof(g_reentrant_config));
        config.intr_mode = EZ_INTERRUPT_NONE;
        irq_config.intr_mode = EZ_INTERRUPT_BOTH;
        g_reentrant_config.intr_mode = EZ_INTERRUPT_BOTH;
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
            /* Handles left open by a test: the driver must be unregistrable, or it stays in the list */
            g_drv.open_count = 0U;
            CHECK(ezGpio_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
        }
    }

    /* Open a handle on the mock driver. */
    void OpenHandle(ezGpioHandle_t *handle, ezDrvCallback cb = OnEvent, void *ctx = NULL)
    {
        REQUIRE(ezGpio_Open(handle, "gpio_mock", cb, ctx) == EZ_DRV_STATUS_OK);
    }

    ezGpioHandle_t first;
    ezGpioHandle_t second;
    ezHwGpioConfig_t config;        /* no interrupt */
    ezHwGpioConfig_t irq_config;    /* both edges */
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
            CHECK(ezGpio_Open(&first, "gpio_other", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == &other);
            CHECK(ezGpio_Open(&second, "gpio_mock", OnEvent, NULL) == EZ_DRV_STATUS_OK);
            CHECK(second.drv_handle.hw == &g_drv);
            CHECK(ezGpio_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(ezGpio_SystemUnregisterHwDriver(&other) == EZ_DRV_STATUS_OK);
        }
    }

    WHEN("A driver is registered")
    {
        THEN("It starts idle with no open handle and no subscription")
        {
            CHECK(g_drv.open_count == 0U);
            CHECK(g_drv.base.state == 0U);
            CHECK(g_drv.base.user == NULL);
            CHECK(ListenerCount() == 0U);
        }
    }

    WHEN("A driver with stale data is registered")
    {
        struct ezGpioDriver other;
        memset(&other, 0, sizeof(other));
        other.base.name = "gpio_other";
        other.open_count = 3U;
        other.listeners[2].handle = &first.drv_handle;
        other.base.state = EZ_DRV_STATE_BUSY;

        THEN("The registration resets it")
        {
            REQUIRE(ezGpio_SystemRegisterHwDriver(&other) == EZ_DRV_STATUS_OK);
            CHECK(other.open_count == 0U);
            CHECK(other.listeners[2].handle == NULL);
            CHECK(other.base.state == 0U);
            CHECK(ezGpio_SystemUnregisterHwDriver(&other) == EZ_DRV_STATUS_OK);
        }
    }

    WHEN("A driver is unregistered")
    {
        THEN("It cannot be found anymore")
        {
            CHECK(ezGpio_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
            driver_registered = false;
            CHECK(ezGpio_Open(&first, "gpio_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
        }
    }

    WHEN("A driver is unregistered while a handle is open")
    {
        OpenHandle(&first);

        THEN("The unregistration is refused until the handle is closed")
        {
            CHECK(ezGpio_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_BUSY);
            REQUIRE(ezGpio_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(ezGpio_SystemUnregisterHwDriver(&g_drv) == EZ_DRV_STATUS_OK);
            driver_registered = false;
        }
    }

    WHEN("A driver is unregistered while an operation is in flight")
    {
        OpenHandle(&first);
        HwWritePin_fake.custom_fake = HwWritePinUnregisters;

        THEN("The unregistration is refused")
        {
            CHECK(ezGpio_WritePin(&first, PIN, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_OK);
            CHECK(g_reentrant_status == EZ_DRV_STATUS_BUSY);
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test open and close", "[hal][gpio]")
{
    WHEN("Invalid arguments are used")
    {
        THEN("The APIs report an argument error")
        {
            CHECK(ezGpio_Open(NULL, "gpio_mock", NULL, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezGpio_Open(&first, NULL, NULL, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezGpio_Close(NULL) == EZ_DRV_STATUS_ERR_ARG);
        }
    }

    WHEN("A handle which was never opened is closed")
    {
        THEN("The call succeeds and the handle stays empty")
        {
            CHECK(ezGpio_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(first.drv_handle.on_event == NULL);
            CHECK(first.drv_handle.ctx == NULL);
        }
    }

    WHEN("The target driver does not exist")
    {
        THEN("The open fails with driver-not-found")
        {
            CHECK(ezGpio_Open(&first, "unknown", OnEvent, NULL) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(first.drv_handle.hw == NULL);
        }
    }

    WHEN("A handle is opened")
    {
        int user_ctx = 0;
        REQUIRE(ezGpio_Open(&first, "gpio_mock", OnEvent, &user_ctx) == EZ_DRV_STATUS_OK);

        THEN("It stores the driver, the callback and the context, and the HW is not called")
        {
            CHECK(first.drv_handle.hw == &g_drv);
            CHECK(first.drv_handle.on_event == OnEvent);
            CHECK(first.drv_handle.ctx == &user_ctx);
            CHECK(g_drv.open_count == 1U);
            CHECK(IsBusy() == false);
            CHECK(HwInitPin_fake.call_count == 0U);
        }

        THEN("Opening it again is refused and the binding is unchanged")
        {
            CHECK(ezGpio_Open(&first, "gpio_mock", NULL, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(first.drv_handle.on_event == OnEvent);
            CHECK(first.drv_handle.ctx == &user_ctx);
            CHECK(g_drv.open_count == 1U);
        }

        THEN("It is cleared by the close and can be opened again")
        {
            CHECK(ezGpio_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(first.drv_handle.on_event == NULL);
            CHECK(first.drv_handle.ctx == NULL);
            CHECK(g_drv.open_count == 0U);
            CHECK(ezGpio_Open(&first, "gpio_mock", NULL, NULL) == EZ_DRV_STATUS_OK);
            CHECK(first.drv_handle.on_event == NULL);
        }
    }

    WHEN("Several handles are open on the same driver")
    {
        OpenHandle(&first);
        OpenHandle(&second);

        THEN("They are counted, and closing one keeps the other")
        {
            CHECK(g_drv.open_count == 2U);
            CHECK(ezGpio_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(g_drv.open_count == 1U);
            CHECK(second.drv_handle.hw == &g_drv);
        }
    }

    WHEN("The maximum number of handles is open")
    {
        g_drv.open_count = UINT8_MAX;

        THEN("The open fails")
        {
            CHECK(ezGpio_Open(&first, "gpio_mock", OnEvent, NULL) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(first.drv_handle.hw == NULL);
            CHECK(g_drv.open_count == UINT8_MAX);
        }
    }

    WHEN("A handle which listens to pins is closed")
    {
        OpenHandle(&first);
        OpenHandle(&second, OnEventB);
        REQUIRE(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);
        REQUIRE(ezGpio_Initialize(&first, PIN + 1U, &irq_config) == EZ_DRV_STATUS_OK);
        REQUIRE(ezGpio_Initialize(&second, PIN_B, &irq_config) == EZ_DRV_STATUS_OK);

        THEN("Only its own subscriptions end")
        {
            REQUIRE(ListenerCount() == 3U);
            CHECK(ezGpio_Close(&first) == EZ_DRV_STATUS_OK);
            CHECK(ListenerCount() == 1U);
            SimulatePinInterrupt(PIN);
            CHECK(OnEvent_fake.call_count == 0U);
            SimulatePinInterrupt(PIN_B);
            CHECK(OnEventB_fake.call_count == 1U);
        }
    }

    WHEN("A HW function closes the handle which holds the driver")
    {
        OpenHandle(&first);
        g_reentrant_handle = &first;
        HwReadPin_fake.custom_fake = HwReadPinReentrant;

        THEN("The close is refused")
        {
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_HIGH);
            CHECK(g_reentrant_status == EZ_DRV_STATUS_BUSY);
            CHECK(first.drv_handle.hw == &g_drv);
        }
    }

    WHEN("A HW function closes another handle")
    {
        OpenHandle(&first);
        OpenHandle(&second);
        g_reentrant_handle = &second;
        HwReadPin_fake.custom_fake = HwReadPinReentrant;

        THEN("It is allowed")
        {
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_HIGH);
            CHECK(g_reentrant_status == EZ_DRV_STATUS_OK);
            CHECK(second.drv_handle.hw == NULL);
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test initialization", "[hal][gpio]")
{
    WHEN("The arguments are invalid")
    {
        OpenHandle(&first);

        THEN("The API reports the matching error and the HW is not called")
        {
            CHECK(ezGpio_Initialize(&first, PIN, NULL) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezGpio_Initialize(NULL, PIN, &config) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezGpio_Initialize(&second, PIN, &config) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(HwInitPin_fake.call_count == 0U);
        }
    }

    WHEN("The HW initialization succeeds without interrupt")
    {
        OpenHandle(&first);
        HwInitPin_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The HW gets driver_h, the pin and the config, nobody listens and the driver is released")
        {
            CHECK(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_OK);
            CHECK(HwInitPin_fake.arg0_val == &g_hw_token);
            CHECK(HwInitPin_fake.arg1_val == PIN);
            CHECK(HwInitPin_fake.arg2_val == &config);
            CHECK(ListenerCount() == 0U);
            CHECK(IsBusy() == false);
            SimulatePinInterrupt(PIN);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }

    WHEN("The config has an initial level")
    {
        OpenHandle(&first);
        HwInitPin_fake.return_val = EZ_DRV_STATUS_OK;
        config.mode = EZ_GPIO_MODE_OUTPUT;
        config.initial_state = EZ_GPIO_PIN_HIGH;

        THEN("The HW gets it unchanged in the config, and a config which leaves it out has the low level")
        {
            CHECK(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_OK);
            REQUIRE(HwInitPin_fake.arg2_val == &config);
            CHECK(HwInitPin_fake.arg2_val->initial_state == EZ_GPIO_PIN_HIGH);

            ezHwGpioConfig_t partial = {EZ_GPIO_MODE_OUTPUT, EZ_GPIO_NO_PULL, EZ_INTERRUPT_NONE};
            CHECK(ezGpio_Initialize(&first, PIN_B, &partial) == EZ_DRV_STATUS_OK);
            REQUIRE(HwInitPin_fake.arg2_val == &partial);
            CHECK(HwInitPin_fake.arg2_val->initial_state == EZ_GPIO_PIN_LOW);
        }
    }

    WHEN("The HW initialization succeeds with an interrupt mode")
    {
        OpenHandle(&first);
        HwInitPin_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The handle listens to this pin only")
        {
            REQUIRE(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);
            CHECK(ListenerCount() == 1U);
            CHECK(IsBusy() == false);
            SimulatePinInterrupt(PIN + 1U);
            CHECK(OnEvent_fake.call_count == 0U);
            SimulatePinInterrupt(PIN);
            CHECK(OnEvent_fake.call_count == 1U);
        }

        THEN("Initializing the same pin again calls the HW again and uses no second subscription")
        {
            REQUIRE(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);
            REQUIRE(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);
            CHECK(HwInitPin_fake.call_count == 2U);
            CHECK(ListenerCount() == 1U);
        }

        THEN("Initializing the pin without interrupt ends the subscription")
        {
            REQUIRE(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);
            REQUIRE(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_OK);
            CHECK(ListenerCount() == 0U);
            SimulatePinInterrupt(PIN);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }

    WHEN("The HW arms the interrupt inside init_pin and an edge comes at once")
    {
        int user_ctx = 0;
        OpenHandle(&first, OnEvent, &user_ctx);
        HwInitPin_fake.custom_fake = HwInitPinWithInterrupt;

        THEN("The edge is delivered during the call, with the pin and the level")
        {
            CHECK(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);
            REQUIRE(OnEvent_fake.call_count == 1U);
            CHECK(OnEvent_fake.arg0_val == &user_ctx);
            CHECK(OnEvent_fake.arg1_val == EZ_GPIO_EVENT_PIN_INTERRUPT);
            CHECK((uintptr_t)OnEvent_fake.arg2_val == PIN);
            CHECK((uintptr_t)OnEvent_fake.arg3_val == (uintptr_t)EZ_GPIO_PIN_HIGH);
        }
    }

    WHEN("The HW initialization fails with an interrupt mode")
    {
        OpenHandle(&first);
        HwInitPin_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;

        THEN("The status is forwarded and the new subscription is removed")
        {
            CHECK(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(ListenerCount() == 0U);
            CHECK(IsBusy() == false);
            SimulatePinInterrupt(PIN);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }

    WHEN("The handle already listens to a pin and a later initialization of it fails")
    {
        OpenHandle(&first);
        REQUIRE(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);
        HwInitPin_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;

        THEN("The subscription which existed before is kept")
        {
            CHECK(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(ListenerCount() == 1U);
            SimulatePinInterrupt(PIN);
            CHECK(OnEvent_fake.call_count == 1U);
        }
    }

    WHEN("Another handle listens to the same pin")
    {
        OpenHandle(&first);
        OpenHandle(&second, OnEventB);
        REQUIRE(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);

        THEN("The request is refused, the HW is not called and the listener is unchanged")
        {
            CHECK(ezGpio_Initialize(&second, PIN, &irq_config) == EZ_DRV_STATUS_BUSY);
            CHECK(HwInitPin_fake.call_count == 1U);
            SimulatePinInterrupt(PIN);
            CHECK(OnEvent_fake.call_count == 1U);
            CHECK(OnEventB_fake.call_count == 0U);
        }

        THEN("The other handle can still initialize the pin without interrupt")
        {
            CHECK(ezGpio_Initialize(&second, PIN, &config) == EZ_DRV_STATUS_OK);
            CHECK(ListenerCount() == 1U);
            SimulatePinInterrupt(PIN);
            CHECK(OnEvent_fake.call_count == 1U);
        }
    }

    WHEN("Another handle listens to a different pin")
    {
        OpenHandle(&first);
        OpenHandle(&second, OnEventB);
        REQUIRE(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);

        THEN("Both listen and each gets its own events")
        {
            CHECK(ezGpio_Initialize(&second, PIN_B, &irq_config) == EZ_DRV_STATUS_OK);
            CHECK(ListenerCount() == 2U);
            SimulatePinInterrupt(PIN_B);
            CHECK(OnEvent_fake.call_count == 0U);
            CHECK(OnEventB_fake.call_count == 1U);
            SimulatePinInterrupt(PIN);
            CHECK(OnEvent_fake.call_count == 1U);
            CHECK(OnEventB_fake.call_count == 1U);
        }
    }

    WHEN("A handle without interrupts initializes a pin of its own, while another handle listens")
    {
        OpenHandle(&first);
        OpenHandle(&second, OnEventB);
        REQUIRE(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);

        THEN("It is not refused")
        {
            CHECK(ezGpio_Initialize(&second, PIN_B, &config) == EZ_DRV_STATUS_OK);
            CHECK(ListenerCount() == 1U);
        }
    }

    WHEN("All the subscriptions are used")
    {
        OpenHandle(&first);
        for(unsigned i = 0U; i < EZ_GPIO_MAX_LISTENERS; i++)
        {
            REQUIRE(ezGpio_Initialize(&first, (uint16_t)(100U + i), &irq_config) == EZ_DRV_STATUS_OK);
        }

        THEN("One more pin with interrupt is refused and the HW is not called")
        {
            unsigned calls = HwInitPin_fake.call_count;
            CHECK(ezGpio_Initialize(&first, 200U, &irq_config) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(HwInitPin_fake.call_count == calls);
            CHECK(ListenerCount() == EZ_GPIO_MAX_LISTENERS);
        }

        THEN("A pin without interrupt is still accepted")
        {
            CHECK(ezGpio_Initialize(&first, 200U, &config) == EZ_DRV_STATUS_OK);
        }

        THEN("A freed entry can be used again")
        {
            REQUIRE(ezGpio_Initialize(&first, 100U, &config) == EZ_DRV_STATUS_OK);
            CHECK(ezGpio_Initialize(&first, 200U, &irq_config) == EZ_DRV_STATUS_OK);
        }
    }

    WHEN("An operation is in flight and an interrupt is requested")
    {
        OpenHandle(&first);
        g_reentrant_handle = &first;
        HwInitPin_fake.custom_fake = HwInitPinReentrant;

        THEN("Both calls from inside the HW function report busy and no subscription is made for them")
        {
            CHECK(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_OK);
            CHECK(g_reentrant_status == EZ_DRV_STATUS_BUSY);
            CHECK(g_reentrant_status2 == EZ_DRV_STATUS_BUSY);
            CHECK(ListenerCount() == 0U);
        }
    }

    WHEN("The HW driver has no init function")
    {
        OpenHandle(&first);
        g_drv.interface.init_pin = NULL;

        THEN("The API reports interface-not-exist and removes the subscription")
        {
            CHECK(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(ListenerCount() == 0U);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The handle is closed after it initialized a pin with an interrupt")
    {
        OpenHandle(&first);
        REQUIRE(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);
        REQUIRE(ezGpio_Close(&first) == EZ_DRV_STATUS_OK);

        THEN("The pin can be listened to by another handle")
        {
            OpenHandle(&second, OnEventB);
            CHECK(ezGpio_Initialize(&second, PIN, &irq_config) == EZ_DRV_STATUS_OK);
            SimulatePinInterrupt(PIN);
            CHECK(OnEventB_fake.call_count == 1U);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test read write and toggle", "[hal][gpio]")
{
    WHEN("The handle is NULL or not open")
    {
        THEN("The APIs report driver-not-found and the HW is not called")
        {
            CHECK(ezGpio_ReadPin(NULL, PIN) == EZ_GPIO_PIN_ERROR);
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_ERROR);
            CHECK(ezGpio_WritePin(NULL, PIN, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezGpio_WritePin(&first, PIN, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezGpio_TogglePin(NULL, PIN) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(ezGpio_TogglePin(&first, PIN) == EZ_DRV_STATUS_ERR_DRV_NOT_FOUND);
            CHECK(HwReadPin_fake.call_count == 0U);
            CHECK(HwWritePin_fake.call_count == 0U);
            CHECK(HwTogglePin_fake.call_count == 0U);
        }
    }

    WHEN("The handle is open and no pin was initialized by it")
    {
        OpenHandle(&first);
        HwReadPin_fake.return_val = EZ_GPIO_PIN_HIGH;
        HwWritePin_fake.return_val = EZ_DRV_STATUS_OK;
        HwTogglePin_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The calls go to the HW, which decides what is valid")
        {
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_HIGH);
            CHECK(ezGpio_WritePin(&first, PIN, EZ_GPIO_PIN_LOW) == EZ_DRV_STATUS_OK);
            CHECK(ezGpio_TogglePin(&first, PIN) == EZ_DRV_STATUS_OK);
        }
    }

    WHEN("The HW functions succeed")
    {
        OpenHandle(&first);
        HwReadPin_fake.return_val = EZ_GPIO_PIN_HIGH;
        HwWritePin_fake.return_val = EZ_DRV_STATUS_OK;
        HwTogglePin_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The arguments are forwarded unchanged and the driver is released each time")
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
        }
    }

    WHEN("The HW reports an error")
    {
        OpenHandle(&first);
        HwReadPin_fake.return_val = EZ_GPIO_PIN_ERROR;
        HwWritePin_fake.return_val = EZ_DRV_STATUS_ERR_ARG;
        HwTogglePin_fake.return_val = EZ_DRV_STATUS_ERR_GENERIC;

        THEN("The values are forwarded unchanged and the driver is released")
        {
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_ERROR);
            CHECK(ezGpio_WritePin(&first, PIN, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_ERR_ARG);
            CHECK(ezGpio_TogglePin(&first, PIN) == EZ_DRV_STATUS_ERR_GENERIC);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("The HW driver has no read, write or toggle function")
    {
        OpenHandle(&first);
        g_drv.interface.read_pin = NULL;
        g_drv.interface.write_pin = NULL;
        g_drv.interface.toggle_pin = NULL;

        THEN("The APIs report interface-not-exist and release the driver")
        {
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_ERROR);
            CHECK(ezGpio_WritePin(&first, PIN, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(ezGpio_TogglePin(&first, PIN) == EZ_DRV_STATUS_ERR_INF_NOT_EXIST);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("An operation is in flight")
    {
        OpenHandle(&first);
        OpenHandle(&second);
        g_reentrant_handle = &second;
        HwWritePin_fake.custom_fake = HwWritePinReentrant;

        THEN("A call of another handle reports busy and the HW is not called again")
        {
            CHECK(ezGpio_WritePin(&first, PIN_B, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_OK);
            CHECK(g_reentrant_status == EZ_DRV_STATUS_BUSY);
            CHECK(HwWritePin_fake.call_count == 1U);
            CHECK(IsBusy() == false);
        }
    }

    WHEN("A handle listens to a pin")
    {
        OpenHandle(&first);
        OpenHandle(&second, OnEventB);
        REQUIRE(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);
        HwReadPin_fake.return_val = EZ_GPIO_PIN_LOW;
        HwWritePin_fake.return_val = EZ_DRV_STATUS_OK;

        THEN("The other handle can use its own pins")
        {
            CHECK(ezGpio_WritePin(&second, PIN_B, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_OK);
            CHECK(ezGpio_ReadPin(&second, PIN_B) == EZ_GPIO_PIN_LOW);
            CHECK(ezGpio_WritePin(&first, PIN, EZ_GPIO_PIN_HIGH) == EZ_DRV_STATUS_OK);
        }
    }
}


TEST_CASE_METHOD(GpioTestFixture, "Test pin events", "[hal][gpio]")
{
    int ctx_first = 0;
    int ctx_second = 0;

    WHEN("The handle did not ask for interrupts")
    {
        OpenHandle(&first, OnEvent, &ctx_first);
        REQUIRE(ezGpio_Initialize(&first, PIN, &config) == EZ_DRV_STATUS_OK);

        THEN("Pin events are dropped")
        {
            SimulatePinInterrupt(PIN);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }

    WHEN("The handle listens to a pin")
    {
        OpenHandle(&first, OnEvent, &ctx_first);
        REQUIRE(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);

        THEN("The callback gets the context, the event code, the pin and the level by value")
        {
            SimulatePinInterrupt(PIN, EZ_GPIO_PIN_LOW);
            REQUIRE(OnEvent_fake.call_count == 1U);
            CHECK(OnEvent_fake.arg0_val == &ctx_first);
            CHECK(OnEvent_fake.arg1_val == EZ_GPIO_EVENT_PIN_INTERRUPT);
            CHECK((uintptr_t)OnEvent_fake.arg2_val == PIN);
            CHECK((uintptr_t)OnEvent_fake.arg3_val == (uintptr_t)EZ_GPIO_PIN_LOW);

            SimulatePinInterrupt(PIN, EZ_GPIO_PIN_HIGH);
            REQUIRE(OnEvent_fake.call_count == 2U);
            CHECK((uintptr_t)OnEvent_fake.arg3_val == (uintptr_t)EZ_GPIO_PIN_HIGH);
        }

        THEN("An event of a pin which nobody listens to is dropped")
        {
            SimulatePinInterrupt(PIN_B);
            CHECK(OnEvent_fake.call_count == 0U);
        }

        THEN("An event while a pin operation runs is still delivered")
        {
            HwReadPin_fake.custom_fake = HwReadPinWithInterrupt;
            CHECK(ezGpio_ReadPin(&first, PIN) == EZ_GPIO_PIN_LOW);
            CHECK(OnEvent_fake.call_count == 1U);
            CHECK((uintptr_t)OnEvent_fake.arg3_val == (uintptr_t)EZ_GPIO_PIN_HIGH);
        }

        THEN("A NULL driver is ignored")
        {
            ezGpio_NotifyPinEvent(NULL, PIN, EZ_GPIO_PIN_LOW);
            CHECK(OnEvent_fake.call_count == 0U);
        }

        THEN("Events stop when the handle is closed")
        {
            REQUIRE(ezGpio_Close(&first) == EZ_DRV_STATUS_OK);
            SimulatePinInterrupt(PIN);
            CHECK(OnEvent_fake.call_count == 0U);
        }
    }

    WHEN("Two handles listen to different pins")
    {
        OpenHandle(&first, OnEvent, &ctx_first);
        OpenHandle(&second, OnEventB, &ctx_second);
        REQUIRE(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);
        REQUIRE(ezGpio_Initialize(&second, PIN_B, &irq_config) == EZ_DRV_STATUS_OK);

        THEN("Each event goes to the handle of its pin, once")
        {
            SimulatePinInterrupt(PIN_B);
            REQUIRE(OnEventB_fake.call_count == 1U);
            CHECK(OnEventB_fake.arg0_val == &ctx_second);
            CHECK(OnEvent_fake.call_count == 0U);

            SimulatePinInterrupt(PIN);
            REQUIRE(OnEvent_fake.call_count == 1U);
            CHECK(OnEvent_fake.arg0_val == &ctx_first);
            CHECK(OnEventB_fake.call_count == 1U);
        }
    }

    WHEN("The handle listens and has no callback")
    {
        OpenHandle(&first, NULL, NULL);
        REQUIRE(ezGpio_Initialize(&first, PIN, &irq_config) == EZ_DRV_STATUS_OK);

        THEN("The event is ignored and nothing crashes")
        {
            SimulatePinInterrupt(PIN);
            CHECK(OnEvent_fake.call_count == 0U);
            CHECK(ListenerCount() == 1U);
        }
    }
}

/* End of file */
