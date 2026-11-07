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
 *  @details The user binds an ezGpioHandle_t to a HW driver by name. All the
 *           operations of a handle go through the HW driver registered with
 *           ezGpio_SystemRegisterHwDriver().
 *
 *           Concurrency: this component does not provide mutual exclusion.
 *           The caller must serialize the calls on the same HW driver (mutex
 *           or semaphore of the upper layer). A pin operation made while another
 *           operation is in flight, or while another handle listens to pin
 *           events, is refused with EZ_DRV_STATUS_BUSY (ezGpio_ReadPin() returns
 *           EZ_GPIO_PIN_ERROR).
 *
 *           All pin operations are synchronous: the result is returned through
 *           the return value of the function. There is no async operation.
 *           The HW driver must NOT call ezDriver_NotifyEvent() with an
 *           EZ_DRV_EVENT_OP_* kind. The only event it reports is a pin
 *           interrupt, see below.
 *
 *           Pin interrupts: when ezGpio_Initialize() is called with an intr_mode
 *           other than EZ_INTERRUPT_NONE, the handle listens to the pin events
 *           of its HW driver. It starts listening before the HW init function
 *           is called, so the HW driver may arm the interrupt inside it. The HW driver reports them, usually from its ISR,
 *           with ezDriver_NotifyEvent(&hw_driver->base, EZ_DRV_EVENT_ASYNC,
 *           EZ_GPIO_EVENT_PIN_INTERRUPT, &pin_index, NULL). The callback given to
 *           ezGpio_RegisterInstance() is then called in the context of the HW
 *           driver. While a handle listens, every other handle is refused with
 *           EZ_DRV_STATUS_BUSY. Fan-out to several consumers is the job of the
 *           upper layer. The subscription ends with ezGpio_UnregisterInstance().
 *
 *           No function of this component is ISR-safe. From an ISR, the HW
 *           driver only calls ezDriver_NotifyEvent().
 *
 *           Argument checking: this component checks the handle, the config
 *           pointer of ezGpio_Initialize() and the driver name. The pin index
 *           and the pin state are passed as they are to the HW driver, which is
 *           responsible for validating them.
 *
 *           Initialization: ezGpio_ReadPin(), ezGpio_WritePin() and
 *           ezGpio_TogglePin() require that ezGpio_Initialize() has succeeded
 *           first. The initialized flag belongs to the HW driver, not to the
 *           pin: it is set by a successful ezGpio_Initialize() of any pin.
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
 *  @details The HW driver chooses the code and reports it with
 *           ezDriver_NotifyEvent(&drv->base, EZ_DRV_EVENT_ASYNC, event_code,
 *           &pin_index, NULL). Delivered only to the handle which listens to
 *           pin events, see ezGpio_Initialize().
 */
typedef enum
{
    EZ_GPIO_EVENT_PIN_INTERRUPT,    /**< A pin interrupt fired. param1 is a pointer to the uint16_t pin index, valid during the callback. param2 is passed as given by the HW driver */
    EZ_GPIO_NUM_EVENTS,             /**< Number of event codes. Not an event */
}EZ_GPIO_EVENT;


/** @brief GPIO handle. Held by the user, bound to a HW driver.
 *
 *  @details Must be zero-initialized (static storage, or memset) before the
 *           first ezGpio_RegisterInstance(), which refuses a handle whose hw
 *           field is not NULL. Bind it with ezGpio_RegisterInstance() before use.
 *           The HW driver keeps a pointer to the handle while the handle listens
 *           to pin events or an operation is in flight, so the handle must stay
 *           valid until ezGpio_UnregisterInstance() returns EZ_DRV_STATUS_OK.
 */
typedef struct
{
    struct ezDrvHandle  drv_handle;     /**< Common handle, bound to a HW driver. Filled by ezGpio_RegisterInstance() */
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
 */
typedef struct ezHwGpioConfig
{
    EZ_GPIO_MODE mode;              /**< mode of the GPIO pin */
    EZ_GPIO_PULL pull;              /**< pull mode of the GPIO pin */
    EZ_INTERRUPT_MODE intr_mode;    /**< interrupt mode of the GPIO pin */
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
 *           ezGpio_Initialize(), not a copy. The driver is marked initialized
 *           only when the function returns EZ_DRV_STATUS_OK.
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
 *           interrupt, reported from its ISR with EZ_DRV_EVENT_ASYNC and
 *           EZ_GPIO_EVENT_PIN_INTERRUPT. See the file description.
 */
struct ezHwGpioInterface
{
    void                *driver_h;  /**< Opaque pointer of the HW driver, passed to every function below */
    ezHwGpio_Initialize init_pin;   /**< Initialize pin (synchronous) */
    ezHwGpio_ReadPin    read_pin;   /**< Read pin (synchronous) */
    ezHwGpio_WritePin   write_pin;  /**< Write pin (synchronous) */
    ezHwGpio_TogglePin  toggle_pin; /**< Toggle pin (synchronous) */
};


/** @brief GPIO HW driver. Provided by the HW driver implementation.
 *
 *  @details base.name and interface must be set before
 *           ezGpio_SystemRegisterHwDriver(). The driver is linked into the list
 *           of registered drivers, so its memory (static storage, no malloc)
 *           must stay valid while it is registered. ezGpio_SystemRegisterHwDriver()
 *           resets base.state, base.user and initialized.
 */
struct ezGpioDriver
{
    struct Node                 ll_node;    /**< Node to link to the list of HW drivers */
    struct ezDrvBase            base;       /**< Common data of driver: name, state and the handle in use */
    struct ezHwGpioInterface    interface;  /**< HW API, see ezHwGpioInterface */
    bool                        initialized; /**< true after ezGpio_Initialize() succeeded for at least one pin. Reset to false on register and unregister. Set by ezGpio_Initialize() to the result of its init call: a failure (HW error, or no init_pin function) clears it for all pins. A call refused before the driver is taken (ERR_ARG, DRV_NOT_FOUND, BUSY) leaves it unchanged */
};

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
*          driver is reset to the idle, not-initialized state: initialized is
*          false, base.state is 0 and base.user is NULL. It is then added to the
*          list of HW drivers. base.name is compared with the names of the
*          drivers already registered.
*
* @param[in]    hw_gpio_driver: HW driver. base.name must be set and unique.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if hw_gpio_driver or its name is NULL, or the
*               name is already registered.
*
* @post base.state is 0, base.user is NULL and initialized is false
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
* uint16_t pin = 7U;
* ezDriver_NotifyEvent(&gpio0_drv.base, EZ_DRV_EVENT_ASYNC, EZ_GPIO_EVENT_PIN_INTERRUPT, &pin, NULL);
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
*          the list of HW drivers, initialized is set to false and base.state is
*          set to 0. That clears the listening state: a handle which listened to
*          pin events stops receiving them. base.user and the handles still bound
*          to the driver are not changed, and the code does not check them.
*
* @param[in]    hw_gpio_driver: HW driver registered earlier.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if hw_gpio_driver is NULL,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver.
*
* @pre hw_gpio_driver was registered with ezGpio_SystemRegisterHwDriver() and
*      is not unregistered yet. Not checked by the code: the unlink dereferences
*      the list pointers of the driver, which are NULL for a driver never
*      registered.
* @note Not checked by the code: the handles bound to the driver. Unregister
*       them with ezGpio_UnregisterInstance() first, otherwise they keep a
*       pointer to the driver.
* @see ezGpio_SystemRegisterHwDriver, ezGpio_UnregisterInstance
* @warning The driver list is changed without a lock. Do not call it while
*          another GPIO call can run.
*
* @isr_safe{no}
* @blocking{no}
*
*****************************************************************************/
EZ_DRV_STATUS ezGpio_SystemUnregisterHwDriver(struct ezGpioDriver *hw_gpio_driver);

/*****************************************************************************
* Function: ezGpio_RegisterInstance
*//**
* @brief Bind a handle to a registered HW driver.
*
* @details Stores the HW driver, the callback and the context in the handle.
*          The driver is not marked busy by this call, and its initialized flag
*          is not changed. A handle which is already bound is refused: call
*          ezGpio_UnregisterInstance() first. The handle and the driver name are
*          checked before the driver is looked up. The driver is looked up by
*          name, so the handle does not need the driver to be initialized.
*
* @param[in]    inst: Handle owned by the caller. Zero-initialized and not
*                     bound. Must stay valid while bound.
* @param[in]    driver_name: Name of the HW driver (base.name).
* @param[in]    callback: Called for the pin events of the handle, see
*                         ezGpio_Initialize(). Can be NULL, then the events are
*                         not reported. Runs in the context of the HW driver,
*                         usually an ISR.
* @param[in]    ctx: User context, passed to callback as first argument. Can be NULL.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if inst or driver_name is NULL, or the
*               handle is already bound,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if no HW driver has the name.
*
* @see ezGpio_UnregisterInstance, ezDrvCallback, ezGpio_Initialize
* @warning The callback runs inside ezDriver_NotifyEvent(), in the context of
*          the HW driver, usually an ISR. It must not block.
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* static ezGpioHandle_t gpio0_handle;  // Static: the handle must stay valid while bound
* static AppGpioCtx_t app_ctx;         // Passed to the callback, see ezGpio_Initialize()
*
* EZ_DRV_STATUS BindGpio0(void)
* {
*     EZ_DRV_STATUS status = ezGpio_RegisterInstance(&gpio0_handle, "gpio0", OnGpioEvent, &app_ctx);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;
*     }
*     return EZ_DRV_STATUS_OK;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezGpio_RegisterInstance(ezGpioHandle_t *inst,
                                      const char *driver_name,
                                      ezDrvCallback callback,
                                      void *ctx);

/*****************************************************************************
* Function: ezGpio_UnregisterInstance
*//**
* @brief Unbind a handle from its HW driver.
*
* @details Ends the pin event subscription of the handle, if it has one, and
*          clears hw, on_event and ctx of the handle. Refused only when this
*          handle has an operation in flight. An operation of another handle
*          does not block it. Since all pin operations are synchronous, this
*          happens when the call is made from a HW function of the same handle
*          while the operation runs. A handle which was never bound is accepted
*          and stays empty.
*
* @param[in]    inst: Handle to unbind. Registered with ezGpio_RegisterInstance(),
*                     or never bound.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if inst is NULL,
*               EZ_DRV_STATUS_BUSY if this handle has an operation in flight.
*
* @see ezGpio_RegisterInstance, ezGpio_Initialize
*
* @isr_safe{no}
* @blocking{no}
*
*****************************************************************************/
EZ_DRV_STATUS ezGpio_UnregisterInstance(ezGpioHandle_t *inst);

/*****************************************************************************
* Function: ezGpio_Initialize
*//**
* @brief Initialize a pin of the HW driver.
*
* @details Checks config first, then finds the HW driver of the handle and takes
*          it for the call. The init_pin function of the HW driver is called with
*          driver_h, pin_index and config. The driver is released after the call
*          and the HW status is returned. config is passed as is, the GPIO
*          component does not copy or check it, and it reads intr_mode after the
*          HW call returns.
*
*          The initialized flag of the driver is set to the result of the HW call,
*          so it is true only on EZ_DRV_STATUS_OK. The flag belongs to the driver:
*          a failure for any pin clears it for all pins, and a call refused
*          before the driver is taken leaves it unchanged. Calling it again on an
*          initialized driver calls the HW function again.
*
*          With config->intr_mode other than EZ_INTERRUPT_NONE, the handle
*          subscribes to the pin events of the driver BEFORE the HW function is
*          called. The HW driver may therefore arm the interrupt inside its
*          init_pin function, and an interrupt which fires there is delivered to
*          the callback during this call, in the context of the HW driver, while
*          the pin may be only partly configured. The callback cannot call another
*          GPIO function for that time: the driver is held and they return
*          EZ_DRV_STATUS_BUSY. If the HW call fails, a subscription made by this
*          call is removed again, and a subscription which existed before is kept.
*          On success the subscription is kept until ezGpio_UnregisterInstance(),
*          also when a later call uses EZ_INTERRUPT_NONE. If another handle
*          listens, the call returns EZ_DRV_STATUS_BUSY and the HW driver is not
*          called.
*
*          Whether the call blocks depends on the HW driver. The GPIO component
*          adds no wait.
*
* @param[in]    inst: Registered handle.
* @param[in]    pin_index: Index of the pin. Passed unchanged to the HW driver, not checked.
* @param[in]    config: Pin configuration. Passed to the HW driver as is.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if config is NULL,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not bound,
*               EZ_DRV_STATUS_BUSY if an operation is in flight, or another handle
*               listens to pin events,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no init_pin
*               function, or the HW driver status otherwise.
*
* @pre ezGpio_RegisterInstance() succeeded for inst.
* @post On EZ_DRV_STATUS_OK the driver is initialized and idle. If config->intr_mode
*       is not EZ_INTERRUPT_NONE, the handle listens to the pin events.
* @see ezHwGpioConfig_t, ezHwGpio_Initialize, ezGpio_UnregisterInstance
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
* @details Synchronous. Refused unless the driver is initialized. The driver is
*          taken for the call, the read_pin function of the HW driver is called
*          with driver_h and pin_index, and the driver is released. The state
*          returned by the HW function is returned unchanged. The HW driver may
*          return EZ_GPIO_PIN_ERROR itself.
*
*          The checks run in this order: handle bound, driver initialized, driver
*          free. ReadPin has no status, so every failure returns EZ_GPIO_PIN_ERROR
*          and the reason is not reported. Use ezGpio_WritePin() or
*          ezGpio_TogglePin() to get a status.
*
* @param[in]    inst: Registered handle.
* @param[in]    pin_index: Index of the pin. Passed unchanged to the HW driver, not checked.
* @return       EZ_GPIO_PIN_LOW or EZ_GPIO_PIN_HIGH, the state of the pin,
*               or EZ_GPIO_PIN_ERROR if inst is NULL or not bound, the driver is
*               not initialized, an operation is in flight or another handle
*               listens to pin events, or the HW driver has no read_pin function.
*
* @pre ezGpio_Initialize() succeeded for the driver of inst.
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
*         return EZ_GPIO_PIN_ERROR;    // Not bound, not initialized, busy or no HW function
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
*          The checks run in this order: handle bound, driver initialized, driver
*          free. A failed check does not call the HW function.
*
* @param[in]    inst: Registered handle.
* @param[in]    pin_index: Index of the pin. Passed unchanged to the HW driver, not checked.
* @param[in]    state: State to write. Passed unchanged to the HW driver, not checked.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not bound,
*               EZ_DRV_STATUS_ERR_NOT_INITIALIZED if ezGpio_Initialize() has not
*               succeeded,
*               EZ_DRV_STATUS_BUSY if an operation is in flight, or another handle
*               listens to pin events,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no write_pin
*               function, or the HW driver status otherwise.
*
* @pre ezGpio_Initialize() succeeded for the driver of inst.
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
*          The checks run in this order: handle bound, driver initialized, driver
*          free. A failed check does not call the HW function.
*
* @param[in]    inst: Registered handle.
* @param[in]    pin_index: Index of the pin. Passed unchanged to the HW driver, not checked.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not bound,
*               EZ_DRV_STATUS_ERR_NOT_INITIALIZED if ezGpio_Initialize() has not
*               succeeded,
*               EZ_DRV_STATUS_BUSY if an operation is in flight, or another handle
*               listens to pin events,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no toggle_pin
*               function, or the HW driver status otherwise.
*
* @pre ezGpio_Initialize() succeeded for the driver of inst.
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
