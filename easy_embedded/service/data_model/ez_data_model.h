/*****************************************************************************
* Filename:         ez_data_model.h
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

/** @file   data_model.h
 *  @author Hai Nguyen
 *  @date   27.02.2024
 *  @brief  Public API of data model component
 *
 *  @details Data model component implements the data driven design pattern.
 */

#ifndef _EZ_DATA_MODEL_H
#define _EZ_DATA_MODEL_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (DATA_MODEL == 1U)
#include <stdbool.h>
#include <stdint.h>
#include "ez_utilities_common.h"
#include "ez_event_bus.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define DATA_POINT_INVALID      UINT32_MAX /**< Invalide dat point */


/*****************************************************************************
* Component Typedefs
*****************************************************************************/

typedef enum{
    TYPE_BOOL = 0,
    TYPE_UINT8,
    TYPE_UINT16,
    TYPE_UINT32,
    TYPE_INT8,
    TYPE_INT16,
    TYPE_INT32,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_BLOB,
    TYPE_UNKNOWN,
}ezDataPointType_t;


typedef struct
{
    uint32_t index;
    ezDataPointType_t type;
    size_t size;
    void* data;
}ezDataPoint_t;


typedef struct
{
    ezDataPoint_t *data_points;
    size_t num_of_data_points;
    uint8_t *data_model_buff;
    size_t data_model_buff_size;
    ezEventBus_t event_bus;
    uint8_t *event_buff;
    size_t event_buff_size;
}ezDataModel_t;



/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/
void ezDataModel_Initialize(
    ezDataModel_t *data_model,
    ezDataPoint_t *data_points,
    size_t num_of_data_points,
    uint8_t *data_model_buff,
    size_t data_model_buff_size,
    uint8_t *event_buff,
    size_t event_buff_size);

ezSTATUS ezDataModel_SetDataPoint(
    ezDataModel_t *data_model,
    uint32_t index,
    void *data,
    ezDataPointType_t type);


ezSTATUS ezDataModel_GetDataPoint(
    ezDataModel_t *data_model,
    uint32_t index,
    void *data,
    ezDataPointType_t type);

ezSTATUS ezDataModel_ListenDataPointChange(
    ezDataModel_t *data_model,
    uint32_t index,
    ezEventListener_t *listener);


ezSTATUS ezDataModel_UnlistenDataPointChange(
    ezDataModel_t *data_model,
    uint32_t index,
    ezEventListener_t *listener);

ezSTATUS ezDataModel_Run(ezDataModel_t *data_model);

#if 0
/*****************************************************************************
* Function : DataModel_Initialization
*//** 
* @brief This function initializes the data model component
*
* @details
*
* @param    None
* @return   true: success
*           false: fail
*
* @pre None
* @post None
*
* \b Example
* @code
* DataModel_Initialization();
* @endcode
*
* @see
*
*****************************************************************************/
bool DataModel_Initialization(void);


/*****************************************************************************
* Function : DataModel_CreateDataPoint
*//** 
* @brief This function creates a data point by getting a free data point
*        from the data model
*
* @details
*
* @param[in]    *data: Pointer to the actual data
* @param[in]    size: Size of data, for sanity checking and future use
* @return       DataPoint handle or DATA_POINT_INVALID
*
* @pre Data model component must be intialized
* @post None
*
* \b Example
* @code
* uint32_t my_data = 100;
* DataPoint my_data_point = DataModel_CreateDataPoint(&my_data, sizeof(uint32_t));
* if(my_data_point != DATA_POINT_INVALID)
* {
*     printf("success");
* }
* @endcode
*
* @see DataModel_Initialization
*
*****************************************************************************/
DataPoint DataModel_CreateDataPoint(void* data,
                                    uint32_t size);


/*****************************************************************************
* Function : DataModel_ReleaseDataPoint
*//** 
* @brief This function releases and reset the data point when it is unused
*
* @details
*
* @param[in]    data_point: data point handle
* @return       None
*
* @pre Data model component must be intialized
* @post None
*
* \b Example
* @code
* DataModel_ReleaseDataPoint(my_data_point);
* @endcode
*
* @see DataModel_Initialization
*
*****************************************************************************/
void DataModel_ReleaseDataPoint(DataPoint data_point);


/*****************************************************************************
* Function : DataModel_WriteDataPoint
*//** 
* @brief This function write new data to a data point
*
* @details
*
* @param[in]    data_point: Data point handle, where data is written to
* @param[in]    *data: Pointer to the written data
* @param[in]    size: Size of data, for sanity checking and future use
* @return       true: success
*               false: fail
*
* @pre Data model component must be intialized. Data point must be created
* @post None
*
* \b Example
* @code
* uint32_t my_data = 100;
* uint32_t new_data = 200;
* 
* DataPoint my_data_point = DataModel_CreateDataPoint(&my_data, sizeof(uint32_t));
* 
* if(my_data_point != DATA_POINT_INVALID)
* {
*     printf("success");
*     DataModel_WriteDataPoint(my_data_point, &new_data, sizeof(uint32_t));
* }
* @endcode
*
* @see DataModel_Initialization, DataModel_CreateDataPoint
*
*****************************************************************************/
bool DataModel_WriteDataPoint(DataPoint data_point,
                              void* data,
                              uint32_t size);


/*****************************************************************************
* Function : DataModel_ReadDataPoint
*//** 
* @brief This function reads data from a data point
*
* @details Detail description
*
* @param[in]    data_point: Data point handle, where data is read to
* @param[in]    *data: Pointer to the read data
* @param[in]    size: Size of data, for sanity checking and future use
* @return       true: success
*               false: fail
*
* @pre Data model component must be intialized. Data point must be created
* @post None
*
* \b Example
* @code
* uint32_t my_data = 100;
* uint32_t new_data;
*
* DataPoint my_data_point = DataModel_CreateDataPoint(&my_data, sizeof(uint32_t));
*
* if(my_data_point != DATA_POINT_INVALID)
* {
*     printf("success");
*    DataModel_ReadDataPoint(my_data_point, &new_data, sizeof(uint32_t));
* }
* @endcode
*
* @see DataModel_Initialization, DataModel_CreateDataPoint
*
*****************************************************************************/
bool DataModel_ReadDataPoint(DataPoint data_point,
                             void * data,
                             uint32_t size);


/*****************************************************************************
* Function : DataModel_SubscribeDataPointEvent
*//** 
* @brief This function allow a module to receive data change event from the data model
*
* @details
*
* @param[in]    data_point: Data point handle, where data is read to
* @param[in]    *observer: (IN)pointer to the observer
* @return       true: success
*               false: fail
*
* @pre Data model component must be intialized. Data point must be created
* @post None
*
* \b Example
* @code
* uint32_t my_data = 100;
* ezObserver observer;
*
* DataPoint my_data_point = DataModel_CreateDataPoint(&my_data, sizeof(uint32_t));
* if(my_data_point != DATA_POINT_INVALID)
* {
*     printf("success");
*     ezEventNotifier_CreateObserver(&observer, FictionalCallback);
*     DataModel_SubscribeDataPointEvent(my_data_point, &observer);
* }
* @endcode
*
* @see DataModel_Initialization, DataModel_CreateDataPoint,
*      ezEventNotifier_CreateObserver
*
*****************************************************************************/
bool DataModel_SubscribeDataPointEvent(DataPoint data_point,
                                       ezObserver *observer);


/*****************************************************************************
* Function : DataModel_UnsubscribeDataPointEvent
*//** 
* @brief This function allow a module to stop receiving data change event
*        from the data model
*
* @details
*
* @param[in]    data_point: Data point handle, where data is read to
* @param[in]    *observer: Pointer to the observer
* @return       true: success
*               false: fail
*
* @pre Data model component must be intialized. Data point must be created
* @post None
*
* \b Example
* @code
* DataModel_UnsubscribeDataPointEvent(my_data_point, &observer);
* @endcode
*
* @see sum
*
*****************************************************************************/
bool DataModel_UnsubscribeDataPointEvent(DataPoint data_point,
                                         ezObserver *observer);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_DATA_MODEL == 1U */
#endif /* _EZ_DATA_MODEL_H */
/* End of file */
