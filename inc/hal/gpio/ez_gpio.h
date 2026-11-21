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
 *  @brief  Hardware Abstraction Layer for GPIO pins
 *
 *  @details A HW driver is one GPIO port. A handle is one user of it, for example
 *           one module. The user opens a handle with ezGpio_Open(), which binds it to
 *           the HW driver by name, and closes it with ezGpio_Close(). All the
 *           operations of a handle go through the HW driver registered with
 *           ezGpio_SystemRegisterHwDriver(). Several handles can be open on the
 *           same HW driver: each module configures and uses its own pins.
 *
 *           Concurrency: this component does not provide mutual exclusion.
 *           The caller must serialize the calls on the same HW driver (mutex
 *           or semaphore of the upper layer). A pin operation made while another
 *           operation is in flight is refused with EZ_DRV_STATUS_BUSY
 *           (ezGpio_ReadPin() returns EZ_GPIO_PIN_ERROR).
 *
 *           All pin operations are synchronous: the result is returned through
 *           the return value of the function. There is no async operation.
 *           The HW driver must NOT call ezDriver_NotifyEvent() with an
 *           EZ_DRV_EVENT_OP_* kind. The only event it reports is a pin
 *           interrupt, see below.
 *
 *           Pin interrupts: the subscription is per pin. When ezGpio_Initialize() is
 *           called with an intr_mode other than EZ_INTERRUPT_NONE, the handle
 *           listens to the interrupt of that pin. It starts listening before the HW
 *           init function is called, so the HW driver may arm the interrupt inside
 *           it. The HW driver reports an interrupt from its ISR with
 *           ezGpio_NotifyPinEvent(). The callback given to ezGpio_Open() is then
 *           called in the context of the HW driver, with the pin index and the level
 *           of the pin. Handles which listen to different pins, and handles which
 *           listen to nothing, work side by side. A pin has one listener. The
 *           subscriptions of a handle end with ezGpio_Close().
 *
 *           No function of this component is ISR-safe, except
 *           ezGpio_NotifyPinEvent(), which is made for the HW driver.
 *
 *           Argument checking: this component checks the handle, the config
 *           pointer of ezGpio_Initialize() and the driver name. The pin index
 *           and the pin state are passed as they are to the HW driver, which
 *           is responsible for validating them.
 *
 *           Every pin operation requires an open handle. Whether a pin is
 *           configured is decided by the HW driver.
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
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "ez_linked_list.h"
#include "ez_driver_def.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/** @brief Event codes reported to the handle callback (event_code argument)
 *
 *  @details Reported by the HW driver with ezGpio_NotifyPinEvent(). Delivered only
 *           to the handle which listens to the pin, see ezGpio_Initialize().
 */
typedef enum
{
    EZ_GPIO_EVENT_PIN_INTERRUPT,    /**< A pin interrupt fired. param1 is the pin index (uint16_t, passed by value as a pointer-sized integer), param2 is the level of the pin (EZ_GPIO_PIN_STATE, by value) */
    EZ_GPIO_NUM_EVENTS,             /**< Number of event codes. Not an event */
}EZ_GPIO_EVENT;


/** @brief GPIO handle. Held by the user, bound to a HW driver.
 *
 *  @details Must be zero-initialized (static storage, or memset) before the
 *           first ezGpio_Open(). Open it with ezGpio_Open() before use.
 *           The HW driver keeps a pointer to the handle while an operation is in
 *           flight and while the handle listens to pin events, so the handle
 *           must stay valid until ezGpio_Close() returns EZ_DRV_STATUS_OK.
 */
typedef struct
{
    struct ezDrvHandle  drv_handle;     /**< Common handle, bound to a HW driver. Filled by ezGpio_Open() */
} ezGpioHandle_t;


/** @brief GPIO mode
 *
 *  @details Member of ezHwGpioConfig_t. The GPIO component does not check or
 *           use it. The HW driver decides which modes it supports.
 */
typedef enum
{
    EZ_GPIO_MODE_INPUT,    /**< input */
    EZ_GPIO_MODE_OUTPUT,   /**< output */
}EZ_GPIO_MODE;


/** @brief GPIO state
 *
 *  @details Returned by ezGpio_ReadPin() and passed to ezGpio_WritePin(). The
 *           GPIO component does not check the state given to ezGpio_WritePin().
 *           The HW driver decides which values it accepts.
 *           EZ_GPIO_PIN_ERROR is returned by ezGpio_ReadPin() on failure, see
 *           its description.
 */
typedef enum
{
    EZ_GPIO_PIN_LOW,    /**< low */
    EZ_GPIO_PIN_HIGH,   /**< high */
    EZ_GPIO_PIN_ERROR,  /**< error */
}EZ_GPIO_PIN_STATE;


/** @brief GPIO pull mode
 *
 *  @details Member of ezHwGpioConfig_t. The GPIO component does not check or
 *           use it. The HW driver decides which pull modes it supports.
 */
typedef enum
{
    EZ_GPIO_PULL_UP,   /**< pull up */
    EZ_GPIO_PULL_DOWN, /**< pull down */
    EZ_GPIO_NO_PULL,   /**< no pull */
}EZ_GPIO_PULL;

/** @brief GPIO interrupt mode
 *
 *  @details Member of ezHwGpioConfig_t. The GPIO component compares it only
 *           with EZ_INTERRUPT_NONE, see ezGpio_Initialize(). The HW driver
 *           applies the edge (rising, falling, both).
 */
typedef enum
{
    EZ_INTERRUPT_RISING,    /**< rising edge */
    EZ_INTERRUPT_FALLING,   /**< falling edge */
    EZ_INTERRUPT_BOTH,      /**< both edges */
    EZ_INTERRUPT_NONE,      /**< no interrupt */
}EZ_INTERRUPT_MODE;

/** @brief Configuration to initialize a pin
 *
 *  @details Passed by pointer to the HW initialize function, unchanged. The
 *           GPIO component does not copy the structure and does not check the
 *           values. It reads intr_mode after the HW call returns, so the HW
 *           driver must not change it.
 *
 *           initial_state is the level of an output pin before the output is
 *           enabled. The HW driver sets the level first and enables the output
 *           after, so the pin never shows a wrong level, for example a chip select
 *           which must stay high or an active-low enable which must stay off.
 *           It is ignored for an input. The last member is zero (EZ_GPIO_PIN_LOW)
 *           when an initializer leaves it out.
 */
typedef struct ezHwGpioConfig
{
    EZ_GPIO_MODE mode;              /**< mode of the GPIO pin */
    EZ_GPIO_PULL pull;              /**< pull mode of the GPIO pin */
    EZ_INTERRUPT_MODE intr_mode;    /**< interrupt mode of the GPIO pin */
    EZ_GPIO_PIN_STATE initial_state; /**< level of an output pin before the output is enabled: EZ_GPIO_PIN_LOW or EZ_GPIO_PIN_HIGH. Ignored for an input. The HW driver validates it */
}ezHwGpioConfig_t;

/****************************************************************************/
/* List of API implemented by the HW driver. Users are not supposed to use
 * these API.
 */

/** @brief Initialize a pin. Called by ezGpio_Initialize().
 *
 *  @details Synchronous: the returned status is passed back to the caller.
 *           The GPIO component adds no wait. Whether the function blocks is up
 *           to the HW driver. driver_h is ezHwGpioInterface.driver_h, pin_index
 *           and config are passed unchanged. config is the pointer given to
 *           ezGpio_Initialize(), not a copy.
 *           pin_index is not checked by the component.
 */
typedef EZ_DRV_STATUS (*ezHwGpio_Initialize)(void *driver_h, uint16_t pin_index, ezHwGpioConfig_t *config);

/** @brief Read the state of a pin. Called by ezGpio_ReadPin().
 *
 *  @details Synchronous. Returns EZ_GPIO_PIN_LOW or EZ_GPIO_PIN_HIGH. The
 *           value is returned to the caller unchanged. driver_h is
 *           ezHwGpioInterface.driver_h, pin_index is passed unchanged and is not
 *           checked by the component.
 */
typedef EZ_GPIO_PIN_STATE(*ezHwGpio_ReadPin)(void *driver_h, uint16_t pin_index);

/** @brief Write the state of a pin. Called by ezGpio_WritePin().
 *
 *  @details Synchronous: the returned status is passed back to the caller
 *           unchanged. driver_h is ezHwGpioInterface.driver_h. pin_index and
 *           state are passed unchanged and are not checked by the component.
 */
typedef EZ_DRV_STATUS(*ezHwGpio_WritePin)(void *driver_h, uint16_t pin_index, EZ_GPIO_PIN_STATE state);

/** @brief Toggle the state of a pin. Called by ezGpio_TogglePin().
 *
 *  @details Synchronous: the returned status is passed back to the caller
 *           unchanged. driver_h is ezHwGpioInterface.driver_h. pin_index is
 *           passed unchanged and is not checked by the component.
 */
typedef EZ_DRV_STATUS(*ezHwGpio_TogglePin)(void *driver_h, uint16_t pin_index);


/** @brief HW API provided by the HW driver. Unused functions can be NULL.
 *
 *  @details A NULL function makes the matching ezGpio_xxx() fail:
 *           ezGpio_Initialize(), ezGpio_WritePin() and ezGpio_TogglePin() return
 *           EZ_DRV_STATUS_ERR_INF_NOT_EXIST, and ezGpio_ReadPin() returns
 *           EZ_GPIO_PIN_ERROR. The HW functions are called with driver_h as the
 *           first argument.
 *
 *           The pin indexes and pin states are not checked by the GPIO
 *           component. Each HW function must validate them and return an error
 *           status when they are not valid. The component forwards that status
 *           unchanged (ezGpio_ReadPin() forwards its state unchanged too).
 *
 *           All the HW functions are synchronous. The HW driver must not call
 *           ezDriver_NotifyEvent() with an EZ_DRV_EVENT_OP_* kind from them: such
 *           events are dropped. The only event of the HW driver is the pin
 *           interrupt, reported from its ISR with ezGpio_NotifyPinEvent(). See the
 *           file description.
 */
struct ezHwGpioInterface
{
    void                *driver_h;  /**< Opaque pointer of the HW driver, passed to every function below */
    ezHwGpio_Initialize init_pin;   /**< Initialize pin (synchronous) */
    ezHwGpio_ReadPin    read_pin;   /**< Read pin (synchronous) */
    ezHwGpio_WritePin   write_pin;  /**< Write pin (synchronous) */
    ezHwGpio_TogglePin  toggle_pin; /**< Toggle pin (synchronous) */
};


/** @brief Maximum number of pins of one HW driver which handles can listen to at the same time
 *
 *  @details Size of the subscription table of struct ezGpioDriver. Can be
 *           overridden at build time.
 */
#ifndef EZ_GPIO_MAX_LISTENERS
#define EZ_GPIO_MAX_LISTENERS       (8U)
#endif

/** @brief One pin interrupt subscription: the pin and the handle which listens to it
 *
 *  @details Not for the user. Managed by ezGpio_Initialize(), ezGpio_Close() and
 *           read by ezGpio_NotifyPinEvent(). The entry is used while handle is not NULL.
 */
struct ezGpioListener
{
    ezDrvHandle_t * volatile    handle;     /**< Handle which listens to the pin, NULL if the entry is free */
    uint16_t                    pin;        /**< Pin index */
};

/** @brief GPIO HW driver. Provided by the HW driver implementation.
 *
 *  @details base.name and interface must be set before
 *           ezGpio_SystemRegisterHwDriver(). The driver is linked into the list
 *           of registered drivers, so its memory (static storage, no malloc)
 *           must stay valid while it is registered. ezGpio_SystemRegisterHwDriver()
 *           resets base.state, base.user, open_count and listeners.
 */
struct ezGpioDriver
{
    struct Node                 ll_node;    /**< Node to link to the list of HW drivers */
    struct ezDrvBase            base;       /**< Common data of driver: name, state and the handle in use */
    struct ezHwGpioInterface    interface;  /**< HW API, see ezHwGpioInterface */
    uint8_t                     open_count; /**< Number of handles open on this driver. Reset to 0 on register */
    struct ezGpioListener       listeners[EZ_GPIO_MAX_LISTENERS];   /**< Pin interrupt subscriptions, see ezGpio_Initialize() */
};

/*****************************************************************************
* Function: ezGpio_NotifyPinEvent
*//**
* @brief Report a pin interrupt of the HW driver to the handle which listens to the pin.
*
* @details Called by the HW driver implementation from its ISR, instead of
*          ezDriver_NotifyEvent(). It looks up the subscription of the pin and
*          calls the callback of that handle with EZ_GPIO_EVENT_PIN_INTERRUPT. A
*          pin which nobody listens to, and a handle without callback, are
*          ignored. The subscription table is only read, so this function never
*          blocks. The table is written by the task side in an order which keeps
*          it consistent for the ISR on a single core.
*
* @param[in]    drv: The HW driver (the object registered with ezGpio_SystemRegisterHwDriver()).
* @param[in]    pin: Index of the pin which fired.
* @param[in]    level: Level of the pin seen by the HW driver, EZ_GPIO_PIN_LOW or
*                      EZ_GPIO_PIN_HIGH. It is passed to the callback as param2.
*
* @warning The callback runs inside this call, in the same context. If the call
*          is made from an ISR, the callback must be ISR-safe. It must not block.
* @see ezGpio_Initialize, EZ_GPIO_EVENT
*
* \b Example
* @code
* // In the ISR of the GPIO port, when a pin interrupt fires
* ezGpio_NotifyPinEvent(&gpio0_drv, 7U, EZ_GPIO_PIN_LOW);
* @endcode
*
* @isr_safe{yes}
* @blocking{no}
*
*****************************************************************************/
static inline void ezGpio_NotifyPinEvent(struct ezGpioDriver *drv,
                                         uint16_t pin,
                                         EZ_GPIO_PIN_STATE level)
{
    uint8_t index = 0U;
    ezDrvHandle_t *handle = NULL;

    if(drv == NULL)
    {
        return;
    }

    for(index = 0U; index < (uint8_t)EZ_GPIO_MAX_LISTENERS; index++)
    {
        handle = drv->listeners[index].handle;
        if((handle != NULL) && (drv->listeners[index].pin == pin))
        {
            if(handle->on_event != NULL)
            {
                handle->on_event(handle->ctx,
                                 (uint8_t)EZ_GPIO_EVENT_PIN_INTERRUPT,
                                 (void *)(uintptr_t)pin,
                                 (void *)(uintptr_t)level);
            }
            return;     /* A pin has one listener */
        }
    }
}

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function: ezGpio_SystemRegisterHwDriver
*//**
* @brief Register a HW driver to the GPIO component.
*
* @details Called by the HW driver implementation, not by the application. The
*          driver is reset to the idle, closed state: open_count is 0, the
*          subscription table is empty, base.state is 0 and base.user is NULL. It
*          is then added to the list of HW drivers. base.name is compared with the names of the
*          drivers already registered.
*
* @param[in]    hw_gpio_driver: HW driver. base.name must be set and unique.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if hw_gpio_driver or its name is NULL, or the
*               name is already registered.
*
* @post base.state is 0, base.user is NULL, open_count is 0 and no pin is listened to
* @see ezGpio_SystemUnregisterHwDriver, ezHwGpioInterface
* @warning The driver list is changed without a lock. Do not call it while
*          another GPIO call can run.
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* // HW driver side, at startup. The driver object is static (no malloc).
* static struct ezGpioDriver gpio0_drv;
*
* EZ_DRV_STATUS RegisterGpio0(void)
* {
*     EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_GENERIC;
*
*     gpio0_drv.base.name = "gpio0";                    // Must be unique
*     gpio0_drv.interface.driver_h = &gpio0_regs;       // Given to every HW function
*     gpio0_drv.interface.init_pin = Gpio0_InitPin;
*     gpio0_drv.interface.read_pin = Gpio0_ReadPin;
*     gpio0_drv.interface.write_pin = Gpio0_WritePin;
*     gpio0_drv.interface.toggle_pin = NULL;            // Unused: ezGpio_TogglePin() returns EZ_DRV_STATUS_ERR_INF_NOT_EXIST
*
*     status = ezGpio_SystemRegisterHwDriver(&gpio0_drv);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;
*     }
*     return EZ_DRV_STATUS_OK;
* }
*
* // In the ISR of the GPIO port, when a pin interrupt fires
* ezGpio_NotifyPinEvent(&gpio0_drv, 7U, EZ_GPIO_PIN_LOW);
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezGpio_SystemRegisterHwDriver(struct ezGpioDriver *hw_gpio_driver);


/*****************************************************************************
* Function: ezGpio_SystemUnregisterHwDriver
*//**
* @brief Unregister a HW driver from the GPIO component.
*
* @details Called by the HW driver implementation. The driver is removed from
*          the list of HW drivers. base.state and base.user are not changed. The
*          driver is refused while a handle is open on it, so no handle keeps a
*          pointer to a driver which is gone.
*
* @param[in]    hw_gpio_driver: HW driver registered earlier.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if hw_gpio_driver is NULL,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver, or a
*               handle is still open on it (open_count is not 0).
*
* @pre hw_gpio_driver was registered with ezGpio_SystemRegisterHwDriver() and
*      is not unregistered yet. Not checked by the code: the unlink dereferences
*      the list pointers of the driver, which are NULL for a driver never
*      registered.
* @see ezGpio_SystemRegisterHwDriver, ezGpio_Close
* @warning The driver list is changed without a lock. Do not call it while
*          another GPIO call can run.
*
* @isr_safe{no}
* @blocking{no}
*
*****************************************************************************/
EZ_DRV_STATUS ezGpio_SystemUnregisterHwDriver(struct ezGpioDriver *hw_gpio_driver);

/*****************************************************************************
* Function: ezGpio_Open
*//**
* @brief Bind a handle to a registered HW driver.
*
* @details Stores the HW driver, the callback and the context in the handle and
*          counts the handle as open. The HW driver is not called and the driver
*          is not marked busy. A handle which is already open is refused: call
*          ezGpio_Close() first. The handle and the driver name are checked
*          before the driver is looked up.
*
*          Several handles can be open on the same HW driver, for example one per
*          module. Each handle configures its own pins with ezGpio_Initialize()
*          and listens to the interrupts of its own pins only.
*
* @param[in]    inst: Handle owned by the caller. Zero-initialized and not open.
*                     Must stay valid while open.
* @param[in]    driver_name: Name of the HW driver (base.name).
* @param[in]    callback: Called for the pin events of the pins which this handle
*                         listens to, see ezGpio_Initialize(). Can be NULL, then
*                         the events are not reported. Runs in the context of the
*                         HW driver, usually an ISR.
* @param[in]    ctx: User context, passed to callback as first argument. Can be NULL.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if inst or driver_name is NULL, or the
*               handle is already open,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if no HW driver has the name,
*               EZ_DRV_STATUS_ERR_GENERIC if the maximum number of open handles
*               (255) is reached.
*
* @post On EZ_DRV_STATUS_OK the handle is open.
* @see ezGpio_Close, ezDrvCallback, ezGpio_Initialize
* @warning The callback runs inside ezGpio_NotifyPinEvent(), in the context of
*          the HW driver, usually an ISR. It must not block.
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* static ezGpioHandle_t gpio0_handle;  // Static: the handle must stay valid while open
* static AppGpioCtx_t app_ctx;         // Passed to the callback, see ezGpio_Initialize()
*
* EZ_DRV_STATUS OpenGpio0(void)
* {
*     EZ_DRV_STATUS status = ezGpio_Open(&gpio0_handle, "gpio0", OnGpioEvent, &app_ctx);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;
*     }
*     return EZ_DRV_STATUS_OK;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezGpio_Open(ezGpioHandle_t *inst,
                          const char *driver_name,
                          ezDrvCallback callback,
                          void *ctx);

/*****************************************************************************
* Function: ezGpio_Close
*//**
* @brief Unbind a handle from its HW driver.
*
* @details Ends the pin event subscriptions of all the pins of the handle, clears
*          hw, on_event and ctx of the handle and counts it as closed. The
*          subscriptions of other handles are not touched. Refused only when this
*          handle has an operation in flight. An operation of another handle does
*          not block it. Since all pin operations are synchronous, this happens
*          when the call is made from a HW function of the same handle while the
*          operation runs. A handle which was never opened is accepted and stays
*          empty.
*
*          The HW pins are not changed: an interrupt which the HW driver armed
*          stays armed, but it is no longer reported to anyone.
*
* @param[in]    inst: Handle to close. Opened with ezGpio_Open(), or never opened.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if inst is NULL,
*               EZ_DRV_STATUS_BUSY if this handle has an operation in flight.
*
* @post On EZ_DRV_STATUS_OK the handle is not open and listens to no pin.
* @see ezGpio_Open, ezGpio_Initialize
*
* @isr_safe{no}
* @blocking{no}
*
*****************************************************************************/
EZ_DRV_STATUS ezGpio_Close(ezGpioHandle_t *inst);

/*****************************************************************************
* Function: ezGpio_Initialize
*//**
* @brief Initialize a pin of the HW driver.
*
* @details Checks config first, then finds the HW driver of the handle and takes
*          it for the call. The init_pin function of the HW driver is called with
*          driver_h, pin_index and config. The driver is released after the call
*          and the HW status is returned. config is passed as is, the GPIO
*          component does not copy or check it.
*
*          With config->intr_mode other than EZ_INTERRUPT_NONE, the handle
*          subscribes to the interrupt of this pin BEFORE the HW function is
*          called. The HW driver may therefore arm the interrupt inside its
*          init_pin function, and an interrupt which fires there is delivered to
*          the callback during this call, in the context of the HW driver, while
*          the pin may be only partly configured. The callback cannot call another
*          GPIO function for that time: the driver is held and they return
*          EZ_DRV_STATUS_BUSY. If the HW call fails, a subscription made by this
*          call is removed again, and a subscription which existed before is kept.
*          On success the subscription of the pin is kept until the pin is
*          initialized again with EZ_INTERRUPT_NONE by the same handle, or the
*          handle is closed. Calling it again for a pin which the handle already
*          listens to calls the HW function again and does not use a second
*          subscription.
*
*          The subscriptions are per pin: handles which listen to different pins
*          do not disturb each other, and a handle without interrupts can use its
*          pins while another handle listens. A pin is listened to by one handle
*          at a time. At most EZ_GPIO_MAX_LISTENERS pins of the driver can be
*          listened to at the same time.
*
*          Whether the call blocks depends on the HW driver. The GPIO component
*          adds no wait.
*
* @param[in]    inst: Open handle.
* @param[in]    pin_index: Index of the pin. Passed unchanged to the HW driver, not checked.
* @param[in]    config: Pin configuration. Passed to the HW driver as is.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if config is NULL,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not open,
*               EZ_DRV_STATUS_BUSY if an operation is in flight, or another handle
*               listens to this pin,
*               EZ_DRV_STATUS_ERR_GENERIC if all the EZ_GPIO_MAX_LISTENERS
*               subscriptions are used,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no init_pin
*               function, or the HW driver status otherwise.
*
* @pre ezGpio_Open() succeeded for inst.
* @post On EZ_DRV_STATUS_OK the pin is initialized. If config->intr_mode is not
*       EZ_INTERRUPT_NONE, the handle listens to the interrupt of the pin.
* @see ezHwGpioConfig_t, ezHwGpio_Initialize, ezGpio_Close
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* static ezHwGpioConfig_t gpio0_config = {
*     EZ_GPIO_MODE_INPUT,
*     EZ_GPIO_PULL_UP,
*     EZ_INTERRUPT_FALLING,         // Pin interrupt on the falling edge, reported to OnGpioEvent()
* };
*
* EZ_DRV_STATUS InitGpio0Pin(void)
* {
*     EZ_DRV_STATUS status = ezGpio_Initialize(&gpio0_handle, 7U, &gpio0_config);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;
*     }
*     return EZ_DRV_STATUS_OK;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezGpio_Initialize(ezGpioHandle_t *inst, uint16_t pin_index, ezHwGpioConfig_t *config);

/*****************************************************************************
* Function: ezGpio_ReadPin
*//**
* @brief Read the state of a pin.
*
* @details Synchronous. The driver is
*          taken for the call, the read_pin function of the HW driver is called
*          with driver_h and pin_index, and the driver is released. The state
*          returned by the HW function is returned unchanged. The HW driver may
*          return EZ_GPIO_PIN_ERROR itself.
*
*          The checks run in this order: handle open, driver free. ReadPin has no status, so every failure returns EZ_GPIO_PIN_ERROR
*          and the reason is not reported. Use ezGpio_WritePin() or
*          ezGpio_TogglePin() to get a status.
*
* @param[in]    inst: Open handle.
* @param[in]    pin_index: Index of the pin. Passed unchanged to the HW driver, not checked.
* @return       EZ_GPIO_PIN_LOW or EZ_GPIO_PIN_HIGH, the state of the pin,
*               or EZ_GPIO_PIN_ERROR if inst is NULL or not open, an operation is
*               in flight, or the HW driver has no read_pin function.
*
* @pre ezGpio_Open() succeeded for inst.
* @note The pin index is not checked by this component, the HW driver validates
*       it. See ezHwGpioInterface.
* @see ezGpio_WritePin, ezGpio_Initialize, ezHwGpio_ReadPin
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* EZ_GPIO_PIN_STATE ReadGpio0Pin(void)
* {
*     EZ_GPIO_PIN_STATE state = ezGpio_ReadPin(&gpio0_handle, 7U);
*     if(state == EZ_GPIO_PIN_ERROR)
*     {
*         return EZ_GPIO_PIN_ERROR;    // Not open, busy or no HW function
*     }
*     return state;
* }
* @endcode
*
*****************************************************************************/
EZ_GPIO_PIN_STATE ezGpio_ReadPin(ezGpioHandle_t *inst, uint16_t pin_index);

/*****************************************************************************
* Function: ezGpio_WritePin
*//**
* @brief Write the state of a pin.
*
* @details Synchronous. The driver is taken for the call, the write_pin function
*          of the HW driver is called with driver_h, pin_index and state, and the
*          driver is released. The status returned by the HW function is returned
*          unchanged, whatever it is. pin_index and state are not checked by this
*          component.
*
*          The checks run in this order: handle open, driver free. A failed check
*          does not call the HW function.
*
* @param[in]    inst: Open handle.
* @param[in]    pin_index: Index of the pin. Passed unchanged to the HW driver, not checked.
* @param[in]    state: State to write. Passed unchanged to the HW driver, not checked.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not open,
*               EZ_DRV_STATUS_BUSY if an operation is in flight,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no write_pin
*               function, or the HW driver status otherwise.
*
* @pre ezGpio_Open() succeeded for inst.
* @note The pin index and the state are not checked by this component, the HW
*       driver validates them. See ezHwGpioInterface.
* @see ezGpio_ReadPin, ezGpio_TogglePin, ezHwGpio_WritePin
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* EZ_DRV_STATUS SetGpio0Pin(void)
* {
*     EZ_DRV_STATUS status = ezGpio_WritePin(&gpio0_handle, 7U, EZ_GPIO_PIN_HIGH);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;
*     }
*     return EZ_DRV_STATUS_OK;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezGpio_WritePin(ezGpioHandle_t *inst, uint16_t pin_index, EZ_GPIO_PIN_STATE state);

/*****************************************************************************
* Function: ezGpio_TogglePin
*//**
* @brief Toggle the state of a pin.
*
* @details Synchronous. The driver is taken for the call, the toggle_pin function
*          of the HW driver is called with driver_h and pin_index, and the driver
*          is released. The status returned by the HW function is returned
*          unchanged, whatever it is. pin_index is not checked by this component.
*
*          The checks run in this order: handle open, driver free. A failed check
*          does not call the HW function.
*
* @param[in]    inst: Open handle.
* @param[in]    pin_index: Index of the pin. Passed unchanged to the HW driver, not checked.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not open,
*               EZ_DRV_STATUS_BUSY if an operation is in flight,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no toggle_pin
*               function, or the HW driver status otherwise.
*
* @pre ezGpio_Open() succeeded for inst.
* @note The pin index is not checked by this component, the HW driver validates
*       it. See ezHwGpioInterface.
* @see ezGpio_WritePin, ezGpio_ReadPin, ezHwGpio_TogglePin
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* EZ_DRV_STATUS ToggleGpio0Pin(void)
* {
*     EZ_DRV_STATUS status = ezGpio_TogglePin(&gpio0_handle, 7U);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;
*     }
*     return EZ_DRV_STATUS_OK;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezGpio_TogglePin(ezGpioHandle_t *inst, uint16_t pin_index);

#endif /* EZ_GPIO == 1 */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_GPIO_H */


/* End of file */
