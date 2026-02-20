/*****************************************************************************
* Filename:         main.c
* Author:           Hai Nguyen
* Original Date:    11.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   main.c
 *  @author Hai Nguyen
 *  @date   11.03.2024
 *  @brief  main.c file for linux target
 *
 *  @details
 */

/******************************************************************************
* Includes
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>


#define DEBUG_LVL   LVL_INFO   /**< logging level */
#define MOD_NAME    "main"       /**< module name */
#include "ez_logging.h"
#include "ez_app_task_worker.h"
#include "ez_app_osal.h"
#include "ez_osal.h"
#include "ez_osal_freertos.h"
#include "ez_app_cli.h"
#include "task.h"
#include "ez_version.h"

/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
/* None */

/******************************************************************************
* Module Typedefs
*******************************************************************************/
/* None */

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static const ezOsal_Interfaces_t *rtos_interface = NULL;

/******************************************************************************
* Function Definitions
*******************************************************************************/
/* None */

/******************************************************************************
* External functions
*******************************************************************************/
void main(void)
{
    EZINFO("Linux target using FreeRTOS - Hello world!");
    EZINFO("Version: %s", EZ_SDK_VERSION);
    rtos_interface = ezOsal_FreeRTOSGetInterface();
    (void) ezOsal_SetInterface(rtos_interface);

    ezApp_OsalInit(NULL);
    AppCli_Init();
    ezOsal_TaskStartScheduler();
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

/******************************************************************************
* Internal functions
*******************************************************************************/
/* None */

/* End of file*/

