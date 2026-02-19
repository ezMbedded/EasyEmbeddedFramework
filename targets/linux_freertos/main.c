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


#define DEBUG_LVL   LVL_TRACE   /**< logging level */
#define MOD_NAME    "main"       /**< module name */
#include "ez_logging.h"
#include "ez_app_task_worker.h"
#include "ez_app_osal.h"
#include "ez_osal.h"
#include "ez_osal_freertos.h"
#include "ez_app_cli.h"

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
    rtos_interface = ezOsal_FreeRTOSGetInterface();
    (void) ezOsal_SetInterface(rtos_interface);

    ezApp_OsalInit();
    AppCli_Init();
    ezOsal_TaskStartScheduler();
}


/******************************************************************************
* Internal functions
*******************************************************************************/
/* None */

/* End of file*/

