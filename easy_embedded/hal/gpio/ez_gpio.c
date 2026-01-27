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
/* None */

/*****************************************************************************
* Public functions
*****************************************************************************/
EZ_DRV_STATUS ezGpio_SystemRegisterHwDriver(struct ezGpioDriver *hw_gpio_driver)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;

    EZTRACE("ezGpio_SystemRegisterHwDriver()");
    if(hw_gpio_driver == NULL)
    {
        EZERROR("hw_gpio_driver == NULL");
        return STATUS_ERR_ARG;
    }
    else
    {
        hw_gpio_driver->initialized = false;
        EZ_LINKEDLIST_ADD_TAIL(&hw_driver_list, &hw_gpio_driver->ll_node);
        #if 0
        if(ezEventBus_CreateBus(&hw_gpio_driver->gpio_event) == ezSUCCESS)
        {
            EZDEBUG("Register OK");
            return STATUS_OK;
        }
        else
        #endif
        {
            EZERROR("Cannot create subject for GPIO driver %s", hw_gpio_driver->common.name);
            return STATUS_ERR_GENERIC;
        }
    }

    return STATUS_ERR_GENERIC;
}


EZ_DRV_STATUS ezGpio_SystemUnregisterHwDriver(struct ezGpioDriver *hw_gpio_driver)
{
    EZTRACE("ezGpio_SystemUnregisterHwDriver()");
    if(hw_gpio_driver == NULL)
    {
        EZERROR("hw_gpio_driver == NULL");
        return STATUS_ERR_ARG;
    }

    hw_gpio_driver->initialized = false;
    EZ_LINKEDLIST_UNLINK_NODE(&hw_gpio_driver->ll_node);
    return STATUS_OK;
}

EZ_DRV_STATUS ezGpio_RegisterInstance(ezGpioDrvInstance_t *inst,
                                      const char *driver_name,
                                      EVENT_CALLBACK callback)
{
    struct Node* it_node = NULL;
    struct ezGpioDriver *gpio_drv = NULL;

    EZTRACE("ezGpio_RegisterInstance(name = %s)", driver_name);
    if((inst == NULL) || (driver_name == NULL))
    {
        return STATUS_ERR_ARG;
    }
    
    EZ_LINKEDLIST_FOR_EACH(it_node, &hw_driver_list)
    {
        gpio_drv = EZ_LINKEDLIST_GET_PARENT_OF(it_node, ll_node, struct ezGpioDriver);
        if(strcmp(gpio_drv->common.name, driver_name) == 0)
        {
            EZDEBUG("Found driver!");
            inst->drv_instance.driver = (void*)gpio_drv;
            inst->drv_instance.calback = NULL;

            if(ezEventBus_CreateListener(&inst->event_subcriber, callback) != ezSUCCESS)
            {
                EZERROR("Cannot create observer");
                return STATUS_ERR_GENERIC;
            }

            if(ezEventBus_Listen(&gpio_drv->gpio_event, &inst->event_subcriber) != ezSUCCESS)
            {
                EZERROR("Cannot subscribe to subject");
                return STATUS_ERR_GENERIC;
            }

            return STATUS_OK;
        }
    }

    return STATUS_ERR_DRV_NOT_FOUND;
}


EZ_DRV_STATUS ezGpio_Initialize(ezGpioDrvInstance_t *inst, uint16_t pin_index, ezHwGpioConfig_t *config)
{
    EZTRACE("ezGpio_Initialize()");
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;
    struct ezGpioDriver *drv = (struct ezGpioDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

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
    if(drv->interface.init_pin != NULL)
    {
        status = drv->interface.init_pin(pin_index, config);
    }
    ezDriver_UnlockDriver(&drv->common);

    if(status == STATUS_OK)
    {
        drv->initialized = true;
        EZDEBUG("Driver %s initialized", drv->common.name);
    }

    return status;
}


EZ_DRV_STATUS ezGpio_UnregisterInstance(ezGpioDrvInstance_t *inst)
{
    EZTRACE("ezGpio_UnregisterInstance()");
    if(inst == NULL)
    {
        return STATUS_ERR_ARG;
    }
    
    inst->drv_instance.driver = NULL;
    EZDEBUG("unregister success");
    return STATUS_OK;
}


EZ_GPIO_PIN_STATE ezGpio_ReadPin(ezGpioDrvInstance_t *inst, uint16_t pin_index)
{
    EZ_GPIO_PIN_STATE state = EZ_GPIO_PIN_ERROR;
    struct ezGpioDriver *drv = NULL;

    EZTRACE("ezGpio_ReadPin()");
    drv = (struct ezGpioDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);
    if(drv != NULL)
    {
        EZTRACE("Found driver");
        if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
        {
            EZTRACE("Driver = %s is available", drv->common.name);
            ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
            if(drv->interface.read_pin)
            {
                state = drv->interface.read_pin(pin_index);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    return state;
}


EZ_DRV_STATUS ezGpio_WritePin(ezGpioDrvInstance_t *inst, uint16_t pin_index, EZ_GPIO_PIN_STATE state)
{
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezGpioDriver *drv = (struct ezGpioDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);

    EZTRACE("ezGpio_WritePin()");
    if(drv != NULL)
    {
        EZTRACE("Found driver");
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
        {
            EZTRACE("Driver = %s is available", drv->common.name);
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
            if(drv->interface.write_pin)
            {
                status = drv->interface.write_pin(pin_index, state);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    return status;
}


EZ_DRV_STATUS ezGpio_TogglePin(ezGpioDrvInstance_t *inst, uint16_t pin_index)
{
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezGpioDriver *drv = NULL;

    EZTRACE("ezGpio_WritePin()");
    drv = (struct ezGpioDriver*)ezDriver_GetDriverFromInstance((struct ezDrvInstance*)inst);
    if(drv != NULL)
    {
        EZTRACE("Found driver");
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable((struct ezDrvInstance*)inst, &drv->common) == true)
        {
            EZTRACE("Driver = %s is available", drv->common.name);
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common);
            if(drv->interface.toggle_pin)
            {
                status = drv->interface.toggle_pin(pin_index);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    return status;
}


/*****************************************************************************
* Local functions
*****************************************************************************/


#endif /* EZ_GPIO == 1 */
/* End of file*/
