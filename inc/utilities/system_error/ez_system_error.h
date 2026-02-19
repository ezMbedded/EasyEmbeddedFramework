
/*******************************************************************************
* Filename:         system_error.h
* Author:           Hai Nguyen
* Original Date:    06.04.2022 - 22:48:51
* Last Update:      06.04.2022 - 22:48:51
*
* -----------------------------------------------------------------------------
* Comany:           Easy Embedded
*                   Address Line 1
*                   Address Line 2
*
* -----------------------------------------------------------------------------
* Contact:          Easy Embedded
*                   hainguyen.ezm@gmail.com
*
* -----------------------------------------------------------------------------
* Copyright (C) 2022 by Easy Embedded
*  
*******************************************************************************/

/** @file   system_error.h
 *  @author Hai Nguyen
 *  @date   06.04.2022 - 22:48:51
 *  @brief  header file for system error module. it contains public api and data
 *  
 * @details
 * 
 */

#ifndef _SYSTEM_ERROR_H
#define _SYSTEM_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif


/*******************************************************************************
* Includes
*******************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_SYS_ERROR == 1U)
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
/* None */

/******************************************************************************
* Module Typedefs
*******************************************************************************/

/** @brief criticality of an error
 *
 */
typedef enum {
    WARNING,        /**< This is fine, system can work properly */
    ATTENTION,      /**< Need attention, analyze, later system works properly */
    CRITICAL,       /**< System cannot behave correctly, reboot might needed */
    CASTATROPHIC,   /**< System is faulty, and must be stopped */
}ERROR_LEVEL;

/** @brief function pointer to handle error
 *
 */
typedef void(*ErrorHandle)(void);

/** @brief define a structure to hold and error object
 *
 */
typedef struct 
{
    int error_id;               /**< id of the error, must be unique */
    ERROR_LEVEL level;          /**< Error level*/
    ErrorHandle ErrorHandle;    /**< Pointer to the function handling the error */
}ErrorObject;


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
/* None */

/******************************************************************************
* Function Prototypes
*******************************************************************************/
void SystemError_Initialize(void);          /**< Initialize module */
bool SystemError_SetError(int error_id);    /**< Trigger error handling */
void SystemError_HandleTestWarningError(void);      /*demo function*/
void SystemError_HandleTestAttentionError(void);    /*demo function*/
void SystemError_HandleTestCriticalError(void);     /*demo function*/
void SystemError_HandleTestCastatrophicError(void); /*demo function*/

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_SYSTEM_ERROR */
#endif /* _SYSTEM_ERROR_H */
/* End of file */

