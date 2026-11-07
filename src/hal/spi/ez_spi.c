/*****************************************************************************
* Filename:         ez_spi.c
* Author:           Hai Nguyen
* Original Date:    24.08.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_spi.c
 *  @author Hai Nguyen
 *  @date   24.08.2025
 *  @brief  Hardware Abstraction Layer for SPI Communication
 *
 *  @details This component provides the hardware abstraction layer for SPI 
 *           communication, supporting both master and slave modes with 
 *           synchronous and asynchronous operations.
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_spi.h"

#if (EZ_SPI == 1)
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_SPI_LOGGING_LEVEL    /**< logging level */
#define MOD_NAME    "ez_spi"                /**< module name */
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

/*****************************************************************************
* Function Definitions
*****************************************************************************/
static struct ezSpiDriver *ezSpi_FindDriver(const char *name);

static EZ_DRV_STATUS ezSpi_Begin(ezSpiHandle_t *inst,
                                 bool require_initialized,
                                 bool is_async,
                                 struct ezSpiDriver **drv_out);

static void ezSpi_End(ezSpiHandle_t *inst,
                      struct ezSpiDriver *drv,
                      EZ_DRV_STATUS status,
                      bool is_async);

/*****************************************************************************
* Public functions
*****************************************************************************/
EZ_DRV_STATUS ezSpi_SystemRegisterHwDriver(struct ezSpiDriver *hw_driver)
{
    EZTRACE("ezSpi_SystemRegisterHwDriver()");
    if((hw_driver == NULL) || (hw_driver->base.name == NULL))
    {
        EZERROR("hw_driver or its name == NULL");
        return EZ_DRV_STATUS_ERR_ARG;
    }

    if(ezSpi_FindDriver(hw_driver->base.name) != NULL)
    {
        EZERROR("driver %s is already registered", hw_driver->base.name);
        return EZ_DRV_STATUS_ERR_ARG;
    }

    hw_driver->initialized = false;
    hw_driver->base.state = 0U;
    hw_driver->base.user = NULL;
    EZ_LINKEDLIST_ADD_TAIL(&hw_driver_list, &hw_driver->ll_node);
    EZDEBUG("Register OK");
    return EZ_DRV_STATUS_OK;
}

EZ_DRV_STATUS ezSpi_SystemUnregisterHwDriver(struct ezSpiDriver *hw_driver)
{
    EZTRACE("ezSpi_SystemUnregisterHwDriver()");
    if(hw_driver == NULL)
    {
        EZERROR("hw_driver == NULL");
        return EZ_DRV_STATUS_ERR_ARG;
    }

    if(EZ_DRV_STATE_IS_SET(&hw_driver->base, EZ_DRV_STATE_BUSY))
    {
        EZERROR("Driver is busy");
        return EZ_DRV_STATUS_BUSY;
    }

    hw_driver->initialized = false;
    EZ_LINKEDLIST_UNLINK_NODE(&hw_driver->ll_node);
    return EZ_DRV_STATUS_OK;
}

EZ_DRV_STATUS ezSpi_RegisterInstance(ezSpiHandle_t *inst,
                                     const char *driver_name,
                                     ezDrvCallback callback,
                                     void *ctx)
{
    struct ezSpiDriver *spi_drv = NULL;

    EZTRACE("ezSpi_RegisterInstance()");
    if((inst == NULL) || (driver_name == NULL))
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    if(inst->drv_handle.hw != NULL)
    {
        EZERROR("The handle is already bound, unregister it first");
        return EZ_DRV_STATUS_ERR_ARG;
    }

    spi_drv = ezSpi_FindDriver(driver_name);
    if(spi_drv == NULL)
    {
        return EZ_DRV_STATUS_ERR_DRV_NOT_FOUND;
    }

    EZDEBUG("Found driver!");
    inst->drv_handle.hw = spi_drv;
    inst->drv_handle.on_event = callback;
    inst->drv_handle.ctx = ctx;
    return EZ_DRV_STATUS_OK;
}

EZ_DRV_STATUS ezSpi_UnregisterInstance(ezSpiHandle_t *inst)
{
    struct ezSpiDriver *drv = NULL;

    EZTRACE("ezSpi_UnregisterInstance()");
    if(inst == NULL)
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    drv = (struct ezSpiDriver*)ezDriver_GetHw(&inst->drv_handle);
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

EZ_DRV_STATUS ezSpi_Initialize(ezSpiHandle_t *inst, ezSpiConfig_t *config)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezSpiDriver *drv = NULL;

    EZTRACE("ezSpi_Initialize()");
    if(config == NULL)
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    status = ezSpi_Begin(inst, false, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.initialize != NULL)
    {
        status = drv->interface.initialize(drv->interface.driver_h, config);
    }
    ezSpi_End(inst, drv, status, false);

    drv->initialized = (status == EZ_DRV_STATUS_OK);
    if(drv->initialized)
    {
        EZDEBUG("Driver %s initialized", drv->base.name);
    }

    return status;
}

EZ_DRV_STATUS ezSpi_TransferSync(ezSpiHandle_t *inst,
                                 const uint8_t *tx_data,
                                 size_t tx_length,
                                 uint8_t *rx_data,
                                 size_t rx_length,
                                 uint32_t timeout_millis)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezSpiDriver *drv = NULL;

    EZTRACE("ezSpi_TransferSync()");
    status = ezSpi_Begin(inst, true, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.transfer_sync != NULL)
    {
        status = drv->interface.transfer_sync(drv->interface.driver_h,
                                              tx_data,
                                              tx_length,
                                              rx_data,
                                              rx_length,
                                              timeout_millis);
    }
    ezSpi_End(inst, drv, status, false);
    return status;
}

EZ_DRV_STATUS ezSpi_TransferAsync(ezSpiHandle_t *inst,
                                  const uint8_t *tx_data,
                                  size_t tx_length,
                                  uint8_t *rx_data,
                                  size_t rx_length)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezSpiDriver *drv = NULL;

    EZTRACE("ezSpi_TransferAsync()");
    status = ezSpi_Begin(inst, true, true, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.transfer_async != NULL)
    {
        status = drv->interface.transfer_async(drv->interface.driver_h,
                                               tx_data,
                                               tx_length,
                                               rx_data,
                                               rx_length);
    }
    ezSpi_End(inst, drv, status, true);
    return status;
}

EZ_DRV_STATUS ezSpi_TransmitSync(ezSpiHandle_t *inst,
                                 const uint8_t *tx_data,
                                 size_t length,
                                 uint32_t timeout_millis)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezSpiDriver *drv = NULL;

    EZTRACE("ezSpi_TransmitSync()");
    status = ezSpi_Begin(inst, true, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.transmit_sync != NULL)
    {
        status = drv->interface.transmit_sync(drv->interface.driver_h,
                                              tx_data,
                                              length,
                                              timeout_millis);
    }
    ezSpi_End(inst, drv, status, false);
    return status;
}

EZ_DRV_STATUS ezSpi_TransmitAsync(ezSpiHandle_t *inst,
                                  const uint8_t *tx_data,
                                  size_t length)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezSpiDriver *drv = NULL;

    EZTRACE("ezSpi_TransmitAsync()");
    status = ezSpi_Begin(inst, true, true, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.transmit_async != NULL)
    {
        status = drv->interface.transmit_async(drv->interface.driver_h,
                                               tx_data,
                                               length);
    }
    ezSpi_End(inst, drv, status, true);
    return status;
}

EZ_DRV_STATUS ezSpi_ReceiveSync(ezSpiHandle_t *inst,
                                uint8_t *rx_data,
                                size_t length,
                                uint32_t timeout_millis)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezSpiDriver *drv = NULL;

    EZTRACE("ezSpi_ReceiveSync()");
    status = ezSpi_Begin(inst, true, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.receive_sync != NULL)
    {
        status = drv->interface.receive_sync(drv->interface.driver_h,
                                             rx_data,
                                             length,
                                             timeout_millis);
    }
    ezSpi_End(inst, drv, status, false);
    return status;
}

EZ_DRV_STATUS ezSpi_ReceiveAsync(ezSpiHandle_t *inst,
                                 uint8_t *rx_data,
                                 size_t length)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezSpiDriver *drv = NULL;

    EZTRACE("ezSpi_ReceiveAsync()");
    status = ezSpi_Begin(inst, true, true, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.receive_async != NULL)
    {
        status = drv->interface.receive_async(drv->interface.driver_h,
                                              rx_data,
                                              length);
    }
    ezSpi_End(inst, drv, status, true);
    return status;
}

EZ_DRV_STATUS ezSpi_ChipSelect(ezSpiHandle_t *inst, bool select)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezSpiDriver *drv = NULL;

    EZTRACE("ezSpi_ChipSelect()");
    status = ezSpi_Begin(inst, true, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.chip_select != NULL)
    {
        status = drv->interface.chip_select(drv->interface.driver_h, select);
    }
    ezSpi_End(inst, drv, status, false);
    return status;
}

/*****************************************************************************
* Local functions
*****************************************************************************/

/* Find a registered HW driver by its name. Return NULL if not found. */
static struct ezSpiDriver *ezSpi_FindDriver(const char *name)
{
    struct Node *it_node = NULL;
    struct ezSpiDriver *spi_drv = NULL;

    EZ_LINKEDLIST_FOR_EACH(it_node, &hw_driver_list)
    {
        spi_drv = EZ_LINKEDLIST_GET_PARENT_OF(it_node, ll_node, struct ezSpiDriver);
        if(strcmp(spi_drv->base.name, name) == 0)
        {
            return spi_drv;
        }
    }

    return NULL;
}


/* Find the HW driver of the handle, check it and take it for an operation.
 * On EZ_DRV_STATUS_OK the caller must call ezSpi_End(). */
static EZ_DRV_STATUS ezSpi_Begin(ezSpiHandle_t *inst,
                                 bool require_initialized,
                                 bool is_async,
                                 struct ezSpiDriver **drv_out)
{
    struct ezSpiDriver *drv = NULL;

    if(inst == NULL)
    {
        EZERROR("Driver not found");
        return EZ_DRV_STATUS_ERR_DRV_NOT_FOUND;
    }

    drv = (struct ezSpiDriver*)ezDriver_GetHw(&inst->drv_handle);
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
static void ezSpi_End(ezSpiHandle_t *inst,
                      struct ezSpiDriver *drv,
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

#endif /* EZ_SPI == 1 */
/* End of file*/
