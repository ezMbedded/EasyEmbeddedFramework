/*****************************************************************************
* Filename:         ez_driver_def.h
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

/** @file   ez_driver_def.h
 *  @author Hai Nguyen
 *  @date   15.03.2024
 *  @brief  Common definitions shared by all HAL driver modules
 *
 *  @details A HW driver embeds an ezDrvBase_t. The user holds an ezDrvHandle_t
 *           which is bound to the HW driver by name. The base routes the
 *           events coming from the HW driver to one handle, the user:
 *           - EZ_DRV_EVENT_OP_PROGRESS and EZ_DRV_EVENT_OP_DONE belong to an
 *             operation in flight (state BUSY).
 *           - EZ_DRV_EVENT_ASYNC events are not tied to an operation (GPIO
 *             edge, UART RX data, bus fault, ...). They are delivered only
 *             while the user is subscribed (state LISTENING).
 *           There is one user per driver at a time. Fan-out to several
 *           consumers is the job of the upper layer.
 *
 *           This component is a primitive layer. It does NOT provide mutual
 *           exclusion. The caller (upper layer) must serialize all calls on
 *           the same driver with a mutex or semaphore. The state is only a
 *           busy flag to detect misuse and a key to route events.
 *
 *           Ownership rules:
 *           - Sync operation: ezDriver_Acquire() before the HW call,
 *             ezDriver_Release(handle, base) right after the HW call returns.
 *           - Async operation: ezDriver_Acquire() before the HW call. If the
 *             HW call fails to start, ezDriver_Release(handle, base).
 *             Otherwise BUSY is kept until the HW driver reports
 *             EZ_DRV_EVENT_OP_DONE with ezDriver_NotifyEvent().
 *           - Listener: ezDriver_Subscribe() / ezDriver_Unsubscribe(). Both
 *             fail (return false) while an operation is in flight.
 *             They are helpers for the module code, not for the user.
 *
 *           Concurrency: the state is a single byte, modified with
 *           read-modify-write. The task side only changes LISTENING while
 *           BUSY is clear, so the ISR never writes the byte at the same
 *           time. Single core only.
 */

#ifndef _EZ_DRIVER_DEF_H
#define _EZ_DRIVER_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_DRIVER == 1)
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ez_linked_list.h"  /* Not used here. Kept because the HAL module headers use struct Node and rely on this include */

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/

/** @brief State bit: an operation is in flight on the driver.
 *
 *  @details Set by ezDriver_Acquire(). Cleared by ezDriver_Release() and by
 *           ezDriver_NotifyEvent() with EZ_DRV_EVENT_OP_DONE.
 */
#define EZ_DRV_STATE_BUSY           (0x01U)

/** @brief State bit: the user is subscribed to EZ_DRV_EVENT_ASYNC events.
 *
 *  @details Set by ezDriver_Subscribe(). Cleared by ezDriver_Unsubscribe().
 *           Not changed by ezDriver_Acquire(), ezDriver_Release() or
 *           ezDriver_NotifyEvent().
 */
#define EZ_DRV_STATE_LISTENING      (0x02U)

/** @brief Check if the state bit(s) are set. ISR-safe: yes
 *
 *  @param[in]  base: Pointer to ezDrvBase_t
 *  @param[in]  flag: One or more EZ_DRV_STATE_* bits. true if ANY of them is set
 */
#define EZ_DRV_STATE_IS_SET(base, flag) \
    ((((base)->state) & (uint8_t)(flag)) != 0U)

/** @brief Set the state bit(s). Read-modify-write, not atomic. ISR-safe: yes
 *
 *  @param[in]  base: Pointer to ezDrvBase_t
 *  @param[in]  flag: One or more EZ_DRV_STATE_* bits
 */
#define EZ_DRV_STATE_SET(base, flag) \
    ((base)->state = (uint8_t)(((base)->state) | (uint8_t)(flag)))

/** @brief Clear the state bit(s). Read-modify-write, not atomic. ISR-safe: yes
 *
 *  @param[in]  base: Pointer to ezDrvBase_t
 *  @param[in]  flag: One or more EZ_DRV_STATE_* bits
 */
#define EZ_DRV_STATE_CLEAR(base, flag) \
    ((base)->state = (uint8_t)(((base)->state) & (uint8_t)(~(uint8_t)(flag))))


/*****************************************************************************
* Component Typedefs
*****************************************************************************/

/** @brief Return status of the Driver API
 *
 *  @details Returned by the modules which use the driver base, e.g. the
 *           status of a call to the driver.
 */
typedef enum
{
    EZ_DRV_STATUS_OK,                   /**< OK, working as expected */
    EZ_DRV_STATUS_BUSY,                 /**< Driver is busy: an operation is in flight, or another handle is subscribed */
    EZ_DRV_STATUS_TIMEOUT,              /**< Operation timeout */
    EZ_DRV_STATUS_ERR_GENERIC,          /**< Generic error */
    EZ_DRV_STATUS_ERR_ARG,              /**< Invalid function argument */
    EZ_DRV_STATUS_ERR_DRV_NOT_FOUND,    /**< HW driver implementation not found */
    EZ_DRV_STATUS_ERR_INF_NOT_EXIST,    /**< Implementation of an interface not found */
    EZ_DRV_STATUS_ERR_NOT_INITIALIZED,  /**< The driver is not initialized */
}EZ_DRV_STATUS;


/** @brief Callback to receive event from the HW implementation
 *
 *  @details Called in the context of the HW driver, usually an ISR, by
 *           ezDriver_NotifyEvent(). For EZ_DRV_EVENT_OP_DONE, BUSY is already
 *           cleared when the callback runs, so it may start a new operation.
 *
 *  @param[in]   ctx: user context, the ctx field of the ezDrvHandle_t
 *  @param[in]   event_code: event code, defined by the driver module
 *  @param[in]   param1: pointer to the first parameter. Depending on event_code
 *  @param[in]   param2: pointer to the second parameter. Depending on event_code
 *
 *  @warning The callback runs inside ezDriver_NotifyEvent(), in the same
 *           context. It must not block.
 */
typedef void (*ezDrvCallback)(void *ctx, uint8_t event_code, void *param1, void *param2);


/** @brief Kind of an event reported by the HW driver. It decides how the event is routed.
 *
 *  @details See ezDriver_NotifyEvent() for the routing rules.
 */
typedef enum
{
    EZ_DRV_EVENT_OP_PROGRESS,   /**< Event of the operation in flight, BUSY is kept (e.g. half transfer). Delivered only while BUSY is set */
    EZ_DRV_EVENT_OP_DONE,       /**< Final event of the operation in flight (done, error, timeout). BUSY is cleared before the callback. Delivered only while BUSY is set */
    EZ_DRV_EVENT_ASYNC,         /**< Event not tied to an operation (edge, RX data, bus fault). Delivered only while LISTENING is set */
}ezDrvEventKind_t;


/** @brief Define a driver handle. It is held by the user and bound to a HW driver.
 *
 *  @details The module fills the fields when the handle is bound to a HW
 *           driver. The caller owns the memory of the handle.
 */
typedef struct ezDrvHandle
{
    ezDrvCallback   on_event;   /**< Callback function to handle the event from the HW driver. Can be NULL */
    void            *ctx;       /**< User context, passed back as the first argument of on_event. Can be NULL */
    void            *hw;        /**< Pointer to the bound HW driver, depending on the implementation. NULL when not bound */
}ezDrvHandle_t;


/** @brief Define structure holding the base data shared by all HW drivers
 *
 *  @details The field order is intended: state fills the padding after
 *           version on 32-bit targets.
 */
typedef struct ezDrvBase
{
    const char*                 name;       /**< Name of the HW driver, used by the module to find it */
    uint8_t                     version[3]; /**< Version number including major, minor, patch */
    volatile uint8_t            state;      /**< EZ_DRV_STATE_* bits. Written from ISR by ezDriver_NotifyEvent(), hence volatile */
    ezDrvHandle_t * volatile    user;       /**< Handle which receives the events. Written only by task code: ezDriver_Acquire(), ezDriver_Subscribe(), and reset to NULL when the HW driver is registered. Not cleared by ezDriver_Release() or ezDriver_Unsubscribe(). Valid only while BUSY or LISTENING is set */
}ezDrvBase_t;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */


/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function: ezDriver_GetHw
*//**
* @brief Return the HW driver which the handle is bound to.
*
* @details Helper function used by other components. The user is not
*          supposed to use this function.
*
* @param[in]    handle: Driver handle bound to a HW driver. @see ezDrvHandle_t
* @return       The hw field of the handle, or NULL if handle is NULL or hw is NULL
*
* @pre None
* @post None
*
* \b Example
* @code
* struct ezSpiDriver *drv = (struct ezSpiDriver *)ezDriver_GetHw(&inst->drv_handle);
* if(drv == NULL)
* {
*     return EZ_DRV_STATUS_ERR_DRV_NOT_FOUND;
* }
* @endcode
*
* @isr_safe{yes}
* @blocking{no}
*
*****************************************************************************/
static inline void *ezDriver_GetHw(ezDrvHandle_t *handle)
{
    void *drv = NULL;
    if(handle != NULL && handle->hw != NULL)
    {
        drv = handle->hw;
    }

    return drv;
}


/*****************************************************************************
* Function: ezDriver_Acquire
*//**
* @brief Mark the driver as busy with an operation of the handle.
*
* @details Not a lock. The caller must already be serialized by the upper
*          layer. Fails if an operation is already in flight (even from the
*          same handle), or if another handle is subscribed to the driver.
*          A handle which is subscribed to the driver can start an operation,
*          and its subscription is kept.
*
* @param[in]    handle: Driver handle
* @param[in]    base: Pointer to the base structure of the driver.
* @return       true if the handle can start an operation, else false (busy,
*               subscribed by another handle, or invalid argument)
*
* @pre None
* @post On true, BUSY is set and base->user == handle
*
* \b Example
* @code
* struct ezSpiDriver *drv = (struct ezSpiDriver *)ezDriver_GetHw(&inst->drv_handle);
* EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_GENERIC;
*
* if(!ezDriver_Acquire(&inst->drv_handle, &drv->base))
* {
*     return EZ_DRV_STATUS_BUSY;
* }
* status = drv->interface.transmit_sync(drv->interface.driver_h, tx_data, length, timeout_millis);
* if(!ezDriver_Release(&inst->drv_handle, &drv->base))
* {
*     // Not released by this call, the driver was not held by this handle
* }
* @endcode
*
* @isr_safe{yes}
* @blocking{no}
*
*****************************************************************************/
static inline bool ezDriver_Acquire(ezDrvHandle_t *handle,
                                    struct ezDrvBase *base)
{
    if(handle == NULL || base == NULL)
    {
        return false;
    }

    if(EZ_DRV_STATE_IS_SET(base, EZ_DRV_STATE_BUSY))
    {
        return false;
    }

    if(EZ_DRV_STATE_IS_SET(base, EZ_DRV_STATE_LISTENING) && base->user != handle)
    {
        return false;
    }

    base->user = handle;
    EZ_DRV_STATE_SET(base, EZ_DRV_STATE_BUSY);
    return true;
}


/*****************************************************************************
* Function: ezDriver_Release
*//**
* @brief Clear BUSY if the handle is the one with the operation in flight.
*
* @details Use it after a sync operation, or when an async operation fails to
*          start. A started async operation is released by
*          ezDriver_NotifyEvent() with EZ_DRV_EVENT_OP_DONE. A handle can
*          never release an operation owned by another handle, e.g. when the
*          operation already completed and a chained callback started a new
*          one. LISTENING is not changed.
*
* @param[in]    handle: Driver handle which wants to release the driver.
* @param[in]    base: Pointer to the base structure of the driver.
* @return       true if BUSY was cleared by this call, else false (BUSY not set,
*               the driver is held by another handle, or invalid argument)
*
* @pre None
* @post If true, BUSY is clear
*
* \b Example
* @code
* struct ezSpiDriver *drv = (struct ezSpiDriver *)ezDriver_GetHw(&inst->drv_handle);
* EZ_DRV_STATUS status = EZ_DRV_STATUS_ERR_GENERIC;
*
* if(!ezDriver_Acquire(&inst->drv_handle, &drv->base))
* {
*     return EZ_DRV_STATUS_BUSY;
* }
* status = drv->interface.transmit_async(drv->interface.driver_h, tx_data, length);
* if(status != EZ_DRV_STATUS_OK)
* {
*     // The transfer did not start, so EZ_DRV_EVENT_OP_DONE will not come
*     if(!ezDriver_Release(&inst->drv_handle, &drv->base))
*     {
*         // Not released by this call, the driver was not held by this handle
*     }
* }
* // On EZ_DRV_STATUS_OK the HW driver ends the operation with ezDriver_NotifyEvent(EZ_DRV_EVENT_OP_DONE)
* @endcode
*
* @isr_safe{yes}
* @blocking{no}
*
*****************************************************************************/
static inline bool ezDriver_Release(ezDrvHandle_t *handle,
                                    struct ezDrvBase *base)
{
    if(handle == NULL || base == NULL ||
       base->user != handle ||
       !EZ_DRV_STATE_IS_SET(base, EZ_DRV_STATE_BUSY))
    {
        return false;
    }

    EZ_DRV_STATE_CLEAR(base, EZ_DRV_STATE_BUSY);
    return true;
}


/*****************************************************************************
* Function: ezDriver_Subscribe
*//**
* @brief Subscribe the handle to events not tied to an operation.
*
* @details Only routing. Arming the HW (e.g. enable the EXTI line or the RX
*          interrupt) is done by the module using this function. Fails while
*          an operation is in flight, because the ISR may write the state
*          byte at the same time. Subscribing again with the same handle is
*          allowed and does nothing. This is a helper for the module code,
*          not for the user.
*
* @param[in]    handle: Driver handle
* @param[in]    base: Pointer to the base structure of the driver.
* @return       true if the handle is subscribed, else false (busy, another
*               handle is subscribed, or invalid argument)
*
* @pre No operation in flight
* @post On true, LISTENING is set and base->user == handle
* @see ezDriver_Unsubscribe, ezDriver_NotifyEvent
*
* @isr_safe{no}
* @blocking{no}
*
*****************************************************************************/
static inline bool ezDriver_Subscribe(ezDrvHandle_t *handle,
                                      struct ezDrvBase *base)
{
    if(handle == NULL || base == NULL)
    {
        return false;
    }

    if(EZ_DRV_STATE_IS_SET(base, EZ_DRV_STATE_BUSY))
    {
        return false;
    }

    if(EZ_DRV_STATE_IS_SET(base, EZ_DRV_STATE_LISTENING) && base->user != handle)
    {
        return false;
    }

    base->user = handle;
    EZ_DRV_STATE_SET(base, EZ_DRV_STATE_LISTENING);
    return true;
}


/*****************************************************************************
* Function: ezDriver_Unsubscribe
*//**
* @brief Remove the subscription of the handle.
*
* @details Fails while an operation is in flight, for the same reason as
*          ezDriver_Subscribe(). This is a helper for the module code, not for
*          the user.
*
* @param[in]    handle: Driver handle
* @param[in]    base: Pointer to the base structure of the driver.
* @return       true if the subscription is removed, else false (busy, handle
*               is not subscribed, or invalid argument)
*
* @pre No operation in flight
* @post On true, LISTENING is clear
* @see ezDriver_Subscribe
*
* @isr_safe{no}
* @blocking{no}
*
*****************************************************************************/
static inline bool ezDriver_Unsubscribe(ezDrvHandle_t *handle,
                                        struct ezDrvBase *base)
{
    if(handle == NULL || base == NULL ||
       EZ_DRV_STATE_IS_SET(base, EZ_DRV_STATE_BUSY) ||
       !EZ_DRV_STATE_IS_SET(base, EZ_DRV_STATE_LISTENING) ||
       base->user != handle)
    {
        return false;
    }

    EZ_DRV_STATE_CLEAR(base, EZ_DRV_STATE_LISTENING);
    return true;
}


/*****************************************************************************
* Function: ezDriver_NotifyEvent
*//**
* @brief Report an event of the HW driver to the user handle.
*
* @details Called by the HW driver implementation, usually from its ISR.
*          Routing by kind:
*          - EZ_DRV_EVENT_OP_PROGRESS: delivered if BUSY, BUSY is kept.
*          - EZ_DRV_EVENT_OP_DONE: delivered if BUSY. BUSY is cleared
*            BEFORE the callback is invoked, so the callback may start a new
*            operation.
*          - EZ_DRV_EVENT_ASYNC: delivered if LISTENING. The state is not
*            changed.
*          An event whose condition is not met is dropped. The event goes to
*          base->user, and only if that handle has a callback (on_event).
*
* @param[in]    base: Pointer to the base structure of the HW driver.
* @param[in]    kind: Kind of the event, decides the routing.
* @param[in]    event_code: Event code, defined by the driver module.
* @param[in]    param1: First event parameter. Depending on event_code.
* @param[in]    param2: Second event parameter. Depending on event_code.
*
* @pre None
* @post If kind is EZ_DRV_EVENT_OP_DONE and BUSY was set on entry, BUSY is
*       clear before the callback runs
* @warning The callback runs inside this call, in the same context. If this
*          call is made from an ISR, the callback must be ISR-safe. It must not
*          block.
* @see ezDrvCallback
*
* \b Example
* @code
* // In the ISR of the HW driver, when the transfer ends
* ezDriver_NotifyEvent(&hw_drv->base, EZ_DRV_EVENT_OP_DONE, EZ_SPI_EVENT_CMPLT, NULL, NULL);
* @endcode
*
* @isr_safe{yes}
* @blocking{no}
*
*****************************************************************************/
static inline void ezDriver_NotifyEvent(struct ezDrvBase *base,
                                        ezDrvEventKind_t kind,
                                        uint8_t event_code,
                                        void *param1,
                                        void *param2)
{
    ezDrvHandle_t *target = NULL;
    uint8_t needed = (kind == EZ_DRV_EVENT_ASYNC) ? EZ_DRV_STATE_LISTENING
                                                  : EZ_DRV_STATE_BUSY;

    if(base == NULL || !EZ_DRV_STATE_IS_SET(base, needed))
    {
        return;
    }

    target = base->user;
    if(kind == EZ_DRV_EVENT_OP_DONE)
    {
        EZ_DRV_STATE_CLEAR(base, EZ_DRV_STATE_BUSY);
    }

    if(target != NULL && target->on_event != NULL)
    {
        target->on_event(target->ctx, event_code, param1, param2);
    }
}

#ifdef __cplusplus
}
#endif

#else
#error "ez_driver_def.h requires EZ_DRIVER == 1 (CMake: ENABLE_EZ_HAL_DRIVER, or ez_target_config.h)"
#endif /* EZ_DRIVER == 1 */
#endif /* _EZ_DRIVER_DEF_H */


/* End of file */
