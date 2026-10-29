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

/** @file   ez_data_model.h
 *  @author Hai Nguyen
 *  @date   27.02.2024
 *  @brief  Public API of data model component
 *
 *  @details Simple key value data model
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
#include <stddef.h>
#include "ez_utilities_common.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define DATA_POINT_INVALID      UINT32_MAX /**< Invalide dat point */

/** Maximum alignment (in bytes) of a data point in the buffer. Must be a power of two.
 *  Each data point is aligned to the lowest set bit of its size, up to this value. */
#ifndef EZ_DATA_MODEL_ALIGNMENT
#define EZ_DATA_MODEL_ALIGNMENT sizeof(void*) /**< Alignment (in bytes) of every data point in the buffer. Must be a power of two. */
#endif


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
typedef struct
{
    uint32_t index;
    size_t size;
    void* data;
    bool isDirty;
}ezDataPoint_t;


typedef struct
{
    ezDataPoint_t *data_points;
    size_t num_of_data_points;
    uint8_t *data_model_buff;
    size_t data_model_buff_size;
}ezDataModel_t;



/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/


/** 
* @brief This function initializes a data model
*
* @details -
*
* @param[in]    data_model: Pointer to the data model
* @param[in]    data_points: Pointer to the array of data points
* @param[in]    num_of_data_points: Number of data points in the array
* @param[in]    data_model_buff: Memory buffer providing to the data model to work
* @param[in]    data_model_buff_size: Size of the memory buffer
* @return       True if initialization is successful, false otherwise
*
* @pre None
* @post None
*/
bool ezDataModel_Initialize(
    ezDataModel_t *data_model,
    ezDataPoint_t *data_points,
    size_t num_of_data_points,
    uint8_t *data_model_buff,
    size_t data_model_buff_size);


/** 
* @brief This function sets a data point value
*
* @details -
*
* @param[in]    data_model: Pointer to the data model
* @param[in]    index: Index of the data point to set
* @param[in]    data: Pointer to the data to set
* @param[in]    size: Size of the data in bytes, must equal the data point size
* @return       ezSUCCESS, ezSTATUS_ARG_INVALID on bad argument or size mismatch,
*               ezFAIL if the data point is not found
*
* @note The data model is not thread safe. Callers sharing it between tasks
*       or ISRs must serialize access.
*
* @pre data_model must be initialized using ezDataModel_Initialize()
* @post None
*/
ezSTATUS ezDataModel_SetDataPoint(
    ezDataModel_t *data_model,
    uint32_t index,
    const void *data,
    size_t size);


/** 
* @brief This function gets a data point value
*
* @details -
*
* @param[in]    data_model: Pointer to the data model
* @param[in]    index: Index of the data point to get
* @param[out]   data_size: Pointer to the size of the data point
* @return       Pointer to the data point value, or NULL if error
*
* @pre data_model must be initialized using ezDataModel_Initialize()
* @post None
*/
const void* ezDataModel_GetDataPoint(
    ezDataModel_t *data_model,
    uint32_t index,
    size_t *data_size);


/** 
* @brief This function gets first dirty data point index
*
* @details -
*
* @param[in]    data_model: Pointer to the data model
* @return       Index of the first dirty data point, or DATA_POINT_INVALID if no dirty data points
*
* @pre data_model must be initialized using ezDataModel_Initialize()
* @post None
*/
uint32_t ezDataModel_GetFirstDirty(ezDataModel_t *data_model);


/** 
* @brief This function clears dirty flag of a data point
*
* @details -
*
* @param[in]    data_model: Pointer to the data model
* @param[in]    index: Index of the data point to clear dirty flag
* @return       None
*
* @pre data_model must be initialized using ezDataModel_Initialize()
* @post None
*/
void ezDataModel_ClearDirtyFlags(ezDataModel_t *data_model, uint32_t index);



/** 
* @brief This function clears all dirty flags of a data point
*
* @details -
*
* @param[in]    data_model: Pointer to the data model
* @return       None
*
* @pre data_model must be initialized using ezDataModel_Initialize()
* @post None
*/
void ezDataModel_ClearAllDirtyFlags(ezDataModel_t *data_model);


#ifdef __cplusplus
}
#endif

#endif /* DATA_MODEL == 1U */
#endif /* _EZ_DATA_MODEL_H */
/* End of file */
