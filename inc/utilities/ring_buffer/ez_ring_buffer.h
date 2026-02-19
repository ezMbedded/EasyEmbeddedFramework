/*****************************************************************************
* Filename:         ez_ring_buffer.h
* Author:           Hai Nguyen
* Original Date:    26.02.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/


/** @file   ez_ring_buffer.h
 *  @author Hai Nguyen
 *  @date   26.02.2024
 *  @brief  Public API of ring buffer component
 *
 *  @details
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* Includes
*******************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#include <stdint.h>
#include <stdbool.h>

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */

/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/** @brief Data structure of a ring buffer
 */
typedef struct
{
    uint8_t *buff;
    /**< pointer to the data buffer*/
    uint16_t capacity;
    /**< size of the buffer*/
    uint16_t head_index;
    /**< buffer head*/
    uint16_t tail_index;
    /**< buffer tail*/
    uint16_t written_byte_count;
    /**< number of byte written*/
}RingBuffer;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/******************************************************************************
* Function Prototypes
*******************************************************************************/

/*****************************************************************************
* Function : ezRingBuffer_Init
*//** 
* @brief This function initializes the ring buffer
*
* @details
*
* @param[in]    ring_buff: pointer to the ring buffer
* @param[in]    buff: pointer of buffer holding data
* @param[in]    size: size of buff
* @return       true if success, else false
*
* @pre None
* @post None
*
* \b Example
* @code
* RingBuffer buffer;
* RingBuffer_Init(&buffer, 30);
* @endcode
*
* @see
*
*****************************************************************************/
bool ezRingBuffer_Init(RingBuffer *ring_buff, uint8_t *buff, uint16_t size);


/*****************************************************************************
* Function : ezRingBuffer_IsEmpty
*//** 
* @brief Check if the buffer is empty
*
* @details
*
* @param[in]    ring_buff: pointer to the ring buffer
* @return       true if empty, otherwise false
*
* @pre A RingBuffer is exsiting
* @post None
*
* \b Example
* @code
* RingBuffer buffer;
* RingBuffer_Init(&buffer, 30);
* RingBuff_Status status;
* status = RingBuffer_IsEmpty(&buffer);
* @endcode
*
* @see ezRingBuffer_Init
*
*****************************************************************************/
bool ezRingBuffer_IsEmpty(RingBuffer *ring_buff);


/*****************************************************************************
* Function : ezRingBuffer_IsFull
*//** 
* @brief Check if the buffer is full
*
* @details
*
* @param[in]    ring_buff: pointer to the ring buffer
* @return       True if full, else false
*
* @pre a RingBuffer is exsiting
* @post None
*
* \b Example
* @code
* RingBuffer buffer;
* RingBuffer_Init(&buffer, 30);
* RingBuff_Status status;
* status = RingBuffer_IsFull(&buffer);
* @endcode
*
* @see ezRingBuffer_Init
*
*****************************************************************************/
bool ezRingBuffer_IsFull(RingBuffer *ring_buff);


/*****************************************************************************
* Function : ezRingBuffer_Push
*//** 
* @brief Push data into the ring buffet
*
* @details
*
* @param[in]    ring_buff: pointer to the ring buffer
* @param[in]    data: pointer to the pushed data
* @param[in]    size: size of the data
* @return       Number of bytes pushed into the ring buffer
*
* @pre a RingBuffer is exsiting
* @post None
*
* \b Example
* @code
* RingBuffer buffer;
* RingBuffer_Init(&buffer, 30);
* uint8_t u8Data[3U] = {0U,1U,2U}
* RingBuffer_Push(&buffer, u8Data, 3U);
* @endcode
*
* @see ezRingBuffer_Init
*
*****************************************************************************/
uint16_t ezRingBuffer_Push(RingBuffer * ring_buff, uint8_t *data, uint16_t size);


/*****************************************************************************
* Function : ezRingBuffer_Pop
*//** 
* @brief Pop data out of the ring buffer
*
* @details
*
* @param[in]    ring_buff: pointer to the ring buffer
* @param[out]   data: pointer to the popped data
* @param[in]    size: size of the popped data
* @return       Number of bytes popped out of the ring buffer
*
* @pre a RingBuffer is exsiting
* @post Postcondition
*
* \b Example
* @code
* RingBuffer buffer;
* RingBuffer_Init(&buffer, 30);
* uint8_t u8Data[3U];
* RingBuffer_Pop(&buffer, u8Data, 3U);
* @endcode
*
* @see ezRingBuffer_Init
*
*****************************************************************************/
uint16_t ezRingBuffer_Pop(RingBuffer *ring_buff, uint8_t *data, uint16_t size);


/*****************************************************************************
* Function : ezRingBuffer_Reset
*//** 
* @brief A RingBuffer is exsiting
*
* @details
*
* @param[in]    ring_buff: pointer to the ring buffer
* @return       None
*
* @pre a RingBuffer is exsiting
* @post None
*
* \b Example
* @code
* RingBuffer_Reset(&buffer);
* @endcode
*
* @see ezRingBuffer_Init
*
*****************************************************************************/
void ezRingBuffer_Reset(RingBuffer *ring_buff);


/*****************************************************************************
* Function : ezRingBuffer_GetAvailableMemory
*//** 
* @brief Return number of bytes available
*
* @details
*
* @param[in]    ring_buff: pointer to the ring buffer
* @return       Number of available bytes
*
* \b Example
* @code
* RingBuffer buffer;
* RingBuffer_Init(&buffer, 30);
* uint8_t u8AvailMem;
* RingBuffer_GetAvailableMemory(&buffer, &u8AvailMem);
* @endcode
*
* @see RingBuffer_Init
*
*****************************************************************************/
uint16_t ezRingBuffer_GetAvailableMemory( RingBuffer *ring_buff);

#ifdef __cplusplus
}
#endif

#endif	/* RING_BUFFER_H */

/* End of file */
