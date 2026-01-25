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
/* None */

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
    EZ_LINKEDLIST_ADD_TAIL(&hw_driver_list, &hw_driver->ll_node);

    if(ezEventBus_CreateBus(&hw_driver->i2c_event) == ezSUCCESS)
    {
        EZDEBUG("Register OK");
        return STATUS_OK;
    }
    else
    {
        EZERROR("Cannot create subject for GPIO driver %s", hw_driver->common.name);
        return STATUS_ERR_GENERIC;
    }
    return STATUS_ERR_GENERIC;
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
                                     EVENT_CALLBACK callback)
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
            inst->drv_instance.driver = (void*)i2c_drv;
            inst->drv_instance.calback = NULL;

            if(ezEventBus_CreateListener(&inst->event_subcriber, callback) != ezSUCCESS)
            {
                EZERROR("Cannot create observer");
                return STATUS_ERR_GENERIC;
            }

            if(ezEventBus_Listen(&i2c_drv->i2c_event, &inst->event_subcriber) != ezSUCCESS)
            {
                EZERROR("Cannot subscribe to subject");
                return STATUS_ERR_GENERIC;
            }

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
    inst->drv_instance.driver = NULL;
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
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
        {
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
            if(drv->interface.transmit_sync)
            {
                status = drv->interface.transmit_sync(drv->interface.driver_h,
                    address,
                    data,
                    length,
                    send_stop,
                    timeout_millis);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
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
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
        {
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
            if(drv->interface.transmit_async)
            {
                status = drv->interface.transmit_async(drv->interface.driver_h,
                    address,
                    data,
                    length,
                    send_stop);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
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
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
        {
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
            if(drv->interface.receive_sync)
            {
                status = drv->interface.receive_sync(drv->interface.driver_h,
                    address,
                    data,
                    length,
                    send_stop,
                    timeout_millis);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
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
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
        {
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
            if(drv->interface.receive_async)
            {
                status = drv->interface.receive_async(drv->interface.driver_h,
                    address,
                    data,
                    length,
                    send_stop);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
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

   if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
    {
        status = STATUS_ERR_INF_NOT_EXIST;
        ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
        if(drv->interface.receive_async)
        {
            status = drv->interface.probe(drv->interface.driver_h,
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

#endif /* EZ_I2C == 1 */
/* End of file*/
