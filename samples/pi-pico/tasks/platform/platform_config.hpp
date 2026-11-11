/** @file   platform_config.hpp
 *  @brief  Configuration of the platform and the sample tasks of the Raspberry Pi Pico sample
 *
 *  @details Declares the one function that configures the platform and creates the sample
 *  tasks. It is defined in platform_config.cpp and called from main() in pi-pico.cpp.
 */
#pragma once

/*****************************************************************************
* Function : appPlatformConfig_Init
*//**
* @brief This function configures the platform and creates the sample tasks.
*
* @details The steps run in this order and each one stops the function on failure:
*  -# Platform::instance().Setup() with the static memory, mutexes and wake signal
*     of platform_config.cpp.
*  -# ezEventBus_CreateListener() and Platform::RegisterListener() for the OnEvent callback.
*  -# SensorTask::Init(), ClimateControlTask::Init() and LoggingTask::Init(). One call each
*     validates the configuration, sets the task up and registers its listener.
*  -# xTaskCreate() of the platform, sensor, climate control, logging and alive tasks, with
*     the task type's Entry() as the entry function.
*
*  Nothing is rolled back on a failure. pi-pico.cpp calls this function before vTaskStartScheduler().
*
* @return       bool
*
* @retval true   Platform configured, the tasks initialized and created.
* @retval false  A step failed, see details
*
* @pre None
* @post On true the platform is configured, the listeners are registered and the five sample
*       tasks are created. On false the earlier steps stay done.
* @warning Call it once. Each call runs Platform::Setup() again, which re-creates the event bus
*          and loses the registered listeners, and creates the five tasks again.
* @isr_safe{no}
* @blocking{yes}
*   Platform::RegisterListener() waits for the event bus mutex for up to lock_timeout_ms.
*   The sample sets lock_timeout_ms to 0, so the default of 500 ms applies.
*   The Init() functions of the climate control and logging tasks also call
*   RegisterListener().
*
* \b Example
* @code
* if(appPlatformConfig_Init() == false)
* {
*     HandleError();
* }
* @endcode
*
*****************************************************************************/
bool appPlatformConfig_Init(void);
