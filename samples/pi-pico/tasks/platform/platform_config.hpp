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
* @details The steps run in this order. The first two stop the function on failure:
*  -# Platform::instance().Setup() with the static memory, mutexes and wake signal
*     of platform_config.cpp.
*  -# ezEventBus_CreateListener() and Platform::RegisterListener() for the OnEvent callback.
*  -# Setup() and Init() of the sensor, climate control and logging tasks. Their results are
*     not checked.
*  -# xTaskCreate() of the main, sensor, alive, climate control and logging tasks. Their results
*     are not checked.
*
*  If Platform::Setup() succeeded and a later step of the first two fails, the platform stays
*  configured. Nothing is rolled back. pi-pico.cpp calls this function before vTaskStartScheduler().
*
* @return       bool
*
* @retval true   Platform configured, listener registered and the tasks created.
*                The results of xTaskCreate() are not checked, so a task that failed to be
*                created is not reported.
* @retval false  Platform::Setup() failed, or the listener could not be created or registered
*
* @pre None
* @post On true the platform is configured, the listener is registered and the five sample
*       tasks are created. On false after a successful Platform::Setup() the platform stays
*       configured without the listener.
* @warning Call it once. Each call runs Platform::Setup() again, which re-creates the event bus
*          and loses the registered listeners, and creates the five tasks again.
* @isr_safe{no}
* @blocking{yes}
*   Platform::RegisterListener() waits for the event bus mutex for up to lock_timeout_ms.
*   The sample sets lock_timeout_ms to 0, so the default of 500 ms applies.
*   The Init() functions of the sensor, climate control and logging tasks also call
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
