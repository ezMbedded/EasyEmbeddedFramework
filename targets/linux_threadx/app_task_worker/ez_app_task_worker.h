/*****************************************************************************
* Filename:         ez_app_task_worker.h
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

/** @file   ez_app_task_worker.h
 *  @author Hai Nguyen
 *  @date   16.03.2025
 *  @brief  Application to test Task Worker component
 *
 *  @details
 */

#ifndef _EZ_APP_TASK_WORKER_H
#define _EZ_APP_TASK_WORKER_H

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
int ezApp_TaskWorkerInit(void *arg);


#endif /* EZ_APP_TASK_WORKER_ENABLE == 1 */

#ifdef __cplusplus
}
#endif


/* End of file */
