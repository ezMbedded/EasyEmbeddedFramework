/** @file   logging_task.hpp
 *  @brief  Platform task that logs the platform events through a ring buffer
 *
 *  @details Sample application task of the pi-pico project. It uses the platform
 *  layer (ez::platform::ezTask and Platform) and the ring buffer utility. The ring
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
 *  @details An event-driven task. OnEvent() runs in the platform task, copies the event into a
 *  ring buffer and wakes this task with Wake(). Run() pops the stored events and writes one log
 *  line per event. The ring buffer is shared by the two tasks, so the configuration must give
 *  the task a mutex.
 *
 *  Only one instance exists, see LoggingTask::instance(). Start it with
 *  LoggingTask::Init(config, LoggingTask::OnEvent) and give LoggingTask::Entry to the RTOS.
 */
class LoggingTask : public platform::ezTask<LoggingTask>
{
    friend class platform::ezTask<LoggingTask>;
public:
    /** @brief Listener callback, give it to LoggingTask::Init(). Runs in the platform task
     *  @param[in]  event_code: Code of the platform event
     *  @param[in]  data: Data of the event
     *  @param[in]  data_size: Size of data in bytes
     *  @return     0
     */
    static int OnEvent(uint32_t event_code, const void *data, size_t data_size);

private:
    LoggingTask() = default;
    ~LoggingTask() = default;

    /** @brief Set up the ring buffer, called by Init() */
    PlatformResult OnInit();

    /** @brief One pass: pop and log all the stored events */
    void Run();

    /** @brief Push data into the ring buffer under the task mutex
     *  @return true if all size bytes were stored
     */
    bool Push(const void *data, size_t size);

    /** @brief Pop data out of the ring buffer under the task mutex
     *  @return true if size bytes were popped
     */
    bool Pop(void *data, size_t size);

    RingBuffer ring_buff{};
    uint8_t buff[256];
};

}}
