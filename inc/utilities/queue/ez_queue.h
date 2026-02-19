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
 *  @brief  Public APi of the queue component
 *
 *  @details Implemenation of the queue data structure
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

/** @brief define ezQueue type
 */
typedef struct ezQueue ezQueue;


/** @brief Construction of the queue
 */
struct ezQueue
{
    struct Node q_item_list;    /**< list of queue element */
    struct MemList mem_list;    /**< memory list, needed for static memory allocation*/
};


/** @brief queue structure
 */
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
* @param    *queue: (IN)pointer to the a queue structure, see ezQueue
* @param    *buff: (IN) memory buffer providind to the queue to work
* @param    *buff_size: (IN)size of the memory buffer
* @return   ezSUCCESS or ezFAIL
*
* @pre None
* @post None
*
* @code
* ezQueue queue;
* uint8_t queue_buff[32] = {0};
* ezSTATUS status = ezQueue_CreateQueue(&queue, queue_buff, 32);
* if(status == ezSUCCESS)
* {
*     printf("Success");
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
* @details
*
* @param    *queue: (IN)pointer to the a queue structure, see ezQueue
* @return   ezSUCCESS or ezFAIL
*
* @pre queue must be initialized
* @post None
*
* @code
* if(ezQueue_Pop(&queue) == ezSUCCESS)
* {
*     printf("Success");
* }
* @endcode
*
* @see ezQueue_CreateQueue
*
*****************************************************************************/
ezSTATUS ezQueue_PopFront(ezQueue *queue);


/*****************************************************************************
* Function : ezQueue_PopBack
*//** 
* @brief This function pops the back element out of the queue
*
* @details
*
* @param    *queue: (IN)pointer to the a queue structure, see ezQueue
* @return   ezSUCCESS or ezFAIL
*
* @pre Precondition
* @post Postcondition
*
* @code
* if(ezQueue_Pop(&queue) == ezSUCCESS)
* {
*     printf("Success");
* }
* @endcode
*
* @see ezQueue_CreateQueue
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
* @param    *queue: (IN)pointer to the a queue structure, see ezQueue
* @param    **data: (IN)pointer to the reserve memory block
* @param    data_size: (IN)size of the reserve memeory
* @return   NULL if fail
*
* @pre queue must be initialized
* @post Postcondition
*
* @code
* ezReservedElement elem;
* 
* elem = ezQueue_ReserveElement(&queue, buff, 32);
* if(elem != NULL)
* {
*     memset(buff, 0xaa, 32);
*     ezQueue_PushReservedElement(&queue, elem);
* }
* @endcode
*
* @see ezQueue_CreateQueue, ezQueue_PushReservedElement, ezQueue_ReleaseReservedElement
*
*****************************************************************************/
ezReservedElement ezQueue_ReserveElement(ezQueue* queue, void **data, uint32_t data_size);



/*****************************************************************************
* Function : ezQueue_PushReservedElement
*//** 
* @brief This function links the reserved element to the queue
* @details
*
* @param    *queue: (IN)pointer to the a queue structure, see ezQueue
* @param    element: (IN)reserved element
* @return   ezSUCCESS or ezFAIL
*
* @pre queue is initialized and element is created using ezQueue_ReserveElement
* @post Postcondition
*
* @code
* @endcode
*
* @see ezQueue_CreateQueue, ezQueue_ReserveElement
*
*****************************************************************************/
ezSTATUS ezQueue_PushReservedElement(ezQueue *queue, ezReservedElement element);


/*****************************************************************************
* Function : ezQueue_ReleaseReservedElement
*//** 
* @brief release the reserve element when it is not nedded
* @details
*
* @param    *queue: (IN)pointer to the a queue structure, see ezQueue
* @param    element: (IN)reserved element
* @return   ezSUCCESS or ezFAIL
*
* @pre queue is initialized and element is created using ezQueue_ReserveElement
* @post Postcondition
*
* @code
* @endcode
*
* @see ezQueue_CreateQueue, ezQueue_ReserveElement
*
*****************************************************************************/
ezSTATUS ezQueue_ReleaseReservedElement(ezQueue *queue, ezReservedElement element);


/*****************************************************************************
* Function : ezQueue_Push
*//** 
* @brief This function pushes data to the queue
*
* @details
*
* @param    *queue: (IN)pointer to the a queue structure, see ezQueue
* @param    *data: (IN)data to be written in the queue
* @param    data_size: (IN)size of the data written to the queue
* @return   ezSUCCESS if success
*           ezFAIL: if the queue is full or invalid function arguments
*
* @pre queue is initialized
* @post None
*
* @code
* @endcode
*
* @see ezQueue_CreateQueue
*
*****************************************************************************/
ezSTATUS ezQueue_Push(ezQueue* queue, void *data, uint32_t data_size);


/*****************************************************************************
* Function : ezQueue_GetFront
*//** 
* @brief This function let the user access to the front element of the queue.
*
* @details Since the users have the access to the queue it is NOT SAFE to write
* more than the size of this element
*
* @param    *queue: (IN)pointer to the a queue structure, see ezQueue
* @param    **data: (OUT)pointer the the data of the front element
* @param    *data_size: (OUT)pointer to the size of data
* @return   ezSUCCESS if success
*           ezFAIL: if the queue is empty or invalid function arguments
*
* @pre queue is initialized
* @post None
*
* @code
* uint8_t *front_element_data = NULL;
* uint32_t *data_size = NULL;
* if(ezQueue_GetFront(&queue, &front_element_data, &data_size) == ezSUCCESS)
* {
*     printf("Success");
* }
* @endcode
*
* @see ezQueue_CreateQueue
*
*****************************************************************************/
ezSTATUS ezQueue_GetFront(ezQueue *queue, void **data, uint32_t *data_size);


/*****************************************************************************
* Function : ezQueue_GetBack
*//** 
* @brief This function let the user access to the back element of the queue.
*
* @details Since the users have the access to the queue it is NOT SAFE to write
* more than the size of this element
*
* @param    *queue: (IN)pointer to the a queue structure, see ezQueue
* @param    **data: (OUT)pointer the the data of the back element
* @param    *data_size: (OUT)pointer to the size of data
* @return   ezSUCCESS if success
*           ezFAIL: if the queue is empty or invalid function arguments
*
* @pre queue is initialized
* @post Postcondition
*
* @code
* uint8_t *back_element_data = NULL;
* uint32_t *data_size = NULL;
* if(ezQueue_GetFront(&queue, &back_element_data, &data_size) == ezSUCCESS)
* {
*     printf("Success");
* }
* @endcode
*
* @see ezQueue_CreateQueue
*
*****************************************************************************/
ezSTATUS ezQueue_GetBack(ezQueue* queue, void **data, uint32_t *data_size);


/*****************************************************************************
* Function : sum
*//** 
* @brief ezQueue_GetNumOfElement
*
* @details
*
* @param    *queue: (IN)pointer to the a queue structure, see ezQueue
* @return   Number of elements
*
* @pre queue is initialized
* @post Postcondition
*
* @code
* uint32_t queue_size = ezQueue_GetNumOfElement(&queue);
* @endcode
*
* @see ezQueue_CreateQueue
*
*****************************************************************************/
uint32_t ezQueue_GetNumOfElement(ezQueue* queue);


/******************************************************************************
* Function : ezQueue_IsQueueReady
*//**
* @Description:
*
* This function returns readay status of the queue
*
* @param    *queue: (IN)pointer to the a queue structure, see ezQueue
* @return   True if ready, else false
*
*******************************************************************************/


/*****************************************************************************
* Function : ezQueue_IsQueueReady
*//** 
* @brief This function returns ready status of the queue
*
* @details Detail description
*
* @param    *queue: (IN)pointer to the a queue structure, see ezQueue
* @return   True if ready, else false
*
* @pre None
* @post None
*
* @code
* @endcode
*
* @see None
*
*****************************************************************************/
uint32_t ezQueue_IsQueueReady(ezQueue *queue);

#endif /* _EZ_QUEUE */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_QUEUE_H */

/* End of file */

