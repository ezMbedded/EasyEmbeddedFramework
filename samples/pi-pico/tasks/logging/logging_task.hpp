/** @file   logging_task.hpp
 *  @brief  Platform task that logs the platform events through a ring buffer
 *
 *  @details Sample application task of the pi-pico project. It uses the platform
 *  layer (ez::platform::Task and Platform) and the ring buffer utility. The ring
 *  buffer memory is a member array; the task does not allocate.
 */
#pragma once

#include "ez_task.hpp"
#include "ez_platform.hpp"
#include "ez_ring_buffer.h"

namespace ez { namespace application {

using namespace ez::platform;

/** @brief Platform task that logs the platform events
 *
 *  @details The task derives from platform::Task. Setup() configures it with the task
 *  mutex, Init() registers an event listener on the platform event bus. The listener
 *  copies the heater start/stop, cooler start/stop and temperature events into a ring
 *  buffer. TaskEntry() pops them and writes one log line per event.
 */
class LoggingTask : public platform::Task
{
public:
    /** @brief Default constructor
     *  @details Only the default member initialization is done. The task is not
     *           configured: call Setup() (inherited from platform::Task), then Init().
     *
     *  @isr_safe{no}
     *  @blocking{no}
     */
    LoggingTask() noexcept = default;

    /** @brief Default destructor
     *  @details Has no custom cleanup. The task owns no memory: the mutex and the
     *           platform objects belong to the caller and the platform.
     *
     *  @isr_safe{no}
     *  @blocking{no}
     */
    ~LoggingTask() = default;

    /** @brief Register the task on the platform and initialize its ring buffer
     *  @details Creates the event listener and registers it on the platform event bus,
     *           then initializes ring_buff over the task buffer. On success the task is
     *           initialized and running, and TaskEntry() can be called.
     *
     *  @return     PlatformResult
     *
     *  @retval PlatformResult::OK                  Task initialized
     *  @retval PlatformResult::TaskNotConfigured   Setup() has not succeeded
     *  @retval PlatformResult::Failed              The listener could not be created, or
     *                                              Platform::RegisterListener() did not return OK,
     *                                              or the ring buffer could not be initialized
     *
     *  @pre Setup() succeeded and Platform::instance().Setup() succeeded
     *  @post On OK the task is initialized and running
     *  @warning Calling it again is not guarded: the ring buffer is re-initialized, so the
     *           stored events are discarded.
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Setup, TaskEntry
     *
     *  \b Example
     *  @code
     *  if(logging.Init() != PlatformResult::OK)
     *  {
     *      HandleError();
     *  }
     *  @endcode
     */
    PlatformResult Init(void);

    /** @brief Clear the running flag
     *  @details Only sets the flag to false. Nothing in this file reads the flag, so
     *           TaskEntry() and Run() keep handling the events after Stop().
     *
     *  @warning Currently has no effect on the logging, see details.
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see Start
     */
    void Stop(void);

    /** @brief Set the running flag
     *  @details Only sets the flag to true. Nothing in this file reads the flag, see Stop().
     *  @isr_safe{no}
     *  @blocking{no}
     *  @see Stop
     */
    void Start(void);

    /** @brief Entry point of the logging task, call it from the RTOS task loop
     *  @details Logs an error and returns if Setup() or Init() was not successful. Otherwise
     *           calls Run(), which pops the stored events and logs each one, until Pop()
     *           returns false (buffer empty, lock timeout or short read).
     *           The check reads the object that Init() stored in a static pointer, so a call
     *           before a successful Init() dereferences a null pointer.
     *
     *  @param[in]  context: Passed on to Run(), not used by this task
     *
     *  @pre Init() succeeded
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Init
     *
     *  \b Example
     *  @code
     *  static void logging_task(void *arg)
     *  {
     *      while(1)
     *      {
     *          logging.TaskEntry(nullptr);
     *          vTaskDelay(pdMS_TO_TICKS(100));
     *      }
     *  }
     *  @endcode
     */
    static void TaskEntry(void *context);

    RingBuffer ring_buff{};     /**< Ring buffer over the task buffer, set up by Init(). Use Push() and Pop() */

    /** @brief Push data into the ring buffer under the task mutex
     *  @details Takes the task mutex for up to the configured lock timeout, copies size bytes
     *           of data into ring_buff and releases the mutex. The data is copied, so the caller
     *           keeps ownership of its memory. size is passed to the ring buffer as uint16_t.
     *
     *  @param[in]  data: Pointer to the data to copy
     *  @param[in]  size: Number of bytes to copy
     *  @return     true if all size bytes were stored, false otherwise
     *
     *  @retval true    All bytes stored. size 0 also gives true and stores nothing
     *  @retval false   Lock timeout, or less free space than size. The bytes that fit are
     *                  stored and not rolled back
     *
     *  @pre Setup() succeeded; data points to at least size bytes
     *  @post The written byte count of ring_buff increased by the number of bytes stored
     *  @warning A partial write is not rolled back. The bytes that fit stay in ring_buff, so
     *           the later Pop() calls read them as the start of the next record and the records
     *           become misaligned.
     *  @warning size is cast to uint16_t, larger values are truncated.
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Pop
     *
     *  \b Example
     *  @code
     *  uint32_t value = 42U;
     *  if(logging.Push(&value, sizeof(value)) == false)
     *  {
     *      HandleError();
     *  }
     *  @endcode
     */
    bool Push(const void *data, size_t size);

    /** @brief Pop data out of the ring buffer under the task mutex
     *  @details Takes the task mutex for up to the configured lock timeout. If ring_buff is not
     *           empty, copies up to size bytes into data in FIFO order and removes them from
     *           ring_buff. Returns true only when exactly size bytes were popped.
     *
     *  @param[out] data: Receives the popped bytes, must be able to hold size bytes
     *  @param[in]  size: Number of bytes to pop
     *  @return     true if size bytes were popped, false otherwise
     *
     *  @retval true    size bytes popped into data. size 0 on a non-empty buffer also gives true
     *  @retval false   Buffer empty, lock timeout, or fewer than size bytes stored
     *
     *  @pre Setup() succeeded; data can hold at least size bytes
     *  @post The written byte count of ring_buff decreased by the number of bytes popped
     *  @warning When fewer than size bytes are stored, those bytes are still removed from
     *           ring_buff and copied into data, but the function returns false.
     *  @warning size is cast to uint16_t, larger values are truncated.
     *  @isr_safe{no}
     *  @blocking{yes}
     *  @see Push
     *
     *  \b Example
     *  @code
     *  uint32_t value = 0U;
     *  if(logging.Pop(&value, sizeof(value)) == true)
     *  {
     *      UseValue(value);
     *  }
     *  @endcode
     */
    bool Pop(void *data, size_t size);

protected:
    void Run(void *context);

private:
    bool running = true;
    uint8_t buff[256];
};

}}