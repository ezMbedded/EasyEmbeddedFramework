/*****************************************************************************
* Filename:         ez_uart.h
* Author:           Hai Nguyen
* Original Date:    15.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_uart.h
 *  @author Hai Nguyen
 *  @date   15.03.2024
 *  @brief  Hardware Abstraction Layer for UART communication
 *
 *  @details The user binds an ezUartHandle_t to a HW driver by name. All the
 *           operations of a handle go through the HW driver registered with
 *           ezUart_SystemRegisterHwDriver(). The HW driver implements the
 *           functions of ezHwUartInterface, which this component calls.
 *
 *           Concurrency: this component does not provide mutual exclusion.
 *           The caller must serialize the calls on the same HW driver (mutex
 *           or semaphore of the upper layer). A call made while another
 *           operation is in flight returns EZ_DRV_STATUS_BUSY.
 *
 *           Sync operations (ezUart_TransmitSync(), ezUart_ReceiveSync()) return
 *           when the HW driver returns. timeout_millis is passed unchanged to the
 *           HW driver. Async operations (ezUart_TransmitAsync(),
 *           ezUart_ReceiveAsync()) return as soon as the transfer is started.
 *           The HW driver reports the end of an async operation by calling
 *           ezDriver_NotifyEvent(&hw_driver->base, EZ_DRV_EVENT_OP_DONE,
 *           UART_EVENT_xxx, NULL, NULL), usually from its ISR. The driver is
 *           busy until then, and the handle callback is invoked after the driver
 *           is released, so it may start the next operation.
 *           A sync operation reports its result only through the return value.
 *           The HW function of a sync operation must NOT call
 *           ezDriver_NotifyEvent(): such events are dropped and the handle
 *           callback is not invoked. A HW driver which waits for an interrupt
 *           uses its own semaphore or flag for that.
 *
 *           No function of this component is ISR-safe. From an ISR, the HW
 *           driver only calls ezDriver_NotifyEvent().
 *
 *           Argument checking: this component checks the handle, the driver
 *           name, the config pointer of ezUart_GetConfig() and the initialized
 *           state of the driver for the transmit and receive functions. The
 *           buffers, buffer sizes and timeouts are passed as they are to the HW
 *           driver, which is responsible for validating them.
 *
 *           Initialization: ezUart_Initialize() must succeed before the transmit
 *           and receive functions, sync or async. Otherwise they return
 *           EZ_DRV_STATUS_ERR_NOT_INITIALIZED. ezUart_UpdateConfig(),
 *           ezUart_GetConfig() and ezUart_Deinitialize() do not require it.
 *
 *           Configuration: the configuration is stored in the HW driver
 *           (ezUartDriver::config). Read it with ezUart_GetConfig(), which makes
 *           no HW call, and apply it with ezUart_UpdateConfig(), which calls the
 *           HW driver.
 *
 *           Completion: this component does not subscribe to the driver, so
 *           EZ_DRV_EVENT_ASYNC events are not delivered to a handle. The end of
 *           an async operation is delivered through the callback given to
 *           ezUart_RegisterInstance().
 */

#ifndef _EZ_UART_H
#define _EZ_UART_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_UART == 1)
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
 *  @details The HW driver reports one of these codes with
 *           ezDriver_NotifyEvent(&drv->base, EZ_DRV_EVENT_OP_DONE, event_code,
 *           NULL, NULL) when an async operation ends. This component defines no
 *           parameters: param1 and param2 are forwarded as the HW driver passes
 *           them. Sync operations never report an event.
 */
typedef enum
{
    UART_EVENT_TX_CMPLT,    /**< The transmit operation is completed */
    UART_EVENT_TX_ERR,      /**< The transmit operation failed */
    UART_EVENT_RX_CMPLT,    /**< The receive operation is completed */
    UART_EVENT_RX_ERR,      /**< The receive operation failed */
    UART_EVENT_TIMEOUT,     /**< The operation timed out */
    UART_NUM_EVENTS,        /**< Number of event codes. Not an event */
}EZ_UART_EVENT;


/** @brief UART parity
 *
 *  @details Member of ezUartConfiguration. The UART component does not check it.
 *           The HW driver decides which values it supports.
 */
typedef enum
{
    PARITY_NONE,    /**< None */
    PARITY_ODD,     /**< Odd */
    PARITY_EVEN,    /**< Even */
    PARITY_MARK,    /**< Mark */
    PARITY_SPACE,   /**< Space */
}EZ_UART_PARITY_ENUM;


/** @brief UART number of stop bits
 *
 *  @details Member of ezUartConfiguration. The UART component does not check it.
 *           The HW driver decides which values it supports.
 */
typedef enum
{
    ONE_BIT,            /**< 1 stop bit */
    ONE_AND_HALF_BIT,   /**< 1.5 stop bit */
    TWO_BITS,           /**< 2 stop bits */
}EZ_UART_NUM_STOP_BIT_ENUM;


/** @brief UART configuration
 *
 *  @details Member of ezUartDriver (config). ezUart_GetConfig() returns a pointer
 *           to it. The UART component does not check or copy the values. The HW
 *           driver decides which values it supports.
 */
struct ezUartConfiguration
{
    const char                  *port_name; /**< Name of the serial port */
    uint32_t                    baudrate;   /**< Baudrate */
    EZ_UART_PARITY_ENUM         parity;     /**< Parity */
    EZ_UART_NUM_STOP_BIT_ENUM   stop_bit;   /**< Number of stop bits */
    uint8_t                     byte_size;  /**< Size of data in bits */
};


/** @brief UART handle. Held by the user and bound to a HW driver.
 *
 *  @details Must be zero-initialized (static storage, or memset) before the
 *           first ezUart_RegisterInstance(), because that call checks drv_handle.hw
 *           to see whether the handle is already bound. Bind it with
 *           ezUart_RegisterInstance() before use. The driver keeps a pointer to
 *           the handle in base.user while an operation is in flight, so the
 *           handle must stay valid until ezUart_UnregisterInstance() returns
 *           EZ_DRV_STATUS_OK.
 */
typedef struct
{
    struct ezDrvHandle  drv_handle;     /**< Common handle, bound to a HW driver. Filled by ezUart_RegisterInstance() */
} ezUartHandle_t;


/****************************************************************************/
/* List of API implemented by the HW driver. Users are not supposed to use
 * these API.
 */

/** @brief Initialize the HW UART. Called by ezUart_Initialize().
 *
 *  @details Synchronous: the returned status is passed back to the caller. The
 *           UART component adds no wait. Whether the function blocks is up to the
 *           HW driver. The driver is marked initialized only when the function
 *           returns EZ_DRV_STATUS_OK.
 *
 *  @param[in]  driver_h: opaque pointer of the HW driver, ezHwUartInterface.driver_h
 *  @return     EZ_DRV_STATUS_OK if success, otherwise EZ_DRV_STATUS_xxx
 */
typedef EZ_DRV_STATUS (*ezHwUart_Initialize)(void *driver_h);


/** @brief Deinitialize the HW UART. Called by ezUart_Deinitialize().
 *
 *  @details Synchronous. Called even when the driver is not initialized. The
 *           driver is marked not initialized only when the function returns
 *           EZ_DRV_STATUS_OK. Whether the function blocks is up to the HW driver.
 *
 *  @param[in]  driver_h: opaque pointer of the HW driver
 *  @return     EZ_DRV_STATUS_OK if success, otherwise EZ_DRV_STATUS_xxx
 */
typedef EZ_DRV_STATUS (*ezHwUart_Deinitialize)(void *driver_h);


/** @brief Transmit data, asynchronous. Called by ezUart_TransmitAsync().
 *
 *  @details Return EZ_DRV_STATUS_OK only if the transmission is started. The
 *           HW driver then calls ezDriver_NotifyEvent() with
 *           EZ_DRV_EVENT_OP_DONE and an EZ_UART_EVENT code exactly once when it
 *           ends. On any other status the transfer is not started, and the HW
 *           driver must not call ezDriver_NotifyEvent().
 *
 *  @param[in]  driver_h: opaque pointer of the HW driver
 *  @param[in]  tx_buff: transmit buffer, must stay valid until the end event
 *  @param[in]  buff_size: size of the buffer. Not checked by the UART component
 *  @return     EZ_DRV_STATUS_OK if started, otherwise EZ_DRV_STATUS_xxx
 */
typedef EZ_DRV_STATUS (*ezHwUart_TransmitAsync)(void *driver_h, const uint8_t *tx_buff, uint16_t buff_size);


/** @brief Receive data, asynchronous. Called by ezUart_ReceiveAsync().
 *
 *  @details Same rules as ezHwUart_TransmitAsync.
 *
 *  @param[in]  driver_h: opaque pointer of the HW driver
 *  @param[in]  rx_buff: receive buffer, must stay valid until the end event
 *  @param[in]  buff_size: size of the buffer. Not checked by the UART component
 *  @return     EZ_DRV_STATUS_OK if started, otherwise EZ_DRV_STATUS_xxx
 */
typedef EZ_DRV_STATUS (*ezHwUart_ReceiveAsync)(void *driver_h, uint8_t *rx_buff, uint16_t buff_size);


/** @brief Transmit data, synchronous. Called by ezUart_TransmitSync().
 *
 *  @details Synchronous: return the final status. The HW function must NOT call
 *           ezDriver_NotifyEvent(): such events are dropped. timeout_millis is
 *           passed unchanged by the UART component.
 *
 *  @param[in]  driver_h: opaque pointer of the HW driver
 *  @param[in]  tx_buff: transmit buffer. Not checked by the UART component
 *  @param[in]  buff_size: size of the buffer. Not checked by the UART component
 *  @param[in]  timeout_millis: timeout in millisecond
 *  @return     EZ_DRV_STATUS_OK if success, otherwise EZ_DRV_STATUS_xxx
 */
typedef EZ_DRV_STATUS (*ezHwUart_TransmitSync)(void *driver_h, const uint8_t *tx_buff, uint16_t buff_size, uint32_t timeout_millis);


/** @brief Receive data, synchronous. Called by ezUart_ReceiveSync().
 *
 *  @details Same rules as ezHwUart_TransmitSync.
 *
 *  @param[in]  driver_h: opaque pointer of the HW driver
 *  @param[in]  rx_buff: receive buffer. Not checked by the UART component
 *  @param[in]  buff_size: size of the buffer. Not checked by the UART component
 *  @param[in]  timeout_millis: timeout in millisecond
 *  @return     EZ_DRV_STATUS_OK if success, otherwise EZ_DRV_STATUS_xxx
 */
typedef EZ_DRV_STATUS (*ezHwUart_ReceiveSync)(void *driver_h, uint8_t *rx_buff, uint16_t buff_size, uint32_t timeout_millis);


/** @brief Apply the configuration of the driver. Called by ezUart_UpdateConfig().
 *
 *  @details Synchronous. Only driver_h is passed. The configuration is not an
 *           argument: it is ezUartDriver::config, read with ezUart_GetConfig().
 *
 *  @param[in]  driver_h: opaque pointer of the HW driver
 *  @return     EZ_DRV_STATUS_OK if success, otherwise EZ_DRV_STATUS_xxx
 */
typedef EZ_DRV_STATUS (*ezHwUart_UpdateConfig)(void *driver_h);


/** @brief HW API provided by the HW driver. Every function is optional.
 *
 *  @details A NULL function makes the matching ezUart_xxx() return
 *           EZ_DRV_STATUS_ERR_INF_NOT_EXIST. Each function is called with driver_h
 *           as its first argument. The buffer pointers and sizes are not checked
 *           by the UART component: each HW function validates them and returns an
 *           error status, which the component forwards unchanged.
 *
 *           An async function returns EZ_DRV_STATUS_OK only if the transfer is
 *           started. The HW driver must then call ezDriver_NotifyEvent() with
 *           EZ_DRV_EVENT_OP_DONE exactly once when it ends.
 */
struct ezHwUartInterface
{
    void                    *driver_h;       /**< Opaque pointer of the HW driver, passed to every function below */
    ezHwUart_Initialize     initialize;      /**< Initialize the UART peripheral (synchronous) */
    ezHwUart_Deinitialize   deinitialize;    /**< Deinitialize the UART peripheral (synchronous) */
    ezHwUart_TransmitAsync  transmit_async;  /**< Transmit data (asynchronous) */
    ezHwUart_ReceiveAsync   receive_async;   /**< Receive data (asynchronous) */
    ezHwUart_TransmitSync   transmit_sync;   /**< Transmit data (synchronous) */
    ezHwUart_ReceiveSync    receive_sync;    /**< Receive data (synchronous) */
    ezHwUart_UpdateConfig   update_conf;     /**< Apply the configuration (synchronous) */
};


/** @brief HW UART driver. Provided by the HW driver implementation.
 *
 *  @details base.name must be set before ezUart_SystemRegisterHwDriver(). The
 *           driver is linked into the list of registered drivers, so its memory
 *           (static storage, no malloc) must stay valid while it is registered.
 *           ezUart_SystemRegisterHwDriver() resets base.state, base.user and
 *           initialized.
 */
struct ezUartDriver
{
    struct Node                 ll_node;     /**< Node to link to the list of HW drivers */
    struct ezDrvBase            base;        /**< Common data of driver: name, state and the handle in use */
    struct ezUartConfiguration  config;      /**< Uart related configuration */
    struct ezHwUartInterface    interface;   /**< HW API, see ezHwUartInterface */
    bool                        initialized; /**< true after ezUart_Initialize() succeeded. Reset to false on register, unregister, when the HW initialize function fails or is missing in ezUart_Initialize(), and when ezUart_Deinitialize() succeeds. Unchanged when a call is refused before the HW call (handle not bound, driver busy) */
};


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */


/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function: ezUart_SystemRegisterHwDriver
*//** 
* @brief Register a HW UART driver to the UART component.
*
* @details Called by the HW driver implementation, not by the application. The
*          driver is reset to the idle, not-initialized state: initialized is
*          false, base.state is 0 and base.user is NULL. It is then added to the
*          list of HW drivers. base.name is compared with the names of the
*          drivers already registered.
*
* @param[in]    hw_uart_driver: HW driver. base.name must be set and unique.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if hw_uart_driver or its name is NULL, or the
*               name is already registered.
*
* @post base.state is 0, base.user is NULL and initialized is false
* @see ezUart_SystemUnregisterHwDriver, ezHwUartInterface
* @warning The driver list is changed without a lock. Do not call it while
*          another UART call can run.
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* // HW driver side, at startup. The driver object is static (no malloc).
* static struct ezUartDriver uart0_drv;
*
* EZ_DRV_STATUS RegisterUart0(void)
* {
*     EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_GENERIC;
*
*     uart0_drv.base.name = "uart0";                  // Must be unique
*     uart0_drv.interface.driver_h = &uart0_regs;     // Given to every HW function
*     uart0_drv.interface.initialize = Uart0_Initialize;
*     uart0_drv.interface.deinitialize = Uart0_Deinitialize;
*     uart0_drv.interface.transmit_async = Uart0_TransmitAsync;
*     uart0_drv.interface.receive_async = NULL;       // Unused: ezUart_ReceiveAsync() returns EZ_DRV_STATUS_ERR_INF_NOT_EXIST
*     uart0_drv.interface.transmit_sync = Uart0_TransmitSync;
*     uart0_drv.interface.receive_sync = Uart0_ReceiveSync;
*     uart0_drv.interface.update_conf = Uart0_UpdateConfig;
*
*     status = ezUart_SystemRegisterHwDriver(&uart0_drv);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;
*     }
*     return EZ_DRV_STATUS_OK;
* }
*
* // In the ISR of the UART peripheral, when an async transfer ends
* ezDriver_NotifyEvent(&uart0_drv.base, EZ_DRV_EVENT_OP_DONE, UART_EVENT_TX_CMPLT, NULL, NULL);
* @endcode
*
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_SystemRegisterHwDriver(struct ezUartDriver *hw_uart_driver);


/*****************************************************************************
* Function: ezUart_SystemUnregisterHwDriver
*//** 
* @brief Unregister a HW UART driver from the UART component.
*
* @details Called by the HW driver implementation. The driver is removed from
*          the list of HW drivers and initialized is set to false. base.state and
*          base.user are not changed. The code does not check the handles: a
*          handle still bound to the driver keeps its pointer to it.
*
* @param[in]    hw_uart_driver: HW driver registered earlier.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if hw_uart_driver is NULL,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver.
*
* @pre Every handle bound to the driver has been unregistered with
*      ezUart_UnregisterInstance(). Not checked by the code.
* @post On EZ_DRV_STATUS_OK initialized is false and the driver is not in the list
* @see ezUart_SystemRegisterHwDriver, ezUart_UnregisterInstance
* @warning The driver list is changed without a lock. Do not call it while
*          another UART call can run.
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* EZ_DRV_STATUS status = ezUart_SystemUnregisterHwDriver(&uart0_drv);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_SystemUnregisterHwDriver(struct ezUartDriver *hw_uart_driver);


/*****************************************************************************
* Function: ezUart_RegisterInstance
*//** 
* @brief Bind a handle to a registered HW UART driver.
*
* @details Stores the HW driver, the callback and the context in the handle.
*          The driver is not marked busy by this call. A handle which is
*          already bound is refused: call ezUart_UnregisterInstance() first. The
*          handle and the driver name are checked before the driver is looked up.
*
* @param[in]    inst: Handle owned by the caller. Zero-initialized and not bound.
*                     Must stay valid while bound.
* @param[in]    driver_name: Name of the HW driver (base.name).
* @param[in]    callback: Called when an async operation ends. Can be NULL, then
*                         the end of the operation is not reported. Runs in the
*                         context of the HW driver, usually an ISR.
* @param[in]    ctx: User context, passed to callback as first argument. Can be NULL.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if inst or driver_name is NULL, or the
*               handle is already bound,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if no HW driver has the name.
*
* @pre ezUart_SystemRegisterHwDriver() succeeded for the driver with driver_name
* @post None
* @warning The callback runs inside ezDriver_NotifyEvent(), in the context of
*          the HW driver, usually an ISR. It must not block.
* @see ezUart_SystemRegisterHwDriver, ezUart_UnregisterInstance, ezDrvCallback
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* static ezUartHandle_t uart0_handle;  // Static: the handle must stay valid while bound
*
* EZ_DRV_STATUS BindUart0(void)
* {
*     EZ_DRV_STATUS status = ezUart_RegisterInstance(&uart0_handle, "uart0", OnUartEvent, NULL);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;
*     }
*     return EZ_DRV_STATUS_OK;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_RegisterInstance(ezUartHandle_t *inst,
                                      const char *driver_name,
                                      ezDrvCallback callback,
                                      void *ctx);


/*****************************************************************************
* Function: ezUart_UnregisterInstance
*//** 
* @brief Unbind a handle from its HW UART driver.
*
* @details Clears hw, on_event and ctx of the handle. Refused only when this
*          handle has an operation in flight. An operation of another handle
*          does not block it. A handle which was never bound is accepted and
*          stays empty.
*
* @param[in]    inst: Handle to unbind. Registered with ezUart_RegisterInstance(),
*                     or never bound.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if inst is NULL,
*               EZ_DRV_STATUS_BUSY if this handle has an operation in flight.
*
* @pre None
* @post On EZ_DRV_STATUS_OK hw, on_event and ctx of the handle are NULL
*
* \b Example
* @code
* EZ_DRV_STATUS status = ezUart_UnregisterInstance(&uart0_handle);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* @endcode
*
* @see ezUart_RegisterInstance
*
* @isr_safe{no}
* @blocking{no}
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_UnregisterInstance(ezUartHandle_t *inst);


/*****************************************************************************
* Function: ezUart_Initialize
*//** 
* @brief Initialize the HW UART of the driver.
*
* @details Calls the initialize function of the HW driver with driver_h. The
*          driver is taken for the call and released after it. It is marked
*          initialized only when the HW function returns EZ_DRV_STATUS_OK, and
*          that status is returned. A failed HW call, or a HW driver without an
*          initialize function, marks the driver as not initialized, also when it
*          was initialized before. A call refused before the HW call (handle not
*          bound, driver busy) does not change it. Calling it again on an
*          initialized driver calls the HW function again.
*
*          Transmit and receive functions require a successful call of this
*          function. Whether it blocks depends on the HW driver: the HW initialize
*          function is synchronous, and the UART component adds no wait.
*
* @param[in]    inst: Registered handle.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not bound,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no initialize
*               function, or the HW driver status otherwise.
*
* @pre ezUart_RegisterInstance() succeeded for inst.
* @post On EZ_DRV_STATUS_OK the driver is initialized and idle.
* @see ezUart_Deinitialize, ezUart_RegisterInstance, ezHwUart_Initialize
*
* @isr_safe{no}
* @blocking{yes}
*
* \b Example
* @code
* EZ_DRV_STATUS status = ezUart_Initialize(&uart0_handle);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_Initialize(ezUartHandle_t *inst);


/*****************************************************************************
* Function: ezUart_Deinitialize
*//** 
* @brief Deinitialize the HW UART of the driver.
*
* @details Calls the deinitialize function of the HW driver with driver_h. It is
*          called even when the driver is not initialized. The driver is taken for
*          the call and released after it. The driver is marked not initialized
*          only when the HW function returns EZ_DRV_STATUS_OK. On any other status
*          the initialized state is kept. A call refused before the HW call
*          (handle not bound, driver busy) does not change it.
*
* @param[in]    inst: Registered handle.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not bound,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no deinitialize
*               function, or the HW driver status otherwise.
*
* @pre ezUart_RegisterInstance() succeeded for inst.
* @post On EZ_DRV_STATUS_OK the driver is not initialized and idle.
* @see ezUart_Initialize, ezUart_RegisterInstance, ezHwUart_Deinitialize
*
* @isr_safe{no}
* @blocking{yes}
*
* \b Example
* @code
* EZ_DRV_STATUS status = ezUart_Deinitialize(&uart0_handle);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_Deinitialize(ezUartHandle_t *inst);


/*****************************************************************************
* Function: ezUart_TransmitAsync
*//** 
* @brief Start to transmit data. Return as soon as the transfer is started or refused.
*
* @details Returns as soon as the HW driver has started, or refused, the
*          transfer. The driver must be initialized (see ezUart_Initialize). On
*          EZ_DRV_STATUS_OK the transfer is running and the driver stays busy. The
*          HW driver must then call ezDriver_NotifyEvent() with
*          EZ_DRV_EVENT_OP_DONE and an EZ_UART_EVENT code exactly once when the
*          transfer ends. The callback of the handle is then called with that
*          event_code. The driver is released before the callback runs, so the
*          callback may start the next operation.
*
*          On any other status this call did not start a transfer, so no callback
*          comes for it. The driver is released by this call.
*
* @param[in]    inst: Registered handle.
* @param[in]    tx_buff: Data to transmit. Must stay valid until the end event.
* @param[in]    buff_size: Number of bytes to transmit. Not checked by this component.
* @return       EZ_DRV_STATUS_OK if the transfer is started,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not bound,
*               EZ_DRV_STATUS_ERR_NOT_INITIALIZED if ezUart_Initialize() has not
*               succeeded,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no
*               transmit_async function, or the HW driver status otherwise.
*
* @pre ezUart_Initialize() succeeded for the driver of inst.
* @post On EZ_DRV_STATUS_OK the driver is busy until the end event is reported
* @warning tx_buff must stay valid until the end event is reported. The UART
*          component does not copy it.
* @warning The callback runs in the context of the HW driver, usually an ISR.
*          It must not block.
* @warning The UART component is not a lock. The caller must serialize the calls
*          on the same HW driver.
* @note The buffer and its size are not checked by this component, the HW driver
*       validates them. See ezHwUartInterface.
* @see ezUart_ReceiveAsync, ezUart_RegisterInstance, ezDriver_NotifyEvent, ezHwUartInterface
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* static uint8_t tx_buf[3] = {1U, 2U, 3U};
*
* EZ_DRV_STATUS StartTx(void)
* {
*     EZ_DRV_STATUS status = ezUart_TransmitAsync(&uart0_handle, tx_buf, 3U);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;               // Not started: no callback will come
*     }
*     return EZ_DRV_STATUS_OK;         // Running: the callback gets the end event
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_TransmitAsync(ezUartHandle_t *inst,
                                   const uint8_t *tx_buff,
                                   uint16_t buff_size);


/*****************************************************************************
* Function: ezUart_ReceiveAsync
*//** 
* @brief Start to receive data. Return as soon as the transfer is started or refused.
*
* @details Same rules as ezUart_TransmitAsync(), with receive data instead of
*          transmit data: the driver stays busy on EZ_DRV_STATUS_OK until the HW
*          driver reports EZ_DRV_EVENT_OP_DONE, and the callback then gets the
*          event code (for example UART_EVENT_RX_CMPLT or UART_EVENT_RX_ERR).
*          The driver must be initialized (see ezUart_Initialize).
*
* @param[in]    inst: Registered handle.
* @param[out]   rx_buff: Buffer for the received data. Must stay valid until the end event.
* @param[in]    buff_size: Number of bytes to receive. Not checked by this component.
* @return       Same as ezUart_TransmitAsync().
*
* @pre ezUart_Initialize() succeeded for the driver of inst.
* @post On EZ_DRV_STATUS_OK the driver is busy until the end event is reported
* @warning rx_buff must stay valid until the end event is reported. The UART
*          component does not copy it.
* @warning The callback runs in the context of the HW driver, usually an ISR.
*          It must not block.
* @warning The UART component is not a lock. The caller must serialize the calls
*          on the same HW driver.
* @note The buffer and its size are not checked by this component, the HW driver
*       validates them. See ezHwUartInterface.
* @see ezUart_ReceiveSync, ezUart_TransmitAsync, ezDriver_NotifyEvent
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* static uint8_t rx_buf[3];
*
* EZ_DRV_STATUS StartRx(void)
* {
*     EZ_DRV_STATUS status = ezUart_ReceiveAsync(&uart0_handle, rx_buf, 3U);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;               // Not started: no callback will come
*     }
*     return EZ_DRV_STATUS_OK;         // Running: the callback gets the end event
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_ReceiveAsync(ezUartHandle_t *inst,
                                  uint8_t *rx_buff,
                                  uint16_t buff_size);


/*****************************************************************************
* Function: ezUart_TransmitSync
*//** 
* @brief Transmit data to the HW. Return when done.
*
* @details Blocks the caller until the HW driver returns. timeout_millis is
*          passed unchanged to the HW driver. The driver is taken for the call and
*          released after the HW call returns, whatever the status. The driver must
*          be initialized (see ezUart_Initialize). A sync operation reports no
*          callback: its result is only the return value.
*
* @param[in]    inst: Registered handle.
* @param[in]    tx_buff: Data to transmit. Not checked by this component.
* @param[in]    buff_size: Number of bytes to transmit. Not checked by this component.
* @param[in]    timeout_millis: Timeout in milliseconds, passed to the HW driver.
* @return       EZ_DRV_STATUS_OK if success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not bound,
*               EZ_DRV_STATUS_ERR_NOT_INITIALIZED if ezUart_Initialize() has not
*               succeeded,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no
*               transmit_sync function, or the HW driver status otherwise (passed
*               through unchanged, e.g. EZ_DRV_STATUS_TIMEOUT).
*
* @pre ezUart_Initialize() succeeded for the driver of inst.
* @warning The UART component is not a lock. The caller must serialize the calls
*          on the same HW driver.
* @note The buffer and its size are not checked by this component, the HW driver
*       validates them. See ezHwUartInterface.
* @see ezUart_TransmitAsync, ezUart_Initialize, ezHwUartInterface
*
* @isr_safe{no}
* @blocking{yes}
*
* \b Example
* @code
* uint8_t tx[2] = {0x10U, 0x20U};
* EZ_DRV_STATUS status = ezUart_TransmitSync(&uart0_handle, tx, 2U, 100U);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_TransmitSync(ezUartHandle_t *inst, 
                                  const uint8_t *tx_buff,
                                  uint16_t buff_size,
                                  uint32_t timeout_millis);


/*****************************************************************************
* Function: ezUart_ReceiveSync
*//** 
* @brief Receive data from the HW. Return when done.
*
* @details Same rules as ezUart_TransmitSync(), with receive data instead of
*          transmit data. The driver is taken for the call and released after the
*          HW call returns, whatever the status.
*
* @param[in]    inst: Registered handle.
* @param[out]   rx_buff: Buffer for the received data. Not checked by this component.
* @param[in]    buff_size: Number of bytes to receive. Not checked by this component.
* @param[in]    timeout_millis: Timeout in milliseconds, passed to the HW driver.
* @return       Same as ezUart_TransmitSync(), with receive_sync instead of transmit_sync.
*
* @pre ezUart_Initialize() succeeded for the driver of inst.
* @warning The UART component is not a lock. The caller must serialize the calls
*          on the same HW driver.
* @note The buffer and its size are not checked by this component, the HW driver
*       validates them. See ezHwUartInterface.
* @see ezUart_ReceiveAsync, ezUart_TransmitSync, ezHwUartInterface
*
* @isr_safe{no}
* @blocking{yes}
*
* \b Example
* @code
* uint8_t rx[2] = {0U, 0U};
* EZ_DRV_STATUS status = ezUart_ReceiveSync(&uart0_handle, rx, 2U, 100U);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_ReceiveSync(ezUartHandle_t *inst,
                                 uint8_t *rx_buff,
                                 uint16_t buff_size,
                                 uint32_t timeout_millis);


/*****************************************************************************
* Function: ezUart_GetConfig
*//** 
* @brief Get the configuration of the HW UART driver.
*
* @details Returns a pointer to the configuration stored in the driver
*          (ezUartDriver::config). No HW call is made, and the call does not check
*          the busy state or the initialized state. It works before
*          ezUart_Initialize(). The values are applied to the HW by
*          ezUart_UpdateConfig(). The pointer refers to the driver object, so it
*          stays valid while the driver is registered.
*
* @param[in]    inst: Registered handle.
* @param[out]   config: Receives the pointer to the configuration. Must not be NULL.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not bound,
*               EZ_DRV_STATUS_ERR_ARG if config is NULL.
*
* @pre ezUart_RegisterInstance() succeeded for inst.
* @post None
* @see ezUart_UpdateConfig, ezUart_RegisterInstance, ezUartConfiguration
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* struct ezUartConfiguration *config = NULL;
* EZ_DRV_STATUS status = ezUart_GetConfig(&uart0_handle, &config);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* @endcode
*
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_GetConfig(ezUartHandle_t *inst,
                               struct ezUartConfiguration **config);


/*****************************************************************************
* Function: ezUart_UpdateConfig
*//** 
* @brief Apply the configuration of the driver to the HW UART.
*
* @details The configuration is read with ezUart_GetConfig(), changed by the
*          caller, and applied by this call. The driver is taken for the HW call
*          and released after it, whatever the status. The HW function receives
*          only driver_h. The call does not require ezUart_Initialize() and does
*          not change the initialized state.
*
* @param[in]    inst: Registered handle.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not bound,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no update_conf
*               function, or the HW driver status otherwise.
*
* @pre ezUart_RegisterInstance() succeeded for inst.
* @post None
* @see ezUart_GetConfig, ezUartConfiguration, ezHwUart_UpdateConfig
*
* @isr_safe{no}
* @blocking{yes}
*
* \b Example
* @code
* struct ezUartConfiguration *config = NULL;
* EZ_DRV_STATUS status = ezUart_GetConfig(&uart0_handle, &config);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* config->baudrate = 115200U;
* status = ezUart_UpdateConfig(&uart0_handle);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* @endcode
*
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_UpdateConfig(ezUartHandle_t *inst);

#ifdef __cplusplus
}
#endif

#endif /* EZ_UART == 1 */
#endif /* _EZ_UART_H */


/* End of file */
