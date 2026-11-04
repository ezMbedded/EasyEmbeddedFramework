/*****************************************************************************
* Filename:         ez_sync.hpp
* Author:           Hai Nguyen
* Original Date:    01.11.2026
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_sync.hpp
 *  @author Hai Nguyen
 *  @date   01.11.2026
 *  @brief  Synchronization interfaces the application provides to the platform
 *
 *  @details The platform needs two kinds of synchronization, which are different
 *           types so they cannot be mixed up:
 *           - Mutex: protects data from concurrent access
 *           - Signal: wakes up a task that waits for something to happen
 */
#pragma once


/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_PLATFORM == 1)

#include <cstdint>


namespace ez {
namespace platform
{

constexpr uint32_t EZ_SEM_NO_WAIT      = 0;             /**< Do not wait */
constexpr uint32_t EZ_SEM_WAIT_FOREVER = 0xFFFFFFFFu;   /**< Wait until the operation succeeds */

/** @brief Mutual exclusion lock
 *  @details The platform uses it from tasks only. The mutex given to the platform for the
 *           event bus MUST be recursive: event listeners run with the lock held and may
 *           call back into the platform. The mutex given for the data model must be a
 *           different object from the event bus mutex: Platform::Setup() rejects the
 *           same object for both.
 *           The application implements this interface; the caller of Platform::Setup()
 *           owns the object and keeps it alive while the platform uses it.
 */
class Mutex
{
public:
    /** @brief Virtual destructor */
    virtual ~Mutex() = default;

    /** @brief Take the lock
     *  @details The platform passes its configured lock timeout, or the timeout given to
     *           Platform::TakeDataModel(). A recursive implementation lets the owner take
     *           the lock again.
     *
     *  @param[in]  timeout_ms: Max time to wait in milliseconds, EZ_SEM_NO_WAIT or
     *              EZ_SEM_WAIT_FOREVER are allowed
     *  @return     true if the lock is taken, false on timeout
     *
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Unlock
     *
     *  \b Example
     *  @code
     *  if (mutex.Lock(100U) == true)
     *  {
     *      // access the protected data
     *      mutex.Unlock();
     *  }
     *  @endcode
     */
    virtual bool Lock(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) = 0;

    /** @brief Release the lock, only the task that took it may call it
     *  @return     true if the lock is released, false otherwise
     *
     *  @pre The calling task holds the lock
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see Lock
     */
    virtual bool Unlock() = 0;

    /** @brief Default constructor */
    Mutex() = default;
    /** @brief Copy is not allowed */
    Mutex(const Mutex&) = delete;
    /** @brief Copy assignment is not allowed */
    Mutex& operator=(const Mutex&) = delete;
};

/** @brief Binary signal: "something happened"
 *  @details Starts cleared. Notify() sets it, Wait() blocks until it is set and clears it.
 *           Several Notify() calls before a Wait() count as one. Any task or interrupt may notify.
 *           The application implements this interface; the caller of Platform::Setup()
 *           owns the object and keeps it alive while the platform uses it.
 */
class Signal
{
public:
    /** @brief Virtual destructor */
    virtual ~Signal() = default;

    /** @brief Wait until the signal is set, then clear it
     *
     *  @param[in]  timeout_ms: Max time to wait in milliseconds, EZ_SEM_NO_WAIT or
     *              EZ_SEM_WAIT_FOREVER are allowed
     *  @return     true if the signal was set, false on timeout
     *
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Notify
     *
     *  \b Example
     *  @code
     *  if (signal.Wait(100U) == true)
     *  {
     *      // something happened, handle it
     *  }
     *  @endcode
     */
    virtual bool Wait(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) = 0;

    /** @brief Set the signal and wake up the waiting task, call it from a task
     *  @return     true if the signal is set, false otherwise
     *
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see NotifyFromISR
     */
    virtual bool Notify() = 0;

    /** @brief Set the signal and wake up the waiting task, call it from an interrupt
     *  @return     true if the signal is set, false otherwise
     *
     *  @isr_safe{yes}
     *  @blocking{no}
     *  @see Notify
     */
    virtual bool NotifyFromISR() = 0;

    /** @brief Default constructor */
    Signal() = default;
    /** @brief Copy is not allowed */
    Signal(const Signal&) = delete;
    /** @brief Copy assignment is not allowed */
    Signal& operator=(const Signal&) = delete;
};

} // namespace platform
} // namespace ez


#endif /* EZ_PLATFORM */

/* End of file */
