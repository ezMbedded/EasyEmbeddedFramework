/*****************************************************************************
* Filename:         ez_middlewares.c
* Author:           Hai Nguyen
* Original Date:    19.11.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_middlewares.c
 *  @author Hai Nguyen
 *  @date   19.11.2024
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_middlewares.h"

#if (EZ_MIDDLEWARES == 1)
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_MIDDLEWARE_LOGGING_LEVEL   /**< logging level */
#define MOD_NAME    "ez_middlewares"       /**< module name */
#include "ez_logging.h"

/*the rest of include go here*/

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */

/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* None */

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Public functions
*****************************************************************************/
ezSTATUS ezMidware_Initialize(void)
{
    EZTRACE("ezMidware_Initialize()");
    return ezSUCCESS;
}

/*****************************************************************************
* Local functions
*****************************************************************************/

#endif /* EZ_MIDDLEWARES_ENABLE == 1 */
/* End of file*/
