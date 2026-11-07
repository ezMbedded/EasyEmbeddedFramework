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
 *  @brief  Hardware Abstraction Layer for I2C Communication
 *
 *  @details Binds a user handle to a HW driver by name and forwards the
 *           operations to it. Sync operations release the driver on return,
 *           async operations keep it busy until the HW driver reports
 *           EZ_DRV_EVENT_OP_DONE.
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
/* None */

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
static struct Node hw_driver_list = EZ_LINKEDLIST_INIT_NODE(hw_driver_list);

/*****************************************************************************
* Function Definitions
*****************************************************************************/
static struct ezI2cDriver *ezI2c_FindDriver(const char *name);

static EZ_DRV_STATUS ezI2c_Begin(ezI2cHandle_t *inst,
                                 bool require_initialized,
                                 bool is_async,
                                 struct ezI2cDriver **drv_out);

static void ezI2c_End(ezI2cHandle_t *inst,
                      struct ezI2cDriver *drv,
                      EZ_DRV_STATUS status,
                      bool is_async);

/*****************************************************************************
* Public functions
*****************************************************************************/
EZ_DRV_STATUS ezI2c_SystemRegisterHwDriver(struct ezI2cDriver *hw_driver)
{
    EZTRACE("ezI2c_SystemRegisterHwDriver()");
    if((hw_driver == NULL) || (hw_driver->base.name == NULL))
    {
        EZERROR("hw_driver or its name == NULL");
        return EZ_DRV_STATUS_ERR_ARG;
    }

    if(ezI2c_FindDriver(hw_driver->base.name) != NULL)
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

EZ_DRV_STATUS ezI2c_SystemUnregisterHwDriver(struct ezI2cDriver *hw_driver)
{
    EZTRACE("ezI2c_SystemUnregisterHwDriver()");
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

EZ_DRV_STATUS ezI2c_RegisterInstance(ezI2cHandle_t *inst,
                                     const char *driver_name,
                                     ezDrvCallback callback,
                                     void *ctx)
{
    struct ezI2cDriver *i2c_drv = NULL;

    EZTRACE("ezI2c_RegisterInstance()");
    if((inst == NULL) || (driver_name == NULL))
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    if(inst->drv_handle.hw != NULL)
    {
        EZERROR("The handle is already bound, unregister it first");
        return EZ_DRV_STATUS_ERR_ARG;
    }

    i2c_drv = ezI2c_FindDriver(driver_name);
    if(i2c_drv == NULL)
    {
        return EZ_DRV_STATUS_ERR_DRV_NOT_FOUND;
    }

    EZDEBUG("Found driver!");
    inst->drv_handle.hw = i2c_drv;
    inst->drv_handle.on_event = callback;
    inst->drv_handle.ctx = ctx;
    return EZ_DRV_STATUS_OK;
}

EZ_DRV_STATUS ezI2c_UnregisterInstance(ezI2cHandle_t *inst)
{
    struct ezI2cDriver *drv = NULL;

    EZTRACE("ezI2c_UnregisterInstance()");
    if(inst == NULL)
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    drv = (struct ezI2cDriver*)ezDriver_GetHw(&inst->drv_handle);
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

EZ_DRV_STATUS ezI2c_Initialize(ezI2cHandle_t *inst, ezI2cConfig_t *config)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezI2cDriver *drv = NULL;

    EZTRACE("ezI2c_Initialize()");
    if(config == NULL)
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    status = ezI2c_Begin(inst, false, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.initialize != NULL)
    {
        status = drv->interface.initialize(drv->interface.driver_h, config);
    }
    ezI2c_End(inst, drv, status, false);

    drv->initialized = (status == EZ_DRV_STATUS_OK);
    if(drv->initialized)
    {
        EZDEBUG("Driver %s initialized", drv->base.name);
    }

    return status;
}

EZ_DRV_STATUS ezI2c_TransmitSync(ezI2cHandle_t *inst,
                                 uint16_t address,
                                 const uint8_t *data,
                                 size_t length,
                                 bool send_stop,
                                 uint32_t timeout_millis)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezI2cDriver *drv = NULL;

    EZTRACE("ezI2c_TransmitSync()");
    status = ezI2c_Begin(inst, true, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.transmit_sync != NULL)
    {
        status = drv->interface.transmit_sync(drv->interface.driver_h,
                                              address,
                                              data,
                                              length,
                                              send_stop,
                                              timeout_millis);
    }
    ezI2c_End(inst, drv, status, false);
    return status;
}

EZ_DRV_STATUS ezI2c_TransmitAsync(ezI2cHandle_t *inst,
                                  uint16_t address,
                                  const uint8_t *data,
                                  size_t length,
                                  bool send_stop)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezI2cDriver *drv = NULL;

    EZTRACE("ezI2c_TransmitAsync()");
    status = ezI2c_Begin(inst, true, true, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.transmit_async != NULL)
    {
        status = drv->interface.transmit_async(drv->interface.driver_h,
                                               address,
                                               data,
                                               length,
                                               send_stop);
    }
    ezI2c_End(inst, drv, status, true);
    return status;
}

EZ_DRV_STATUS ezI2c_ReceiveSync(ezI2cHandle_t *inst,
                                uint16_t address,
                                uint8_t *data,
                                size_t length,
                                bool send_stop,
                                uint32_t timeout_millis)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezI2cDriver *drv = NULL;

    EZTRACE("ezI2c_ReceiveSync()");
    status = ezI2c_Begin(inst, true, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.receive_sync != NULL)
    {
        status = drv->interface.receive_sync(drv->interface.driver_h,
                                             address,
                                             data,
                                             length,
                                             send_stop,
                                             timeout_millis);
    }
    ezI2c_End(inst, drv, status, false);
    return status;
}

EZ_DRV_STATUS ezI2c_ReceiveAsync(ezI2cHandle_t *inst,
                                 uint16_t address,
                                 uint8_t *data,
                                 size_t length,
                                 bool send_stop)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezI2cDriver *drv = NULL;

    EZTRACE("ezI2c_ReceiveAsync()");
    status = ezI2c_Begin(inst, true, true, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.receive_async != NULL)
    {
        status = drv->interface.receive_async(drv->interface.driver_h,
                                              address,
                                              data,
                                              length,
                                              send_stop);
    }
    ezI2c_End(inst, drv, status, true);
    return status;
}

EZ_DRV_STATUS ezI2c_Probe(ezI2cHandle_t *inst,
                          uint16_t address,
                          uint32_t timeout_millis)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezI2cDriver *drv = NULL;

    EZTRACE("ezI2c_Probe()");
    status = ezI2c_Begin(inst, true, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.probe != NULL)
    {
        status = drv->interface.probe(drv->interface.driver_h,
                                      address,
                                      timeout_millis);
    }
    ezI2c_End(inst, drv, status, false);
    return status;
}

/*****************************************************************************
* Local functions
*****************************************************************************/

/* Find a registered HW driver by its name. Return NULL if not found. */
static struct ezI2cDriver *ezI2c_FindDriver(const char *name)
{
    struct Node *it_node = NULL;
    struct ezI2cDriver *i2c_drv = NULL;

    EZ_LINKEDLIST_FOR_EACH(it_node, &hw_driver_list)
    {
        i2c_drv = EZ_LINKEDLIST_GET_PARENT_OF(it_node, ll_node, struct ezI2cDriver);
        if(strcmp(i2c_drv->base.name, name) == 0)
        {
            return i2c_drv;
        }
    }

    return NULL;
}


/* Find the HW driver of the handle, check it and take it for an operation.
 * On EZ_DRV_STATUS_OK the caller must call ezI2c_End(). */
static EZ_DRV_STATUS ezI2c_Begin(ezI2cHandle_t *inst,
                                 bool require_initialized,
                                 bool is_async,
                                 struct ezI2cDriver **drv_out)
{
    struct ezI2cDriver *drv = NULL;

    if(inst == NULL)
    {
        EZERROR("Driver not found");
        return EZ_DRV_STATUS_ERR_DRV_NOT_FOUND;
    }

    drv = (struct ezI2cDriver*)ezDriver_GetHw(&inst->drv_handle);
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
static void ezI2c_End(ezI2cHandle_t *inst,
                      struct ezI2cDriver *drv,
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

#endif /* EZ_I2C == 1 */
/* End of file*/
