/*****************************************************************************
* Filename:         ez_ring_buffer.c
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

/** @file   ez_ring_buffer.c
 *  @author Hai Nguyen
 *  @date   26.02.2024
 *  @brief  Implementation of the ring data structure
 *
 *  @details
 */

/******************************************************************************
* Includes
*******************************************************************************/
#include "ez_ring_buffer.h"

#if (EZ_RING_BUFFER == 1U)
#include "string.h"


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


/*****************************************************************************
* Function Definitions
*****************************************************************************/
/* None */


/*****************************************************************************
* Public functions
*****************************************************************************/
bool ezRingBuffer_Init(RingBuffer *ring_buff, uint8_t *buff, uint16_t size)
{
    bool ret = false;
    if(buff != NULL && size > 0)
    {
        ring_buff->head_index = 0;
        ring_buff->tail_index = 0;
        ring_buff->written_byte_count = 0;
        ring_buff->buff = buff;
        ring_buff->capacity = size;
        ret = true;
    }
    return ret;
}


bool ezRingBuffer_IsEmpty(RingBuffer *ring_buff)
{
    return (0 == ring_buff->written_byte_count);
}


bool ezRingBuffer_IsFull(RingBuffer * ring_buff)
{
    return (ring_buff->written_byte_count == ring_buff->capacity);
}


uint16_t ezRingBuffer_Push(RingBuffer * ring_buff, uint8_t *data, uint16_t size)
{
    uint16_t remain_byte_count = 0U;
    uint16_t push_byte_count = ring_buff->capacity - ring_buff->written_byte_count;

    if(push_byte_count >= size)
    {
        push_byte_count = size;
    }

    if (ring_buff->capacity - ring_buff->head_index >= push_byte_count)
    {
        memcpy(&ring_buff->buff[ring_buff->head_index], data, push_byte_count);
        ring_buff->head_index = ring_buff->head_index + push_byte_count;
        ring_buff->written_byte_count = ring_buff->written_byte_count + push_byte_count;
    }
    else
    {
        /* Handle warpping */
        remain_byte_count = ring_buff->capacity - ring_buff->head_index;
        memcpy(&ring_buff->buff[ring_buff->head_index], data, remain_byte_count);
        ring_buff->head_index = 0;
        ring_buff->written_byte_count = ring_buff->written_byte_count + remain_byte_count;
        data = data + remain_byte_count;

        remain_byte_count = push_byte_count - remain_byte_count;
        memcpy(&ring_buff->buff[ring_buff->head_index], data, remain_byte_count);
        ring_buff->head_index = ring_buff->head_index + remain_byte_count;
        ring_buff->written_byte_count = ring_buff->written_byte_count + remain_byte_count;
    }

    return push_byte_count;
}


uint16_t ezRingBuffer_Pop(RingBuffer *ring_buff, uint8_t *data, uint16_t size)
{

    uint16_t popped_byte_count = size;
    uint16_t remained_byte_count = 0U;

    if(ring_buff->written_byte_count < popped_byte_count)
    {
        popped_byte_count = ring_buff->written_byte_count;
    }

    if (ring_buff->capacity - ring_buff->tail_index >= popped_byte_count)
    {
        memcpy(data, &ring_buff->buff[ring_buff->tail_index], popped_byte_count);
        ring_buff->tail_index = ring_buff->tail_index + popped_byte_count;
        ring_buff->written_byte_count = ring_buff->written_byte_count - popped_byte_count;
    }
    else
    {
        /* Handle warpping */
        remained_byte_count = ring_buff->capacity - ring_buff->tail_index;
        memcpy(data, &ring_buff->buff[ring_buff->tail_index], remained_byte_count);
        ring_buff->tail_index = 0;
        ring_buff->written_byte_count = ring_buff->written_byte_count - remained_byte_count;

        data = data + remained_byte_count;

        remained_byte_count = popped_byte_count - remained_byte_count;
        memcpy(data, &ring_buff->buff[ring_buff->tail_index], remained_byte_count);
        ring_buff->tail_index = ring_buff->tail_index + remained_byte_count;
        ring_buff->written_byte_count = ring_buff->written_byte_count - remained_byte_count;
    }

    return popped_byte_count;
}


void ezRingBuffer_Reset(RingBuffer * ring_buff)
{
    ring_buff->head_index = 0;
    ring_buff->tail_index = 0;
    ring_buff->written_byte_count = 0;
    memset(ring_buff->buff, 0, ring_buff->capacity);
}


uint16_t ezRingBuffer_GetAvailableMemory(RingBuffer *ring_buff)
{
    return ring_buff->capacity - ring_buff->written_byte_count;
}

#endif /* CONFIG_RING_BUFFER */

/* End of file */
