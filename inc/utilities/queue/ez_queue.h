/*****************************************************************************
* Filename:         ez_queue.h
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

/** @file   ez_queue.h
 *  @author Hai Nguyen
 *  @date   26.02.2024
 *  @brief  Public API of the queue component.
 *
 *  @details FIFO queue of variable-size elements. Elements are copied into a
 *  buffer provided by the caller; static storage only, no dynamic allocation.
 *  The module is compiled only when EZ_QUEUE is 1U. The API takes no lock, so
 *  concurrent access to one queue must be serialized by the caller.
 */

#ifndef _EZ_QUEUE_H
#define _EZ_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_QUEUE == 1U)
#include <stdint.h>
#include <stdbool.h>
#include "ez_utilities_common.h"
#include "ez_linked_list.h"
#include "ez_static_alloc.h"


/*****************************************************************************
* Module Preprocessor Macros
*****************************************************************************/
/* None */

/*****************************************************************************
* Module Typedefs
*****************************************************************************/

/** @brief ezQueue type. */
typedef struct ezQueue ezQueue;


/** @brief Construction of the queue. Use ezQueue_CreateQueue to initialize it. */
struct ezQueue
{
    struct Node q_item_list;    /**< list of queue element */
    struct MemList mem_list;    /**< memory list, needed for static memory allocation*/
    uint32_t num_of_element;    /**< number of element in the queue */
};


/** @brief Handle of an element reserved by ezQueue_ReserveElement. */
typedef void* ezReservedElement;

/*****************************************************************************
* Module Variable Definitions
*****************************************************************************/
/* None */


/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function : ezQueue_CreateQueue
*//** 
* @brief This function creates a data queue
*
* @details The queue stores its elements (copy of the data plus a header) in
* the buffer given by the caller. The whole buffer is cleared (set to 0) by this
* function. The caller owns the buffer and it must stay valid as long as the
* queue is used. The usable size is reduced by the alignment padding and the
* bookkeeping records of the static allocator (see ez_static_alloc.h).
*
* @param[in]    queue: Pointer to the queue structure, see ezQueue
* @param[in]    buff: Memory buffer given to the queue to work with
* @param[in]    buff_size: Size of the memory buffer in bytes
* @return       ezSTATUS
*
* @retval ezSUCCESS    Queue created and empty
* @retval ezFAIL       queue or buff is NULL, buff_size is 0, or the buffer is
*                      too small for the static allocator
*
* @pre None
* @post The queue is empty and ready (see ezQueue_IsQueueReady)
* @isr_safe{no}
* @blocking{no}
* @see ezQueue_IsQueueReady
*
* \b Example
* @code
* ezQueue queue;
* uint8_t queue_buff[128] = {0};
* if (ezQueue_CreateQueue(&queue, queue_buff, sizeof(queue_buff)) != ezSUCCESS)
* {
*     HandleError();
* }
* @endcode
*
*****************************************************************************/
ezSTATUS ezQueue_CreateQueue(ezQueue *queue, uint8_t *buff, uint32_t buff_size);


/*****************************************************************************
* Function : ezQueue_PopFront
*//** 
* @brief This function pops the front element out of the queue
*
* @details The element is removed and its memory is given back to the queue.
* Pointers previously obtained from ezQueue_GetFront for this element become
* invalid.
*
* @param[in]    queue: Pointer to the queue structure, see ezQueue
* @return       ezSTATUS
*
* @retval ezSUCCESS    An element was removed
* @retval ezFAIL       queue is NULL, the queue is empty, or freeing the
*                      element failed (the element is removed from the queue
*                      anyway)
*
* @pre queue must be initialized with ezQueue_CreateQueue
* @post Number of elements decreased by one if the queue was not empty
* @isr_safe{no}
* @blocking{no}
* @see ezQueue_CreateQueue, ezQueue_PopBack, ezQueue_GetFront
*
* \b Example
* @code
* if (ezQueue_PopFront(&queue) != ezSUCCESS)
* {
*     HandleError();
* }
* @endcode
*
*****************************************************************************/
ezSTATUS ezQueue_PopFront(ezQueue *queue);


/*****************************************************************************
* Function : ezQueue_PopBack
*//** 
* @brief This function pops the back element out of the queue
*
* @details The element is removed and its memory is given back to the queue.
* Pointers previously obtained from ezQueue_GetBack for this element become
* invalid.
*
* @param[in]    queue: Pointer to the queue structure, see ezQueue
* @return       ezSTATUS
*
* @retval ezSUCCESS    An element was removed
* @retval ezFAIL       queue is NULL, the queue is empty, or freeing the
*                      element failed (the element is removed from the queue
*                      anyway)
*
* @pre queue must be initialized with ezQueue_CreateQueue
* @post Number of elements decreased by one if the queue was not empty
* @isr_safe{no}
* @blocking{no}
* @see ezQueue_CreateQueue, ezQueue_PopFront, ezQueue_GetBack
*
* \b Example
* @code
* if (ezQueue_PopBack(&queue) != ezSUCCESS)
* {
*     HandleError();
* }
* @endcode
*
*****************************************************************************/
ezSTATUS ezQueue_PopBack(ezQueue *queue);


/*****************************************************************************
* Function : ezQueue_ReserveElement
*//** 
* @brief This function reserves an element in the queue.
*
* @details This function reserves an element but does not link it to the queue
* and returns the pointer to the memory buff of the element. To link it to the
* queue user must call ezQueue_PushReservedElement(). In case this reserved
* element is not needed, the user MUST call ezQueue_ReleaseReservedElement()
* to "free" the element. This function gives the ability to block the memory
* first, and let the user to write the data into the queue later.
*
* @param[in]    queue: Pointer to the queue structure, see ezQueue
* @param[out]   data: Receives the pointer to the data area of the reserved
*                      element. Not modified on failure.
* @param[in]    data_size: Size of the data area in bytes, must be > 0
* @return       ezReservedElement
*
* @retval handle    Element reserved, to be given to ezQueue_PushReservedElement
*                   or ezQueue_ReleaseReservedElement
* @retval NULL      queue or data is NULL, data_size is 0 or too large, or the
*                   queue has not enough free memory
*
* @pre queue must be initialized with ezQueue_CreateQueue
* @post The element is not counted by ezQueue_GetNumOfElement until it is pushed
* @warning Do not write more than data_size bytes into the data area.
* @isr_safe{no}
* @blocking{no}
* @see ezQueue_CreateQueue, ezQueue_PushReservedElement, ezQueue_ReleaseReservedElement
*
* \b Example
* @code
* ezReservedElement elem;
* uint8_t *buff = NULL;
*
* elem = ezQueue_ReserveElement(&queue, (void**)&buff, 32);
* if (elem != NULL)
* {
*     memset(buff, 0xaa, 32);
*     if (ezQueue_PushReservedElement(&queue, elem) != ezSUCCESS)
*     {
*         HandleError();
*     }
* }
* @endcode
*
* @see ezQueue_CreateQueue, ezQueue_PushReservedElement, ezQueue_ReleaseReservedElement
*
*****************************************************************************/
ezReservedElement ezQueue_ReserveElement(ezQueue* queue, void **data, size_t data_size);



/*****************************************************************************
* Function : ezQueue_PushReservedElement
*//** 
* @brief This function links the reserved element to the queue
*
* @details The element is appended at the back of the queue.
*
* @param[in]    queue: Pointer to the queue structure, see ezQueue
* @param[in]    element: Reserved element returned by ezQueue_ReserveElement
* @return       ezSTATUS
*
* @retval ezSUCCESS    Element appended to the queue
* @retval ezFAIL       queue or element is NULL, the element is already linked
*                      in the queue, or linking failed
*
* @pre queue is initialized and element is created using ezQueue_ReserveElement
* @post Number of elements increased by one on success
* @isr_safe{no}
* @blocking{no}
* @see ezQueue_CreateQueue, ezQueue_ReserveElement, ezQueue_ReleaseReservedElement
*
* \b Example
* @code
* ezReservedElement elem;
* uint8_t *buff = NULL;
*
* elem = ezQueue_ReserveElement(&queue, (void**)&buff, 4);
* if (elem != NULL)
* {
*     buff[0] = 1U;
*     if (ezQueue_PushReservedElement(&queue, elem) != ezSUCCESS)
*     {
*         HandleError();
*     }
* }
* @endcode
*
*****************************************************************************/
ezSTATUS ezQueue_PushReservedElement(ezQueue *queue, ezReservedElement element);


/*****************************************************************************
* Function : ezQueue_ReleaseReservedElement
*//** 
* @brief release the reserve element when it is not needed
*
* @details The memory of the element is given back to the queue. Only an element
* that was not pushed to the queue can be released; use ezQueue_PopFront or
* ezQueue_PopBack for elements in the queue.
*
* @param[in]    queue: Pointer to the queue structure, see ezQueue
* @param[in]    element: Reserved element returned by ezQueue_ReserveElement
* @return       ezSTATUS
*
* @retval ezSUCCESS    Element released
* @retval ezFAIL       queue or element is NULL, the element is linked in the
*                      queue, or freeing the memory failed
*
* @pre queue is initialized and element is created using ezQueue_ReserveElement
* @post The memory of the element can be reserved again
* @isr_safe{no}
* @blocking{no}
* @see ezQueue_CreateQueue, ezQueue_ReserveElement, ezQueue_PushReservedElement
*
* \b Example
* @code
* ezReservedElement elem;
* uint8_t *buff = NULL;
*
* elem = ezQueue_ReserveElement(&queue, (void**)&buff, 16);
* if (elem != NULL)
* {
*     if (ezQueue_ReleaseReservedElement(&queue, elem) != ezSUCCESS)
*     {
*         HandleError();
*     }
* }
* @endcode
*
*****************************************************************************/
ezSTATUS ezQueue_ReleaseReservedElement(ezQueue *queue, ezReservedElement element);


/*****************************************************************************
* Function : ezQueue_Push
*//** 
* @brief This function pushes data to the queue
*
* @details The data is copied into the queue and appended at the back.
*
* @param[in]    queue: Pointer to the queue structure, see ezQueue
* @param[in]    data: Data to be copied into the queue
* @param[in]    data_size: Size of the data in bytes, must be > 0
* @return       ezSTATUS
*
* @retval ezSUCCESS    Data copied and appended to the queue
* @retval ezFAIL       queue or data is NULL, data_size is 0, or the queue has
*                      not enough free memory
*
* @pre queue is initialized
* @post Number of elements increased by one on success
* @isr_safe{no}
* @blocking{no}
* @see ezQueue_CreateQueue, ezQueue_ReserveElement
*
* \b Example
* @code
* uint8_t item[3] = {1, 2, 3};
* if (ezQueue_Push(&queue, item, sizeof(item)) != ezSUCCESS)
* {
*     HandleError();
* }
* @endcode
*
*****************************************************************************/
ezSTATUS ezQueue_Push(ezQueue* queue, void *data, uint32_t data_size);


/*****************************************************************************
* Function : ezQueue_GetFront
*//** 
* @brief This function let the user access to the front element of the queue.
*
* @details The element stays in the queue. The returned pointer refers to the
* data stored in the queue (by reference, no copy). Since the users have the
* access to the queue it is NOT SAFE to write more than the size of this element.
*
* @param[in]    queue: Pointer to the queue structure, see ezQueue
* @param[out]   data: Receives the pointer to the data of the front element
* @param[out]   data_size: Receives the size of the data in bytes
* @return       ezSTATUS
*
* @retval ezSUCCESS    data and data_size are set
* @retval ezFAIL       queue, data or data_size is NULL, or the queue is empty
*
* @pre queue is initialized
* @post None
* @warning The pointer is invalid once the element is popped.
* @isr_safe{no}
* @blocking{no}
* @see ezQueue_CreateQueue, ezQueue_PopFront, ezQueue_GetBack
*
* \b Example
* @code
* uint8_t *front_element_data = NULL;
* size_t data_size = 0;
* if (ezQueue_GetFront(&queue, (void**)&front_element_data, &data_size) != ezSUCCESS)
* {
*     HandleError();
* }
* @endcode
*
*****************************************************************************/
ezSTATUS ezQueue_GetFront(ezQueue *queue, void **data, size_t *data_size);


/*****************************************************************************
* Function : ezQueue_GetBack
*//** 
* @brief This function let the user access to the back element of the queue.
*
* @details The element stays in the queue. The returned pointer refers to the
* data stored in the queue (by reference, no copy). Since the users have the
* access to the queue it is NOT SAFE to write more than the size of this element.
*
* @param[in]    queue: Pointer to the queue structure, see ezQueue
* @param[out]   data: Receives the pointer to the data of the back element
* @param[out]   data_size: Receives the size of the data in bytes
* @return       ezSTATUS
*
* @retval ezSUCCESS    data and data_size are set
* @retval ezFAIL       queue, data or data_size is NULL, or the queue is empty
*
* @pre queue is initialized
* @post None
* @warning The pointer is invalid once the element is popped.
* @isr_safe{no}
* @blocking{no}
* @see ezQueue_CreateQueue, ezQueue_PopBack, ezQueue_GetFront
*
* \b Example
* @code
* uint8_t *back_element_data = NULL;
* size_t data_size = 0;
* if (ezQueue_GetBack(&queue, (void**)&back_element_data, &data_size) != ezSUCCESS)
* {
*     HandleError();
* }
* @endcode
*
*****************************************************************************/
ezSTATUS ezQueue_GetBack(ezQueue* queue, void **data, size_t *data_size);


/*****************************************************************************
* Function : ezQueue_GetNumOfElement
*//** 
* @brief Get the number of elements in the queue
*
* @details Reserved elements that are not pushed yet are not counted.
*
* @param[in]    queue: Pointer to the queue structure, see ezQueue
* @return       Number of elements in the queue, 0 if queue is NULL
*
* @pre queue is initialized
* @post None
* @isr_safe{no}
* @blocking{no}
* @see ezQueue_CreateQueue
*
* \b Example
* @code
* uint32_t queue_size = ezQueue_GetNumOfElement(&queue);
* @endcode
*
*****************************************************************************/
uint32_t ezQueue_GetNumOfElement(ezQueue* queue);


/*****************************************************************************
* Function : ezQueue_IsQueueReady
*//** 
* @brief This function returns ready status of the queue
*
* @details The queue is ready when its memory list was successfully set up by
* ezQueue_CreateQueue.
*
* @param[in]    queue: Pointer to the queue structure, see ezQueue
* @return       bool
*
* @retval true    Queue is ready
* @retval false   queue is NULL or the queue is not created
*
* @pre None
* @post None
* @isr_safe{no}
* @blocking{no}
* @see ezQueue_CreateQueue
*
* \b Example
* @code
* if (!ezQueue_IsQueueReady(&queue))
* {
*     HandleError();
* }
* @endcode
*
*****************************************************************************/
bool ezQueue_IsQueueReady(ezQueue *queue);

#endif /* EZ_QUEUE */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_QUEUE_H */

/* End of file */

