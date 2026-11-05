/** @file   ez_ring_buffer.h
 *  @author Hai Nguyen
 *  @date   26.02.2024
 *  @brief  Public API of ring buffer component
 *
 *  @details Byte-oriented FIFO ring buffer working on a caller-provided
 *  memory buffer. No dynamic allocation. The implementation is compiled only
 *  when EZ_RING_BUFFER is 1U. The functions take no lock and do not check
 *  the ring_buff pointer for NULL.
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
/** @brief Data structure of a ring buffer. Use the ezRingBuffer_* functions
 *  to access it; do not modify the members directly.
 */
typedef struct
{
    uint8_t *buff;
    /**< pointer to the data buffer (owned by the caller)*/
    uint16_t capacity;
    /**< size of the buffer in bytes*/
    uint16_t head_index;
    /**< index where the next byte is written*/
    uint16_t tail_index;
    /**< index where the next byte is read*/
    uint16_t written_byte_count;
    /**< number of bytes currently stored*/
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
* @details The ring buffer stores the pointer to buff and does not copy it.
* The caller owns buff, which must stay valid while the ring buffer is used.
* On success the buffer is empty.
*
* @param[in]    ring_buff: pointer to the ring buffer
* @param[in]    buff: pointer of buffer holding data
* @param[in]    size: size of buff in bytes
* @return       true if success, else false
*
* @retval true     Ring buffer initialized
* @retval false    buff is NULL or size is 0; ring_buff is left unchanged
*
* @pre ring_buff is not NULL
* @post The ring buffer is empty and its capacity is size
* @blocking{no}
* @see ezRingBuffer_Reset
*
* \b Example
* @code
* RingBuffer buffer;
* uint8_t storage[30];
* if (ezRingBuffer_Init(&buffer, storage, sizeof(storage)) != true)
* {
*     HandleError();
* }
* @endcode
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
* @retval true     No byte is stored
* @retval false    At least one byte is stored
*
* @pre The ring buffer was initialized with ezRingBuffer_Init
* @post None
* @blocking{no}
* @see ezRingBuffer_Init
*
* \b Example
* @code
* if (ezRingBuffer_IsEmpty(&buffer) == false)
* {
*     ProcessData();
* }
* @endcode
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
* @retval true     Stored byte count equals the capacity
* @retval false    Free space is left
*
* @pre The ring buffer was initialized with ezRingBuffer_Init
* @post None
* @blocking{no}
* @see ezRingBuffer_Init
*
* \b Example
* @code
* if (ezRingBuffer_IsFull(&buffer) == true)
* {
*     HandleOverflow();
* }
* @endcode
*
*****************************************************************************/
bool ezRingBuffer_IsFull(RingBuffer *ring_buff);


/*****************************************************************************
* Function : ezRingBuffer_Push
*//** 
* @brief Push data into the ring buffer
*
* @details The data is copied into the ring buffer. If the free space is
* smaller than size, only the bytes that fit are copied and existing data is
* never overwritten. Wrapping at the end of the buffer is handled.
*
* @param[in]    ring_buff: pointer to the ring buffer
* @param[in]    data: pointer to the pushed data
* @param[in]    size: size of the data in bytes
* @return       Number of bytes pushed into the ring buffer
*
* @retval 0         Buffer is full or size is 0
* @retval <size     Only part of the data fitted
*
* @pre The ring buffer was initialized with ezRingBuffer_Init; data is not
* NULL and points to at least size bytes
* @post written byte count increased by the returned value
* @warning The caller must compare the return value with size to detect
* truncated data.
* @blocking{no}
* @see ezRingBuffer_Init, ezRingBuffer_Pop
*
* \b Example
* @code
* uint8_t data[3U] = {0U, 1U, 2U};
* if (ezRingBuffer_Push(&buffer, data, sizeof(data)) != sizeof(data))
* {
*     HandleOverflow();
* }
* @endcode
*
*****************************************************************************/
uint16_t ezRingBuffer_Push(RingBuffer * ring_buff, uint8_t *data, uint16_t size);


/*****************************************************************************
* Function : ezRingBuffer_Pop
*//** 
* @brief Pop data out of the ring buffer
*
* @details Bytes are returned in the order they were pushed (FIFO) and are
* copied to data. If fewer than size bytes are stored, only the stored bytes
* are copied.
*
* @param[in]    ring_buff: pointer to the ring buffer
* @param[out]   data: pointer to the memory receiving the popped data
* @param[in]    size: number of bytes requested
* @return       Number of bytes popped out of the ring buffer
*
* @retval 0         Buffer is empty or size is 0
* @retval <size     Fewer bytes than requested were stored
*
* @pre The ring buffer was initialized with ezRingBuffer_Init; data is not
* NULL and can hold at least size bytes
* @post written byte count decreased by the returned value
* @warning The caller must check the return value to know how many bytes of
* data are valid.
* @blocking{no}
* @see ezRingBuffer_Init, ezRingBuffer_Push
*
* \b Example
* @code
* uint8_t data[3U];
* uint16_t count = ezRingBuffer_Pop(&buffer, data, sizeof(data));
* if (count == 0U)
* {
*     HandleEmpty();
* }
* @endcode
*
*****************************************************************************/
uint16_t ezRingBuffer_Pop(RingBuffer *ring_buff, uint8_t *data, uint16_t size);


/*****************************************************************************
* Function : ezRingBuffer_Reset
*//** 
* @brief Discard all stored data and clear the buffer
*
* @details Head, tail and byte count are set to 0 and the whole data buffer
* is filled with 0. The buffer pointer and capacity are kept.
*
* @param[in]    ring_buff: pointer to the ring buffer
*
* @pre The ring buffer was initialized with ezRingBuffer_Init
* @post The ring buffer is empty
* @blocking{no}
* @see ezRingBuffer_Init
*
* \b Example
* @code
* ezRingBuffer_Reset(&buffer);
* @endcode
*
*****************************************************************************/
void ezRingBuffer_Reset(RingBuffer *ring_buff);


/*****************************************************************************
* Function : ezRingBuffer_GetAvailableMemory
*//** 
* @brief Return number of free bytes in the buffer
*
* @details Capacity minus the number of bytes currently stored.
*
* @param[in]    ring_buff: pointer to the ring buffer
* @return       Number of free bytes
*
* @pre The ring buffer was initialized with ezRingBuffer_Init
* @post None
* @blocking{no}
* @see ezRingBuffer_Init
*
* \b Example
* @code
* uint16_t free_bytes = ezRingBuffer_GetAvailableMemory(&buffer);
* @endcode
*
*****************************************************************************/
uint16_t ezRingBuffer_GetAvailableMemory( RingBuffer *ring_buff);

#ifdef __cplusplus
}
#endif

#endif	/* RING_BUFFER_H */

/* End of file */
