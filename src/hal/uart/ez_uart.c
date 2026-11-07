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
 *  @details Migrated to the driver model of ez_driver_def.h. The module does
 *           not lock: the upper layer serializes the calls.
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

/*****************************************************************************
* Function Definitions
*****************************************************************************/
static struct ezUartDriver *ezUart_FindDriver(const char *name);

static EZ_DRV_STATUS ezUart_Begin(ezUartHandle_t *inst,
                                  bool require_initialized,
                                  bool is_async,
                                  struct ezUartDriver **drv_out);

static void ezUart_End(ezUartHandle_t *inst,
                       struct ezUartDriver *drv,
                       EZ_DRV_STATUS status,
                       bool is_async);

/*****************************************************************************
 * Public functions
 *****************************************************************************/
EZ_DRV_STATUS ezUart_SystemRegisterHwDriver(struct ezUartDriver *hw_uart_driver)
{
    EZTRACE("ezUart_SystemRegisterHwDriver()");
    if((hw_uart_driver == NULL) || (hw_uart_driver->base.name == NULL))
    {
        EZERROR("hw_uart_driver or its name == NULL");
        return EZ_DRV_STATUS_ERR_ARG;
    }

    if(ezUart_FindDriver(hw_uart_driver->base.name) != NULL)
    {
        EZERROR("driver %s is already registered", hw_uart_driver->base.name);
        return EZ_DRV_STATUS_ERR_ARG;
    }

    hw_uart_driver->initialized = false;
    hw_uart_driver->base.state = 0U;
    hw_uart_driver->base.user = NULL;
    EZ_LINKEDLIST_ADD_TAIL(&hw_driver_list, &hw_uart_driver->ll_node);
    EZDEBUG("Register OK");
    return EZ_DRV_STATUS_OK;
}


EZ_DRV_STATUS ezUart_SystemUnregisterHwDriver(struct ezUartDriver *hw_uart_driver)
{
    EZTRACE("ezUart_SystemUnregisterHwDriver()");
    if(hw_uart_driver == NULL)
    {
        EZERROR("hw_uart_driver == NULL");
        return EZ_DRV_STATUS_ERR_ARG;
    }

    if(EZ_DRV_STATE_IS_SET(&hw_uart_driver->base, EZ_DRV_STATE_BUSY))
    {
        EZERROR("Driver is busy");
        return EZ_DRV_STATUS_BUSY;
    }

    hw_uart_driver->initialized = false;
    EZ_LINKEDLIST_UNLINK_NODE(&hw_uart_driver->ll_node);
    return EZ_DRV_STATUS_OK;
}


EZ_DRV_STATUS ezUart_RegisterInstance(ezUartHandle_t *inst,
                                      const char *driver_name,
                                      ezDrvCallback callback,
                                      void *ctx)
{
    struct ezUartDriver *uart_drv = NULL;

    EZTRACE("ezUart_RegisterInstance()");
    if((inst == NULL) || (driver_name == NULL))
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    if(inst->drv_handle.hw != NULL)
    {
        EZERROR("The handle is already bound, unregister it first");
        return EZ_DRV_STATUS_ERR_ARG;
    }

    uart_drv = ezUart_FindDriver(driver_name);
    if(uart_drv == NULL)
    {
        return EZ_DRV_STATUS_ERR_DRV_NOT_FOUND;
    }

    EZDEBUG("Found driver!");
    inst->drv_handle.hw = uart_drv;
    inst->drv_handle.on_event = callback;
    inst->drv_handle.ctx = ctx;
    return EZ_DRV_STATUS_OK;
}


EZ_DRV_STATUS ezUart_UnregisterInstance(ezUartHandle_t *inst)
{
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_UnregisterInstance()");
    if(inst == NULL)
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    drv = (struct ezUartDriver*)ezDriver_GetHw(&inst->drv_handle);
    if((drv != NULL) &&
       EZ_DRV_STATE_IS_SET(&drv->base, EZ_DRV_STATE_BUSY) &&
       (drv->base.user == &inst->drv_handle))
    {
        EZERROR("The handle has an operation in flight");
        return EZ_DRV_STATUS_BUSY;
    }

    inst->drv_handle.hw = NULL;
    inst->drv_handle.on_event = NULL;
    inst->drv_handle.ctx = NULL;
    EZDEBUG("unregister success");
    return EZ_DRV_STATUS_OK;
}


EZ_DRV_STATUS ezUart_Initialize(ezUartHandle_t *inst)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_Initialize()");
    status = ezUart_Begin(inst, false, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.initialize != NULL)
    {
        status = drv->interface.initialize(drv->interface.driver_h);
    }
    ezUart_End(inst, drv, status, false);

    drv->initialized = (status == EZ_DRV_STATUS_OK);
    if(drv->initialized)
    {
        EZDEBUG("Driver %s initialized", drv->base.name);
    }

    return status;
}


EZ_DRV_STATUS ezUart_Deinitialize(ezUartHandle_t *inst)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_Deinitialize()");
    status = ezUart_Begin(inst, false, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.deinitialize != NULL)
    {
        status = drv->interface.deinitialize(drv->interface.driver_h);
    }
    ezUart_End(inst, drv, status, false);

    if(status == EZ_DRV_STATUS_OK)
    {
        drv->initialized = false;
    }

    return status;
}


EZ_DRV_STATUS ezUart_TransmitAsync(ezUartHandle_t *inst, const uint8_t *tx_buff, uint16_t buff_size)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_TransmitAsync()");
    status = ezUart_Begin(inst, true, true, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.transmit_async != NULL)
    {
        status = drv->interface.transmit_async(drv->interface.driver_h,
                                               tx_buff,
                                               buff_size);
    }
    ezUart_End(inst, drv, status, true);
    return status;
}


EZ_DRV_STATUS ezUart_ReceiveAsync(ezUartHandle_t *inst, uint8_t *rx_buff, uint16_t buff_size)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_ReceiveAsync()");
    status = ezUart_Begin(inst, true, true, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.receive_async != NULL)
    {
        status = drv->interface.receive_async(drv->interface.driver_h,
                                              rx_buff,
                                              buff_size);
    }
    ezUart_End(inst, drv, status, true);
    return status;
}


EZ_DRV_STATUS ezUart_TransmitSync(ezUartHandle_t *inst, const uint8_t *tx_buff, uint16_t buff_size, uint32_t timeout_millis)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_TransmitSync()");
    status = ezUart_Begin(inst, true, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.transmit_sync != NULL)
    {
        status = drv->interface.transmit_sync(drv->interface.driver_h,
                                              tx_buff,
                                              buff_size,
                                              timeout_millis);
    }
    ezUart_End(inst, drv, status, false);
    return status;
}


EZ_DRV_STATUS ezUart_ReceiveSync(ezUartHandle_t *inst, uint8_t *rx_buff, uint16_t buff_size, uint32_t timeout_millis)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_ReceiveSync()");
    status = ezUart_Begin(inst, true, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.receive_sync != NULL)
    {
        status = drv->interface.receive_sync(drv->interface.driver_h,
                                             rx_buff,
                                             buff_size,
                                             timeout_millis);
    }
    ezUart_End(inst, drv, status, false);
    return status;
}


EZ_DRV_STATUS ezUart_GetConfig(ezUartHandle_t *inst, struct ezUartConfiguration **config)
{
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_GetConfig()");
    if(inst == NULL)
    {
        return EZ_DRV_STATUS_ERR_DRV_NOT_FOUND;
    }

    drv = (struct ezUartDriver*)ezDriver_GetHw(&inst->drv_handle);
    if(drv == NULL)
    {
        EZERROR("Driver not found");
        return EZ_DRV_STATUS_ERR_DRV_NOT_FOUND;
    }

    if(config == NULL)
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    *config = &drv->config;
    return EZ_DRV_STATUS_OK;
}


EZ_DRV_STATUS ezUart_UpdateConfig(ezUartHandle_t *inst)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezUartDriver *drv = NULL;

    EZTRACE("ezUart_UpdateConfig()");
    status = ezUart_Begin(inst, false, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.update_conf != NULL)
    {
        status = drv->interface.update_conf(drv->interface.driver_h);
    }
    ezUart_End(inst, drv, status, false);
    return status;
}


/*****************************************************************************
* Local functions
*****************************************************************************/

/* Find a registered HW driver by its name. Return NULL if not found. */
static struct ezUartDriver *ezUart_FindDriver(const char *name)
{
    struct Node *it_node = NULL;
    struct ezUartDriver *uart_drv = NULL;

    EZ_LINKEDLIST_FOR_EACH(it_node, &hw_driver_list)
    {
        uart_drv = EZ_LINKEDLIST_GET_PARENT_OF(it_node, ll_node, struct ezUartDriver);
        if(strcmp(uart_drv->base.name, name) == 0)
        {
            return uart_drv;
        }
    }

    return NULL;
}


/* Find the HW driver of the handle, check it and take it for an operation.
 * On EZ_DRV_STATUS_OK the caller must call ezUart_End(). */
static EZ_DRV_STATUS ezUart_Begin(ezUartHandle_t *inst,
                                  bool require_initialized,
                                  bool is_async,
                                  struct ezUartDriver **drv_out)
{
    struct ezUartDriver *drv = NULL;

    if(inst == NULL)
    {
        EZERROR("Driver not found");
        return EZ_DRV_STATUS_ERR_DRV_NOT_FOUND;
    }

    drv = (struct ezUartDriver*)ezDriver_GetHw(&inst->drv_handle);
    if(drv == NULL)
    {
        EZERROR("Driver not found");
        return EZ_DRV_STATUS_ERR_DRV_NOT_FOUND;
    }

    if(require_initialized && !drv->initialized)
    {
        EZERROR("Driver is not initialized");
        return EZ_DRV_STATUS_ERR_NOT_INITIALIZED;
    }

    if(!ezDriver_Acquire(&inst->drv_handle, &drv->base, is_async))
    {
        EZERROR("Driver is busy");
        return EZ_DRV_STATUS_BUSY;
    }

    *drv_out = drv;
    return EZ_DRV_STATUS_OK;
}


/* Give the driver back after the HW call.
 * - sync: always.
 * - async: only if the transfer did not start. A started transfer is released
 *   by the HW driver with ezDriver_NotifyEvent(EZ_DRV_EVENT_OP_DONE). */
static void ezUart_End(ezUartHandle_t *inst,
                       struct ezUartDriver *drv,
                       EZ_DRV_STATUS status,
                       bool is_async)
{
    if(is_async && (status == EZ_DRV_STATUS_OK))
    {
        return;
    }

    if(!ezDriver_Release(&inst->drv_handle, &drv->base))
    {
        EZDEBUG("Driver was already released by the HW driver");
    }
}

#endif /* EZ_UART == 1 */
/* End of file*/
