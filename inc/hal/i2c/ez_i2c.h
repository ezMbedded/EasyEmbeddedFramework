/*****************************************************************************
* Filename:         ez_i2c.h
* Author:           Hai Nguyen
* Original Date:    25.05.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_i2c.h
 *  @author Hai Nguyen
 *  @date   25.05.2025
 *  @brief  Hardware Abstraction Layer for I2C communication
 *
 *  @details A HW driver is one I2C bus. A handle is one user of that bus, for
 *           example the driver of one device. The user opens a handle with
 *           ezI2c_Open(), which binds it to the HW driver by name and makes the bus
 *           ready in one step, and closes it with ezI2c_Close(). All the operations
 *           of a handle go through the HW driver registered with
 *           ezI2c_SystemRegisterHwDriver().
 *
 *           Shared bus: several handles can be open on the same HW driver, for
 *           example one per device on the bus. They all share the configuration of
 *           the bus (mode, speed, addressing mode), which belongs to the board
 *           and not to the devices: the HW driver implementation puts it in
 *           ezI2cDriver::config before it registers the driver, and applies it in its
 *           initialize function. A port whose peripheral is already configured (for
 *           example by a generated start-up code) has no initialize function: an
 *           ezI2c_Open() then only binds the handle. The bus is initialized by the
 *           first ezI2c_Open() and released by the last ezI2c_Close(). The component
 *           counts the open handles. An ezI2c_Open() which fails leaves nothing
 *           behind, so the call can simply be repeated. The address of a device is
 *           given with every transfer.

 *           Concurrency: this component does not provide mutual exclusion for
 *           its calls. The caller must serialize the calls on the same HW driver.
 *           A call made while any handle of the same HW driver has an operation
 *           in flight returns EZ_DRV_STATUS_BUSY. A device which needs several
 *           transfers in a row (for example a register address write followed by
 *           a read) reserves the bus for the whole sequence with ezI2c_LockBus() and
 *           ezI2c_UnlockBus(), so that the handles of other tasks or modules cannot
 *           transfer in between. The HW driver provides the waiting (for example an
 *           RTOS mutex). Without an RTOS the lock is a try-lock.

 *           Sync operations return when the transfer is done. Async
 *           operations return as soon as the transfer is started. The HW
 *           driver reports the end of an async operation by calling
 *           ezDriver_NotifyEvent(&hw_driver->base, EZ_DRV_EVENT_OP_DONE,
 *           EZ_I2C_EVENT_xxx, NULL, NULL), usually from its ISR. The driver
 *           is busy until then, and the handle callback is invoked after the
 *           driver is released, so it may start the next operation.
 *           A sync operation reports its result only through the return
 *           value. The HW function of a sync operation must NOT call
 *           ezDriver_NotifyEvent(): such events are dropped and the handle
 *           callback is not invoked. A HW driver which waits for an interrupt
 *           uses its own semaphore or flag for that.
 *
 *           No function of this component is ISR-safe. From an ISR, the HW
 *           driver only calls ezDriver_NotifyEvent().
 *
 *           Argument checking: this component checks the handle, the config
 *           pointer of ezI2c_Open() and the driver name. The buffers,
 *           lengths and addresses are passed as they are to the HW driver,
 *           which is responsible for validating them.
 *
 *           Completion: there is no subscription. The end of an async
 *           operation is delivered through the callback given to
 *           ezI2c_Open(). Users never call ezDriver_Subscribe().
 *           Every operation requires an open handle.
 */

#ifndef _EZ_I2C_H
#define _EZ_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_I2C == 1)
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

/** @brief I2C mode
 *
 *  @details Member of ezI2cConfig_t. The I2C component does not check it. The
 *           HW driver decides which modes it supports.
 */
typedef enum
{
    EZ_I2C_MODE_MASTER,  /**< I2C master mode */
    EZ_I2C_MODE_SLAVE,   /**< I2C slave mode */
} EZ_I2C_MODE;


/** @brief I2C bus speed
 *
 *  @details Member of ezI2cConfig_t. The I2C component does not use it. The
 *           HW driver applies it.
 */
typedef enum
{
    EZ_I2C_SPEED_STANDARD,  /**< Standard speed (100 kHz) */
    EZ_I2C_SPEED_FAST,      /**< Fast speed (400 kHz) */
    EZ_I2C_SPEED_HIGH,      /**< High speed (1 MHz+), depending on HW support */
} EZ_I2C_SPEED;


/** @brief I2C addressing mode
 *
 *  @details Member of ezI2cConfig_t. The I2C component does not use it. The
 *           HW driver interprets the address of each transfer according to it.
 */
typedef enum
{
    EZ_I2C_ADDRESSING_MODE_7BIT,  /**< 7-bit addressing mode */
    EZ_I2C_ADDRESSING_MODE_10BIT, /**< 10-bit addressing mode */
} EZ_I2C_ADDRESSING_MODE;


/** @brief Event codes reported to the handle callback (event_code argument)
 *
 *  @details The HW driver chooses the code and reports it with
 *           ezDriver_NotifyEvent(&drv->base, EZ_DRV_EVENT_OP_DONE, event_code,
 *           NULL, NULL) when an async operation ends. The I2C component defines
 *           no parameters: ezDriver_NotifyEvent() forwards param1 and param2 as
 *           the HW driver passes them.
 */
typedef enum
{
    EZ_I2C_EVENT_CMPLT,     /**< The async operation is completed */
    EZ_I2C_EVENT_ERR,       /**< The async operation failed (e.g. NACK, bus error) */
    EZ_I2C_EVENT_TIMEOUT,   /**< The async operation timed out */
    EZ_I2C_NUM_EVENTS,      /**< Number of event codes. Not an event */
} EZ_I2C_EVENT;


/** @brief I2C bus configuration
 *
 *  @details Member of ezI2cDriver (config). The HW driver implementation fills it
 *           before ezI2c_SystemRegisterHwDriver() and applies it in its initialize
 *           function. The I2C component does not read, check or copy it. All
 *           handles open on the same HW driver share it. The HW driver decides
 *           which values it supports, and may ignore the structure when the
 *           peripheral is configured elsewhere.
 */
typedef struct
{
    EZ_I2C_MODE mode;                /**< I2C mode (master/slave) */
    EZ_I2C_SPEED speed;              /**< I2C speed (standard/fast/high) */
    EZ_I2C_ADDRESSING_MODE addressing_mode; /**< Addressing mode (7-bit/10-bit) */
} ezI2cConfig_t;


/** @brief Initialize the bus of the HW driver. Called by ezI2c_Open() for the first open handle.
 *
 *  @details Optional: without it, the peripheral is considered ready and
 *           ezI2c_Open() only binds the handle. Synchronous: the returned status is
 *           passed back to the caller. The I2C component adds no wait. Whether the
 *           function blocks is up to the HW driver. Not called again until every
 *           handle is closed. The HW driver reads the configuration from its own
 *           ezI2cDriver::config.
 */
typedef EZ_DRV_STATUS (*ezHwI2c_Initialize)(void *driver_h);

/** @brief Release the bus of the HW driver. Called by ezI2c_Close() for the last open handle.
 *
 *  @details Optional: without it, nothing is released. Synchronous: the returned
 *           status is passed back to the caller.
 */
typedef EZ_DRV_STATUS (*ezHwI2c_Deinitialize)(void *driver_h);

/** @brief Take the bus for the caller. Called by ezI2c_LockBus().
 *  @details Blocking, task context only. The HW driver waits up to
 *           timeout_millis until the bus is free, for example on an RTOS mutex.
 *           Return EZ_DRV_STATUS_OK when the bus is taken, EZ_DRV_STATUS_TIMEOUT
 *           when it was not free in time. A HW driver without an RTOS leaves lock
 *           and unlock NULL, see ezI2c_LockBus().
 */
typedef EZ_DRV_STATUS (*ezHwI2c_Lock)(void *driver_h, uint32_t timeout_millis);

/** @brief Give the bus back. Called by ezI2c_UnlockBus().
 *  @details Synchronous. It is called from the task which took the bus.
 */
typedef EZ_DRV_STATUS (*ezHwI2c_Unlock)(void *driver_h);

/** @brief Transmit data, return when done. Called by ezI2c_TransmitSync().
 *
 *  @details Synchronous: return the final status. address, send_stop and
 *           timeout_millis are passed unchanged, the HW driver is responsible
 *           for them. See ezHwI2cInterface for their meaning.
 */
typedef EZ_DRV_STATUS (*ezHwI2c_TransmitSync)(void *driver_h,
                                              uint16_t address,
                                              const uint8_t *data,
                                              size_t length,
                                              bool send_stop,
                                              uint32_t timeout_millis);

/** @brief Start to transmit data, return immediately. Called by ezI2c_TransmitAsync().
 *
 *  @details Asynchronous. On EZ_DRV_STATUS_OK the transfer is started and the
 *           HW driver must later call
 *           ezDriver_NotifyEvent(&drv->base, EZ_DRV_EVENT_OP_DONE, EZ_I2C_EVENT_xxx, NULL, NULL)
 *           exactly once when it ends. On any other status the transfer is not
 *           started, and the HW driver must not call ezDriver_NotifyEvent().
 *           data must stay valid until the end event.
 */
typedef EZ_DRV_STATUS (*ezHwI2c_TransmitAsync)(void *driver_h,
                                               uint16_t address,
                                               const uint8_t *data,
                                               size_t length,
                                               bool send_stop);

/** @brief Receive data, return when done. Called by ezI2c_ReceiveSync().
 *
 *  @details Synchronous: return the final status. address, send_stop and
 *           timeout_millis are passed unchanged, the HW driver is responsible
 *           for them. See ezHwI2cInterface for their meaning.
 */
typedef EZ_DRV_STATUS (*ezHwI2c_ReceiveSync)(void *driver_h,
                                             uint16_t address,
                                             uint8_t *data,
                                             size_t length,
                                             bool send_stop,
                                             uint32_t timeout_millis);

/** @brief Start to receive data, return immediately. Called by ezI2c_ReceiveAsync().
 *
 *  @details Asynchronous. Same rules as ezHwI2c_TransmitAsync(): on
 *           EZ_DRV_STATUS_OK the HW driver must later call
 *           ezDriver_NotifyEvent(&drv->base, EZ_DRV_EVENT_OP_DONE, EZ_I2C_EVENT_xxx, NULL, NULL)
 *           exactly once. On any other status it must not call it.
 *           data must stay valid until the end event.
 */
typedef EZ_DRV_STATUS (*ezHwI2c_ReceiveAsync)(void *driver_h,
                                              uint16_t address,
                                              uint8_t *data,
                                              size_t length,
                                              bool send_stop);

/** @brief Probe the device at an address, return when done. Called by ezI2c_Probe().
 *
 *  @details Synchronous: return the final status. address and timeout_millis
 *           are passed unchanged, the HW driver is responsible for them. What
 *           counts as an answer from the device is decided by the HW driver.
 */
typedef EZ_DRV_STATUS (*ezHwI2c_Probe)(void *driver_h,
                                       uint16_t address,
                                       uint32_t timeout_millis);


/** @brief HW API provided by the HW driver. Unused functions can be NULL.
 *
 *  @details A NULL function makes the matching ezI2c_xxx() return
 *           EZ_DRV_STATUS_ERR_INF_NOT_EXIST, except initialize, deinitialize, lock and
 *           unlock, which are optional: without initialize and deinitialize the
 *           peripheral is considered ready (already configured) and
 *           ezI2c_Open() only binds the handle. lock and unlock come as a pair, see
 *           ezI2c_SystemRegisterHwDriver(). The HW functions are called with
 *           driver_h as the first argument.
 *
 *           The buffer pointers, lengths and addresses are not checked by the
 *           I2C component. Each HW function must validate them and return an
 *           error status when they are not valid. The component forwards that
 *           status unchanged.
 *
 *           Meaning of the transfer arguments, decided by the HW driver:
 *           - address: device address, passed unchanged. How it is
 *             interpreted (7-bit or 10-bit, see addressing_mode) is defined by
 *             the HW driver.
 *           - send_stop: passed unchanged. The I2C component does not use it.
 *             Its meaning is defined by the HW driver.
 *           - timeout_millis: passed unchanged. The HW driver enforces it. Only
 *             the synchronous functions have it. An async timeout is reported
 *             with EZ_I2C_EVENT_TIMEOUT.
 *
 *           An async function returns EZ_DRV_STATUS_OK only if the transfer
 *           is started. The HW driver must then call
 *           ezDriver_NotifyEvent(&drv->base, EZ_DRV_EVENT_OP_DONE, EZ_I2C_EVENT_xxx, NULL, NULL)
 *           exactly once when it ends. If it does not start the transfer, it
 *           must return an error and must not call ezDriver_NotifyEvent().
 *           See ezI2c_SystemRegisterHwDriver() for an example.
 */
struct ezHwI2cInterface
{
    void                   *driver_h;       /**< Opaque pointer of the HW driver, passed to every function below */
    ezHwI2c_Initialize     initialize;      /**< Initialize the bus, for the first open handle (synchronous). NULL: already configured */
    ezHwI2c_Deinitialize   deinitialize;    /**< Release the bus, for the last closed handle (synchronous). NULL: nothing to release */
    ezHwI2c_Lock           lock;            /**< Take the bus, waits (blocking). NULL: no RTOS, ezI2c_LockBus() is a try-lock */
    ezHwI2c_Unlock         unlock;          /**< Give the bus back. NULL together with lock */
    ezHwI2c_TransmitSync   transmit_sync;   /**< Transmit data (synchronous) */
    ezHwI2c_TransmitAsync  transmit_async;  /**< Transmit data (asynchronous) */
    ezHwI2c_ReceiveSync    receive_sync;    /**< Receive data (synchronous) */
    ezHwI2c_ReceiveAsync   receive_async;   /**< Receive data (asynchronous) */
    ezHwI2c_Probe          probe;           /**< Probe the device at an address (synchronous) */
};


/** @brief I2C HW driver. Provided by the HW driver implementation.
 *
 *  @details base.name must be set before ezI2c_SystemRegisterHwDriver().
 *           The driver is linked into the list of registered drivers, so its
 *           memory (static storage, no malloc) must stay valid while it is
 *           registered. config must be filled before ezI2c_SystemRegisterHwDriver().
 *           ezI2c_SystemRegisterHwDriver() resets base.state, base.user, open_count
 *           and lock_owner, and keeps config.
 */
struct ezI2cDriver
{
    struct Node                 ll_node;     /**< Node to link to the list of HW drivers */
    struct ezDrvBase            base;        /**< Common data of driver: name, state and the handle in use */
    struct ezHwI2cInterface     interface;   /**< HW API, see ezHwI2cInterface */
    ezI2cConfig_t               config;      /**< Configuration of the bus, filled by the HW driver implementation before registration. Not used by the I2C component */
    uint8_t                     open_count;  /**< Number of handles open on this bus. The bus is initialized while it is not 0. Reset to 0 on register */
    struct ezDrvHandle * volatile lock_owner; /**< Handle which holds the bus lock, NULL if free. Reset to NULL on register */
};


/** @brief I2C handle. Held by the user, one per user of the bus.
 *
 *  @details Must be zero-initialized (static storage, or memset) before the
 *           first ezI2c_Open(). Open it with ezI2c_Open() before use. The driver
 *           keeps a pointer to the handle in base.user while an operation is in
 *           flight and in lock_owner while the handle holds the bus, so the
 *           handle must stay valid until ezI2c_Close() returns EZ_DRV_STATUS_OK.
 */
typedef struct
{
    struct ezDrvHandle  drv_handle;     /**< Common handle, bound to a HW driver. Filled by ezI2c_Open() */
} ezI2cHandle_t;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function: ezI2c_SystemRegisterHwDriver
*//**
* @brief Register a HW driver to the I2C component.
*
* @details Called by the HW driver implementation, not by the application. The
*          driver is reset to the idle, closed state: open_count is 0,
*          lock_owner is NULL, base.state is 0 and base.user is NULL. It is then added to the
*          list of HW drivers. base.name is compared with the names of the
*          drivers already registered.
*
* @param[in]    hw_driver: HW driver. base.name must be set and unique.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if hw_driver or its name is NULL, the
*               name is already registered, or only one of the lock and unlock
*               functions is set.
*
* @post base.state is 0, base.user is NULL, open_count is 0 and lock_owner is NULL
* @see ezI2c_SystemUnregisterHwDriver, ezHwI2cInterface
* @warning The driver list is changed without a lock. Do not call it while
*          another I2C call can run.
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* // HW driver side, at startup. The driver object is static (no malloc).
* static struct ezI2cDriver i2c0_drv;
*
* EZ_DRV_STATUS RegisterI2c0(void)
* {
*     EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_GENERIC;
*
*     i2c0_drv.base.name = "i2c0";                      // Must be unique
*     i2c0_drv.interface.driver_h = &i2c0_regs;         // Given to every HW function
*     i2c0_drv.config.mode = EZ_I2C_MODE_MASTER;        // Bus configuration, applied by I2c0_Initialize()
*     i2c0_drv.config.speed = EZ_I2C_SPEED_FAST;
*     i2c0_drv.config.addressing_mode = EZ_I2C_ADDRESSING_MODE_7BIT;
*     i2c0_drv.interface.initialize = I2c0_Initialize;  // NULL if the peripheral is already configured
*     i2c0_drv.interface.deinitialize = I2c0_Deinitialize;
*     i2c0_drv.interface.lock = I2c0_Lock;              // Without an RTOS: NULL for lock and unlock
*     i2c0_drv.interface.unlock = I2c0_Unlock;
*     i2c0_drv.interface.transmit_sync = I2c0_TransmitSync;
*     i2c0_drv.interface.transmit_async = I2c0_TransmitAsync;
*     i2c0_drv.interface.receive_sync = NULL;           // Unused: ezI2c_ReceiveSync() returns EZ_DRV_STATUS_ERR_INF_NOT_EXIST
*     i2c0_drv.interface.receive_async = NULL;
*     i2c0_drv.interface.probe = I2c0_Probe;
*
*     status = ezI2c_SystemRegisterHwDriver(&i2c0_drv);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;
*     }
*     return EZ_DRV_STATUS_OK;
* }
*
* // In the ISR of the I2C peripheral, when an async transfer ends
* ezDriver_NotifyEvent(&i2c0_drv.base, EZ_DRV_EVENT_OP_DONE, EZ_I2C_EVENT_CMPLT, NULL, NULL);
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezI2c_SystemRegisterHwDriver(struct ezI2cDriver *hw_driver);


/*****************************************************************************
* Function: ezI2c_SystemUnregisterHwDriver
*//**
* @brief Unregister a HW driver from the I2C component.
*
* @details Called by the HW driver implementation. The driver is removed from
*          the list of HW drivers. base.state and base.user are not changed. The
*          driver is refused while a handle is open on it, so no handle keeps a
*          pointer to a driver which is gone.
*
* @param[in]    hw_driver: HW driver registered earlier.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if hw_driver is NULL,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver, or a
*               handle is still open on it (open_count is not 0).
*
* @see ezI2c_SystemRegisterHwDriver, ezI2c_Close
* @warning The driver list is changed without a lock. Do not call it while
*          another I2C call can run.
*
* @isr_safe{no}
* @blocking{no}
*
*****************************************************************************/
EZ_DRV_STATUS ezI2c_SystemUnregisterHwDriver(struct ezI2cDriver *hw_driver);

/*****************************************************************************
* Function: ezI2c_Open
*//**
* @brief Bind a handle to a registered HW driver and make the bus ready.
*
* @details One call replaces "bind the handle" and "initialize the bus". The
*          driver is taken for the call and released after it.
*          - If no handle is open on the driver yet and the HW driver has an
*            initialize function, it is called. The bus is initialized with the
*            configuration of the HW driver (ezI2cDriver::config). The caller does
*            not pass a configuration: all users of one bus share it.
*          - If a handle is already open, the bus is already initialized and the
*            HW driver is not called.
*          Then the HW driver, the callback and the context are stored in the
*          handle and the handle is counted as open.
*
*          If the HW initialization fails, the handle is left unbound, so the
*          call can simply be repeated.
*
*          Whether it blocks depends on the HW driver. The I2C component adds no
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
* @post On EZ_DRV_STATUS_OK the handle is open, the bus is initialized and the
*       driver is idle. Otherwise the handle is not bound and the open count is
*       unchanged.
* @see ezI2c_Close, ezI2cDriver, ezHwI2cInterface, ezDrvCallback
* @warning The callback runs inside ezDriver_NotifyEvent(), in the context of
*          the HW driver, usually an ISR. It must not block.
* @warning The I2C component is not a lock. Call it from one task, or let the
*          upper layer serialize the calls on the same HW driver.
*
* @isr_safe{no}
* @blocking{yes}
*
* \b Example
* @code
* static ezI2cHandle_t i2c0_handle;   // Static: must stay valid while open
* static AppI2cCtx_t app_ctx;         // Passed to the callback, see ezI2c_TransferAsync()
*
* EZ_DRV_STATUS OpenI2c0(void)
* {
*     EZ_DRV_STATUS status = ezI2c_Open(&i2c0_handle, "i2c0", OnI2cEvent, &app_ctx);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;               // Nothing is left bound: the call can be repeated
*     }
*     return EZ_DRV_STATUS_OK;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezI2c_Open(ezI2cHandle_t *inst,
                         const char *driver_name,
                         ezDrvCallback callback,
                         void *ctx);


/*****************************************************************************
* Function: ezI2c_Close
*//**
* @brief Unbind a handle from its HW driver.
*
* @details Clears hw, on_event and ctx of the handle and counts the handle as
*          closed. If the handle holds the bus lock, the lock is released. When
*          it was the last open handle of the driver, the deinitialize function
*          of the HW driver is called, if it has one. Refused only when this handle has an
*          operation in flight. An operation of another handle does not block it.
*
*          The handle is unbound even if the HW driver reports an error. The error
*          is returned after the cleanup is finished. A handle which is not open
*          is accepted and stays empty.
*
* @param[in]    inst: Handle opened with ezI2c_Open(), or never opened.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if inst is NULL,
*               EZ_DRV_STATUS_BUSY if this handle has an operation in flight,
*               or the HW driver status of deinitialize otherwise.
*
* @post On EZ_DRV_STATUS_OK, and on a HW error, the handle is not open.
* @see ezI2c_Open
* @warning The I2C component is not a lock. Call it from one task, or let the
*          upper layer serialize the calls on the same HW driver.
*
* @isr_safe{no}
* @blocking{yes}
*
*****************************************************************************/
EZ_DRV_STATUS ezI2c_Close(ezI2cHandle_t *inst);


/*****************************************************************************
* Function: ezI2c_LockBus
*//**
* @brief Reserve the bus for a handle, for a sequence of several transfers.
*
* @details The bus is shared by all the handles open on the HW driver. A device
*          which needs several transfers in a row (for example a register
*          address write followed by a read) locks the bus first, so that no other
*          handle transfers in between.
*
*          With a lock function in the HW driver, the call waits up to
*          timeout_millis for the bus (for example on an RTOS mutex) and returns
*          EZ_DRV_STATUS_TIMEOUT if it is not free in time. Without one (no RTOS),
*          nothing can wait: the call is a try-lock, timeout_millis is ignored and
*          it returns EZ_DRV_STATUS_BUSY when another handle holds the bus. The
*          caller retries later, for example in the next loop iteration.
*
*          While a handle holds the bus, every transfer of another handle returns
*          EZ_DRV_STATUS_BUSY and the HW is not called. The lock is not recursive.
*          It is independent of the busy state of a single operation: the holder
*          can run any number of transfers.
*
* @param[in]    inst: Open handle.
* @param[in]    timeout_millis: Longest time to wait for the bus. Ignored without a HW lock function.
* @return       EZ_DRV_STATUS_OK when the handle holds the bus,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if the handle is not open,
*               EZ_DRV_STATUS_ERR_ARG if the handle already holds the bus,
*               EZ_DRV_STATUS_BUSY if another handle holds the bus (no HW lock function),
*               EZ_DRV_STATUS_TIMEOUT if the bus was not free in time,
*               or the HW driver status otherwise.
*
* @pre ezI2c_Open() succeeded for inst.
* @post On EZ_DRV_STATUS_OK the handle holds the bus until ezI2c_UnlockBus() or ezI2c_Close().
* @see ezI2c_UnlockBus
* @warning Task or main-loop context only, never from an ISR. Always unlock, also on error paths.
*
* @isr_safe{no}
* @blocking{yes}
*
* \b Example
* @code
* EZ_DRV_STATUS status = ezI2c_LockBus(&i2c0_handle, 100U);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* // ... ezI2c_TransmitSync(register address, send_stop = false), then ezI2c_ReceiveSync() ...
* status = ezI2c_UnlockBus(&i2c0_handle);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezI2c_LockBus(ezI2cHandle_t *inst, uint32_t timeout_millis);


/*****************************************************************************
* Function: ezI2c_UnlockBus
*//**
* @brief Give the bus back.
*
* @details The lock is released first, then the unlock function of the HW driver
*          is called, if it has one. Only the handle which holds the bus can
*          unlock it.
*
* @param[in]    inst: Open handle which holds the bus.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if the handle is not open,
*               EZ_DRV_STATUS_ERR_ARG if the handle does not hold the bus,
*               or the HW driver status of unlock otherwise.
*
* @pre ezI2c_LockBus() succeeded for inst.
* @post The bus is free, also when the HW unlock reports an error.
* @see ezI2c_LockBus
*
* @isr_safe{no}
* @blocking{no}
*
*****************************************************************************/
EZ_DRV_STATUS ezI2c_UnlockBus(ezI2cHandle_t *inst);

/*****************************************************************************
* Function: ezI2c_TransmitSync
*//**
* @brief Transmit data to a device. Return when done.
*
* @details Blocks the caller until the HW driver returns. address, send_stop
*          and timeout_millis are passed unchanged to the HW driver, which
*          enforces the timeout. The driver is taken for the call and released
*          after the HW call returns, whatever the status. See
*          ezHwI2cInterface for the meaning of the arguments.
*
* @param[in]    inst: Open handle.
* @param[in]    address: Address of the device on the bus. Passed unchanged to
*                        the HW driver, which validates it.
* @param[in]    data: Data to transmit. Not checked by this component.
* @param[in]    length: Number of bytes to transmit. Not checked by this component.
* @param[in]    send_stop: Passed unchanged to the HW driver. Its meaning is
*                          defined by the HW driver.
* @param[in]    timeout_millis: Timeout in milliseconds, enforced by the HW driver.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if inst is NULL or not open,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no such
*               function, or the HW driver status otherwise (e.g. EZ_DRV_STATUS_TIMEOUT).
*
* @pre ezI2c_Open() succeeded for inst.
* @warning The I2C component is not a lock. The caller must serialize the calls
*          on the same HW driver.
* @note The buffers, lengths and addresses are not checked by this component,
*       the HW driver validates them. See ezHwI2cInterface.
* @see ezI2c_TransmitAsync, ezI2c_Open, ezHwI2cInterface
*
* @isr_safe{no}
* @blocking{yes}
*
* \b Example
* @code
* uint8_t tx[2] = {0x10U, 0x20U};
* uint16_t address = 0x50U;
* EZ_DRV_STATUS status = ezI2c_TransmitSync(&i2c0_handle, address, tx, sizeof(tx), true, 100U);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezI2c_TransmitSync(ezI2cHandle_t *inst,
                                 uint16_t address,
                                 const uint8_t *data,
                                 size_t length,
                                 bool send_stop,
                                 uint32_t timeout_millis);

/*****************************************************************************
* Function: ezI2c_TransmitAsync
*//**
* @brief Start to transmit data to a device. Return immediately.
*
* @details Returns as soon as the HW driver has started, or refused, the
*          transfer. On EZ_DRV_STATUS_OK the transfer is running and the driver
*          stays busy. The HW driver must then call
*          ezDriver_NotifyEvent(&drv->base, EZ_DRV_EVENT_OP_DONE, EZ_I2C_EVENT_xxx, NULL, NULL)
*          exactly once when the transfer ends. The callback of the handle is
*          then called with that event_code. The driver is released before the
*          callback runs, so the callback may start the next operation.
*
*          On any other status this call did not start a transfer, so no
*          callback comes for it. The driver is released by this call. The HW
*          driver must not call ezDriver_NotifyEvent() then.
*
* @param[in]    inst: Open handle.
* @param[in]    address: Address of the device on the bus. Passed unchanged to
*                        the HW driver, which validates it.
* @param[in]    data: Data to transmit. Must stay valid until the end event.
* @param[in]    length: Number of bytes to transmit. Not checked by this component.
* @param[in]    send_stop: Passed unchanged to the HW driver. Its meaning is
*                          defined by the HW driver.
* @return       Same as ezI2c_TransmitSync().
*
* @pre ezI2c_Open() succeeded for inst.
* @post On EZ_DRV_STATUS_OK the driver is busy until the end event is reported
* @warning data must stay valid until the end event is reported. The I2C
*          component does not copy it.
* @warning The callback runs in the context of the HW driver, usually an ISR.
*          It must not block.
* @warning The I2C component is not a lock. The caller must serialize the calls
*          on the same HW driver.
* @note The buffers, lengths and addresses are not checked by this component,
*       the HW driver validates them. See ezHwI2cInterface.
* @see ezI2c_Open, ezDriver_NotifyEvent, ezHwI2cInterface
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* typedef struct
* {
*     volatile bool done;   // Set by the callback
* } AppI2cCtx_t;
*
* static AppI2cCtx_t app_ctx;     // Given to the callback as ctx, see ezI2c_Open()
* static uint8_t tx_buf[2];
*
* // Runs in the context of the HW driver, usually an ISR. Must not block.
* static void OnI2cEvent(void *ctx, uint8_t event_code, void *param1, void *param2)
* {
*     AppI2cCtx_t *app = (AppI2cCtx_t *)ctx;
*     if(event_code == EZ_I2C_EVENT_CMPLT)
*     {
*         app->done = true;
*     }
* }
*
* EZ_DRV_STATUS StartWrite(void)
* {
*     EZ_DRV_STATUS status = ezI2c_TransmitAsync(&i2c0_handle, 0x50U, tx_buf, sizeof(tx_buf), true);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;               // Not started: no callback will come
*     }
*     return EZ_DRV_STATUS_OK;         // Running: OnI2cEvent() gets the end event
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezI2c_TransmitAsync(ezI2cHandle_t *inst,
                                  uint16_t address,
                                  const uint8_t *data,
                                  size_t length,
                                  bool send_stop);

/*****************************************************************************
* Function: ezI2c_ReceiveSync
*//**
* @brief Receive data from a device. Return when done.
*
* @details Same rules as ezI2c_TransmitSync(), with receive data instead of
*          transmit data. The driver is taken for the call and released after
*          the HW call returns, whatever the status.
*
* @param[in]    inst: Open handle.
* @param[in]    address: Address of the device on the bus. Passed unchanged to
*                        the HW driver, which validates it.
* @param[out]   data: Buffer for the received data. Not checked by this component.
* @param[in]    length: Number of bytes to receive. Not checked by this component.
* @param[in]    send_stop: Passed unchanged to the HW driver. Its meaning is
*                          defined by the HW driver.
* @param[in]    timeout_millis: Timeout in milliseconds, enforced by the HW driver.
* @return       Same as ezI2c_TransmitSync().
*
* @pre ezI2c_Open() succeeded for inst.
* @note The buffers, lengths and addresses are not checked by this component,
*       the HW driver validates them. See ezHwI2cInterface.
* @see ezI2c_ReceiveAsync, ezI2c_TransmitSync, ezHwI2cInterface
*
* @isr_safe{no}
* @blocking{yes}
*
* \b Example
* @code
* uint8_t rx[2] = {0U, 0U};
* uint16_t address = 0x50U;
* EZ_DRV_STATUS status = ezI2c_ReceiveSync(&i2c0_handle, address, rx, sizeof(rx), true, 100U);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezI2c_ReceiveSync(ezI2cHandle_t *inst,
                                uint16_t address,
                                uint8_t *data,
                                size_t length,
                                bool send_stop,
                                uint32_t timeout_millis);

/*****************************************************************************
* Function: ezI2c_ReceiveAsync
*//**
* @brief Start to receive data from a device. Return immediately.
*
* @details Same rules as ezI2c_TransmitAsync(), with receive data instead of
*          transmit data: the driver stays busy on EZ_DRV_STATUS_OK until the HW
*          driver reports EZ_DRV_EVENT_OP_DONE, and the callback then gets the
*          event code.
*
* @param[in]    inst: Open handle.
* @param[in]    address: Address of the device on the bus. Passed unchanged to
*                        the HW driver, which validates it.
* @param[out]   data: Buffer for the received data. Must stay valid until the end event.
* @param[in]    length: Number of bytes to receive. Not checked by this component.
* @param[in]    send_stop: Passed unchanged to the HW driver. Its meaning is
*                          defined by the HW driver.
* @return       Same as ezI2c_TransmitSync().
*
* @pre ezI2c_Open() succeeded for inst.
* @post On EZ_DRV_STATUS_OK the driver is busy until the end event is reported
* @warning data must stay valid until the end event is reported. The I2C
*          component does not copy it.
* @warning The callback runs in the context of the HW driver, usually an ISR.
*          It must not block.
* @warning The I2C component is not a lock. The caller must serialize the calls
*          on the same HW driver.
* @note The buffers, lengths and addresses are not checked by this component,
*       the HW driver validates them. See ezHwI2cInterface.
* @see ezI2c_ReceiveSync, ezI2c_TransmitAsync, ezDriver_NotifyEvent
*
* @isr_safe{no}
* @blocking{no}
*
*****************************************************************************/
EZ_DRV_STATUS ezI2c_ReceiveAsync(ezI2cHandle_t *inst,
                                 uint16_t address,
                                 uint8_t *data,
                                 size_t length,
                                 bool send_stop);

/*****************************************************************************
* Function: ezI2c_Probe
*//**
* @brief Probe the device at an address. Return when done.
*
* @details Synchronous. The probe function of the HW driver is called with
*          address and timeout_millis, and its status is returned. The driver is
*          taken for the call and released after it, whatever the status. What
*          counts as an answer from the device is decided by the HW driver.
*
* @param[in]    inst: Open handle.
* @param[in]    address: Address of the device on the bus. Passed unchanged to
*                        the HW driver, which validates it.
* @param[in]    timeout_millis: Timeout in milliseconds, enforced by the HW driver.
* @return       Same as ezI2c_TransmitSync().
*
* @pre ezI2c_Open() succeeded for inst.
* @note The address is not checked by this component, the HW driver validates
*       it. See ezHwI2cInterface.
* @see ezHwI2c_Probe, ezI2c_Open
*
* @isr_safe{no}
* @blocking{yes}
*
* \b Example
* @code
* uint16_t address = 0x50U;
* EZ_DRV_STATUS status = ezI2c_Probe(&i2c0_handle, address, 20U);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezI2c_Probe(ezI2cHandle_t *inst,
                          uint16_t address,
                          uint32_t timeout_millis);
#endif /* EZ_I2C == 1 */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_I2C_H */


/* End of file */
