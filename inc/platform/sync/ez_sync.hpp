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
 *           call back into the platform.
 */
class Mutex
{
public:
    virtual ~Mutex() = default;

    /** @param timeout_ms   Max time to wait, EZ_SEM_NO_WAIT or EZ_SEM_WAIT_FOREVER are allowed
     *  @return true if the lock is taken
     */
    virtual bool Lock(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) = 0;

    /** @brief Release the lock, only the task that took it may call it */
    virtual bool Unlock() = 0;

    Mutex() = default;
    Mutex(const Mutex&) = delete;
    Mutex& operator=(const Mutex&) = delete;
};

/** @brief Binary signal: "something happened"
 *  @details Starts cleared. Notify() sets it, Wait() blocks until it is set and clears it.
 *           Several Notify() calls before a Wait() count as one. Any task or interrupt may notify.
 */
class Signal
{
public:
    virtual ~Signal() = default;

    /** @param timeout_ms   Max time to wait
     *  @return true if the signal was set, false on timeout
     */
    virtual bool Wait(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) = 0;

    /** @brief Set the signal and wake up the waiting task, call it from a task */
    virtual bool Notify() = 0;

    /** @brief Set the signal and wake up the waiting task, call it from an interrupt */
    virtual bool NotifyFromISR() = 0;

    Signal() = default;
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;
};

} // namespace platform
} // namespace ez


#endif /* EZ_PLATFORM */

/* End of file */
