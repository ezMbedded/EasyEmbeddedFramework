/*****************************************************************************
* Filename:         ez_osal_freertos.h
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

/** @file   ez_osal_freertos.h
 *  @author Hai Nguyen
 *  @date   16.03.2025
 *  @brief  Provide the OSAL interface for FreeRTOS
 *
 *  @details None
 */

#ifndef _EZ_OSAL_FREERTOS_H
#define _EZ_OSAL_FREERTOS_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_FREERTOS_PORT == 1)
#include <stdint.h>
#include "ez_utilities_common.h"
#include "ez_osal.h"
#include "FreeRTOS.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
typedef struct
{
    StaticTask_t task_block;
    StackType_t *stack;
}ezOsal_TaskResource_t;
typedef StaticSemaphore_t ezOsal_SemaphoreResource_t;
typedef StackType_t ezOsal_Stack_t;
typedef StaticEventGroup_t ezOsal_EventResource_t;

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */


/*****************************************************************************
* Function Prototypes
*****************************************************************************/
const ezOsal_Interfaces_t *ezOsal_FreeRTOSGetInterface(void);


#endif /* EZ_FREERTOS_PORT == 1 */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_OSAL_FREERTOS_H */


/* End of file */
