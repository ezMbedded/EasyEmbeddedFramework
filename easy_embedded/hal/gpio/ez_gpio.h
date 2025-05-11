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
 *  @brief  Public API of the GPIO component
 *
 *  @details -
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


/** @brief GPIO pull mode */
typedef enum
{
    EZ_GPIO_PULL_UP,   /**< pull up */
    EZ_GPIO_PULL_DOWN, /**< pull down */
    EZ_GPIO_NO_PULL,   /**< no pull */
}EZ_GPIO_PULL;

/** @brief GPIO interrupt mode */
typedef enum
{
    EZ_INTTERTUP_RISING,    /**< rising edge */
    EZ_INTTERTUP_FALLING,   /**< falling edge */
    EZ_INTTERTUP_BOTH,      /**< both edges */
    EZ_INTTERTUP_NONE,      /**< no interrupt */
}EZ_INTTERTUP_MODE;

/** @brief Configuration to intialize the pin */
typedef struct ezHwGpioConfig
{
    EZ_GPIO_MODE mode;              /**< mode of the GPIO pin */
    EZ_GPIO_PULL pull;              /**< pull mode of the GPIO pin */
    EZ_INTTERTUP_MODE intr_mode;    /**< interrupt mode of the GPIO pin */
}ezHwGpioConfig_t;

/****************************************************************************/
/* List of API implemented by the HW driver. Users are not supposed to use
 * these API.
 */

 /** @brief Initialize a pin 
  * @param[in]  pin_index: index of the GPIO pin
  * @param[in]  config: configuration of the GPIO pin
  * @return     EZ_DRV_STATUS
  */
typedef EZ_DRV_STATUS (*ezHwGpio_Initialize)(uint16_t pin_index, ezHwGpioConfig_t *config);

/** @brief Read state of a pin
 *  @param[in]  pin_index: index of the GPIO pin
 *  @return     EZ_GPIO_PIN_LOW or EZ_GPIO_PIN_HIGH
 */
typedef EZ_GPIO_PIN_STATE(*ezHwGpio_ReadPin)(uint16_t pin_index);

/** @brief Write state of a pin
 *  @param[in]  pin_index: index of the GPIO pin
 *  @param[in]  state: state of the pin
 *  @return     EZ_DRV_STATUS
 */
typedef EZ_DRV_STATUS(*ezHwGpio_WritePin)(uint16_t pin_index, EZ_GPIO_PIN_STATE state);

/** @brief Toggle state of a pin
 *  @param[in]  pin_index: index of the GPIO pin
 *  @return     EZ_DRV_STATUS
 */
typedef EZ_DRV_STATUS(*ezHwGpio_TogglePin)(uint16_t pin_index);


/** @brief GPIO driver interface
 *  @details This structure contains the list of API implemented by the HW driver.
 *           Users are not supposed to use these API.
 */
struct ezHwGpioInterface
{
    ezHwGpio_Initialize init_pin;   /**< Initialize pin */
    ezHwGpio_ReadPin    read_pin;   /**< Read pin */
    ezHwGpio_WritePin   write_pin;  /**< Write pin */
    ezHwGpio_TogglePin  toggle_pin; /**< Toggle pin */
};

struct ezGpioDriver
{
    struct Node                 ll_node;    /* linked list node to link to list of hw driver implmentation */
    struct ezDriverCommon       common;     /* Common data of driver */
    struct ezHwGpioInterface    interface;  /* HW API */
    ezSubject                   gpio_event; /* Subject for event notification */
    bool                        initialized; /* Flag to check if the driver is initialized */
};

/** @brief Define Uart Driver Instance
 */
typedef struct{
    struct ezDrvInstance    drv_instance;    /**< Driver instance */
    ezObserver              event_subcriber; /**< Pointer to the event subscriber */
} ezGpioDrvInstance_t;


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
                                      EVENT_CALLBACK callback);

EZ_DRV_STATUS ezGpio_UnregisterInstance(ezGpioDrvInstance_t *inst);

EZ_DRV_STATUS ezGpio_Initialize(ezGpioDrvInstance_t *inst, uint16_t pin_index, ezHwGpioConfig_t *config);

EZ_GPIO_PIN_STATE ezGpio_ReadPin(ezGpioDrvInstance_t *inst, uint16_t pin_index);

EZ_DRV_STATUS ezGpio_WritePin(ezGpioDrvInstance_t *inst, uint16_t pin_index, EZ_GPIO_PIN_STATE state);

EZ_DRV_STATUS ezGpio_TogglePin(ezGpioDrvInstance_t *inst, uint16_t pin_index);

#endif /* EZ_GPIO == 1 */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_GPIO_H */


/* End of file */
