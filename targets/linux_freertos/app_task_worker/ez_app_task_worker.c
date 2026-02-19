/*****************************************************************************
* Filename:         ez_app_task_worker.c
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

/** @file   ez_app_task_worker.c
 *  @author Hai Nguyen
 *  @date   16.03.2025
 *  @brief  Application to test Task Worker component
 *
 *  @details
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_app_task_worker.h"

#define DEBUG_LVL   LVL_TRACE   /**< logging level */
#define MOD_NAME    "ez_app_task_worker"       /**< module name */
#include "ez_logging.h"
#include "ez_easy_embedded.h"
#include "ez_task_worker.h"

#if(EZ_OSAL == 1)
#include "ez_osal.h"
#include "ez_osal_freertos.h"
#include "task.h"
#endif

#include "ez_worker1.h"
#include "ez_worker2.h"

/*the rest of include go here*/

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define BUFF_SIZE   256
#define PRIORITY    10
#define STACK_SIZE  512

/******************************************************************************
* Module Typedefs
*******************************************************************************/
/* None */

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
/* None */


/******************************************************************************
* Function Definitions
*******************************************************************************/


/******************************************************************************
* External functions
*******************************************************************************/


/******************************************************************************
* Internal functions
*******************************************************************************/

int ezApp_TaskWorkerInit(void *arg)
{
    ezApp_Worker1Init();
    ezApp_Worker2Init();
}


#if ( configCHECK_FOR_STACK_OVERFLOW > 0 )

    void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                        char * pcTaskName )
    {
        /* Check pcTaskName for the name of the offending task,
         * or pxCurrentTCB if pcTaskName has itself been corrupted. */
        ( void ) xTask;
        printf("%s overflow\n", pcTaskName);
    }

#endif /* #if ( configCHECK_FOR_STACK_OVERFLOW > 0 ) */


/* End of file*/
