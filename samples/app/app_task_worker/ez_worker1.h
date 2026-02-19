/*****************************************************************************
* Filename:         ez_worker1.h
* Author:           Hai Nguyen
* Original Date:    30.03.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_worker1.h
 *  @author Hai Nguyen
 *  @date   30.03.2025
 *  @brief  public API for worker 1
 *
 *  @details -
 */

#ifndef _EZ_WORKER1_H
#define _EZ_WORKER1_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#include "stdbool.h"
#include "ez_task_worker.h"


/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
typedef struct
{
    int a;
    int b;
}Worker1_SumContext;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/

void ezApp_Worker1Init(void);
bool worker1_sum(Worker1_SumContext *context, ezTaskWorkerCallbackFunc callback);

#ifdef __cplusplus
}
#endif

#endif /* _EZ_WORKER1_H */


/* End of file */
