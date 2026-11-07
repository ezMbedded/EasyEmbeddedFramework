/*****************************************************************************
* Filename:         ez_gpio.c
* Author:           Hai Nguyen
* Original Date:    09.05.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_gpio.c
 *  @author Hai Nguyen
 *  @date   09.05.2025
 *  @brief  Implementation of the GPIO component
 *
 *  @details -
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_gpio.h"

#if (EZ_GPIO == 1)
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_GPIO_LOGGING_LEVEL   /**< logging level */
#define MOD_NAME    "ez_gpio"       /**< module name */
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
static struct ezGpioDriver *ezGpio_FindDriver(const char *name);

static EZ_DRV_STATUS ezGpio_Lookup(ezGpioHandle_t *inst,
                                   bool require_initialized,
                                   struct ezGpioDriver **drv_out);

static EZ_DRV_STATUS ezGpio_Begin(ezGpioHandle_t *inst,
                                  bool require_initialized,
                                  struct ezGpioDriver **drv_out);

static void ezGpio_RollbackSubscription(ezGpioHandle_t *inst,
                                        struct ezGpioDriver *drv,
                                        bool newly_subscribed);

static void ezGpio_End(ezGpioHandle_t *inst, struct ezGpioDriver *drv);


/*****************************************************************************
* Public functions
*****************************************************************************/
EZ_DRV_STATUS ezGpio_SystemRegisterHwDriver(struct ezGpioDriver *hw_gpio_driver)
{
    EZTRACE("ezGpio_SystemRegisterHwDriver()");
    if((hw_gpio_driver == NULL) || (hw_gpio_driver->base.name == NULL))
    {
        EZERROR("hw_gpio_driver or its name == NULL");
        return EZ_DRV_STATUS_ERR_ARG;
    }

    if(ezGpio_FindDriver(hw_gpio_driver->base.name) != NULL)
    {
        EZERROR("driver %s is already registered", hw_gpio_driver->base.name);
        return EZ_DRV_STATUS_ERR_ARG;
    }

    hw_gpio_driver->initialized = false;
    hw_gpio_driver->base.state = 0U;
    hw_gpio_driver->base.user = NULL;
    EZ_LINKEDLIST_ADD_TAIL(&hw_driver_list, &hw_gpio_driver->ll_node);
    EZDEBUG("Register OK");
    return EZ_DRV_STATUS_OK;
}


EZ_DRV_STATUS ezGpio_SystemUnregisterHwDriver(struct ezGpioDriver *hw_gpio_driver)
{
    EZTRACE("ezGpio_SystemUnregisterHwDriver()");
    if(hw_gpio_driver == NULL)
    {
        EZERROR("hw_gpio_driver == NULL");
        return EZ_DRV_STATUS_ERR_ARG;
    }

    if(EZ_DRV_STATE_IS_SET(&hw_gpio_driver->base, EZ_DRV_STATE_BUSY))
    {
        EZERROR("Driver is busy");
        return EZ_DRV_STATUS_BUSY;
    }

    hw_gpio_driver->initialized = false;
    hw_gpio_driver->base.state = 0U;
    EZ_LINKEDLIST_UNLINK_NODE(&hw_gpio_driver->ll_node);
    return EZ_DRV_STATUS_OK;
}


EZ_DRV_STATUS ezGpio_RegisterInstance(ezGpioHandle_t *inst,
                                      const char *driver_name,
                                      ezDrvCallback callback,
                                      void *ctx)
{
    struct ezGpioDriver *gpio_drv = NULL;

    EZTRACE("ezGpio_RegisterInstance()");
    if((inst == NULL) || (driver_name == NULL))
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    if(inst->drv_handle.hw != NULL)
    {
        EZERROR("The handle is already bound, unregister it first");
        return EZ_DRV_STATUS_ERR_ARG;
    }

    gpio_drv = ezGpio_FindDriver(driver_name);
    if(gpio_drv == NULL)
    {
        return EZ_DRV_STATUS_ERR_DRV_NOT_FOUND;
    }

    EZDEBUG("Found driver!");
    inst->drv_handle.hw = gpio_drv;
    inst->drv_handle.on_event = callback;
    inst->drv_handle.ctx = ctx;
    return EZ_DRV_STATUS_OK;
}


EZ_DRV_STATUS ezGpio_UnregisterInstance(ezGpioHandle_t *inst)
{
    struct ezGpioDriver *drv = NULL;

    EZTRACE("ezGpio_UnregisterInstance()");
    if(inst == NULL)
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    drv = (struct ezGpioDriver*)ezDriver_GetHw(&inst->drv_handle);
    if(drv != NULL)
    {
        if(EZ_DRV_STATE_IS_SET(&drv->base, EZ_DRV_STATE_BUSY) &&
           (drv->base.user == &inst->drv_handle))
        {
            EZERROR("The handle has an operation in flight");
            return EZ_DRV_STATUS_BUSY;
        }

        if(ezDriver_Unsubscribe(&inst->drv_handle, &drv->base))
        {
            EZDEBUG("pin events of the handle are stopped");
        }
    }

    inst->drv_handle.hw = NULL;
    inst->drv_handle.on_event = NULL;
    inst->drv_handle.ctx = NULL;
    EZDEBUG("unregister success");
    return EZ_DRV_STATUS_OK;
}


EZ_DRV_STATUS ezGpio_Initialize(ezGpioHandle_t *inst, uint16_t pin_index, ezHwGpioConfig_t *config)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezGpioDriver *drv = NULL;
    bool newly_subscribed = false;

    EZTRACE("ezGpio_Initialize()");
    if(config == NULL)
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    status = ezGpio_Lookup(inst, false, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    /* Subscribe BEFORE the HW call: the HW driver may arm the interrupt inside
     * init_pin, and an event is delivered only while the handle listens.
     * ezDriver_Subscribe() is not allowed while the driver is busy. */
    if(config->intr_mode != EZ_INTERRUPT_NONE)
    {
        newly_subscribed = !(EZ_DRV_STATE_IS_SET(&drv->base, EZ_DRV_STATE_LISTENING) &&
                             (drv->base.user == &inst->drv_handle));
        if(!ezDriver_Subscribe(&inst->drv_handle, &drv->base))
        {
            EZERROR("Driver is busy or listened by another handle");
            return EZ_DRV_STATUS_BUSY;
        }
    }

    if(!ezDriver_Acquire(&inst->drv_handle, &drv->base, false))
    {
        EZERROR("Driver is busy");
        ezGpio_RollbackSubscription(inst, drv, newly_subscribed);
        return EZ_DRV_STATUS_BUSY;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.init_pin != NULL)
    {
        status = drv->interface.init_pin(drv->interface.driver_h, pin_index, config);
    }
    ezGpio_End(inst, drv);

    drv->initialized = (status == EZ_DRV_STATUS_OK);
    if(drv->initialized)
    {
        EZDEBUG("Driver %s initialized", drv->base.name);
    }
    else
    {
        ezGpio_RollbackSubscription(inst, drv, newly_subscribed);
    }

    return status;
}


EZ_GPIO_PIN_STATE ezGpio_ReadPin(ezGpioHandle_t *inst, uint16_t pin_index)
{
    EZ_GPIO_PIN_STATE state = EZ_GPIO_PIN_ERROR;
    struct ezGpioDriver *drv = NULL;

    EZTRACE("ezGpio_ReadPin()");
    if(ezGpio_Begin(inst, true, &drv) != EZ_DRV_STATUS_OK)
    {
        return EZ_GPIO_PIN_ERROR;
    }

    if(drv->interface.read_pin != NULL)
    {
        state = drv->interface.read_pin(drv->interface.driver_h, pin_index);
    }
    ezGpio_End(inst, drv);
    return state;
}


EZ_DRV_STATUS ezGpio_WritePin(ezGpioHandle_t *inst, uint16_t pin_index, EZ_GPIO_PIN_STATE state)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezGpioDriver *drv = NULL;

    EZTRACE("ezGpio_WritePin()");
    status = ezGpio_Begin(inst, true, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.write_pin != NULL)
    {
        status = drv->interface.write_pin(drv->interface.driver_h, pin_index, state);
    }
    ezGpio_End(inst, drv);
    return status;
}


EZ_DRV_STATUS ezGpio_TogglePin(ezGpioHandle_t *inst, uint16_t pin_index)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezGpioDriver *drv = NULL;

    EZTRACE("ezGpio_TogglePin()");
    status = ezGpio_Begin(inst, true, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.toggle_pin != NULL)
    {
        status = drv->interface.toggle_pin(drv->interface.driver_h, pin_index);
    }
    ezGpio_End(inst, drv);
    return status;
}


/*****************************************************************************
* Local functions
*****************************************************************************/

/* Find a registered HW driver by its name. Return NULL if not found. */
static struct ezGpioDriver *ezGpio_FindDriver(const char *name)
{
    struct Node *it_node = NULL;
    struct ezGpioDriver *gpio_drv = NULL;

    EZ_LINKEDLIST_FOR_EACH(it_node, &hw_driver_list)
    {
        gpio_drv = EZ_LINKEDLIST_GET_PARENT_OF(it_node, ll_node, struct ezGpioDriver);
        if(strcmp(gpio_drv->base.name, name) == 0)
        {
            return gpio_drv;
        }
    }

    return NULL;
}


/* Find the HW driver of the handle and check it. Nothing is changed. */
static EZ_DRV_STATUS ezGpio_Lookup(ezGpioHandle_t *inst,
                                   bool require_initialized,
                                   struct ezGpioDriver **drv_out)
{
    struct ezGpioDriver *drv = NULL;

    if(inst == NULL)
    {
        EZERROR("Driver not found");
        return EZ_DRV_STATUS_ERR_DRV_NOT_FOUND;
    }

    drv = (struct ezGpioDriver*)ezDriver_GetHw(&inst->drv_handle);
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

    *drv_out = drv;
    return EZ_DRV_STATUS_OK;
}


/* Find the HW driver of the handle, check it and take it for a (sync) operation.
 * On EZ_DRV_STATUS_OK the caller must call ezGpio_End(). */
static EZ_DRV_STATUS ezGpio_Begin(ezGpioHandle_t *inst,
                                  bool require_initialized,
                                  struct ezGpioDriver **drv_out)
{
    struct ezGpioDriver *drv = NULL;
    EZ_DRV_STATUS status = ezGpio_Lookup(inst, require_initialized, &drv);

    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    if(!ezDriver_Acquire(&inst->drv_handle, &drv->base, false))
    {
        EZERROR("Driver is busy");
        return EZ_DRV_STATUS_BUSY;
    }

    *drv_out = drv;
    return EZ_DRV_STATUS_OK;
}


/* Undo the subscription made by ezGpio_Initialize() when it did not complete.
 * A subscription which existed before the call is kept. */
static void ezGpio_RollbackSubscription(ezGpioHandle_t *inst,
                                        struct ezGpioDriver *drv,
                                        bool newly_subscribed)
{
    if(newly_subscribed && !ezDriver_Unsubscribe(&inst->drv_handle, &drv->base))
    {
        EZERROR("Cannot remove the subscription");
    }
}


/* Give the driver back after the HW call. All GPIO operations are sync. */
static void ezGpio_End(ezGpioHandle_t *inst, struct ezGpioDriver *drv)
{
    if(!ezDriver_Release(&inst->drv_handle, &drv->base))
    {
        EZDEBUG("Driver was already released");
    }
}

#endif /* EZ_GPIO == 1 */
/* End of file*/
