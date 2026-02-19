/*****************************************************************************
* Filename:         ez_driver_def.h
* Author:           Hai Nguyen
* Original Date:    15.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_driver_def.h
 *  @author Hai Nguyen
 *  @date   15.03.2024
 *  @brief  Definition of the driver component
 *
 *  @details
 */

#ifndef _EZ_DRIVER_DEF_H
#define _EZ_DRIVER_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_DRIVER == 1)
#include "stdint.h"
#include "stdbool.h"
#include "ez_linked_list.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None*/


/*****************************************************************************
* Component Typedefs
*****************************************************************************/

/** @brief Return status of the Driver API
 */
typedef enum
{
    STATUS_OK,                  /**< OK, working as expected */
    STATUS_BUSY,                /**< Driver is occupied by other ezDrvInstance instantace*/
    STATUS_TIMEOUT,             /**< Operation timeout */
    STATUS_ERR_GENERIC,         /**< Generic error */
    STATUS_ERR_ARG,             /**< Invalid function argument */
    STATUS_ERR_DRV_NOT_FOUND,   /**< HW driver implmentation not found */
    STATUS_ERR_INF_NOT_EXIST,   /**< Implementation of an interface not found */
}EZ_DRV_STATUS;


/** @brief Callback to receive event from the HW implementation
 *
 *  @param[out]  event_code: index of the HW Uart instance
 *  @param[out]  param1: point to the first parameter. Depending on event_code
 *  @param[out]  param2: point to the second parameter. Depending on event_code
 *  @return      None
 */
typedef void (*ezDrvCallback)(uint8_t event_code, void *param1, void *param2);


/** @brief Define a driver instance.
 */
struct ezDrvInstance
{
    ezDrvCallback   calback;   /**< Callback funtion to handle the event from the HW driver */
    void            *driver;    /**< Pointer to the HAL driver, depending on the implmentation */
};


/** @brief Define a driver instance type.
 */
typedef struct ezDrvInstance ezDrvInstance_t;


/** @brief Define structure holding common data of a driver
 */
struct ezDriverCommon
{
    const char*     name;           /* Name of the driver instance */
    uint8_t         version[3];     /* Version number including major, minor, patch */
    ezDrvInstance_t *curr_inst;     /* Point to the current instance using the driver. NULL means the driver is available. */
};


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */


/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function: ezDriver_GetDriverFromInstance
*//** 
* @brief Return the driver which the instance is registered to.
*
* @details Helper function used by other components. THe user are not
*          supposed to used this function
*
* @param[in]    inst: Instance to be register. @see ezUartDrvInstance_t
* @return       Pointer driver if success, else NULL
*
* @pre None
* @post None
*
* \b Example
* @code
* @endcode
*
* @see
*
*****************************************************************************/
static inline void *ezDriver_GetDriverFromInstance(ezDrvInstance_t *inst)
{
    void *drv = NULL;
    if(inst != NULL && inst->driver != NULL)
    {
        drv = inst->driver;
    }

    return drv;
}


/*****************************************************************************
* Function: ezDriver_IsDriverAvailable
*//** 
* @brief Check if the driver is availabe and ready to be used
*
* @details Helper function used by other components. THe user are not
*          supposed to used this function
*
* @param[in]    inst: Driver instance
* @param[in]    drv_common: Pointer to the common structure of the driver.
*               @see ezDriverCommon
* @return       true is the driver is available, else false
*
* @pre None
* @post None
*
* \b Example
* @code
* @endcode
*
* @see
*
*****************************************************************************/
static inline bool ezDriver_IsDriverAvailable(ezDrvInstance_t *inst,
                                              struct ezDriverCommon *drv_common)
{
    bool ret = false;
    if(inst != NULL && drv_common != NULL)
    {
        ret = ((drv_common->curr_inst == NULL) || (drv_common->curr_inst == inst));
    }
    return ret;
}


/*****************************************************************************
* Function: ezDriver_LockDriver
*//** 
* @brief Lock the driver. Prevent other instances use it
*
* @param[in]    inst: Driver instance
* @param[in]    drv_common: Pointer to the common structure of the driver.
* @return       None
*
* @pre None
* @post None
*
* \b Example
* @code
* @endcode
*
* @see
*
*****************************************************************************/
static inline void ezDriver_LockDriver(ezDrvInstance_t *inst,
                                       struct ezDriverCommon *drv_common)
{
    if(inst != NULL && drv_common != NULL)
    {
        drv_common->curr_inst = inst;
    }
}


/*****************************************************************************
* Function: ezDriver_UnlockDriver
*//** 
* @brief Unlock the driver.
*
* @param[in]    inst: Driver instance
* @param[in]    drv_common: Pointer to the common structure of the driver.
* @return       None
*
* @pre None
* @post None
*
* \b Example
* @code
* @endcode
*
* @see
*
*****************************************************************************/
static inline void ezDriver_UnlockDriver(struct ezDriverCommon *drv_common)
{
    if(drv_common != NULL)
    {
        drv_common->curr_inst = NULL;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* EZ_DRIVER == 1 */
#endif /* _EZ_DRIVER_DEF_H */


/* End of file */
