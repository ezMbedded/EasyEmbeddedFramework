/*****************************************************************************
* Filename:         ez_osal_threadx.h
* Author:           Hai Nguyen
* Original Date:    21.03.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_osal_threadx.h
 *  @author Hai Nguyen
 *  @date   21.03.2025
 *  @brief  Implementing osal interface for ThreadX
 *
 *  @details
 */

#ifndef _EZ_OSAL_THREADX_H
#define _EZ_OSAL_THREADX_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_THREADX_PORT == 1)
#include "ez_osal.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
typedef TX_THREAD ezOsal_TaskResource_t;
typedef TX_SEMAPHORE ezOsal_SemaphoreResource_t;
typedef TX_TIMER ezOsal_TimerResource_t;
typedef TX_EVENT_FLAGS_GROUP ezOsal_EventResource_t;

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/

const ezOsal_Interfaces_t *ezOsal_ThreadXGetInterface(void);

#endif /* EZ_OSAL_THREADX_ENABLE == 1 */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_OSAL_THREADX_H */


/* End of file */
