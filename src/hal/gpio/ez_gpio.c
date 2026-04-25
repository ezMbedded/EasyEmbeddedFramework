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
static void ezGpio_OnReceiveEvent(
    void *driver_h,
    uint8_t event_code,
    void *param1,
    void *param2
);


/*****************************************************************************
* Public functions
*****************************************************************************/
EZ_DRV_STATUS ezGpio_SystemRegisterHwDriver(struct ezGpioDriver *hw_gpio_driver)
{
    EZTRACE("ezGpio_SystemRegisterHwDriver()");
    if(hw_gpio_driver == NULL)
    {
        EZERROR("hw_gpio_driver == NULL");
        return STATUS_ERR_ARG;
    }
    else
    {
        hw_gpio_driver->initialized = false;
        hw_gpio_driver->common.callback = ezGpio_OnReceiveEvent;
        EZ_LINKEDLIST_ADD_TAIL(&hw_driver_list, &hw_gpio_driver->ll_node);
        EZDEBUG("Register OK");
        return STATUS_OK;
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
                                      ezDrvCallback callback)
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
            inst->driver = (void*)gpio_drv;
            inst->callback = callback;
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

    if(ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common) == false)
    {
        EZDEBUG("Driver is busy");
        return STATUS_BUSY;
    }

    status = STATUS_ERR_INF_NOT_EXIST;
    if(drv->interface.init_pin != NULL)
    {
        status = drv->interface.init_pin(&drv->common, pin_index, config);
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
    
    inst->driver = NULL;
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
        if(ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common) == false)
        {
            EZTRACE("Driver is busy");
            return EZ_GPIO_PIN_ERROR;
        }

        EZTRACE("Driver = %s is available", drv->common.name);
        if(drv->interface.read_pin)
        {
            state = drv->interface.read_pin(&drv->common, pin_index);
        }
        ezDriver_UnlockDriver(&drv->common);
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
        if(ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common) == false)
        {
            EZTRACE("Driver is busy");
            return status;
        }
        
        EZTRACE("Driver = %s is available", drv->common.name);
        status = STATUS_ERR_INF_NOT_EXIST;
        if(drv->interface.write_pin)
        {
            status = drv->interface.write_pin(&drv->common, pin_index, state);
        }
        ezDriver_UnlockDriver(&drv->common);
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
        if(ezDriver_LockDriver((struct ezDrvInstance*)inst, &drv->common) == false)
        {
            EZTRACE("Driver is busy");
            return status;
        }

        EZTRACE("Driver = %s is available", drv->common.name);
        status = STATUS_ERR_INF_NOT_EXIST;
        if(drv->interface.toggle_pin)
        {
            status = drv->interface.toggle_pin(&drv->common, pin_index);
        }
        ezDriver_UnlockDriver(&drv->common);
    }
    return status;
}


/*****************************************************************************
* Local functions
*****************************************************************************/
static void ezGpio_OnReceiveEvent(
    void *driver_h,
    uint8_t event_code,
    void *param1,
    void *param2
)
{
    struct Node* it_node = NULL;
    struct ezGpioDriver *gpio_drv = NULL;
    
    EZ_LINKEDLIST_FOR_EACH(it_node, &hw_driver_list)
    {
        gpio_drv = EZ_LINKEDLIST_GET_PARENT_OF(it_node, ll_node, struct ezGpioDriver);
        if(gpio_drv->common.curr_inst != NULL && gpio_drv->common.curr_inst->driver == driver_h)
        {
            if(gpio_drv->common.callback)
            {
                gpio_drv->common.curr_inst->callback(event_code, param1, param2);
            }
            ezDriver_UnlockDriver(&gpio_drv->common);
            break;
        }
    }
}

#endif /* EZ_GPIO == 1 */
/* End of file*/
