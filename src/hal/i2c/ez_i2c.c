/*****************************************************************************
* Filename:         ez_i2c.c
* Author:           Hai Nguyen
* Original Date:    25.05.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_i2c.c
 *  @author Hai Nguyen
 *  @date   25.05.2025
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_i2c.h"

#if (EZ_I2C == 1)
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_I2C_LOGGING_LEVEL    /**< logging level */
#define MOD_NAME    "ez_i2c"                /**< module name */
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
static void ezI2c_OnReceiveEvent(
    void *driver_h,
    uint8_t event_code,
    void *param1,
    void *param2
);

/*****************************************************************************
* Public functions
*****************************************************************************/
EZ_DRV_STATUS ezI2c_SystemRegisterHwDriver(struct ezI2cDriver *hw_driver)
{
    EZTRACE("ezI2c_SystemRegisterHwDriver()");
    if(hw_driver == NULL)
    {
        EZERROR("hw_driver == NULL");
        return STATUS_ERR_ARG;
    }

    hw_driver->initialized = false;
    hw_driver->common.callback = ezI2c_OnReceiveEvent;
    EZ_LINKEDLIST_ADD_TAIL(&hw_driver_list, &hw_driver->ll_node);
    EZDEBUG("Register OK");
    return STATUS_OK;
}

EZ_DRV_STATUS ezI2c_SystemUnregisterHwDriver(struct ezI2cDriver *hw_driver)
{
    EZTRACE("ezI2c_SystemUnregisterHwDriver()");
    if(hw_driver == NULL)
    {
        EZERROR("hw_driver == NULL");
        return STATUS_ERR_ARG;
    }
    hw_driver->initialized = false;
    EZ_LINKEDLIST_UNLINK_NODE(&hw_driver->ll_node);
    return STATUS_OK;
}

EZ_DRV_STATUS ezI2c_RegisterInstance(ezI2cDrvInstance_t *inst,
                                     const char *driver_name,
                                     ezDrvCallback callback)
{
    struct Node* it_node = NULL;
    struct ezI2cDriver *i2c_drv = NULL;

    EZTRACE("ezI2c_RegisterInstance(name = %s)", driver_name);
    if((inst == NULL) || (driver_name == NULL))
    {
        return STATUS_ERR_ARG;
    }

    EZ_LINKEDLIST_FOR_EACH(it_node, &hw_driver_list)
    {
        i2c_drv = EZ_LINKEDLIST_GET_PARENT_OF(it_node, ll_node, struct ezI2cDriver);
        if(strcmp(i2c_drv->common.name, driver_name) == 0)
        {
            EZDEBUG("Found driver!");
            inst->driver = (void*)i2c_drv;
            inst->callback = callback;
            return STATUS_OK;
        }
    }

    return STATUS_ERR_DRV_NOT_FOUND;
}

EZ_DRV_STATUS ezI2c_UnregisterInstance(ezI2cDrvInstance_t *inst)
{
    EZTRACE("ezI2c_UnregisterInstance()");
    if(inst == NULL)
    {
        return STATUS_ERR_ARG;
    }
    inst->driver = NULL;
    EZDEBUG("unregister success");
    return STATUS_OK;
}

EZ_DRV_STATUS ezI2c_Initialize(ezI2cDrvInstance_t *inst, ezI2cConfig_t *config)
{
    EZTRACE("ezI2c_Initialize()");
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;
    struct ezI2cDriver *drv = (struct ezI2cDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    if(drv == NULL)
    {
        EZERROR("Driver not found");
        return STATUS_ERR_DRV_NOT_FOUND;
    }

    if(ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common) == false)
    {
        EZERROR("Driver is busy");
        return STATUS_BUSY;
    }

    if(drv->interface.initialize != NULL)
    {
        status = drv->interface.initialize(&drv->common, config);
    }
    ezDriver_UnlockDriver(&drv->common);

    if(status == STATUS_OK)
    {
        drv->initialized = true;
        EZDEBUG("Driver %s initialized", drv->common.name);
    }

    return status;
}

EZ_DRV_STATUS ezI2c_TransmitSync(ezI2cDrvInstance_t *inst,
                                 uint16_t address,
                                 const uint8_t *data,
                                 size_t length,
                                 bool send_stop,
                                 uint32_t timeout_millis)
{
    EZTRACE("ezI2c_TransmitSync()");
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezI2cDriver *drv = (struct ezI2cDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    if(drv != NULL)
    {
        if(ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common) == false)
        {
            EZERROR("Driver is busy");
            return STATUS_BUSY;
        }

        if(drv->interface.transmit_sync)
        {
            status = drv->interface.transmit_sync(&drv->common,
                address,
                data,
                length,
                send_stop,
                timeout_millis);
        }
        ezDriver_UnlockDriver(&drv->common);
    }
    return status;
}

EZ_DRV_STATUS ezI2c_TransmitAsync(ezI2cDrvInstance_t *inst,
                                  uint16_t address,
                                  const uint8_t *data,
                                  size_t length,
                                  bool send_stop)
{
    EZTRACE("ezI2c_TransmitAsync()");
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezI2cDriver *drv = (struct ezI2cDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    if(drv != NULL)
    {
        if(ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common) == false)
        {
            EZERROR("Driver is busy");
            return STATUS_BUSY;
        }
        
        if(drv->interface.transmit_async)
        {
            status = drv->interface.transmit_async(&drv->common,
                address,
                data,
                length,
                send_stop);
        }
        //Unlock will be done in the callbak when the async operation is done
    }
    return status;
}

EZ_DRV_STATUS ezI2c_ReceiveSync(ezI2cDrvInstance_t *inst,
                                uint16_t address,
                                uint8_t *data,
                                size_t length,
                                bool send_stop,
                                uint32_t timeout_millis)
{
    EZTRACE("ezI2c_ReceiveSync()");
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezI2cDriver *drv = (struct ezI2cDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    if(drv != NULL)
    {
        if(ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common) == false)
        {
            EZERROR("Driver is busy");
            return STATUS_BUSY;
        }

        if(drv->interface.receive_sync)
        {
            status = drv->interface.receive_sync(&drv->common,
                address,
                data,
                length,
                send_stop,
                timeout_millis);
        }
        ezDriver_UnlockDriver(&drv->common);
    }
    return status;
}

EZ_DRV_STATUS ezI2c_ReceiveAsync(ezI2cDrvInstance_t *inst,
                                 uint16_t address,
                                 uint8_t *data,
                                 size_t length,
                                 bool send_stop)
{
    EZTRACE("ezI2c_ReceiveAsync()");
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezI2cDriver *drv = (struct ezI2cDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    if(drv != NULL)
    {
        if(ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common) == false)
        {
            EZERROR("Driver is busy");
            return STATUS_BUSY;
        }

        if(drv->interface.receive_async)
        {
            status = drv->interface.receive_async(&drv->common,
                address,
                data,
                length,
                send_stop);
        }
        //Unlock will be done in the callbachk when the async operation is done
    }
    return status;
}


EZ_DRV_STATUS ezI2c_Probe(ezI2cDrvInstance_t *inst,
                          uint16_t address,
                          uint32_t timeout_millis)
{
    EZTRACE("ezI2c_Probe()");
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezI2cDriver *drv = (struct ezI2cDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    if(drv != NULL)
    {
        if(ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common) == false)
        {
            EZERROR("Driver is busy");
            return STATUS_BUSY;
        }
        
        if(drv->interface.probe)
        {
            status = drv->interface.probe(&drv->common,
                address,
                timeout_millis);
        }
        ezDriver_UnlockDriver(&drv->common);
    }
    return status;
}
/*****************************************************************************
* Local functions
*****************************************************************************/
static void ezI2c_OnReceiveEvent(
    void *driver_h,
    uint8_t event_code,
    void *param1,
    void *param2)
{
    struct Node* it_node = NULL;
    struct ezI2cDriver *i2c_drv = NULL;
    
    EZ_LINKEDLIST_FOR_EACH(it_node, &hw_driver_list)
    {
        i2c_drv = EZ_LINKEDLIST_GET_PARENT_OF(it_node, ll_node, struct ezI2cDriver);
        if(i2c_drv->common.curr_inst != NULL && i2c_drv->common.curr_inst->driver == driver_h)
        {
            if(i2c_drv->common.callback)
            {
                i2c_drv->common.curr_inst->callback(event_code, param1, param2);
                ezDriver_UnlockDriver(&i2c_drv->common);
            }
            break;
        }
    }
}



#endif /* EZ_I2C == 1 */
/* End of file*/
