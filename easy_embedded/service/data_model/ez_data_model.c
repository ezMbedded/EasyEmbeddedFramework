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

#include "ez_data_model.h"

/*the rest of include go here*/

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define NUM_OF_DATA_ELEMENT     10   /**< a macro*/

/*****************************************************************************
* Component Typedefs
*****************************************************************************/

/** @brief Definition of a data point
 */
struct DataModelElement
{
    bool            is_avail;
    /**< Availability flag*/

    bool            is_locked;
    /**< Data model locked flag, indicating that other component is using
     *   this model */
    uint32_t        data_size;
    /**< Size of the data point */
    void            *data;
    /**< Pointer to memory location where data is stored */
    ezSubject       subject;
    /**< Event subject, managing the observer receiving event notification */
};


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
static struct DataModelElement data_model[NUM_OF_DATA_ELEMENT] = {0};

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
/* None */


/*****************************************************************************
* Public functions
*****************************************************************************/

bool DataModel_Initialization(void)
{
    bool is_success = true;

    EZTRACE("DataModel_Initialization");

    for (uint32_t i = 0; i < NUM_OF_DATA_ELEMENT; i++)
    {
        data_model[i].is_avail = true;
        data_model[i].data = NULL;
        data_model[i].data_size = 0;
        data_model[i].is_locked = false;

        if (ezEventNotifier_CreateSubject(&data_model[i].subject) == ezFAIL)
        {
            EZERROR("init failed");
            is_success = false;
            break;
        }
    }

    return is_success;
}


void DataModel_ReleaseDataPoint(DataPoint data_point)
{
    if (data_point < NUM_OF_DATA_ELEMENT)
    {
        data_model[data_point].data = NULL;
        data_model[data_point].data_size = 0;
        data_model[data_point].is_avail = true;
        data_model[data_point].is_locked = false;
        ezEventNotifier_ResetSubject(&data_model[data_point].subject);

#if (DEBUG_LVL > LVL_INFO)
        data_point_count--;
        EZDEBUG("num of data point [size = %lu]", data_point_count);
#endif
    }
}


DataPoint DataModel_CreateDataPoint(void * data,
                                    uint32_t size)
{
    EZTRACE("DataModel_CreateDataElement()");

    bool is_success = true;
    DataPoint data_point = DATA_POINT_INVALID;

    for (uint32_t i = 0; i < NUM_OF_DATA_ELEMENT; i++)
    {
        if (data_model[i].is_avail)
        {
            data_point = i;
            data_model[i].is_avail = false;
            data_model[i].data = data;
            data_model[i].data_size = size;

            EZDEBUG("found free data element [index = %lu]", i);

#if (DEBUG_LVL > LVL_INFO)
            data_point_count++;
            EZDEBUG("num of data point [size = %lu]", data_point_count);
#endif
            break;
        }
    }

    if (!is_success)
    {
        EZERROR("Do not have enough data point");
    }

    return data_point;
}


bool DataModel_WriteDataPoint(DataPoint data_point,
                              void* data,
                              uint32_t size)
{
    EZTRACE("DataModel_WriteDataPoint()");

    bool is_success = false;

    if (data_point < NUM_OF_DATA_ELEMENT &&
        data_model[data_point].is_avail == false &&
        data != NULL)
    {
        EZTRACE("params sanity check OK");

        if (size == data_model[data_point].data_size &&
            data_model[data_point].is_locked == false)
        {
            EZTRACE("internal data check OK");

#if (DEBUG_LVL > LVL_DEBUG)
            HEXDUMP(data, size);
#endif /* (DEBUG_LVL > LVL_DEBUG) */

            data_model[data_point].is_locked = true;

            if (memcmp(data_model[data_point].data, data, size) != 0)
            {
                EZTRACE("data @ [index = %lu] has changed", data_point);

                memcpy(data_model[data_point].data, data, size);
                ezEventNotifier_NotifyEvent(&data_model[data_point].subject,
                                            DATA_MODIFY,
                                            data_model[data_point].data,
                                            &data_model[data_point].data_size);
            }
            else
            {
                EZTRACE("data unchanged");
            }

            data_model[data_point].is_locked = false;

            is_success = true;
        }
    }

    return is_success;
}


bool DataModel_ReadDataPoint(DataPoint data_point,
                             void* data,
                             uint32_t size)
{
    EZTRACE("DataModel_ReadDataElement()");

    bool is_success = false;

    if (data_point < NUM_OF_DATA_ELEMENT &&
        data_model[data_point].is_avail == false &&
        data != NULL)
    {
        if (size == data_model[data_point].data_size &&
            data_model[data_point].is_locked == false)
        {
            EZTRACE("Read data @ [index = %lu]", data_point);

            data_model[data_point].is_locked = true;

            memcpy(data_model[data_point].data, data, size);

            data_model[data_point].is_locked = false;

            is_success = true;
        }
    }
    return is_success;
}


bool DataModel_SubscribeDataPointEvent(DataPoint data_point,
    ezObserver * observer)
{
    EZTRACE("DataModel_SubscribeDataPointEvent()");

    bool is_success = false;

    if (data_point < NUM_OF_DATA_ELEMENT &&
        data_model[data_point].is_avail == false &&
        observer != NULL)
    {
        is_success = (ezEventNotifier_SubscribeToSubject(&data_model[data_point].subject, observer) == ezSUCCESS);
    }
    return is_success;
}


bool DataModel_UnsubscribeDataPointEvent(DataPoint data_point,
    ezObserver * observer)
{
    EZTRACE("DataModel_UnsubscribeDataPointEvent()");

    bool is_success = false;

    if (data_point < NUM_OF_DATA_ELEMENT &&
        data_model[data_point].is_avail == false &&
        observer != NULL)
    {

        is_success = (ezEventNotifier_UnsubscribeFromSubject(&data_model[data_point].subject, observer) == ezSUCCESS);
    }
    return is_success;
}


/******************************************************************************
* Internal functions
*******************************************************************************/

#endif /* (CONFIG_DATA_MODEL == 1U) */

/* End of file*/
