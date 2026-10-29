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

/** @file   ez_data_model.c
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

#define DEBUG_LVL   EZ_DATA_MODEL_LOGGING_LEVEL
#define MOD_NAME    "DATA_MODEL"
#include "ez_logging.h"
#include "ez_assert.h"
#include "ez_data_model.h"
#include "ez_utilities_common.h"


/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */


/******************************************************************************
* Function Definitions
*******************************************************************************/
/* None */


/*****************************************************************************
* Function Definitions
*****************************************************************************/

/**
* @brief This function allocate data points in the data model buffer
*
* @details -
*
* @param[in]    data_model: Pointer to the data model
* @return       true if allocation is successful, false otherwise
*
* @pre data_model must be initialized using ezDataModel_Initialize()
* @post None
*/
static bool ezDataModel_AllocateDataPoints(ezDataModel_t *data_model);


/*
* @brief This function returns pointer to data point by index
*
* @details -
*
* @param[in]    data_model: Pointer to the data model
* @param[in]    index: Index of the data point to find
* @return       Pointer to the data point, or NULL if not found
*
* @pre data_model must be initialized using ezDataModel_Initialize()
* @post None
*/
static ezDataPoint_t *ezDataModel_FindDataPointByIndex(
    ezDataModel_t *data_model,
    uint32_t index);


/*****************************************************************************
* Public functions
*****************************************************************************/
bool ezDataModel_Initialize(
    ezDataModel_t *data_model,
    ezDataPoint_t *data_points,
    size_t num_of_data_points,
    uint8_t *data_model_buff,
    size_t data_model_buff_size)
{
    if(data_model == NULL
       || data_points == NULL
       || num_of_data_points == 0U
       || data_model_buff == NULL
       || data_model_buff_size == 0U)
    {
        EZERROR("Invalid argument");
        return false;
    }

    memset(data_model_buff, 0, data_model_buff_size);

    data_model->data_points = data_points;
    data_model->num_of_data_points = num_of_data_points;
    data_model->data_model_buff = data_model_buff;
    data_model->data_model_buff_size = data_model_buff_size;

    if(ezDataModel_AllocateDataPoints(data_model) == false)
    {
        EZERROR("Data point allocation failed");
        /* Leave the model empty so that other APIs fail safely */
        data_model->data_points = NULL;
        data_model->num_of_data_points = 0U;
        return false;
    }

    return true;
}


ezSTATUS ezDataModel_SetDataPoint(
    ezDataModel_t *data_model,
    uint32_t index,
    const void *data,
    size_t size)
{
    if(data_model == NULL
        || data == NULL
        || data_model->data_points == NULL
        || data_model->num_of_data_points == 0U)
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

    if(size != data_point->size)
    {
        EZDEBUG("Data size mismatch");
        return ezSTATUS_ARG_INVALID;
    }

    memcpy(data_point->data, data, data_point->size);
    data_point->isDirty = true;
    
    return ezSUCCESS;
}


const void* ezDataModel_GetDataPoint(
    ezDataModel_t *data_model,
    uint32_t index,
    size_t *data_size)
{
    if(data_model == NULL
       || data_size == NULL)
    {
        EZDEBUG("Invalid argument");
        return NULL;
    }

    *data_size = 0;

    ezDataPoint_t *data_point = ezDataModel_FindDataPointByIndex(data_model, index);
    if(data_point == NULL)
    {
        EZDEBUG("Data point not found");
        return NULL;
    }

    *data_size = data_point->size;

    return data_point->data;
}


void ezDataModel_ClearDirtyFlags(ezDataModel_t *data_model, uint32_t index)
{
    if(data_model == NULL)
    {
        EZDEBUG("Invalid argument");
        return;
    }

    ezDataPoint_t *data_point = ezDataModel_FindDataPointByIndex(data_model, index);
    if(data_point == NULL)
    {
        EZDEBUG("Data point not found");
        return;
    }

    data_point->isDirty = false;
}


void ezDataModel_ClearAllDirtyFlags(ezDataModel_t *data_model)
{
    if(data_model == NULL)
    {
        EZDEBUG("Invalid argument");
        return;
    }

    for (size_t i = 0; i < data_model->num_of_data_points; i++)
    {
        data_model->data_points[i].isDirty = false;
    }
}


uint32_t ezDataModel_GetFirstDirty(ezDataModel_t *data_model)
{
    uint32_t dirty_index = DATA_POINT_INVALID;

    if(data_model == NULL)
    {
        EZDEBUG("Invalid argument");
        return dirty_index;
    }

    for (size_t i = 0; i < data_model->num_of_data_points; i++)
    {
        if (data_model->data_points[i].isDirty == true)
        {
            dirty_index = data_model->data_points[i].index;
            break;
        }
    }
    
    return dirty_index;
}


/******************************************************************************
* Internal functions
*******************************************************************************/
static bool ezDataModel_AllocateDataPoints(ezDataModel_t *data_model)
{
    if(data_model == NULL)
    {
        EZERROR("Invalid argument");
        return false;
    }

    size_t remain_byte = data_model->data_model_buff_size;
    uint8_t *current_buff_ptr = data_model->data_model_buff;

    for(size_t i = 0U; i < data_model->num_of_data_points; i++)
    {
        if(data_model->data_points[i].index == DATA_POINT_INVALID)
        {
            EZERROR("Data point %zu uses the reserved invalid index", i);
            return false;
        }

        /* Indexes must be strictly increasing for the binary search */
        if(i > 0U
           && data_model->data_points[i - 1U].index >= data_model->data_points[i].index)
        {
            EZERROR("Data point %zu index (%u) is not greater than the previous one (%u)",
                i,
                (unsigned)data_model->data_points[i].index,
                (unsigned)data_model->data_points[i - 1U].index);
            return false;
        }

        if(data_model->data_points[i].size == 0)
        {
            EZERROR("Data point size is zero");
            return false;
        }

        /* sizeof() is a multiple of alignof(), so the lowest set bit of the size
         * is a safe alignment for the data point, capped at the maximum one. */
        size_t alignment = data_model->data_points[i].size & (~data_model->data_points[i].size + 1U);
        if(alignment > EZ_DATA_MODEL_ALIGNMENT)
        {
            alignment = EZ_DATA_MODEL_ALIGNMENT;
        }

        /* Pad so that the data point address is aligned, based on the real address */
        size_t misalign = (size_t)((uintptr_t)current_buff_ptr % alignment);
        size_t padding = (misalign == 0U) ? 0U : (alignment - misalign);

        if(padding > remain_byte
           || data_model->data_points[i].size > (remain_byte - padding))
        {
            EZERROR("Not enough buffer size for data point %zu", i);
            return false;
        }

        current_buff_ptr += padding;
        remain_byte -= padding;

        data_model->data_points[i].data = (void *)current_buff_ptr;
        data_model->data_points[i].isDirty = false;
        current_buff_ptr += data_model->data_points[i].size;
        remain_byte -= data_model->data_points[i].size;
    }

    EZDEBUG("Data point allocation successful");
    EZDEBUG("Total data points allocated: %zu", data_model->num_of_data_points);
    EZDEBUG("Total buffer size: %zu bytes", data_model->data_model_buff_size);
    EZDEBUG("Remaining buffer size: %zu bytes", remain_byte);

    return true;
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
            right = mid;
        }
        else
        {
            left = mid + 1;
        }
    }

    return NULL;
}

#endif /* (DATA_MODEL == 1U) */

/* End of file*/
