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
static struct Node hw_driver_list = EZ_LINKEDLIST_INIT_NODE(hw_driver_list);

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Public functions
*****************************************************************************/
EZ_DRV_STATUS ezSpi_SystemRegisterHwDriver(struct ezSpiDriver *hw_driver)
{
    EZTRACE("ezSpi_SystemRegisterHwDriver()");
    if(hw_driver == NULL)
    {
        EZERROR("hw_driver == NULL");
        return STATUS_ERR_ARG;
    }

    hw_driver->initialized = false;
    EZ_LINKEDLIST_ADD_TAIL(&hw_driver_list, &hw_driver->ll_node);
    EZDEBUG("Register OK");
    return STATUS_OK;
}

EZ_DRV_STATUS ezSpi_SystemUnregisterHwDriver(struct ezSpiDriver *hw_driver)
{
    EZTRACE("ezSpi_SystemUnregisterHwDriver()");
    if(hw_driver == NULL)
    {
        EZERROR("hw_driver == NULL");
        return STATUS_ERR_ARG;
    }
    hw_driver->initialized = false;
    EZ_LINKEDLIST_UNLINK_NODE(&hw_driver->ll_node);
    return STATUS_OK;
}

EZ_DRV_STATUS ezSpi_RegisterInstance(ezSpiDrvInstance_t *inst,
                                     const char *driver_name,
                                     ezDrvCallback callback)
{
    struct Node* it_node = NULL;
    struct ezSpiDriver *spi_drv = NULL;

    EZTRACE("ezSpi_RegisterInstance(name = %s)", driver_name);
    if((inst == NULL) || (driver_name == NULL))
    {
        return STATUS_ERR_ARG;
    }

    EZ_LINKEDLIST_FOR_EACH(it_node, &hw_driver_list)
    {
        spi_drv = EZ_LINKEDLIST_GET_PARENT_OF(it_node, ll_node, struct ezSpiDriver);
        if(strcmp(spi_drv->common.name, driver_name) == 0)
        {
            EZDEBUG("Found driver!");
            inst->drv_instance.driver = (void*)spi_drv;
            inst->drv_instance.calback = callback;
            return STATUS_OK;
        }
    }

    return STATUS_ERR_DRV_NOT_FOUND;
}

EZ_DRV_STATUS ezSpi_UnregisterInstance(ezSpiDrvInstance_t *inst)
{
    EZTRACE("ezSpi_UnregisterInstance()");
    if(inst == NULL)
    {
        return STATUS_ERR_ARG;
    }
    inst->drv_instance.driver = NULL;
    EZDEBUG("unregister success");
    return STATUS_OK;
}

EZ_DRV_STATUS ezSpi_Initialize(ezSpiDrvInstance_t *inst, ezSpiConfig_t *config)
{
    EZTRACE("ezSpi_Initialize()");
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;
    struct ezSpiDriver *drv = (struct ezSpiDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    if(drv == NULL)
    {
        EZERROR("Driver not found");
        return STATUS_ERR_DRV_NOT_FOUND;
    }

    if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == false)
    {
        EZTRACE("Driver = %s is not available", drv->common.name);
        return STATUS_ERR_INF_NOT_EXIST;
    }

    ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
    if(drv->interface.initialize != NULL)
    {
        status = drv->interface.initialize(drv->interface.driver_h, config);
    }
    ezDriver_UnlockDriver(&drv->common);

    if(status == STATUS_OK)
    {
        drv->initialized = true;
        EZDEBUG("Driver %s initialized", drv->common.name);
    }

    return status;
}

EZ_DRV_STATUS ezSpi_TransferSync(ezSpiDrvInstance_t *inst,
                                const uint8_t *tx_data,
                                size_t tx_length,
                                uint8_t *rx_data,
                                size_t rx_length,
                                uint32_t timeout_millis)
{
    EZTRACE("ezSpi_TransferSync()");
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezSpiDriver *drv = (struct ezSpiDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    if(drv != NULL)
    {
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
        {
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
            if(drv->interface.transfer_sync)
            {
                status = drv->interface.transfer_sync(drv->interface.driver_h,
                    tx_data,
                    tx_length,
                    rx_data,
                    rx_length,
                    timeout_millis);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    return status;
}

EZ_DRV_STATUS ezSpi_TransferAsync(ezSpiDrvInstance_t *inst,
                                 const uint8_t *tx_data,
                                 size_t tx_length,
                                 uint8_t *rx_data,
                                 size_t rx_length)
{
    EZTRACE("ezSpi_TransferAsync()");
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezSpiDriver *drv = (struct ezSpiDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    if(drv != NULL)
    {
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
        {
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
            if(drv->interface.transfer_async)
            {
                status = drv->interface.transfer_async(drv->interface.driver_h,
                    tx_data,
                    tx_length,
                    rx_data,
                    rx_length);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    return status;
}

EZ_DRV_STATUS ezSpi_TransmitSync(ezSpiDrvInstance_t *inst,
                                const uint8_t *tx_data,
                                size_t length,
                                uint32_t timeout_millis)
{
    EZTRACE("ezSpi_TransmitSync()");
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezSpiDriver *drv = (struct ezSpiDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    if(drv != NULL)
    {
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
        {
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
            if(drv->interface.transmit_sync)
            {
                status = drv->interface.transmit_sync(drv->interface.driver_h,
                    tx_data,
                    length,
                    timeout_millis);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    return status;
}

EZ_DRV_STATUS ezSpi_TransmitAsync(ezSpiDrvInstance_t *inst,
                                 const uint8_t *tx_data,
                                 size_t length)
{
    EZTRACE("ezSpi_TransmitAsync()");
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezSpiDriver *drv = (struct ezSpiDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    if(drv != NULL)
    {
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
        {
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
            if(drv->interface.transmit_async)
            {
                status = drv->interface.transmit_async(drv->interface.driver_h,
                    tx_data,
                    length);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    return status;
}

EZ_DRV_STATUS ezSpi_ReceiveSync(ezSpiDrvInstance_t *inst,
                               uint8_t *rx_data,
                               size_t length,
                               uint32_t timeout_millis)
{
    EZTRACE("ezSpi_ReceiveSync()");
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezSpiDriver *drv = (struct ezSpiDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    if(drv != NULL)
    {
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
        {
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
            if(drv->interface.receive_sync)
            {
                status = drv->interface.receive_sync(drv->interface.driver_h,
                    rx_data,
                    length,
                    timeout_millis);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    return status;
}

EZ_DRV_STATUS ezSpi_ReceiveAsync(ezSpiDrvInstance_t *inst,
                                uint8_t *rx_data,
                                size_t length)
{
    EZTRACE("ezSpi_ReceiveAsync()");
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezSpiDriver *drv = (struct ezSpiDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    if(drv != NULL)
    {
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
        {
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
            if(drv->interface.receive_async)
            {
                status = drv->interface.receive_async(drv->interface.driver_h,
                    rx_data,
                    length);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    return status;
}

EZ_DRV_STATUS ezSpi_ChipSelect(ezSpiDrvInstance_t *inst, bool select)
{
    EZTRACE("ezSpi_ChipSelect()");
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezSpiDriver *drv = (struct ezSpiDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    if(drv != NULL)
    {
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
        {
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
            if(drv->interface.chip_select)
            {
                status = drv->interface.chip_select(drv->interface.driver_h, select);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    return status;
}

/*****************************************************************************
* Local functions
*****************************************************************************/

#endif /* EZ_SPI == 1 */
/* End of file*/
