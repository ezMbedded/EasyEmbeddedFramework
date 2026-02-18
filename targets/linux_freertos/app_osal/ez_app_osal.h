/*****************************************************************************
* Filename:         ez_app_osal.h
* Author:           Hai Nguyen
* Original Date:    16.03.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_app_osal.h
 *  @author Hai Nguyen
 *  @date   16.03.2025
 *  @brief  Application to test OSAL component
 *
 *  @details None
 */

#ifndef _EZ_APP_OSAL_H
#define _EZ_APP_OSAL_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

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
* Function Prototypes
*****************************************************************************/
void ezApp_OsalInit(void);
void ezApp_StartOsalApp(void);

#endif /* EZ_APP_OSAL_ENABLE == 1 */

#ifdef __cplusplus
}
#endif


/* End of file */
