#include "climate_control_task.hpp"
#include "platform/platform_data.hpp"

#define DEBUG_LVL   LVL_DEBUG           /**< logging level */
#define MOD_NAME    "climate_control"       /**< module name */
#include "ez_logging.h"


using namespace ez::platform;
using namespace ez::application;

enum StateId : uint16_t
{
    STATE_IDLE = 0,
    STATE_HEATING,
    STATE_COOLING,
};

enum EventId : uint16_t
{
    EVENT_HEATING = 0,
    EVENT_COOLING,
};

static uint8_t sm_buff[256];
static ezSm_StateMachine_t sm;
static ezSm_Config_t sm_config;

int ClimateControlTask::OnEvent(uint32_t event_code, const void *data, size_t data_size)
{
    switch (event_code)
    {
        case EVENT_DATA_POINT_CHANGED:
        {
            if(data_size != sizeof(uint32_t))
            {
                break;
            }
            const uint32_t index = *(uint32_t*)data;
            switch (index)
            {
                case DATA_POINT_TEMPERATURE:
                {
                    float temperature = 0.0f;
                    ezSm_Event_t event = {};
                    bool have_event = false;

                    if(Platform::Get(index, temperature, 100) != PlatformResult::OK)
                    {
                        EZERROR("cannot read temperature");
                        break;
                    }
                    EZINFO("Temperature = %f", temperature);

                    ClimateControlTask &task = instance();
                    task.SetCurrentTemperature(temperature);
                    if(temperature > task.GetTargetTemperature() + offset_temperature)
                    {
                        event.id = EVENT_COOLING;
                        have_event = true;
                    }
                    else if (temperature < task.GetTargetTemperature() - offset_temperature)
                    {
                        event.id = EVENT_HEATING;
                        have_event = true;
                    }

                    if(have_event)
                    {
                        if(!ezSm_PostEvent(&sm, &event))
                        {
                            EZERROR("cannot post event to state machine");
                        }
                        else if(Wake() == false)
                        {
                            EZERROR("cannot wake the climate control task");
                        }
                    }
                    break;
                }
                
                default:
                    break;
            }
            break;
        }
    }
    return 0;
}

static void OnEnterStateIdle(void *context);
static void OnExitStateIdle(void *context);
static void OnEnterStateHeating(void *context);
static void OnExitStateHeating(void *context);
static void OnEnterStateCooling(void *context);
static void OnExitStateCooling(void *context);


static const ezSm_State_t state_idle = {.id = STATE_IDLE, .on_entry = OnEnterStateIdle, .on_exit = OnExitStateIdle};
static const ezSm_State_t state_heating = {.id = STATE_HEATING, .on_entry = OnEnterStateHeating, .on_exit = OnExitStateHeating};
static const ezSm_State_t state_cooling = {.id = STATE_COOLING, .on_entry = OnEnterStateCooling, .on_exit = OnExitStateCooling};

static const ezSm_Transition_t transition[] = {
    {&state_idle,    &state_heating,    nullptr,    nullptr,    EVENT_HEATING},
    {&state_idle,    &state_cooling,    nullptr,    nullptr,    EVENT_COOLING},
    {&state_heating, &state_heating,    nullptr,    nullptr,   EVENT_HEATING},
    {&state_heating, &state_cooling,    nullptr,    nullptr,   EVENT_COOLING},
    {&state_cooling, &state_cooling,    nullptr,    nullptr,   EVENT_COOLING},
    {&state_cooling, &state_heating,    nullptr,    nullptr,   EVENT_HEATING},
};

PlatformResult ClimateControlTask::OnInit()
{
    memset(&sm, 0, sizeof(sm));
    memset(sm_buff, 0, sizeof(sm_buff));

    sm_config.initial_state = &state_idle;
    sm_config.on_transition = nullptr;
    sm_config.on_unhandled = nullptr;
    sm_config.transitions_table = transition;
    sm_config.transitions_count = sizeof(transition) / sizeof(transition[0]);
    this->target_temperature = 20.0f;

    if(ezSm_Init(&sm, &sm_config, nullptr, sm_buff, sizeof(sm_buff)) == false)
    {
        EZERROR("cannot init statemachine");
        return PlatformResult::Failed;
    }

    ezSm_Start(&sm);
    return PlatformResult::OK;
}

void ClimateControlTask::Run()
{
    if(ezSm_Run(&sm) == 0)
    {
        EZTRACE("no event handled");
    }
}

static void OnEnterStateIdle(void *context)
{
    EZINFO("Start machine");
}


static void OnExitStateIdle(void *context)
{

}


static void OnEnterStateHeating(void *context)
{
    EZINFO("curr=%f < target=%f: start heater", ClimateControlTask::instance().GetCurrentTemperature(), ClimateControlTask::instance().GetTargetTemperature());
    if(Platform::SendEvent(EVENT_HEATER_START, nullptr, 0) != PlatformResult::OK)
    {
        EZERROR("Send EVENT_HEATER_START failed");
    }
}


static void OnExitStateHeating(void *context)
{
    EZINFO("Stop heater");
    if(Platform::SendEvent(EVENT_HEATER_STOP, nullptr, 0) != PlatformResult::OK)
    {
        EZERROR("Send EVENT_HEATER_STOP failed");
    }
}


static void OnEnterStateCooling(void *context)
{
    EZINFO("curr=%f > target=%f: start cooler", ClimateControlTask::instance().GetCurrentTemperature(), ClimateControlTask::instance().GetTargetTemperature());
    if(Platform::SendEvent(EVENT_COOLER_START, nullptr, 0) != PlatformResult::OK)
    {
        EZERROR("Send EVENT_COOLER_START failed");
    }
}


static void OnExitStateCooling(void *context)
{
    EZINFO("Stop cooler");
    if(Platform::SendEvent(EVENT_COOLER_STOP, nullptr, 0) != PlatformResult::OK)
    {
        EZERROR("Send EVENT_COOLER_STOP failed");
    }
}
