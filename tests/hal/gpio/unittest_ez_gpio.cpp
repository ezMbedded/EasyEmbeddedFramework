/*****************************************************************************
* Filename:         unittest_ez_osal.c
* Author:           Hai Nguyen
* Original Date:    15.03.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   unittest_ez_osal.c
 *  @author Hai Nguyen
 *  @date   15.03.2025
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
#include "ez_gpio.h"
#include "ez_event_notifier.h"
#include "fff.h"
#include <catch2/catch_test_macros.hpp>


/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(EZ_DRV_STATUS, Init, uint16_t, ezHwGpioConfig_t*);
FAKE_VALUE_FUNC(EZ_GPIO_PIN_STATE, ReadPin, uint16_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, WritePin, uint16_t, EZ_GPIO_PIN_STATE);
FAKE_VALUE_FUNC(int, Callback, uint32_t, void *, void *);

/******************************************************************************
* Module Typedefs
*******************************************************************************/
static ezGpioDriver gpio_driver;

class GpioTestFixture {
private:
protected:
public:
    GpioTestFixture();
    ~GpioTestFixture(){}
protected:
};


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/


/******************************************************************************
* Function Definitions
*******************************************************************************/


/******************************************************************************
* External functions
*******************************************************************************/
TEST_CASE_METHOD(GpioTestFixture, "Test no interface is implemented", "[hal][gpio]")
{
    ezGpioDrvInstance_t instance;
    REQUIRE(ezGpio_SystemRegisterHwDriver(&gpio_driver) == STATUS_OK);
    REQUIRE(ezGpio_RegisterInstance(&instance, "Mock GPIO Driver", Callback) == STATUS_OK);
    
    Callback_fake.return_val = STATUS_ERR_GENERIC;
    ezEventNotifier_NotifyEvent(&gpio_driver.gpio_event, 0x01, NULL, NULL);

    CHECK(Callback_fake.call_count == 1);
    CHECK(Callback_fake.arg0_val == 0x01);
    /* Trigger callback */

}


/******************************************************************************
* Internal functions
*******************************************************************************/
GpioTestFixture::GpioTestFixture()
{
    RESET_FAKE(Init);
    RESET_FAKE(ReadPin);
    RESET_FAKE(WritePin);
    RESET_FAKE(Callback);

    gpio_driver.common.name = "Mock GPIO Driver";
    gpio_driver.common.version[0] = 1;
    gpio_driver.common.version[1] = 0;
    gpio_driver.common.version[2] = 0;

    gpio_driver.initialized = false;
    gpio_driver.interface.init_pin = Init;
    gpio_driver.interface.read_pin = ReadPin;
    gpio_driver.interface.write_pin = WritePin;
    gpio_driver.interface.toggle_pin = NULL;
}


/* End of file */
