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
#define DATA_POINT_INVALID      UINT32_MAX /**< Invalid data point index, reserved and cannot be used as a data point index */

/** Maximum alignment (in bytes) of a data point in the buffer. Must be a power of two.
 *  Each data point is aligned to the lowest set bit of its size, up to this value. */
#ifndef EZ_DATA_MODEL_ALIGNMENT
#define EZ_DATA_MODEL_ALIGNMENT sizeof(void*) /**< Alignment (in bytes) of every data point in the buffer. Must be a power of two. */
#endif


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/** @brief A single data point descriptor. */
typedef struct
{
    uint32_t index;     /**< Unique index of the data point. Set by the user, must be strictly increasing in the array and not DATA_POINT_INVALID */
    size_t size;        /**< Size of the data point in bytes. Set by the user, must be greater than zero */
    void* data;         /**< Pointer to the value inside the model buffer. Set by ezDataModel_Initialize() */
    bool isDirty;       /**< True if the value was set since the flag was last cleared */
}ezDataPoint_t;


/** @brief Data model object. Fill it with ezDataModel_Initialize(), do not modify the members directly. */
typedef struct
{
    ezDataPoint_t *data_points;     /**< Array of data points, owned by the caller */
    size_t num_of_data_points;      /**< Number of data points in the array */
    uint8_t *data_model_buff;       /**< Buffer holding the data point values, owned by the caller */
    size_t data_model_buff_size;    /**< Size of the buffer in bytes */
}ezDataModel_t;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function : ezDataModel_Initialize
*//**
* @brief This function initializes a data model
*
* @details The buffer is zeroed, then each data point gets a slice of the
* buffer in array order, padded so that its address is aligned (see
* EZ_DATA_MODEL_ALIGNMENT). The dirty flags are cleared. The array must be
* sorted by strictly increasing index, because lookups use a binary search.
* The data points and the buffer are owned by the caller and must outlive
* the data model. If the arguments are NULL or zero, data_model is not
* touched. If the initialization fails after that, the model is left empty
* (no data points) and the other APIs fail safely.
*
* @param[in]    data_model: Pointer to the data model
* @param[in]    data_points: Pointer to the array of data points, with index and size filled in
* @param[in]    num_of_data_points: Number of data points in the array
* @param[in]    data_model_buff: Memory buffer given to the data model to hold the values
* @param[in]    data_model_buff_size: Size of the memory buffer in bytes
* @return       True if initialization is successful, false otherwise
*
* @retval true     Model initialized
* @retval false    A pointer is NULL, a count or size is zero, an index is
*                  DATA_POINT_INVALID or not strictly increasing, a data point
*                  size is zero, or the buffer is too small
*
* @pre None
* @post The data point values are zero and all dirty flags are cleared.
* @warning The buffer is zeroed before the data points are validated, so its
*          content is lost even when the function returns false.
* @isr_safe{no}
* @blocking{no}
* @see ezDataModel_SetDataPoint
*
* \b Example
* @code
* ezDataPoint_t points[] = {
*     {0U, sizeof(uint32_t), NULL, false},
*     {1U, sizeof(uint8_t), NULL, false},
* };
* uint8_t buffer[16];
* ezDataModel_t model;
* if (ezDataModel_Initialize(&model, points, sizeof(points) / sizeof(points[0]), buffer, sizeof(buffer)) == false)
* {
*     HandleError();
* }
* @endcode
*
*****************************************************************************/
bool ezDataModel_Initialize(
    ezDataModel_t *data_model,
    ezDataPoint_t *data_points,
    size_t num_of_data_points,
    uint8_t *data_model_buff,
    size_t data_model_buff_size);


/*****************************************************************************
* Function : ezDataModel_SetDataPoint
*//**
* @brief This function sets a data point value
*
* @details The value is copied into the model buffer and the dirty flag of
* the data point is set.
*
* @param[in]    data_model: Pointer to the data model
* @param[in]    index: Index of the data point to set
* @param[in]    data: Pointer to the data to set
* @param[in]    size: Size of the data in bytes, must equal the data point size
* @return       ezSTATUS
*
* @retval ezSUCCESS             Value copied, dirty flag set
* @retval ezSTATUS_ARG_INVALID  data_model or data is NULL, the model is not initialized, or size differs from the data point size
* @retval ezFAIL                No data point with this index
*
* @note The data model does not lock. Callers sharing it between tasks must
*       serialize access (the platform layer does it with a mutex).
*
* @pre data_model must be initialized using ezDataModel_Initialize()
* @post The dirty flag of the data point is set on success.
* @isr_safe{no}
* @blocking{no}
* @see ezDataModel_GetDataPoint
*
* \b Example
* @code
* uint32_t value = 42U;
* if (ezDataModel_SetDataPoint(&model, 0U, &value, sizeof(value)) != ezSUCCESS)
* {
*     HandleError();
* }
* @endcode
*
*****************************************************************************/
ezSTATUS ezDataModel_SetDataPoint(
    ezDataModel_t *data_model,
    uint32_t index,
    const void *data,
    size_t size);


/*****************************************************************************
* Function : ezDataModel_GetDataPoint
*//**
* @brief This function gets a data point value
*
* @details The returned pointer references the model buffer, no copy is made.
* It stays valid as long as the model buffer exists, but the value can change
* when another context calls ezDataModel_SetDataPoint().
*
* @param[in]    data_model: Pointer to the data model
* @param[in]    index: Index of the data point to get
* @param[out]   data_size: Pointer to the size of the data point. Set to 0 if the data point is not found.
* @return       Pointer to the data point value, or NULL if error
*
* @retval NULL  data_model or data_size is NULL, or no data point with this index
*
* @pre data_model must be initialized using ezDataModel_Initialize()
* @post None
* @warning The pointer is by reference. Do not write through it; use ezDataModel_SetDataPoint().
* @isr_safe{no}
* @blocking{no}
* @see ezDataModel_SetDataPoint
*
* \b Example
* @code
* size_t size = 0U;
* const void *value = ezDataModel_GetDataPoint(&model, 0U, &size);
* if (value == NULL || size != sizeof(uint32_t))
* {
*     HandleError();
* }
* else
* {
*     uint32_t copy;
*     memcpy(&copy, value, sizeof(copy));
* }
* @endcode
*
*****************************************************************************/
const void* ezDataModel_GetDataPoint(
    ezDataModel_t *data_model,
    uint32_t index,
    size_t *data_size);


/*****************************************************************************
* Function : ezDataModel_GetFirstDirty
*//**
* @brief This function gets first dirty data point index
*
* @details The data points are searched in array order. The dirty flag stays
* set until it is cleared with ezDataModel_ClearDirtyFlags() or
* ezDataModel_ClearAllDirtyFlags().
*
* @param[in]    data_model: Pointer to the data model
* @return       Index of the first dirty data point, or DATA_POINT_INVALID if no dirty data points
*
* @retval DATA_POINT_INVALID    No dirty data point, or data_model is NULL
*
* @pre data_model must be initialized using ezDataModel_Initialize()
* @post None
* @isr_safe{no}
* @blocking{no}
* @see ezDataModel_ClearDirtyFlags
*
* \b Example
* @code
* uint32_t index = ezDataModel_GetFirstDirty(&model);
* while (index != DATA_POINT_INVALID)
* {
*     ezDataModel_ClearDirtyFlags(&model, index);
*     index = ezDataModel_GetFirstDirty(&model);
* }
* @endcode
*
*****************************************************************************/
uint32_t ezDataModel_GetFirstDirty(ezDataModel_t *data_model);


/*****************************************************************************
* Function : ezDataModel_ClearDirtyFlags
*//**
* @brief This function clears dirty flag of a data point
*
* @details Nothing happens if data_model is NULL or the index is not found.
*
* @param[in]    data_model: Pointer to the data model
* @param[in]    index: Index of the data point to clear dirty flag
*
* @pre data_model must be initialized using ezDataModel_Initialize()
* @post The dirty flag of the data point is cleared.
* @isr_safe{no}
* @blocking{no}
* @see ezDataModel_ClearAllDirtyFlags
*
*****************************************************************************/
void ezDataModel_ClearDirtyFlags(ezDataModel_t *data_model, uint32_t index);


/*****************************************************************************
* Function : ezDataModel_ClearAllDirtyFlags
*//**
* @brief This function clears the dirty flags of all data points
*
* @details Nothing happens if data_model is NULL.
*
* @param[in]    data_model: Pointer to the data model
*
* @pre data_model must be initialized using ezDataModel_Initialize()
* @post All dirty flags are cleared.
* @isr_safe{no}
* @blocking{no}
* @see ezDataModel_ClearDirtyFlags
*
*****************************************************************************/
void ezDataModel_ClearAllDirtyFlags(ezDataModel_t *data_model);


#ifdef __cplusplus
}
#endif

#endif /* DATA_MODEL == 1U */
#endif /* _EZ_DATA_MODEL_H */
/* End of file */
