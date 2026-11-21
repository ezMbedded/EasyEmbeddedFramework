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
 *  @details A HW driver is one UART port. A handle is one user of it. The user
 *           opens a handle with ezUart_Open(), which binds it to the HW driver by
 *           name and makes the UART ready in one step, and closes it with
 *           ezUart_Close(). All the operations of a handle go through the HW driver
 *           registered with ezUart_SystemRegisterHwDriver(). The HW driver
 *           implements the functions of ezHwUartInterface, which this component calls.
 *
 *           Lifecycle: several handles can be open on the same HW driver. The UART
 *           is initialized by the first ezUart_Open() and released by the last
 *           ezUart_Close(). The component counts the open handles. A port whose
 *           UART is already configured (for example by a generated start-up code)
 *           has no initialize and deinitialize function: ezUart_Open() then only
 *           binds the handle. An ezUart_Open() which fails leaves nothing behind, so
 *           the call can simply be repeated.

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
 *           name and the config pointer of ezUart_GetConfig(). The buffers, buffer
 *           sizes and timeouts are passed as they are to the HW driver, which is
 *           responsible for validating them. Every operation requires an open
 *           handle.
 *
 *           Configuration: the configuration belongs to the port. The HW driver
 *           implementation stores it in the HW driver (ezUartDriver::config) before
 *           it registers the driver, and applies it in its initialize function. It
 *           is not an argument of ezUart_Open(). Read it with ezUart_GetConfig(),
 *           which makes no HW call, and apply a change with ezUart_UpdateConfig(),
 *           which calls the HW driver.
 *
 *           Completion: this component does not subscribe to the driver, so
 *           EZ_DRV_EVENT_ASYNC events are not delivered to a handle. The end of
 *           an async operation is delivered through the callback given to
 *           ezUart_Open().
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
 *           first ezUart_Open(), because that call checks drv_handle.hw to see
 *           whether the handle is already open. Open it with ezUart_Open() before
 *           use. The driver keeps a pointer to the handle in base.user while an
 *           operation is in flight, so the handle must stay valid until
 *           ezUart_Close() returns EZ_DRV_STATUS_OK.
 */
typedef struct
{
    struct ezDrvHandle  drv_handle;     /**< Common handle, bound to a HW driver. Filled by ezUart_Open() */
} ezUartHandle_t;


/****************************************************************************/
/* List of API implemented by the HW driver. Users are not supposed to use
 * these API.
 */

/** @brief Initialize the HW UART. Called by ezUart_Open() for the first open handle.
 *
 *  @details Optional: without it, the UART is considered ready and ezUart_Open()
 *           only binds the handle. Synchronous: the returned status is passed
 *           back to the caller. The UART component adds no wait. Whether the
 *           function blocks is up to the HW driver. Not called again until every
 *           handle is closed. The HW driver applies its own ezUartDriver::config.
 *
 *  @param[in]  driver_h: opaque pointer of the HW driver, ezHwUartInterface.driver_h
 *  @return     EZ_DRV_STATUS_OK if success, otherwise EZ_DRV_STATUS_xxx
 */
typedef EZ_DRV_STATUS (*ezHwUart_Initialize)(void *driver_h);

/** @brief Deinitialize the HW UART. Called by ezUart_Close() for the last open handle.
 *
 *  @details Optional: without it, nothing is released. Synchronous: the returned
 *           status is passed back to the caller. Whether the function blocks is
 *           up to the HW driver.
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
 *           EZ_DRV_STATUS_ERR_INF_NOT_EXIST, except initialize and deinitialize:
 *           without them the UART is considered ready (already configured) and
 *           nothing is released. Each function is called with driver_h
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
    ezHwUart_Initialize     initialize;      /**< Initialize the UART peripheral, for the first open handle (synchronous). NULL: already configured */
    ezHwUart_Deinitialize   deinitialize;    /**< Deinitialize the UART peripheral, for the last closed handle (synchronous). NULL: nothing to release */
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
 *           config must be filled before ezUart_SystemRegisterHwDriver(), which
 *           resets base.state, base.user and open_count and keeps config.
 */
struct ezUartDriver
{
    struct Node                 ll_node;     /**< Node to link to the list of HW drivers */
    struct ezDrvBase            base;        /**< Common data of driver: name, state and the handle in use */
    struct ezUartConfiguration  config;      /**< Uart related configuration, filled by the HW driver implementation before registration */
    struct ezHwUartInterface    interface;   /**< HW API, see ezHwUartInterface */
    uint8_t                     open_count;  /**< Number of handles open on this driver. The UART is initialized while it is not 0. Reset to 0 on register */
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
*          driver is reset to the idle, closed state: open_count is 0,
*          base.state is 0 and base.user is NULL. config is kept. It is then added to the
*          list of HW drivers. base.name is compared with the names of the
*          drivers already registered.
*
* @param[in]    hw_uart_driver: HW driver. base.name must be set and unique.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if hw_uart_driver or its name is NULL, or the
*               name is already registered.
*
* @post base.state is 0, base.user is NULL and open_count is 0
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
*          the list of HW drivers. base.state and base.user are not changed. The
*          driver is refused while a handle is open on it, so no handle keeps a
*          pointer to a driver which is gone.
*
* @param[in]    hw_uart_driver: HW driver registered earlier.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if hw_uart_driver is NULL,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver, or a
*               handle is still open on it (open_count is not 0).
*
* @post On EZ_DRV_STATUS_OK the driver is not in the list
* @see ezUart_SystemRegisterHwDriver, ezUart_Close
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
* Function: ezUart_Open
*//**
* @brief Bind a handle to a registered HW driver and make the UART ready.
*
* @details One call replaces "bind the handle" and "initialize the UART". The
*          driver is taken for the call and released after it.
*          - If no handle is open on the driver yet and the HW driver has an
*            initialize function, it is called. The UART is initialized with the
*            configuration of the HW driver (ezUartDriver::config).
*          - If a handle is already open, the UART is already initialized and the
*            HW driver is not called.
*          Then the HW driver, the callback and the context are stored in the
*          handle and the handle is counted as open.
*
*          If the HW initialization fails, the handle is left unbound, so the
*          call can simply be repeated.
*
*          Whether it blocks depends on the HW driver. The UART component adds no
*          wait, but the HW function is synchronous, so it may block.
*
* @param[in]    inst: Handle owned by the caller. Zero-initialized and not open.
*                     Must stay valid while open.
* @param[in]    driver_name: Name of the HW driver (base.name).
* @param[in]    callback: Called when an async operation of this handle ends. Can be
*                         NULL, then the end of the operation is not reported.
*                         Runs in the context of the HW driver, usually an ISR.
* @param[in]    ctx: User context, passed to callback as first argument. Can be NULL.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if inst or driver_name is NULL, or the
*               handle is already open,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if no HW driver has the name,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver,
*               EZ_DRV_STATUS_ERR_GENERIC if the maximum number of open handles
*               (255) is reached, or the HW driver status otherwise.
*
* @post On EZ_DRV_STATUS_OK the handle is open, the UART is initialized and the
*       driver is idle. Otherwise the handle is not bound and the open count is
*       unchanged.
* @see ezUart_Close, ezUartDriver, ezHwUartInterface, ezDrvCallback
* @warning The callback runs inside ezDriver_NotifyEvent(), in the context of
*          the HW driver, usually an ISR. It must not block.
* @warning The UART component is not a lock. Call it from one task, or let the
*          upper layer serialize the calls on the same HW driver.
*
* @isr_safe{no}
* @blocking{yes}
*
* \b Example
* @code
* static ezUartHandle_t uart0_handle;   // Static: must stay valid while open
*
* EZ_DRV_STATUS OpenUart0(void)
* {
*     EZ_DRV_STATUS status = ezUart_Open(&uart0_handle, "uart0", OnUartEvent, NULL);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;               // Nothing is left bound: the call can be repeated
*     }
*     return EZ_DRV_STATUS_OK;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_Open(ezUartHandle_t *inst,
                          const char *driver_name,
                          ezDrvCallback callback,
                          void *ctx);


/*****************************************************************************
* Function: ezUart_Close
*//**
* @brief Unbind a handle from its HW driver.
*
* @details Clears hw, on_event and ctx of the handle and counts the handle as
*          closed. When it was the last open handle of the driver, the
*          deinitialize function of the HW driver is called, if it has one.
*          Refused only when this handle has an operation in flight. An operation
*          of another handle does not block it.
*
*          The handle is unbound even if the HW driver reports an error. The error
*          is returned after the cleanup is finished. A handle which is not open
*          is accepted and stays empty.
*
* @param[in]    inst: Handle opened with ezUart_Open(), or never opened.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if inst is NULL,
*               EZ_DRV_STATUS_BUSY if this handle has an operation in flight,
*               or the HW driver status of deinitialize otherwise.
*
* @post On EZ_DRV_STATUS_OK, and on a HW error, the handle is not open.
* @see ezUart_Open
* @warning The UART component is not a lock. Call it from one task, or let the
*          upper layer serialize the calls on the same HW driver.
*
* @isr_safe{no}
* @blocking{yes}
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_Close(ezUartHandle_t *inst);


/*****************************************************************************
* Function: ezUart_TransmitAsync
*//** 
* @brief Start to transmit data. Return as soon as the transfer is started or refused.
*
* @details Returns as soon as the HW driver has started, or refused, the
*          transfer. The handle must be open (see ezUart_Open). On
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
* @param[in]    inst: Open handle.
* @param[in]    tx_buff: Data to transmit. Must stay valid until the end event.
* @param[in]    buff_size: Number of bytes to transmit. Not checked by this component.
* @return       EZ_DRV_STATUS_OK if the transfer is started,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not open,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no
*               transmit_async function, or the HW driver status otherwise.
*
* @pre ezUart_Open() succeeded for inst.
* @post On EZ_DRV_STATUS_OK the driver is busy until the end event is reported
* @warning tx_buff must stay valid until the end event is reported. The UART
*          component does not copy it.
* @warning The callback runs in the context of the HW driver, usually an ISR.
*          It must not block.
* @warning The UART component is not a lock. The caller must serialize the calls
*          on the same HW driver.
* @note The buffer and its size are not checked by this component, the HW driver
*       validates them. See ezHwUartInterface.
* @see ezUart_ReceiveAsync, ezUart_Open, ezDriver_NotifyEvent, ezHwUartInterface
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
*          The handle must be open (see ezUart_Open).
*
* @param[in]    inst: Open handle.
* @param[out]   rx_buff: Buffer for the received data. Must stay valid until the end event.
* @param[in]    buff_size: Number of bytes to receive. Not checked by this component.
* @return       Same as ezUart_TransmitAsync().
*
* @pre ezUart_Open() succeeded for inst.
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
*          be open (see ezUart_Open). A sync operation reports no
*          callback: its result is only the return value.
*
* @param[in]    inst: Open handle.
* @param[in]    tx_buff: Data to transmit. Not checked by this component.
* @param[in]    buff_size: Number of bytes to transmit. Not checked by this component.
* @param[in]    timeout_millis: Timeout in milliseconds, passed to the HW driver.
* @return       EZ_DRV_STATUS_OK if success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not open,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no
*               transmit_sync function, or the HW driver status otherwise (passed
*               through unchanged, e.g. EZ_DRV_STATUS_TIMEOUT).
*
* @pre ezUart_Open() succeeded for inst.
* @warning The UART component is not a lock. The caller must serialize the calls
*          on the same HW driver.
* @note The buffer and its size are not checked by this component, the HW driver
*       validates them. See ezHwUartInterface.
* @see ezUart_TransmitAsync, ezUart_Open, ezHwUartInterface
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
* @param[in]    inst: Open handle.
* @param[out]   rx_buff: Buffer for the received data. Not checked by this component.
* @param[in]    buff_size: Number of bytes to receive. Not checked by this component.
* @param[in]    timeout_millis: Timeout in milliseconds, passed to the HW driver.
* @return       Same as ezUart_TransmitSync(), with receive_sync instead of transmit_sync.
*
* @pre ezUart_Open() succeeded for inst.
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
*          the busy state. The values are applied to the HW by
*          ezUart_UpdateConfig(). The pointer refers to the driver object, so it
*          stays valid while the driver is registered.
*
* @param[in]    inst: Open handle.
* @param[out]   config: Receives the pointer to the configuration. Must not be NULL.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not open,
*               EZ_DRV_STATUS_ERR_ARG if config is NULL.
*
* @pre ezUart_Open() succeeded for inst.
* @post None
* @see ezUart_UpdateConfig, ezUart_Open, ezUartConfiguration
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
*          only driver_h.
*
* @param[in]    inst: Open handle.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not open,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no update_conf
*               function, or the HW driver status otherwise.
*
* @pre ezUart_Open() succeeded for inst.
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
