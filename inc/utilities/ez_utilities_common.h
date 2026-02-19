/*****************************************************************************
* Filename:         ez_utilities_common.h
* Author:           Hai Nguyen
* Original Date:    09.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_utilities_common.h
 *  @author Hai Nguyen
 *  @date   09.03.2024
 *  @brief  Contain common data that share accross components
 *
 *  @details Detail description of the component
 */

#ifndef _EZ_UTILITIES_COMMON_H
#define _EZ_UTILITIES_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
/* INCLUDE HEADER HERE */

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* @brief return status of the Easy Embedded SDK API
 *
 */
typedef enum
{
    ezSUCCESS = 0,      /**< operation success */
    ezFAIL = -1,         /**< operation fail */
    ezSTATUS_ARG_INVALID = -2,  /**< invalid argument */
    ezSTATUS_TIMEOUT = -3,      /**< timeout */
}ezSTATUS;


/** @brief definition of status of the executed task
 *
 */
typedef enum
{
    TASK_STATUS_OK,         /**< status OK, task will be removed from list */
    TASK_STATUS_EXEC_AGAIN, /**< task will be executed again with the same interval */
    TASK_STATUS_ERROR,      /**< task error, will be moved from list */
}KERNEL_TASK_STATUS;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */


/*****************************************************************************
* Function Prototypes
*****************************************************************************/
/* None */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_UTILITIES_COMMON_H */
/* End of file */
