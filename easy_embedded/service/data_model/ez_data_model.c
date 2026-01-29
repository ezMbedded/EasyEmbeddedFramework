/*****************************************************************************
* Filename:         ez_data_model.c
* Author:           Hai Nguyen
* Original Date:    27.02.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   data_model.c
 *  @author Hai Nguyen
 *  @date   27.02.2024
 *  @brief  Implementation of the data model component
 *
 *  @details
 */


/*****************************************************************************
* Includes
*****************************************************************************/

#if (DATA_MODEL == 1U)
#include <string.h>
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_DATA_MODEL_LOGGING_LEVEL       /**< logging level */
#define MOD_NAME    "DATA_MODEL"    /**< module name */
#include "ez_logging.h"
#include "ez_assert.h"
#include "ez_data_model.h"

/*the rest of include go here*/

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define NUM_OF_DATA_ELEMENT     10   /**< a macro*/

/*****************************************************************************
* Component Typedefs
*****************************************************************************/



/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
#if (DEBUG_LVL > LVL_INFO)
static uint32_t data_point_count = 0U;
#endif


/******************************************************************************
* Function Definitions
*******************************************************************************/
/* None */


/*****************************************************************************
* Function Definitions
*****************************************************************************/
static bool ezDataModel_AllocateDataPoint(ezDataModel_t *data_model);
static size_t ezDataModel_GetRequiredBufferSize(ezDataPointType_t type);
static ezDataPoint_t *ezDataModel_FindDataPointByIndex(
    ezDataModel_t *data_model,
    uint32_t index);


/*****************************************************************************
* Public functions
*****************************************************************************/
void ezDataModel_Initialize(
    ezDataModel_t *data_model,
    ezDataPoint_t *data_points,
    size_t num_of_data_points,
    uint8_t *data_model_buff,
    size_t data_model_buff_size,
    uint8_t *event_buff,
    size_t event_buff_size)
{
    if(data_model == NULL
       || data_points == NULL
       || num_of_data_points == 0U
       || data_model_buff == NULL
       || data_model_buff_size == 0U
       || event_buff == NULL
       || event_buff_size == 0U)
    {
        EZERROR("Invalid argument");
        return;
    }

    data_model->data_points = data_points;
    data_model->num_of_data_points = num_of_data_points;
    data_model->data_model_buff = data_model_buff;
    data_model->data_model_buff_size = data_model_buff_size;
    data_model->event_buff = event_buff;
    data_model->event_buff_size = event_buff_size;

    if(ezDataModel_AllocateDataPoint(data_model) == false)
    {
        EZERROR("Data point allocation failed");
        return;
    }

    if(ezEventBus_CreateBus(&data_model->event_bus,
                             data_model->event_buff,
                             data_model->event_buff_size) != ezSUCCESS)
    {
        EZERROR("Event bus creation failed");
        return;
    }
}


ezSTATUS ezDataModel_SetDataPoint(
    ezDataModel_t *data_model,
    uint32_t index,
    void *data,
    ezDataPointType_t type)
{
    if(data_model == NULL
       || data == NULL)
    {
        EZDEBUG("Invalid argument");
        return ezSTATUS_ARG_INVALID;
    }

    ezDataPoint_t *data_point = ezDataModel_FindDataPointByIndex(data_model, index);
    if(data_point == NULL)
    {
        EZDEBUG("Data point not found");
        return ezFAIL;
    }

    if(data_point->type != type)
    {
        EZDEBUG("Data type mismatch");
        return ezFAIL;
    }

    memcpy(data_point->data, data, data_point->size);
    
    ezEventBus_SendEvent(
        &data_model->event_bus,
        index,
        data_point->data,
        data_point->size);

    return ezSUCCESS;
}


ezSTATUS ezDataModel_GetDataPoint(
    ezDataModel_t *data_model,
    uint32_t index,
    void *data,
    ezDataPointType_t type)
{
        if(data_model == NULL
       || data == NULL)
    {
        EZDEBUG("Invalid argument");
        return ezSTATUS_ARG_INVALID;
    }

    ezDataPoint_t *data_point = ezDataModel_FindDataPointByIndex(data_model, index);
    if(data_point == NULL)
    {
        EZDEBUG("Data point not found");
        return ezFAIL;
    }

    if(data_point->type != type)
    {
        EZDEBUG("Data type mismatch");
        return ezFAIL;
    }

    memcpy(data, data_point->data, data_point->size);
    return ezSUCCESS;
}


ezSTATUS ezDataModel_ListenDataPointChange(
    ezDataModel_t *data_model,
    uint32_t index,
    ezEventListener_t *listener)
{
    if(data_model == NULL
       || listener == NULL)
    {
        EZDEBUG("Invalid argument");
        return ezSTATUS_ARG_INVALID;
    }

    return ezEventBus_Listen(&data_model->event_bus, listener);
}


ezSTATUS ezDataModel_UnlistenDataPointChange(
    ezDataModel_t *data_model,
    uint32_t index,
    ezEventListener_t *listener)
{
    if(data_model == NULL
       || listener == NULL)
    {
        EZDEBUG("Invalid argument");
        return ezSTATUS_ARG_INVALID;
    }

    return ezEventBus_Unlisten(&data_model->event_bus, listener);
}


ezSTATUS ezDataModel_Run(ezDataModel_t *data_model)
{
    if(data_model == NULL)
    {
        EZDEBUG("Invalid argument");
        return ezSTATUS_ARG_INVALID;
    }

    return ezEventBus_Run(&data_model->event_bus);
}


/******************************************************************************
* Internal functions
*******************************************************************************/
static bool ezDataModel_AllocateDataPoint(ezDataModel_t *data_model)
{
    if(data_model == NULL)
    {
        EZERROR("Invalid argument");
        return false;
    }

    size_t remain_byte = data_model->data_model_buff_size;
    uint32_t current_index = data_model->data_points[0].index;
    uint8_t *current_buff_ptr = data_model->data_model_buff;
    size_t required_size = 0;

    for(size_t i = 0U; i < data_model->num_of_data_points; i++)
    {
        if (i > 0)
        {
            ASSERT_CUST_MSG(current_index < data_model->data_points[i].index,
            "current index (%lu) > data point index (%lu)",
            current_index,
            data_model->data_points[i].index);
        }

        required_size = ezDataModel_GetRequiredBufferSize(data_model->data_points[i].type);
        ASSERT_CUST_MSG(required_size <= remain_byte,
            "required size (%lu) > remain byte (%lu)",
            required_size,
            remain_byte);

        data_model->data_points[i].data = (void *)current_buff_ptr;
        data_model->data_points[i].size = required_size;
        current_buff_ptr += required_size;
        remain_byte -= required_size;
        current_index = data_model->data_points[i].index;
    }
    return true;
}

static size_t ezDataModel_GetRequiredBufferSize(ezDataPointType_t type)
{
    switch(type)
    {
        case TYPE_BOOL:
            return sizeof(bool);
        case TYPE_UINT8:
            return sizeof(uint8_t);
        case TYPE_UINT16:
            return sizeof(uint16_t);
        case TYPE_UINT32:
            return sizeof(uint32_t);
        case TYPE_INT8:
            return sizeof(int8_t);
        case TYPE_INT16:
            return sizeof(int16_t);
        case TYPE_INT32:
            return sizeof(int32_t);
        case TYPE_FLOAT:
            return sizeof(float);
        case TYPE_DOUBLE:
            return sizeof(double);
        default:
            EZERROR("Unsupported data type");
            return 0U;
    }
}


static ezDataPoint_t *ezDataModel_FindDataPointByIndex(
    ezDataModel_t *data_model,
    uint32_t index)
{
    if (data_model == NULL)
    {
        EZDEBUG("Invalid argument");
        return NULL;
    }

    size_t left = 0;
    size_t right = data_model->num_of_data_points;
    while (left < right)
    {
        size_t mid = left + (right - left) / 2;
        uint32_t mid_index = data_model->data_points[mid].index;

        if (mid_index == index)
        {
            return &data_model->data_points[mid];
        }
        else if (index < mid_index)
        {
            if (mid == 0)
            {
                break;
            }
            right = mid;
        }
        else
        {
            left = mid + 1;
        }
    }

    EZDEBUG("Data point not found");
    return NULL;
}

#endif /* (CONFIG_DATA_MODEL == 1U) */

/* End of file*/
