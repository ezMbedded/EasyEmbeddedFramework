/*****************************************************************************
* Filename:         unittest_ez_<module>.cpp
* Author:           <author>
* Original Date:    <dd.mm.yyyy>
*
* ----------------------------------------------------------------------------
* Contact:          <author>
*                   <email>
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   unittest_ez_<module>.cpp
 *  @author <author>
 *  @date   <dd.mm.yyyy>
 *  @brief  Unit tests for the <module> component
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

#include "ez_<module>.h"
#include "fff.h"

#include <catch2/catch_test_macros.hpp>


/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
DEFINE_FFF_GLOBALS;
/* One fake per dependency function, for example: */
FAKE_VALUE_FUNC(EZ_DRV_STATUS, HwDoSomething, void *, uint32_t);
FAKE_VOID_FUNC(OnEvent, void *, uint8_t, void *, void *);


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
/* State shared with callbacks and fakes. Reset it in the fixture constructor. */


/******************************************************************************
* Fixture
*******************************************************************************/
struct ModuleTestFixture
{
    ModuleTestFixture()
    {
        RESET_FAKE(HwDoSomething);
        RESET_FAKE(OnEvent);
        FFF_RESET_HISTORY();
        /* Reset every static and global the tests touch, then set up the module. */
    }

    ~ModuleTestFixture()
    {
        /* Clear custom fakes and callbacks first, so cleanup cannot start new
         * work. Then undo everything the constructor registered. */
    }
};


/******************************************************************************
* External functions
*******************************************************************************/
TEST_CASE_METHOD(ModuleTestFixture, "Test <topic>", "[<layer>][<module>]")
{
    WHEN("<situation>")
    {
        THEN("<expected result>")
        {
            CHECK(ez<Module>_<Function>(/* ... */) == /* expected */);
        }
    }
}

/* End of file */
