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
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_gpio.h"

#if (EZ_GPIO == 1)

#define DEBUG_LVL   LVL_TRACE   /**< logging level */
#define MOD_NAME    "ez_gpio"       /**< module name */
#include "ez_logging.h"

#include <string.h>

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define A_MACRO     1   /**< a macro*/

/*****************************************************************************
* Component Typedefs
*****************************************************************************/

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
static struct Node hw_driver_list = EZ_LINKEDLIST_INIT_NODE(hw_driver_list);
/**< List of HW driver implementation  */

static struct Node instance_list = EZ_LINKEDLIST_INIT_NODE(instance_list);
/**< Keep tracks of instance register to this driver */

/*****************************************************************************
* Function Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Public functions
*****************************************************************************/


/*****************************************************************************
* Local functions
*****************************************************************************/
EZ_DRV_STATUS ezGpio_SystemRegisterHwDriver(struct ezGpioDriver *hw_gpio_driver)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;

    EZTRACE("ezGpio_SystemRegisterHwDriver()");
    if(hw_gpio_driver == NULL)
    {
        status = STATUS_ERR_ARG;
        EZERROR("hw_gpio_driver == NULL");
    }
    else
    {
        EZ_LINKEDLIST_ADD_TAIL(&hw_driver_list, &hw_gpio_driver->ll_node);
        status = ezEventNotifier_CreateSubject(&hw_gpio_driver->gpio_event);;
    }

    return status;
}


EZ_DRV_STATUS ezGpio_SystemUnregisterHwDriver(struct ezGpioDriver *hw_gpio_driver)
{
        EZ_DRV_STATUS status = STATUS_ERR_GENERIC;

    EZTRACE("ezUart_SystemUnregisterHwDriver()");
    if(hw_gpio_driver == NULL)
    {
        status = STATUS_ERR_ARG;
        EZERROR("hw_gpio_driver == NULL");
    }
    else
    {
        EZ_LINKEDLIST_UNLINK_NODE(&hw_gpio_driver->ll_node);
        status = STATUS_OK;
    }

    return status;
}

EZ_DRV_STATUS ezGpio_RegisterInstance(ezGpioDrvInstance_t *inst,
                                      const char *driver_name,
                                      ezObserver *event_subcriber,
                                      EVENT_CALLBACK callback)
{
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct Node* it_node = NULL;
    struct ezGpioDriver *gpio_drv = NULL;

    EZTRACE("ezGpio_RegisterInstance(name = %s)", driver_name);
    if((inst == NULL) || (driver_name == NULL))
    {
        status = STATUS_ERR_ARG;
    }
    else
    {
        EZ_LINKEDLIST_FOR_EACH(it_node, &hw_driver_list)
        {
            gpio_drv = EZ_LINKEDLIST_GET_PARENT_OF(it_node, ll_node, struct ezGpioDriver);
            if(strcmp(gpio_drv->common.name, driver_name) == 0)
            {
                EZDEBUG("Found driver!");
                inst->driver = (void*)gpio_drv;
                inst->calback = NULL;
                if(ezEventNotifier_CreateObserver(event_subcriber, callback) != ezSUCCESS)
                {
                    EZERROR("Cannot create observer");
                    status = STATUS_ERR_GENERIC;
                }
                else
                {
                    ezEventNotifier_SubscribeToSubject(&gpio_drv->gpio_event, event_subcriber);
                    status = STATUS_OK;
                }
                break;
            }
        }
    }

    return status;
}


EZ_DRV_STATUS ezUart_UnregisterInstance(ezGpioDrvInstance_t *inst)
{
    EZ_DRV_STATUS status = STATUS_ERR_GENERIC;

    EZTRACE("ezGpio_UnregisterInstance()");
    if(inst == NULL)
    {
        status = STATUS_ERR_ARG;
    }
    else
    {
        inst->driver = NULL;
        EZDEBUG("unregister success");
        status = STATUS_OK;
    }

    return status;
}


EZ_GPIO_PIN_STATE ezGpio_ReadPin(ezGpioDrvInstance_t *inst, uint16_t port_index, uint16_t pin_index)
{
    EZ_GPIO_PIN_STATE state = EZ_GPIO_PIN_ERROR;
    struct ezGpioDriver *drv = NULL;

    EZTRACE("ezGpio_ReadPin()");
    drv = (struct ezGpioDriver*)ezDriver_GetDriverFromInstance(inst);
    if(drv != NULL)
    {
        EZTRACE("Found driver");
        if(ezDriver_IsDriverAvailable(inst, &drv->common) == true)
        {
            EZTRACE("Driver = %s is available", drv->common.name);
            ezDriver_LockDriver(inst, &drv->common);
            if(drv->interface.read_pin)
            {
                state = drv->interface.read_pin(port_index, pin_index);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    return state;
}

EZ_DRV_STATUS ezGpio_WritePin(ezGpioDrvInstance_t *inst, uint16_t port_index, uint16_t pin_index, EZ_GPIO_PIN_STATE state)
{
    EZ_DRV_STATUS status = STATUS_ERR_DRV_NOT_FOUND;
    struct ezGpioDriver *drv = NULL;

    EZTRACE("ezUart_AsyncTransmit()");
    drv = (struct ezGpioDriver*)ezDriver_GetDriverFromInstance(inst);
    if(drv != NULL)
    {
        EZTRACE("Found driver");
        status = STATUS_BUSY;
        if(ezDriver_IsDriverAvailable(inst, &drv->common) == true)
        {
            EZTRACE("Driver = %s is available", drv->common.name);
            status = STATUS_ERR_INF_NOT_EXIST;
            ezDriver_LockDriver(inst, &drv->common);
            if(drv->interface.write_pin)
            {
                status = drv->interface.write_pin(port_index, pin_index, state);
            }
            ezDriver_UnlockDriver(&drv->common);
        }
    }
    return status;
}


#endif /* EZ_GPIO == 1 */
/* End of file*/
