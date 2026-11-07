/*****************************************************************************
* Filename:         ez_spi.h
* Author:           Hai Nguyen
* Original Date:    24.08.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_spi.h
 *  @author Hai Nguyen
 *  @date   24.08.2025
 *  @brief  Hardware Abstraction Layer for SPI communication
 *
 *  @details The user binds an ezSpiHandle_t to a HW driver by name. All the
 *           operations of a handle go through the HW driver registered with
 *           ezSpi_SystemRegisterHwDriver().
 *
 *           Concurrency: this component does not provide mutual exclusion.
 *           The caller must serialize the calls on the same HW driver (mutex
 *           or semaphore of the upper layer). A call made while another
 *           operation is in flight returns EZ_DRV_STATUS_BUSY.
 *
 *           Sync operations return when the transfer is done. Async
 *           operations return as soon as the transfer is started. The HW
 *           driver reports the end of an async operation by calling
 *           ezDriver_NotifyEvent(&hw_driver->base, EZ_DRV_EVENT_OP_DONE,
 *           EZ_SPI_EVENT_xxx, NULL, NULL), usually from its ISR. The driver
 *           is busy until then, and the handle callback is invoked after the
 *           driver is released, so it may start the next operation.
 *
 *           No function of this component is ISR-safe. From an ISR, the HW
 *           driver only calls ezDriver_NotifyEvent().
 *
 *           Argument checking: this component only checks the handle. The
 *           buffers (NULL, size) and lengths (zero, too large) of the
 *           transfer functions are passed as they are to the HW driver, which
 *           is responsible for validating them, because only the HW driver
 *           knows what is valid for its peripheral (e.g. a zero-length
 *           transfer, or rx_length different from tx_length).
 */

#ifndef _EZ_SPI_H
#define _EZ_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_SPI == 1)
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

/** @brief SPI mode
 */
typedef enum
{
    EZ_SPI_MODE_MASTER,  /**< SPI master mode */
    EZ_SPI_MODE_SLAVE,   /**< SPI slave mode */
} EZ_SPI_MODE;


/** @brief SPI clock polarity and phase
 */
typedef enum
{
    EZ_SPI_CPOL0_CPHA0,  /**< CPOL=0, CPHA=0 */
    EZ_SPI_CPOL0_CPHA1,  /**< CPOL=0, CPHA=1 */
    EZ_SPI_CPOL1_CPHA0,  /**< CPOL=1, CPHA=0 */
    EZ_SPI_CPOL1_CPHA1,  /**< CPOL=1, CPHA=1 */
} EZ_SPI_CLOCK_MODE;


/** @brief SPI bit order
 */
typedef enum
{
    EZ_SPI_BIT_ORDER_MSB_FIRST,  /**< Most significant bit first */
    EZ_SPI_BIT_ORDER_LSB_FIRST,  /**< Least significant bit first */
} EZ_SPI_BIT_ORDER;


/** @brief Event codes passed to the handle callback as event_code
 *
 *  @details The HW driver chooses the code and reports it with
 *           ezDriver_NotifyEvent(&drv->base, EZ_DRV_EVENT_OP_DONE, event_code,
 *           NULL, NULL) when an async operation ends. The SPI component defines
 *           no parameters: ezDriver_NotifyEvent() forwards param1 and param2 as
 *           the HW driver passes them.
 */
typedef enum
{
    EZ_SPI_EVENT_CMPLT,     /**< The async operation is completed */
    EZ_SPI_EVENT_ERR,       /**< The async operation failed */
    EZ_SPI_EVENT_TIMEOUT,   /**< The async operation timed out */
    EZ_SPI_NUM_EVENTS,      /**< Number of event codes. Not an event */
} EZ_SPI_EVENT;


/** @brief SPI configuration
 *
 *  @details Passed by pointer to the HW initialize function, unchanged. The
 *           SPI component does not check the values or copy the structure.
 *           The HW driver decides which values it supports.
 */
typedef struct
{
    EZ_SPI_MODE mode;                /**< SPI mode (master/slave) */
    EZ_SPI_CLOCK_MODE clock_mode;    /**< SPI clock mode */
    EZ_SPI_BIT_ORDER bit_order;      /**< SPI bit order (MSB/LSB first) */
    uint32_t clock_frequency;        /**< SPI clock frequency in Hz */
    uint8_t data_size;               /**< Data size in bits (usually 8 or 16) */
} ezSpiConfig_t;


/** @brief Initialize the SPI peripheral. Called by ezSpi_Initialize().
 *
 *  @details Synchronous: the returned status is passed back to the caller.
 *           The SPI component adds no wait. Whether the function blocks is up
 *           to the HW driver. The driver is marked initialized only when the
 *           function returns EZ_DRV_STATUS_OK.
 */
typedef EZ_DRV_STATUS (*ezHwSpi_Initialize)(void *driver_h, ezSpiConfig_t *config);

/** @brief Transmit and receive at the same time, return when done. Called by ezSpi_TransferSync().
 *
 *  @details Synchronous: return the final status. timeout_millis is passed
 *           unchanged, the HW driver is responsible for the timeout.
 */
typedef EZ_DRV_STATUS (*ezHwSpi_TransferSync)(void *driver_h,
                                              const uint8_t *tx_data,
                                              size_t tx_length,
                                              uint8_t *rx_data,
                                              size_t rx_length,
                                              uint32_t timeout_millis);

/** @brief Start to transmit and receive at the same time, return immediately. Called by ezSpi_TransferAsync().
 *
 *  @details Asynchronous. On EZ_DRV_STATUS_OK the transfer is started and the
 *           HW driver must later call
 *           ezDriver_NotifyEvent(&drv->base, EZ_DRV_EVENT_OP_DONE, EZ_SPI_EVENT_xxx, NULL, NULL)
 *           exactly once when it ends. On any other status the transfer is not
 *           started, and the HW driver must not call ezDriver_NotifyEvent().
 *           tx_data and rx_data must stay valid until the end event.
 */
typedef EZ_DRV_STATUS (*ezHwSpi_TransferAsync)(void *driver_h,
                                               const uint8_t *tx_data,
                                               size_t tx_length,
                                               uint8_t *rx_data,
                                               size_t rx_length);

/** @brief Transmit, return when done. Called by ezSpi_TransmitSync().
 *
 *  @details Synchronous: return the final status. timeout_millis is passed
 *           unchanged, the HW driver is responsible for the timeout.
 */
typedef EZ_DRV_STATUS (*ezHwSpi_TransmitSync)(void *driver_h,
                                              const uint8_t *tx_data,
                                              size_t length,
                                              uint32_t timeout_millis);

/** @brief Start to transmit, return immediately. Called by ezSpi_TransmitAsync().
 *
 *  @details Asynchronous. Same rules as ezHwSpi_TransferAsync().
 */
typedef EZ_DRV_STATUS (*ezHwSpi_TransmitAsync)(void *driver_h,
                                               const uint8_t *tx_data,
                                               size_t length);

/** @brief Receive, return when done. Called by ezSpi_ReceiveSync().
 *
 *  @details Synchronous: return the final status. timeout_millis is passed
 *           unchanged, the HW driver is responsible for the timeout.
 */
typedef EZ_DRV_STATUS (*ezHwSpi_ReceiveSync)(void *driver_h,
                                             uint8_t *rx_data,
                                             size_t length,
                                             uint32_t timeout_millis);

/** @brief Start to receive, return immediately. Called by ezSpi_ReceiveAsync().
 *
 *  @details Asynchronous. Same rules as ezHwSpi_TransferAsync().
 */
typedef EZ_DRV_STATUS (*ezHwSpi_ReceiveAsync)(void *driver_h,
                                              uint8_t *rx_data,
                                              size_t length);

/** @brief Control the chip select line, return when done. Called by ezSpi_ChipSelect().
 *
 *  @details Synchronous: return the final status. select is true to select
 *           the device, false to deselect it.
 */
typedef EZ_DRV_STATUS (*ezHwSpi_ChipSelect)(void *driver_h,
                                            bool select);


/** @brief HW API provided by the HW driver. Unused functions can be NULL.
 *
 *  @details A NULL function makes the matching ezSpi_xxx() return
 *           EZ_DRV_STATUS_ERR_INF_NOT_EXIST. The HW functions are called with
 *           driver_h as the first argument.
 *
 *           The buffer pointers and lengths are not checked by the SPI
 *           component. Each HW function must validate them and return
 *           EZ_DRV_STATUS_ERR_ARG when they are not valid for the peripheral.
 *
 *           An async function returns EZ_DRV_STATUS_OK only if the transfer
 *           is started. The HW driver must then call
 *           ezDriver_NotifyEvent(&drv->base, EZ_DRV_EVENT_OP_DONE, EZ_SPI_EVENT_xxx, NULL, NULL)
 *           exactly once when it ends. If it does not start the transfer, it
 *           must return an error and must not call ezDriver_NotifyEvent().
 *           See ezSpi_SystemRegisterHwDriver() for an example.
 */
struct ezHwSpiInterface
{
    void                   *driver_h;       /**< Opaque pointer of the HW driver, passed to every function below */
    ezHwSpi_Initialize     initialize;      /**< Initialize the SPI peripheral (synchronous) */
    ezHwSpi_TransferSync   transfer_sync;   /**< Transmit and receive at the same time (synchronous) */
    ezHwSpi_TransferAsync  transfer_async;  /**< Transmit and receive at the same time (asynchronous) */
    ezHwSpi_TransmitSync   transmit_sync;   /**< Transmit (synchronous) */
    ezHwSpi_TransmitAsync  transmit_async;  /**< Transmit (asynchronous) */
    ezHwSpi_ReceiveSync    receive_sync;    /**< Receive (synchronous) */
    ezHwSpi_ReceiveAsync   receive_async;   /**< Receive (asynchronous) */
    ezHwSpi_ChipSelect     chip_select;     /**< Control the chip select line (synchronous) */
};


/** @brief SPI HW driver. Provided by the HW driver implementation.
 *
 *  @details base.name must be set before ezSpi_SystemRegisterHwDriver().
 *           The driver is linked into the list of registered drivers, so its
 *           memory (static storage, no malloc) must stay valid while it is
 *           registered. ezSpi_SystemRegisterHwDriver() resets base.state,
 *           base.user and initialized.
 */
struct ezSpiDriver
{
    struct Node                 ll_node;     /**< Node to link to the list of HW drivers */
    struct ezDrvBase            base;        /**< Common data of driver: name, state and the handle in use */
    struct ezHwSpiInterface     interface;   /**< HW API, see ezHwSpiInterface */
    bool                        initialized; /**< true after ezSpi_Initialize() succeeded. Reset to false on register and unregister */
};


/** @brief SPI handle. Held by the user.
 *
 *  @details Must be zero-initialized (static storage, or memset) before the
 *           first ezSpi_RegisterInstance(). Bind it with
 *           ezSpi_RegisterInstance() before use. The driver keeps
 *           a pointer to the handle in base.user while an operation is in
 *           flight, so the handle must stay valid until
 *           ezSpi_UnregisterInstance() returns EZ_DRV_STATUS_OK.
 */
typedef struct
{
    struct ezDrvHandle  drv_handle;     /**< Common handle, bound to a HW driver. Filled by ezSpi_RegisterInstance() */
} ezSpiHandle_t;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function: ezSpi_SystemRegisterHwDriver
*//**
* @brief Register a HW driver to the SPI component.
*
* @details Called by the HW driver implementation, not by the application. The
*          driver is reset to the idle, not-initialized state: initialized is
*          false, base.state is 0 and base.user is NULL. It is then added to the
*          list of HW drivers. base.name is compared with the names of the
*          drivers already registered.
*
* @param[in]    hw_driver: HW driver. base.name must be set and unique.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if hw_driver or its name is NULL, or the
*               name is already registered.
*
* @post base.state is 0, base.user is NULL and initialized is false
* @see ezSpi_SystemUnregisterHwDriver, ezHwSpiInterface
* @warning The driver list is changed without a lock. Call it from task
*          context only, and not while another SPI call runs.
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* // HW driver side, at startup. The driver object is static (no malloc).
* static struct ezSpiDriver spi0_drv;
*
* EZ_DRV_STATUS RegisterSpi0(void)
* {
*     EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_GENERIC;
*
*     spi0_drv.base.name = "spi0";                      // Must be unique
*     spi0_drv.interface.driver_h = &spi0_regs;         // Given to every HW function
*     spi0_drv.interface.initialize = Spi0_Initialize;
*     spi0_drv.interface.transfer_sync = Spi0_TransferSync;
*     spi0_drv.interface.transfer_async = Spi0_TransferAsync;
*     spi0_drv.interface.transmit_sync = NULL;          // Unused: ezSpi_TransmitSync() returns EZ_DRV_STATUS_ERR_INF_NOT_EXIST
*     spi0_drv.interface.transmit_async = NULL;
*     spi0_drv.interface.receive_sync = NULL;
*     spi0_drv.interface.receive_async = NULL;
*     spi0_drv.interface.chip_select = NULL;
*
*     status = ezSpi_SystemRegisterHwDriver(&spi0_drv);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;
*     }
*     return EZ_DRV_STATUS_OK;
* }
*
* // In the ISR of the SPI peripheral, when an async transfer ends
* ezDriver_NotifyEvent(&spi0_drv.base, EZ_DRV_EVENT_OP_DONE, EZ_SPI_EVENT_CMPLT, NULL, NULL);
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezSpi_SystemRegisterHwDriver(struct ezSpiDriver *hw_driver);


/*****************************************************************************
* Function: ezSpi_SystemUnregisterHwDriver
*//**
* @brief Unregister a HW driver from the SPI component.
*
* @details Called by the HW driver implementation. The driver is removed from
*          the list of HW drivers and initialized is set to false. base.state and
*          base.user are not changed. The code does not check the handles: a
*          handle still bound to the driver keeps its pointer to it.
*
* @param[in]    hw_driver: HW driver registered earlier.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if hw_driver is NULL,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver.
*
* @pre Every handle bound to the driver has been unregistered with
*      ezSpi_UnregisterInstance(). Not checked by the code.
* @see ezSpi_SystemRegisterHwDriver, ezSpi_UnregisterInstance
* @warning The driver list is changed without a lock. Call it from task
*          context only, and not while another SPI call runs.
*
* @isr_safe{no}
* @blocking{no}
*
*****************************************************************************/
EZ_DRV_STATUS ezSpi_SystemUnregisterHwDriver(struct ezSpiDriver *hw_driver);


/*****************************************************************************
* Function: ezSpi_RegisterInstance
*//**
* @brief Bind a handle to a registered HW driver.
*
* @details Stores the HW driver, the callback and the context in the handle.
*          The driver is not marked busy by this call. A handle which is
*          already bound is refused: call ezSpi_UnregisterInstance() first.
*          The handle must be zero-initialized before the first call.
*
* @param[in]    inst: Handle owned by the caller. Zero-initialized and not
*                      bound. Must stay valid while bound.
* @param[in]    driver_name: Name of the HW driver (base.name).
* @param[in]    callback: Called when an async operation ends. Can be NULL,
*                         then the end of the operation is not reported.
*                         Runs in the context of the HW driver, usually an ISR.
* @param[in]    ctx: User context, passed to callback as first argument. Can be NULL.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if inst or driver_name is NULL, or the
*               handle is already bound,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if no HW driver has the name.
*
* @see ezSpi_UnregisterInstance, ezDrvCallback
* @warning The callback runs inside ezDriver_NotifyEvent(), in the context of
*          the HW driver, usually an ISR. It must not block.
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* static ezSpiHandle_t spi0_handle;   // Static: the handle must stay valid while bound
* static AppSpiCtx_t app_ctx;         // Passed to the callback, see ezSpi_TransferAsync()
*
* EZ_DRV_STATUS BindSpi0(void)
* {
*     EZ_DRV_STATUS status = ezSpi_RegisterInstance(&spi0_handle, "spi0", OnSpiEvent, &app_ctx);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;
*     }
*     return EZ_DRV_STATUS_OK;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezSpi_RegisterInstance(ezSpiHandle_t *inst,
                                     const char *driver_name,
                                     ezDrvCallback callback,
                                     void *ctx);


/*****************************************************************************
* Function: ezSpi_UnregisterInstance
*//**
* @brief Unbind a handle from its HW driver.
*
* @details Clears hw, on_event and ctx of the handle. Refused only when this
*          handle has an operation in flight. An operation of another handle
*          does not block it.
*
* @param[in]    inst: Handle registered with ezSpi_RegisterInstance().
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if inst is NULL,
*               EZ_DRV_STATUS_BUSY if this handle has an operation in flight.
*
* @see ezSpi_RegisterInstance
*
* @isr_safe{no}
* @blocking{no}
*
*****************************************************************************/
EZ_DRV_STATUS ezSpi_UnregisterInstance(ezSpiHandle_t *inst);


/*****************************************************************************
* Function: ezSpi_Initialize
*//**
* @brief Initialize the SPI peripheral of the HW driver.
*
* @details Calls the initialize function of the HW driver with driver_h and
*          config. The config is passed as is, the SPI component does not check
*          or copy it. The driver is taken for the call and released after it.
*          It is marked initialized only when the HW function returns
*          EZ_DRV_STATUS_OK, and that status is returned. Calling it again on an
*          initialized driver calls the HW function again.
*
*          Whether it blocks depends on the HW driver. The SPI component adds no
*          wait, but the HW initialize function is synchronous (there is no
*          async variant), so it may block.
*
* @param[in]    inst: Registered handle.
* @param[in]    config: SPI configuration.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_ARG if config is NULL,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if the handle is not registered,
*               EZ_DRV_STATUS_BUSY if an operation is in flight,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no
*               initialize function, or the HW driver status otherwise.
*
* @pre ezSpi_RegisterInstance() succeeded for inst.
* @post On EZ_DRV_STATUS_OK the driver is initialized and idle.
* @see ezSpiConfig_t, ezHwSpi_Initialize
*
* @isr_safe{no}
* @blocking{yes}
*
* \b Example
* @code
* static ezSpiConfig_t spi0_config = {
*     EZ_SPI_MODE_MASTER,
*     EZ_SPI_CPOL0_CPHA0,
*     EZ_SPI_BIT_ORDER_MSB_FIRST,
*     1000000U,   // 1 MHz
*     8U,         // 8 bits per frame
* };
*
* EZ_DRV_STATUS InitSpi0(void)
* {
*     EZ_DRV_STATUS status = ezSpi_Initialize(&spi0_handle, &spi0_config);
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;
*     }
*     return EZ_DRV_STATUS_OK;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezSpi_Initialize(ezSpiHandle_t *inst, ezSpiConfig_t *config);


/*****************************************************************************
* Function: ezSpi_TransferSync
*//**
* @brief Transmit and receive at the same time. Return when done.
*
* @details Blocks the caller until the HW driver returns. timeout_millis is
*          passed unchanged to the HW driver, which enforces it. The driver is
*          taken for the call and released after the HW call returns, whatever
*          the status.
*
* @param[in]    inst: Registered handle.
* @param[in]    tx_data: Data to transmit.
* @param[in]    tx_length: Number of bytes to transmit.
* @param[out]   rx_data: Buffer for the received data.
* @param[in]    rx_length: Number of bytes to receive.
* @param[in]    timeout_millis: Timeout in milliseconds.
* @return       EZ_DRV_STATUS_OK on success,
*               EZ_DRV_STATUS_ERR_DRV_NOT_FOUND if the handle is not registered,
*               EZ_DRV_STATUS_ERR_NOT_INITIALIZED if ezSpi_Initialize() has not
*               succeeded,
*               EZ_DRV_STATUS_BUSY if an operation is in flight on the driver,
*               EZ_DRV_STATUS_ERR_INF_NOT_EXIST if the HW driver has no such
*               function, or the HW driver status otherwise (e.g. EZ_DRV_STATUS_TIMEOUT).
*
* @pre ezSpi_Initialize() succeeded for the driver of inst.
* @warning The SPI component is not a lock. The caller must serialize the calls
*          on the same HW driver.
* @note The buffers and lengths are not checked by this component, the HW
*       driver validates them. See ezHwSpiInterface.
* @see ezSpi_TransferAsync, ezSpi_Initialize
*
* @isr_safe{no}
* @blocking{yes}
*
* \b Example
* @code
* uint8_t tx[2] = {0x9FU, 0x00U};
* uint8_t rx[2] = {0U, 0U};
* EZ_DRV_STATUS status = ezSpi_TransferSync(&spi0_handle, tx, sizeof(tx), rx, sizeof(rx), 100U);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezSpi_TransferSync(ezSpiHandle_t *inst,
                                 const uint8_t *tx_data,
                                 size_t tx_length,
                                 uint8_t *rx_data,
                                 size_t rx_length,
                                 uint32_t timeout_millis);


/*****************************************************************************
* Function: ezSpi_TransferAsync
*//**
* @brief Start to transmit and receive at the same time. Return immediately.
*
* @details Returns as soon as the HW driver has started, or refused, the
*          transfer. On EZ_DRV_STATUS_OK the transfer is running and the driver
*          stays busy. The HW driver must then call
*          ezDriver_NotifyEvent(&drv->base, EZ_DRV_EVENT_OP_DONE, EZ_SPI_EVENT_xxx, NULL, NULL)
*          exactly once when the transfer ends. The callback of the handle is
*          then called with that event_code. The driver is released before the
*          callback runs, so the callback may start the next operation.
*
*          On any other status this call did not start a transfer, so no
*          callback comes for it. The driver is released if this call had
*          taken it. The HW driver must not call ezDriver_NotifyEvent() then.
*
* @param[in]    inst: Registered handle.
* @param[in]    tx_data: Data to transmit.
* @param[in]    tx_length: Number of bytes to transmit.
* @param[out]   rx_data: Buffer for the received data.
* @param[in]    rx_length: Number of bytes to receive.
* @return       Same as ezSpi_TransferSync().
*
* @pre ezSpi_Initialize() succeeded for the driver of inst.
* @post On EZ_DRV_STATUS_OK the driver is busy until the end event is reported
* @warning tx_data and rx_data must stay valid until the end event is reported.
*          The SPI component does not copy them.
* @warning The callback runs in the context of the HW driver, usually an ISR.
*          It must not block.
* @warning The SPI component is not a lock. The caller must serialize the calls
*          on the same HW driver.
* @note The buffers and lengths are not checked by this component, the HW
*       driver validates them. See ezHwSpiInterface.
* @see ezSpi_RegisterInstance, ezDriver_NotifyEvent, ezHwSpiInterface
*
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* typedef struct
* {
*     volatile bool rx_done;   // Set by the callback
* } AppSpiCtx_t;
*
* static AppSpiCtx_t app_ctx;     // Given to the callback as ctx
* static uint8_t tx_buf[4];
* static uint8_t rx_buf[4];
*
* // Runs in the context of the HW driver, usually an ISR. Must not block.
* static void OnSpiEvent(void *ctx, uint8_t event_code, void *param1, void *param2)
* {
*     AppSpiCtx_t *app = (AppSpiCtx_t *)ctx;
*     if(event_code == EZ_SPI_EVENT_CMPLT)
*     {
*         app->rx_done = true;
*     }
* }
*
* EZ_DRV_STATUS StartRead(void)
* {
*     EZ_DRV_STATUS status = ezSpi_TransferAsync(&spi0_handle, tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf));
*     if(status != EZ_DRV_STATUS_OK)
*     {
*         return status;               // Not started: no callback will come
*     }
*     return EZ_DRV_STATUS_OK;         // Running: OnSpiEvent() gets the end event
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezSpi_TransferAsync(ezSpiHandle_t *inst,
                                  const uint8_t *tx_data,
                                  size_t tx_length,
                                  uint8_t *rx_data,
                                  size_t rx_length);


/*****************************************************************************
* Function: ezSpi_TransmitSync
*//**
* @brief Transmit data. Return when done.
*
* @details Same rules as ezSpi_TransferSync(), without receive data.
*
* @param[in]    inst: Registered handle.
* @param[in]    tx_data: Data to transmit.
* @param[in]    length: Number of bytes to transmit.
* @param[in]    timeout_millis: Timeout in milliseconds.
* @return       Same as ezSpi_TransferSync().
*
* @pre ezSpi_Initialize() succeeded for the driver of inst.
* @note The buffers and lengths are not checked by this component, the HW
*       driver validates them. See ezHwSpiInterface.
* @see ezSpi_TransmitAsync, ezSpi_TransferSync
*
* @isr_safe{no}
* @blocking{yes}
*
*****************************************************************************/
EZ_DRV_STATUS ezSpi_TransmitSync(ezSpiHandle_t *inst,
                                 const uint8_t *tx_data,
                                 size_t length,
                                 uint32_t timeout_millis);


/*****************************************************************************
* Function: ezSpi_TransmitAsync
*//**
* @brief Start to transmit data. Return immediately.
*
* @details See ezSpi_TransferAsync() for the busy, callback and buffer rules.
*
* @param[in]    inst: Registered handle.
* @param[in]    tx_data: Data to transmit.
* @param[in]    length: Number of bytes to transmit.
* @return       Same as ezSpi_TransferSync().
*
* @pre ezSpi_Initialize() succeeded for the driver of inst.
* @warning tx_data must stay valid until the end event is reported.
* @note The buffers and lengths are not checked by this component, the HW
*       driver validates them. See ezHwSpiInterface.
* @see ezSpi_TransferAsync, ezSpi_TransmitSync
*
* @isr_safe{no}
* @blocking{no}
*
*****************************************************************************/
EZ_DRV_STATUS ezSpi_TransmitAsync(ezSpiHandle_t *inst,
                                  const uint8_t *tx_data,
                                  size_t length);


/*****************************************************************************
* Function: ezSpi_ReceiveSync
*//**
* @brief Receive data. Return when done.
*
* @details Same rules as ezSpi_TransferSync(), without transmit data.
*
* @param[in]    inst: Registered handle.
* @param[out]   rx_data: Buffer for the received data.
* @param[in]    length: Number of bytes to receive.
* @param[in]    timeout_millis: Timeout in milliseconds.
* @return       Same as ezSpi_TransferSync().
*
* @pre ezSpi_Initialize() succeeded for the driver of inst.
* @note The buffers and lengths are not checked by this component, the HW
*       driver validates them. See ezHwSpiInterface.
* @see ezSpi_ReceiveAsync, ezSpi_TransferSync
*
* @isr_safe{no}
* @blocking{yes}
*
*****************************************************************************/
EZ_DRV_STATUS ezSpi_ReceiveSync(ezSpiHandle_t *inst,
                                uint8_t *rx_data,
                                size_t length,
                                uint32_t timeout_millis);


/*****************************************************************************
* Function: ezSpi_ReceiveAsync
*//**
* @brief Start to receive data. Return immediately.
*
* @details See ezSpi_TransferAsync() for the busy, callback and buffer rules.
*
* @param[in]    inst: Registered handle.
* @param[out]   rx_data: Buffer for the received data.
* @param[in]    length: Number of bytes to receive.
* @return       Same as ezSpi_TransferSync().
*
* @pre ezSpi_Initialize() succeeded for the driver of inst.
* @warning rx_data must stay valid until the end event is reported.
* @note The buffers and lengths are not checked by this component, the HW
*       driver validates them. See ezHwSpiInterface.
* @see ezSpi_TransferAsync, ezSpi_ReceiveSync
*
* @isr_safe{no}
* @blocking{no}
*
*****************************************************************************/
EZ_DRV_STATUS ezSpi_ReceiveAsync(ezSpiHandle_t *inst,
                                 uint8_t *rx_data,
                                 size_t length);


/*****************************************************************************
* Function: ezSpi_ChipSelect
*//**
* @brief Control the chip select line.
*
* @details Synchronous: returns the status of the HW chip select function. The
*          SPI component adds no wait. The transfer functions do not change the
*          chip select line, so the caller selects the device before a transfer
*          and deselects it after.
*
* @param[in]    inst: Registered handle.
* @param[in]    select: true to select the device, false to deselect.
* @return       Same as ezSpi_TransferSync().
*
* @pre ezSpi_Initialize() succeeded for the driver of inst.
* @see ezSpi_TransferSync, ezHwSpi_ChipSelect
*
* @isr_safe{no}
* @blocking{yes}
*
* \b Example
* @code
* uint8_t cmd[1] = {0x9FU};
* uint8_t id[3] = {0U, 0U, 0U};
* EZ_DRV_STATUS status = ezSpi_ChipSelect(&spi0_handle, true);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* status = ezSpi_TransferSync(&spi0_handle, cmd, sizeof(cmd), id, sizeof(id), 100U);
*
* // Deselect even if the transfer failed, then report the first error
* EZ_DRV_STATUS cs_status = ezSpi_ChipSelect(&spi0_handle, false);
* if(status != EZ_DRV_STATUS_OK)
* {
*     return status;
* }
* if(cs_status != EZ_DRV_STATUS_OK)
* {
*     return cs_status;
* }
* @endcode
*
*****************************************************************************/
EZ_DRV_STATUS ezSpi_ChipSelect(ezSpiHandle_t *inst, bool select);

#endif /* EZ_SPI == 1 */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_SPI_H */


/* End of file */
