/*****************************************************************************
* Filename:         ez_gpio.h
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

/** @file   ez_gpio.h
 *  @author Hai Nguyen
 *  @date   09.05.2025
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 */

#ifndef _EZ_GPIO_H
#define _EZ_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_GPIO == 1)
#include <stdint.h>
#include <stdbool.h>

#include "ez_driver_def.h"
#include "ez_event_notifier.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define A_MACRO     1   /**< a macro*/

/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/** @brief GPIO mode */
typedef enum
{
    EZ_GPIO_MODE_INPUT,    /**< input */
    EZ_GPIO_MODE_OUTPUT,   /**< output */
}EZ_GPIO_MODE;


/** @brief GPIO state */
typedef enum
{
    EZ_GPIO_PIN_LOW,    /**< low */
    EZ_GPIO_PIN_HIGH,   /**< high */
    EZ_GPIO_PIN_ERROR,  /**< error */
}EZ_GPIO_PIN_STATE;


typedef enum
{
    EZ_GPIO_PULL_UP,   /**< pull up */
    EZ_GPIO_PULL_DOWN, /**< pull down */
    EZ_GPIO_NO_PULL,   /**< no pull */
}EZ_GPIO_PULL;

/****************************************************************************/
/* List of API implemented by the HW driver. Users are not supposed to use
 * these API.
 */

/** @brief Read state of a pin
 *  @param[in]  port_index: index of the GPIO port
 *  @param[in]  pin_index: index of the GPIO pin
 *  @return     EZ_GPIO_PIN_LOW or EZ_GPIO_PIN_HIGH
 */
typedef EZ_GPIO_PIN_STATE(*ezHwGpio_ReadPin)(uint16_t port_index, uint16_t pin_index);

/** @brief Write state of a pin
 *  @param[in]  port_index: index of the GPIO port
 *  @param[in]  pin_index: index of the GPIO pin
 *  @param[in]  state: state of the pin
 *  @return     EZ_DRV_STATUS
 */
typedef EZ_DRV_STATUS(*ezHwGpio_WritePin)(uint16_t port_index, uint16_t pin_index, EZ_GPIO_PIN_STATE state);

struct ezHwGpioInterface
{
    ezHwGpio_ReadPin   read_pin;   /**< Read pin */
    ezHwGpio_WritePin  write_pin;  /**< Write pin */
};

struct ezGpioDriver
{
    struct Node                 ll_node;    /* linked list node to link to list of hw driver implmentation */
    struct ezDriverCommon       common;     /* Common data of driver */
    struct ezHwGpioInterface    interface;  /* HW API */
    ezSubject                   gpio_event; /* Subject for event notification */
};

/** @brief Define Uart Driver Instance
 */
typedef struct ezDrvInstance ezGpioDrvInstance_t;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/

EZ_DRV_STATUS ezGpio_SystemRegisterHwDriver(struct ezGpioDriver *hw_gpio_driver);

EZ_DRV_STATUS ezGpio_SystemUnregisterHwDriver(struct ezGpioDriver *hw_gpio_driver);

EZ_DRV_STATUS ezGpio_RegisterInstance(ezGpioDrvInstance_t *inst,
                                      const char *driver_name,
                                      ezObserver *event_subcriber,
                                      EVENT_CALLBACK callback);

EZ_DRV_STATUS ezUart_UnregisterInstance(ezGpioDrvInstance_t *inst);

EZ_GPIO_PIN_STATE ezGpio_ReadPin(ezGpioDrvInstance_t *inst, uint16_t port_index, uint16_t pin_index);

EZ_DRV_STATUS ezGpio_WritePin(ezGpioDrvInstance_t *inst, uint16_t port_index, uint16_t pin_index, EZ_GPIO_PIN_STATE state);


#endif /* EZ_GPIO == 1 */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_GPIO_H */


/* End of file */
