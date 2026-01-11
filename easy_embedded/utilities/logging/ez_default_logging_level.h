
/*****************************************************************************
* Filename:         ez_default_logging_level.h
* Author:           Hai Nguyen
* Original Date:    09.06.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_default_logging_level.h
 *  @author Hai Nguyen
 *  @date   26.02.2024
 *  @brief  Defualt logging level for modules
 *
 */

#ifndef _EZ_DEFAULT_LOGGING_LEVEL_H
#define _EZ_DEFAULT_LOGGING_LEVEL_H

#ifdef __cplusplus
extern "C" {
#endif

#if _EZ_LOGGING_LEVEL_H
#include "ez_logging_level.h"
#endif

#ifndef EZ_GPIO_LOGGING_LEVEL
#define EZ_GPIO_LOGGING_LEVEL           LVL_ERROR
#endif /* EZ_GPIO_LOGGING_LEVEL */

#ifndef EZ_I2C_LOGGING_LEVEL
#define EZ_I2C_LOGGING_LEVEL            LVL_ERROR
#endif /* EZ_I2C_LOGGING_LEVEL */

#ifndef EZ_UART_LOGGING_LEVEL
#define EZ_UART_LOGGING_LEVEL           LVL_ERROR
#endif /* EZ_UART_LOGGING_LEVEL */

#ifndef EZ_HAL_LOGGING_LEVEL
#define EZ_HAL_LOGGING_LEVEL            LVL_ERROR
#endif /* EZ_HAL_LOGGING_LEVEL */

#ifndef EZ_OSAL_FREERTOS_LOGGING_LEVEL
#define EZ_OSAL_FREERTOS_LOGGING_LEVEL  LVL_ERROR
#endif /* EZ_OSAL_FREERTOS_LOGGING_LEVEL */

#ifndef EZ_OSAL_THREADX_LOGGING_LEVEL
#define EZ_OSAL_THREADX_LOGGING_LEVEL   LVL_ERROR
#endif /* EZ_OSAL_THREADX_LOGGING_LEVEL */

#ifndef EZ_OSAL_LOGGING_LEVEL
#define EZ_OSAL_LOGGING_LEVEL           LVL_ERROR
#endif /* EZ_OSAL_THREADX_LOGGING_LEVEL */

#ifndef EZ_MIDDLEWARE_LOGGING_LEVEL
#define EZ_MIDDLEWARE_LOGGING_LEVEL     LVL_ERROR
#endif /* EZ_OSAL_THREADX_LOGGING_LEVEL */

#ifndef EZ_CLI_LOGGING_LEVEL
#define EZ_CLI_LOGGING_LEVEL            LVL_ERROR
#endif /* EZ_OSAL_THREADX_LOGGING_LEVEL */

#ifndef EZ_DATA_MODEL_LOGGING_LEVEL
#define EZ_DATA_MODEL_LOGGING_LEVEL     LVL_ERROR
#endif /* EZ_DATA_MODEL_LOGGING_LEVEL */

#ifndef EZ_EVENT_LOGGING_LEVEL
#define EZ_EVENT_LOGGING_LEVEL          LVL_ERROR
#endif /* EZ_EVENT_LOGGING_LEVEL */

#ifndef EZ_IPC_LOGGING_LEVEL
#define EZ_IPC_LOGGING_LEVEL            LVL_ERROR
#endif /* EZ_IPC_LOGGING_LEVEL */

#ifndef EZ_RPC_LOGGING_LEVEL
#define EZ_RPC_LOGGING_LEVEL            LVL_DEBUG
#endif /* EZ_RPC_LOGGING_LEVEL */

#ifndef EZ_STATE_MACHINE_LOGGING_LEVEL
#define EZ_STATE_MACHINE_LOGGING_LEVEL  LVL_ERROR
#endif /* EZ_STATE_MACHINE_LOGGING_LEVEL */

#ifndef EZ_TASK_WORKER_LOGGING_LEVEL
#define EZ_TASK_WORKER_LOGGING_LEVEL    LVL_ERROR
#endif /* EZ_TASK_WORKER_LOGGING_LEVEL */

#ifndef EZ_SERVICE_LOGGING_LEVEL
#define EZ_SERVICE_LOGGING_LEVEL        LVL_ERROR
#endif /* EZ_SERVICE_LOGGING_LEVEL */

#ifndef EZ_QUEUE_LOGGING_LEVEL
#define EZ_QUEUE_LOGGING_LEVEL        LVL_ERROR
#endif /* EZ_QUEUE_LOGGING_LEVEL */


#ifdef __cplusplus
}
#endif

#endif /* _EZ_DEFAULT_LOGGING_LEVEL_H */