/*****************************************************************************
* Filename:         ez_uart.c
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

/** @file   ez_uart.c
 *  @author Hai Nguyen
 *  @date   15.03.2024
 *  @brief  Implementation of the UART component
 *
 *  @details
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_uart.h"

#if (EZ_UART == 1)
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_UART_LOGGING_LEVEL       /**< logging level */
#define MOD_NAME    "ez_uart"       /**< module name */
#include "ez_logging.h"

#include <string.h>


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
static struct Node hw_driver_list = EZ_LINKEDLIST_INIT_NODE(hw_driver_list);
/**< List of HW driver implementation  */

static struct Node instance_list = EZ_LINKEDLIST_INIT_NODE(instance_list);
/**< Keep tracks of instance register to this driver */

/*****************************************************************************
* Function Definitions
*****************************************************************************/
static void ezUart_PrintStatus(EZ_DRV_STATUS status);


/*****************************************************************************
* Public functions
*****************************************************************************/
EZ_DRV_STATUS ezUart_SystemRegisterHwDriver(struct ezUartDriver *hw_uart_driver)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;

    EZTRACE("ezUart_SystemRegisterHwDriver()");
    if(hw_uart_driver == NULL)
    {
        status = STATUS_ERR_ARG;
        EZERROR("hw_uart_driver == NULL");
    }
    else
    {
        EZ_LINKEDLIST_ADD_TAIL(&hw_driver_list, &hw_uart_driver->ll_node);
        status = STATUS_OK;
    }

    return status;
}


EZ_DRV_STATUS ezUart_SystemUnregisterHwDriver(struct ezUartDriver *hw_uart_driver)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;

    EZTRACE("ezUart_SystemUnregisterHwDriver()");
    if(hw_uart_driver == NULL)
    {
        status = STATUS_ERR_ARG;
        EZERROR("hw_uart_driver == NULL");
    }
    else
    {
        EZ_LINKEDLIST_UNLINK_NODE(&hw_uart_driver->ll_node);
        status = STATUS_OK;
    }

    return status;
}


EZ_DRV_STATUS ezUart_RegisterInstance(ezUartDrvInstance_t *inst,
                                      const char *driver_name,
                                      ezDrvCallback callback)
{
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct Node* it_node = NULL;
    struct ezUartDriver *uart_drv = NULL;

    EZTRACE("ezUart_RegisterInstance(name = %s)", driver_name);
    if((inst == NULL) || (driver_name == NULL))
    {
        status = STATUS_ERR_ARG;
    }
    else
    {
        EZ_LINKEDLIST_FOR_EACH(it_node, &hw_driver_list)
        {
            uart_drv = EZ_LINKEDLIST_GET_PARENT_OF(it_node, ll_node, struct ezUartDriver);
            if(strcmp(uart_drv->common.name, driver_name) == 0)
            {
                inst->driver = (void*)uart_drv;
                inst->calback = callback;
                status = STATUS_OK;
                EZDEBUG("Found driver!");
                break;
            }
        }
    }

    return status;
}


EZ_DRV_STATUS ezUart_UnregisterInstance(ezUartDrvInstance_t *inst)
{
    EZTRACE("ezUart_UnregisterInstance()");
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;

    if(inst == NULL)
    {
        status = STATUS_ERR_ARG;
    }
    else
    {
        inst->driver = NULL;
        EZDEBUG("unregister success");
        status = STATUS_OK;
    }

    return status;
}


EZ_DRV_STATUS ezUart_Initialize(ezUartDrvInstance_t *inst)
{
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_Initialize()");
    drv = (struct ezUartDriver*)ezDriver_GetDriverFromInstance(inst);
    if(drv != NULL)
    {
        EZTRACE("Found driver");
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable(inst, &drv->common) == true)
        {
            EZTRACE("Driver = %s is available", drv->common.name);
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver(inst, &drv->common);
            if(drv->interface.initialize)
            {
                status = drv->interface.initialize(drv->interface.index);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    ezUart_PrintStatus(status);
    return status;
}


EZ_DRV_STATUS ezUart_Deinitialize(ezUartDrvInstance_t *inst)
{
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_Deinitialize()");
    drv = (struct ezUartDriver*)ezDriver_GetDriverFromInstance(inst);
    if(drv != NULL)
    {
        EZTRACE("Found driver");
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable(inst, &drv->common) == true)
        {
            EZTRACE("Driver = %s is available", drv->common.name);
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver(inst, &drv->common);
            if(drv->interface.deinitialize)
            {
                status = drv->interface.deinitialize(drv->interface.index);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    ezUart_PrintStatus(status);
    return status;
}


EZ_DRV_STATUS ezUart_AsyncTransmit(ezUartDrvInstance_t *inst, const uint8_t *tx_buff, uint16_t buff_size)
{
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_AsyncTransmit()");
    drv = (struct ezUartDriver*)ezDriver_GetDriverFromInstance(inst);
    if(drv != NULL)
    {
        EZTRACE("Found driver");
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable(inst, &drv->common) == true)
        {
            EZTRACE("Driver = %s is available", drv->common.name);
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver(inst, &drv->common);
            if(drv->interface.async_transmit)
            {
                status = drv->interface.async_transmit(drv->interface.index,
                                                       tx_buff,
                                                       buff_size);
            }
            /* Driver is unlocked by the HW implementation in the callback function */
        }
    }
    ezUart_PrintStatus(status);
    return status;
}


EZ_DRV_STATUS ezUart_AsyncReceive(ezUartDrvInstance_t *inst, uint8_t *rx_buff, uint16_t buff_size)
{
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_AsyncReceive()");
    drv = (struct ezUartDriver*)ezDriver_GetDriverFromInstance(inst);
    if(drv != NULL)
    {
        EZTRACE("Found driver");
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable(inst, &drv->common) == true)
        {
            EZTRACE("Driver = %s is available", drv->common.name);
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver(inst, &drv->common);
            if(drv->interface.async_receive)
            {
                status = drv->interface.async_receive(drv->interface.index,
                                                       rx_buff,
                                                       buff_size);
            }
            /* Driver is unlocked by the HW implementation in the callback function */
        }
    }
    ezUart_PrintStatus(status);
    return status;
}


EZ_DRV_STATUS ezUart_SyncTransmit(ezUartDrvInstance_t *inst, const uint8_t *tx_buff, uint16_t buff_size, uint32_t timeout_millis)
{
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_SyncTransmit()");
    
    drv = (struct ezUartDriver*)ezDriver_GetDriverFromInstance(inst);
    if(drv != NULL)
    {
        EZTRACE("Found driver");
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable(inst, &drv->common) == true)
        {
            EZTRACE("Driver = %s is available", drv->common.name);
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver(inst, &drv->common);
            if(drv->interface.sync_transmit)
            {
                status = drv->interface.sync_transmit(drv->interface.index,
                                                      tx_buff,
                                                      buff_size,
                                                      timeout_millis);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    ezUart_PrintStatus(status);
    return status;
}


EZ_DRV_STATUS ezUart_SyncReceive(ezUartDrvInstance_t *inst, uint8_t *rx_buff, uint16_t buff_size, uint32_t timeout_millis)
{
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_SyncReceive()");
    
    drv = (struct ezUartDriver*)ezDriver_GetDriverFromInstance(inst);
    if(drv != NULL)
    {
        EZTRACE("Found driver");
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable(inst, &drv->common) == true)
        {
            EZTRACE("Driver = %s is available", drv->common.name);
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver(inst, &drv->common);
            if(drv->interface.sync_receive)
            {
                status = drv->interface.sync_receive(drv->interface.index,
                                                     rx_buff,
                                                     buff_size,
                                                     timeout_millis);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    ezUart_PrintStatus(status);
    return status;
}


EZ_DRV_STATUS ezUart_GetConfig(ezUartDrvInstance_t *inst, struct ezUartConfiguration **config)
{
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_GetConfig()");
    drv = (struct ezUartDriver*)ezDriver_GetDriverFromInstance(inst);
    if(drv != NULL)
    {
        EZTRACE("Found driver");
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable(inst, &drv->common) == true)
        {
            EZTRACE("Driver = %s is available", drv->common.name);
            status = STATUS_ERR_ARG;
            ezDriver_LockDriver(inst, &drv->common);
            if(config != NULL)
            {
                *config = &drv->config;
                status = STATUS_OK;
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    ezUart_PrintStatus(status);
    return status;
}


EZ_DRV_STATUS ezUart_UpdateConfig(ezUartDrvInstance_t *inst)
{
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_UpdateConfig()");
    
    drv = (struct ezUartDriver*)ezDriver_GetDriverFromInstance(inst);
    if(drv != NULL)
    {
        EZTRACE("Found driver");
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable(inst, &drv->common) == true)
        {
            EZTRACE("Driver = %s is available", drv->common.name);
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver(inst, &drv->common);
            if(drv->interface.update_conf)
            {
                status = drv->interface.update_conf(drv->interface.index);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    ezUart_PrintStatus(status);
    return status;
}


/*****************************************************************************
* Local functions
*****************************************************************************/


/*****************************************************************************
* Function: ezUart_PrintStatus
*//** 
* @brief Print the EZ_DRV_STATUS
*
* @details This function is activated only when DEBUG_LVL >= LVL_DEBUG
*
* @param[in]    status: status to be printed
* @return       None
*
* @pre DEBUG_LVL >= LVL_DEBUG
* @post None
*
* \b Example
* @code
* ezUart_PrintStatus(STATUS_ERR_ARG);
* @endcode
*
* @see
*
*****************************************************************************/
static void ezUart_PrintStatus(EZ_DRV_STATUS status)
{
#if (DEBUG_LVL >= LVL_DEBUG)
    switch(status)
    {
    case STATUS_BUSY: EZDEBUG("STATUS_BUSY"); break;
    case STATUS_ERR_ARG: EZDEBUG("STATUS_ERR_ARG"); break;
    case STATUS_ERR_DRV_NOT_FOUND: EZDEBUG("STATUS_ERR_DRV_NOT_FOUND"); break;
    case STATUS_ERR_GENERIC: EZDEBUG("STATUS_ERR_GENERIC"); break;
    case STATUS_ERR_INF_NOT_EXIST: EZDEBUG("STATUS_ERR_INF_NOT_EXIST"); break;
    case STATUS_OK: EZDEBUG("STATUS_OK"); break;
    case STATUS_TIMEOUT: EZDEBUG("STATUS_TIMEOUT"); break;
    default: break;
    }
#endif
}



#endif /* EZ_UART_ENABLE == 1 */
/* End of file*/
