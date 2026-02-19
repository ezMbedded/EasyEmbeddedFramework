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
#include "ez_event_bus.h"
#include "fff.h"
#include <catch2/catch_test_macros.hpp>


/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(EZ_DRV_STATUS, Init, uint16_t, ezHwGpioConfig_t*);
FAKE_VALUE_FUNC(EZ_GPIO_PIN_STATE, ReadPin, uint16_t);
FAKE_VALUE_FUNC(EZ_DRV_STATUS, WritePin, uint16_t, EZ_GPIO_PIN_STATE);
FAKE_VOID_FUNC(Callback, uint8_t, void *, void*);

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
TEST_CASE_METHOD(GpioTestFixture, "Test write pin", "[hal][gpio]")
{
    ezGpioDrvInstance_t instance;
    REQUIRE(ezGpio_SystemRegisterHwDriver(&gpio_driver) == STATUS_OK);
    REQUIRE(ezGpio_RegisterInstance(&instance, "Mock GPIO Driver", Callback) == STATUS_OK);

    EZ_DRV_STATUS status = ezGpio_WritePin(&instance, 0x01, EZ_GPIO_PIN_HIGH);
    CHECK(status == STATUS_OK);
    CHECK(WritePin_fake.call_count == 1);
    CHECK(WritePin_fake.arg0_val == 0x01);
    CHECK(WritePin_fake.arg1_val == EZ_GPIO_PIN_HIGH);
}


TEST_CASE_METHOD(GpioTestFixture, "Test read pin", "[hal][gpio]")
{
    ezGpioDrvInstance_t instance;
    REQUIRE(ezGpio_SystemRegisterHwDriver(&gpio_driver) == STATUS_OK);
    REQUIRE(ezGpio_RegisterInstance(&instance, "Mock GPIO Driver", Callback) == STATUS_OK);


    ReadPin_fake.return_val = EZ_GPIO_PIN_HIGH;
    EZ_GPIO_PIN_STATE state = ezGpio_ReadPin(&instance, 0x01);
    CHECK(state == EZ_GPIO_PIN_HIGH);
    CHECK(ReadPin_fake.call_count == 1);
    CHECK(ReadPin_fake.arg0_val == 0x01);

}

TEST_CASE_METHOD(GpioTestFixture, "Test callback triggered", "[hal][gpio]")
{
    ezGpioDrvInstance_t instance;
    REQUIRE(ezGpio_SystemRegisterHwDriver(&gpio_driver) == STATUS_OK);
    REQUIRE(ezGpio_RegisterInstance(&instance, "Mock GPIO Driver", Callback) == STATUS_OK);

    instance.drv_instance.calback(0x10, (void*)0x1234, (void*)0x5678);
    CHECK(Callback_fake.call_count == 1);
    CHECK(Callback_fake.arg0_val == 0x10);
    CHECK(Callback_fake.arg1_val == (void*)0x1234);
    CHECK(Callback_fake.arg2_val == (void*)0x5678);
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
