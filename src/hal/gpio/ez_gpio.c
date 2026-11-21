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

static EZ_DRV_STATUS ezGpio_Lookup(ezGpioHandle_t *inst, struct ezGpioDriver **drv_out);

static EZ_DRV_STATUS ezGpio_Begin(ezGpioHandle_t *inst, struct ezGpioDriver **drv_out);

static struct ezGpioListener *ezGpio_FindListener(struct ezGpioDriver *drv, uint16_t pin);

static struct ezGpioListener *ezGpio_FindFreeListener(struct ezGpioDriver *drv);

static void ezGpio_RemoveListeners(struct ezGpioDriver *drv, const ezDrvHandle_t *handle);

static void ezGpio_End(ezGpioHandle_t *inst, struct ezGpioDriver *drv);


/*****************************************************************************
* Public functions
*****************************************************************************/
EZ_DRV_STATUS ezGpio_SystemRegisterHwDriver(struct ezGpioDriver *hw_gpio_driver)
{
    uint8_t index = 0U;

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

    hw_gpio_driver->open_count = 0U;
    for(index = 0U; index < (uint8_t)EZ_GPIO_MAX_LISTENERS; index++)
    {
        hw_gpio_driver->listeners[index].handle = NULL;
        hw_gpio_driver->listeners[index].pin = 0U;
    }
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

    if(hw_gpio_driver->open_count != 0U)
    {
        EZERROR("%u handle(s) are still open", (unsigned)hw_gpio_driver->open_count);
        return EZ_DRV_STATUS_BUSY;
    }

    EZ_LINKEDLIST_UNLINK_NODE(&hw_gpio_driver->ll_node);
    return EZ_DRV_STATUS_OK;
}


EZ_DRV_STATUS ezGpio_Open(ezGpioHandle_t *inst,
                          const char *driver_name,
                          ezDrvCallback callback,
                          void *ctx)
{
    struct ezGpioDriver *gpio_drv = NULL;

    EZTRACE("ezGpio_Open()");
    if((inst == NULL) || (driver_name == NULL))
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    if(inst->drv_handle.hw != NULL)
    {
        EZERROR("The handle is already open, close it first");
        return EZ_DRV_STATUS_ERR_ARG;
    }

    gpio_drv = ezGpio_FindDriver(driver_name);
    if(gpio_drv == NULL)
    {
        return EZ_DRV_STATUS_ERR_DRV_NOT_FOUND;
    }

    if(gpio_drv->open_count == UINT8_MAX)
    {
        EZERROR("Too many open handles on driver %s", gpio_drv->base.name);
        return EZ_DRV_STATUS_ERR_GENERIC;
    }

    inst->drv_handle.hw = gpio_drv;
    inst->drv_handle.on_event = callback;
    inst->drv_handle.ctx = ctx;
    gpio_drv->open_count++;
    EZDEBUG("Driver %s opened, %u handle(s)", gpio_drv->base.name, (unsigned)gpio_drv->open_count);
    return EZ_DRV_STATUS_OK;
}


EZ_DRV_STATUS ezGpio_Close(ezGpioHandle_t *inst)
{
    struct ezGpioDriver *drv = NULL;

    EZTRACE("ezGpio_Close()");
    if(inst == NULL)
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    drv = (struct ezGpioDriver*)ezDriver_GetHw(&inst->drv_handle);
    if(drv == NULL)
    {
        /* Never opened: nothing to do */
        return EZ_DRV_STATUS_OK;
    }

    if(EZ_DRV_STATE_IS_SET(&drv->base, EZ_DRV_STATE_BUSY) &&
       (drv->base.user == &inst->drv_handle))
    {
        EZERROR("The handle has an operation in flight");
        return EZ_DRV_STATUS_BUSY;
    }

    ezGpio_RemoveListeners(drv, &inst->drv_handle);
    inst->drv_handle.hw = NULL;
    inst->drv_handle.on_event = NULL;
    inst->drv_handle.ctx = NULL;
    if(drv->open_count > 0U)
    {
        drv->open_count--;
    }

    EZDEBUG("Driver %s closed, %u handle(s) left", drv->base.name, (unsigned)drv->open_count);
    return EZ_DRV_STATUS_OK;
}


EZ_DRV_STATUS ezGpio_Initialize(ezGpioHandle_t *inst, uint16_t pin_index, ezHwGpioConfig_t *config)
{
    EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    struct ezGpioDriver *drv = NULL;
    struct ezGpioListener *listener = NULL;
    bool newly_subscribed = false;

    EZTRACE("ezGpio_Initialize()");
    if(config == NULL)
    {
        return EZ_DRV_STATUS_ERR_ARG;
    }

    status = ezGpio_Lookup(inst, &drv);
    if(status != EZ_DRV_STATUS_OK)
    {
        return status;
    }

    /* Subscribe BEFORE the HW call: the HW driver may arm the interrupt inside
     * init_pin, and an event is delivered only to a pin which has a listener.
     * The pin is written first and the handle last, so the ISR never sees a
     * half-written entry. */
    if(config->intr_mode != EZ_INTERRUPT_NONE)
    {
        listener = ezGpio_FindListener(drv, pin_index);
        if((listener != NULL) && (listener->handle != &inst->drv_handle))
        {
            EZERROR("Pin %u is listened by another handle", (unsigned)pin_index);
            return EZ_DRV_STATUS_BUSY;
        }

        if(listener == NULL)
        {
            listener = ezGpio_FindFreeListener(drv);
            if(listener == NULL)
            {
                EZERROR("All the pin subscriptions are used");
                return EZ_DRV_STATUS_ERR_GENERIC;
            }
            listener->pin = pin_index;
            listener->handle = &inst->drv_handle;
            newly_subscribed = true;
        }
    }

    if(!ezDriver_Acquire(&inst->drv_handle, &drv->base, false))
    {
        EZERROR("Driver is busy");
        if(newly_subscribed)
        {
            listener->handle = NULL;
        }
        return EZ_DRV_STATUS_BUSY;
    }

    status = EZ_DRV_STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.init_pin != NULL)
    {
        status = drv->interface.init_pin(drv->interface.driver_h, pin_index, config);
    }
    ezGpio_End(inst, drv);

    if(status != EZ_DRV_STATUS_OK)
    {
        /* A subscription which existed before the call is kept */
        if(newly_subscribed)
        {
            listener->handle = NULL;
        }
    }
    else if(config->intr_mode == EZ_INTERRUPT_NONE)
    {
        /* The handle does not want interrupts on this pin anymore */
        listener = ezGpio_FindListener(drv, pin_index);
        if((listener != NULL) && (listener->handle == &inst->drv_handle))
        {
            listener->handle = NULL;
        }
    }
    else
    {
        EZDEBUG("Pin %u initialized with an interrupt", (unsigned)pin_index);
    }

    return status;
}


EZ_GPIO_PIN_STATE ezGpio_ReadPin(ezGpioHandle_t *inst, uint16_t pin_index)
{
    EZ_GPIO_PIN_STATE state = EZ_GPIO_PIN_ERROR;
    struct ezGpioDriver *drv = NULL;

    EZTRACE("ezGpio_ReadPin()");
    if(ezGpio_Begin(inst, &drv) != EZ_DRV_STATUS_OK)
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
    status = ezGpio_Begin(inst, &drv);
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
    status = ezGpio_Begin(inst, &drv);
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
static EZ_DRV_STATUS ezGpio_Lookup(ezGpioHandle_t *inst, struct ezGpioDriver **drv_out)
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

    *drv_out = drv;
    return EZ_DRV_STATUS_OK;
}


/* Find the HW driver of the handle, check it and take it for a (sync) operation.
 * On EZ_DRV_STATUS_OK the caller must call ezGpio_End(). */
static EZ_DRV_STATUS ezGpio_Begin(ezGpioHandle_t *inst, struct ezGpioDriver **drv_out)
{
    struct ezGpioDriver *drv = NULL;
    EZ_DRV_STATUS status = ezGpio_Lookup(inst, &drv);

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


/* Find the subscription of a pin. Return NULL if nobody listens to it. */
static struct ezGpioListener *ezGpio_FindListener(struct ezGpioDriver *drv, uint16_t pin)
{
    uint8_t index = 0U;

    for(index = 0U; index < (uint8_t)EZ_GPIO_MAX_LISTENERS; index++)
    {
        if((drv->listeners[index].handle != NULL) && (drv->listeners[index].pin == pin))
        {
            return &drv->listeners[index];
        }
    }
    return NULL;
}


/* Find an unused subscription entry. Return NULL if the table is full. */
static struct ezGpioListener *ezGpio_FindFreeListener(struct ezGpioDriver *drv)
{
    uint8_t index = 0U;

    for(index = 0U; index < (uint8_t)EZ_GPIO_MAX_LISTENERS; index++)
    {
        if(drv->listeners[index].handle == NULL)
        {
            return &drv->listeners[index];
        }
    }
    return NULL;
}


/* End all the subscriptions of a handle. The subscriptions of other handles are kept. */
static void ezGpio_RemoveListeners(struct ezGpioDriver *drv, const ezDrvHandle_t *handle)
{
    uint8_t index = 0U;

    for(index = 0U; index < (uint8_t)EZ_GPIO_MAX_LISTENERS; index++)
    {
        if(drv->listeners[index].handle == handle)
        {
            drv->listeners[index].handle = NULL;
        }
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
