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
 *  @details The platform needs three interfaces, which are different
 *           types so they cannot be mixed up:
 *           - Mutex: protects data from concurrent access
 *           - Signal: wakes up a task that waits for something to happen
 *           - Delay: puts a task to sleep for a fixed time, nothing can wake it up early
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

/** @brief Timeout value: do not wait */
constexpr uint32_t EZ_SEM_NO_WAIT      = 0;             /**< Timeout of 0 ms, the call does not wait */
/** @brief Timeout value: wait without a time limit */
constexpr uint32_t EZ_SEM_WAIT_FOREVER = 0xFFFFFFFFu;   /**< Wait until the operation succeeds */

/** @brief Mutual exclusion lock
 *  @details The platform uses it from tasks only. The mutex given to the platform for the
 *           event bus MUST be recursive: event listeners run with the lock held and may
 *           call back into the platform. The mutex given for the data model must be a
 *           different object from the event bus mutex: Platform::Setup() rejects the
 *           same object for both, and rejects a NULL mutex.
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
     *           the lock again. With EZ_SEM_NO_WAIT the call does not wait.
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
     *      if (mutex.Unlock() == false)
     *      {
     *          HandleError();
     *      }
     *  }
     *  else
     *  {
     *      HandleError();
     *  }
     *  @endcode
     */
    virtual bool Lock(std::uint32_t timeout_ms = EZ_SEM_NO_WAIT) = 0;

    /** @brief Release the lock, only the task that took it may call it
     *  @details The platform calls it only after its own Lock() succeeded. When it returns false, the
     *           platform task base logs an error (ezTaskCore::Unlock).
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
 *           Several Notify() calls before a Wait() count as one. A task sets it with Notify(),
 *           an interrupt with NotifyFromISR().
 *           The application implements this interface; the caller of Platform::Setup()
 *           owns the object and keeps it alive while the platform uses it.
 */
class Signal
{
public:
    /** @brief Virtual destructor */
    virtual ~Signal() = default;

    /** @brief Wait until the signal is set, then clear it
     *  @details Blocks the calling task until Notify() or NotifyFromISR() sets the signal, or the
     *           timeout expires. With EZ_SEM_NO_WAIT it only checks the signal and returns at once.
     *
     *  @param[in]  timeout_ms: Max time to wait in milliseconds, EZ_SEM_NO_WAIT or
     *              EZ_SEM_WAIT_FOREVER are allowed
     *  @return     true if the signal was set (and is now cleared), false on timeout
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
     *  @details Does not wait. If the signal is already set, the call has no further effect.
     *  @return     true if the signal is set (also when it was already set), false if the signal object is not usable
     *
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see NotifyFromISR
     */
    virtual bool Notify() = 0;

    /** @brief Set the signal and wake up the waiting task, call it from an interrupt
     *  @details Does not wait. The implementation must use the interrupt-safe RTOS call.
     *  @return     true if the signal is set (also when it was already set), false if the signal object is not usable
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

/** @brief Fixed-time sleep of the calling task
 *  @details For tasks that run on a period and need no wake-up. Unlike Signal it holds no
 *           state, so one object can be shared by all tasks. A task configured with a delay
 *           (ezTaskConfiguration_t::delay) calls Sleep() with its task_delay_ms between two
 *           passes of its body; the platform never passes 0 because Init() replaces 0 with the
 *           default. The application implements this interface; its owner keeps the object
 *           alive while it is used.
 */
class Delay
{
public:
    /** @brief Virtual destructor */
    virtual ~Delay() = default;

    /** @brief Put the calling task to sleep
     *  @details Nothing wakes the task before the time has passed. Other tasks can run meanwhile.
     *           The sleep is not ended by a signal, unlike Signal::Wait().
     *
     *  @param[in]  time_ms: Time to sleep in milliseconds. EZ_SEM_WAIT_FOREVER must not be passed,
     *              the platform does not check the value
     *
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Signal::Wait
     *
     *  \b Example
     *  @code
     *  // sleep 100 ms between two passes of a periodic task
     *  delay.Sleep(100U);
     *  @endcode
     */
    virtual void Sleep(std::uint32_t time_ms) = 0;

    /** @brief Default constructor */
    Delay() = default;
    /** @brief Copy is not allowed */
    Delay(const Delay&) = delete;
    /** @brief Copy assignment is not allowed */
    Delay& operator=(const Delay&) = delete;
};

} // namespace platform
} // namespace ez


#endif /* EZ_PLATFORM */

/* End of file */
