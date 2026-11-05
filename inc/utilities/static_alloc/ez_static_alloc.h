/*****************************************************************************
* Filename:         ez_static_alloc.h
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

/** @file   ez_static_alloc.h
 *  @author Hai Nguyen
 *  @date   26.02.2024
 *  @brief  Public API of the static allocation component
 *
 *  @details This is the implementation of the malloc and free but it works
 * on a static memory buffer.
 */

#ifndef EZ_STATIC_ALLOC_H
#define EZ_STATIC_ALLOC_H

#ifdef __cplusplus
extern "C" {
#endif


/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_STATIC_ALLOC == 1)

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "ez_linked_list.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/**@brief Alignment of the allocated memory in byte. Must be a power of two.
 *
 * Every address returned by ezStaticAlloc_Malloc() is a multiple of this
 * value and every request is rounded up to a multiple of it. Define it in the
 * build configuration to override the default (size of a pointer).
 */
#ifndef CONFIG_STATIC_ALLOC_ALIGNMENT
#define CONFIG_STATIC_ALLOC_ALIGNMENT   (sizeof(void*))
#endif /* CONFIG_STATIC_ALLOC_ALIGNMENT */


/*****************************************************************************
* Component Typedefs
*****************************************************************************/

/**@brief List to manage the memory block
 */
struct MemList
{
    struct Node free_list_head;
    /**< List to manage the free memory blocks */
    struct Node alloc_list_head;
    /**< List to manage the allocated blocks*/
    uint8_t* buff;
    /**< Pointer to the memory buffer */
    size_t buff_size;
    /**< Size of the managed buffer in byte */
};


/**@brief Record of a memory chunk
 *
 * The record is stored inside the managed buffer, directly in front of the data
 * it describes. It is taken from the buffer together with the data when a chunk
 * is allocated and given back with it when the chunk is freed.
 */
struct MemBlock
{
    struct Node node;   /**< Linked list node */
    void* buff;         /**< Pointer to the data of the chunk */
    size_t buff_size;   /**< Size of the data of the chunk in byte */
};


/**@brief Define ezmMemList data type
 */
typedef struct MemList ezmMemList;


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
/* None */

/******************************************************************************
* Function Prototypes
*******************************************************************************/

/*****************************************************************************
* Function : ezStaticAlloc_InitMemList
*//** 
* @brief This function initializes memory handle to manage the memory buffer
*
* @details After the initialization, the memory buffer can not be used
* directly, but through the ezStaticAlloc API
*
* The handle is only marked as ready when the initialization succeeded. The
* records which track the chunks are stored inside the buffer, in front of the
* data of each chunk (see struct MemBlock), so every allocation uses
* sizeof(struct MemBlock) (rounded up to the alignment) bytes more than
* requested. Initialization fails if the buffer can not hold one record and one
* unit of data.
*
* The managed area starts at the first address inside the buffer which is a
* multiple of CONFIG_STATIC_ALLOC_ALIGNMENT and its size is rounded down to a
* multiple of it. Up to (alignment - 1) bytes at each end of the buffer are
* therefore not used, and initialization fails if nothing is left.
*
* The caller owns the buffer; it must stay valid while the list is used. Any
* previous state of the handle is discarded, chunks which were still allocated
* are forgotten.
*
* @param[in]    mem_list:       handle to manage memory buffer
* @param[in]    buff:           buffer to be managed
* @param[in]    buff_size:      size of the buffer in bytes
* @return       true if the initialization is success, false if mem_list or
*               buff is NULL, or the buffer is too small (see above). The handle
*               is not ready in that case.
*
* @pre None
* @post On success the handle is ready and holds one free block that spans the
* managed area
* @isr_safe{no}
* @blocking{no}
* @see ezStaticAlloc_DeinitMemList
*
* \b Example
* @code
* ezmMemList mem_list;
* uint8_t buff[128];
* if (!ezStaticAlloc_InitMemList(&mem_list, buff, sizeof(buff)))
* {
*     HandleError();
* }
* @endcode
*
*****************************************************************************/
bool ezStaticAlloc_InitMemList(ezmMemList* mem_list, void* buff, size_t buff_size);


/*****************************************************************************
* Function : ezStaticAlloc_DeinitMemList
*//** 
* @brief Release a memory list which was initialized before
*
* @details The handle is not ready anymore. Pointers returned by
* ezStaticAlloc_Malloc() of this list must not be used after this call.
*
* @param[in]    mem_list:  handle to manage memory buffer
* @return       true if the list was ready and is released, false if mem_list
*               is NULL or not ready
*
* @pre mem_list must initialized
* @post mem_list is not ready
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* if (!ezStaticAlloc_DeinitMemList(&mem_list))
* {
*     HandleError();
* }
* @endcode
*
* @see ezStaticAlloc_InitMemList
*
*****************************************************************************/
bool ezStaticAlloc_DeinitMemList(ezmMemList* mem_list);


/*****************************************************************************
* Function : ezStaticAlloc_IsMemListReady
*//** 
* @brief Return the status if the mem list is ready 
*
* @details List is ready when it is initialized with the function
* ezStaticAlloc_InitMemList
*
* @param[in]    mem_list:  handle to manage memory buffer
* @return       true if ready, false if mem_list is NULL or not ready
*
* @pre None
* @post None
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* if (!ezStaticAlloc_IsMemListReady(&mem_list))
* {
*     HandleError();
* }
* @endcode
*
* @see ezStaticAlloc_InitMemList
*
*****************************************************************************/
bool ezStaticAlloc_IsMemListReady(ezmMemList *mem_list);


/*****************************************************************************
* Function : ezStaticAlloc_Malloc
*//** 
* @brief This function allocate the number of bytes in the initialized memory
*
* @details This is the implementation of the malloc() function
*
* The search is first fit over the free blocks. The memory is not cleared
* by this function (it is zeroed when it is freed).
*
* @param[in]    mem_list:   handle to manage memory buffer
* @param[in]    alloc_size: number of byte to be allocated
* @return       address of the allocated memory, NULL if the list is NULL or not
*               ready, the size is 0 or there is no free block that is big
*               enough
*
* @note The record of the chunk is taken from the buffer in front of the data,
* so the allocation fails if the record and the data do not fit together. The
* record is released together with the data by ezStaticAlloc_Free().
* The returned address is aligned to CONFIG_STATIC_ALLOC_ALIGNMENT and
* the size is rounded up to a multiple of it. The size is not limited to 16 bit.
*
* @pre mem_list must initialized
* @post The returned block is in the allocated list
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* ezmMemList mem_list;
* uint8_t buff[128];
* if (!ezStaticAlloc_InitMemList(&mem_list, buff, sizeof(buff)))
* {
*     HandleError();
* }
* uint32_t *foo = (uint32_t*)ezStaticAlloc_Malloc(&mem_list, sizeof(uint32_t));
* if (foo == NULL)
* {
*     HandleError();
* }
* @endcode
*
* @see ezStaticAlloc_InitMemList
* @see ezStaticAlloc_Free
*
*****************************************************************************/
void *ezStaticAlloc_Malloc(ezmMemList* mem_list, size_t alloc_size);


/*****************************************************************************
* Function : ezStaticAlloc_Free
*//** 
* @brief This function frees the allocated memory in the memory buffer
*
* @details Implementation of the free() function
*
* The data of the chunk is set to 0 and the chunk (record and data) is merged
* with its free neighbours.
*
* @param[in]    mem_list:   handle to manage memory buffer
* @param[in]    alloc_addr: the address returned by ezStaticAlloc_Malloc() of
*                           the same list
* @return       true if free is success, false if mem_list is NULL or not
*               ready, alloc_addr is NULL, or it is not the start of an
*               allocated block of this list (for example a double free)
*
* @pre mem_list must initialized
* @post The block and its content are released, the pointer must not be used
* anymore
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* ezmMemList mem_list;
* uint8_t buff[128];
* if (!ezStaticAlloc_InitMemList(&mem_list, buff, sizeof(buff)))
* {
*     HandleError();
* }
* uint32_t *foo = (uint32_t*)ezStaticAlloc_Malloc(&mem_list, sizeof(uint32_t));
* if (foo == NULL)
* {
*     HandleError();
* }
*
* if (!ezStaticAlloc_Free(&mem_list, foo))
* {
*     HandleError();
* }
* @endcode
*
* @see ezStaticAlloc_Malloc
*
*****************************************************************************/
bool ezStaticAlloc_Free(ezmMemList *mem_list, void *alloc_addr);


/*****************************************************************************
* Function : ezStaticAlloc_ReserveMemoryBlock
*//** 
* @brief Reserve a chunk of a free list without moving it to the alloc list
*
* @details Advanced function for modules which manage the life cycle of a
* block on their own (for example ez_ipc). The first free block that is big
* enough is split if possible and returned. The size is rounded up to a
* multiple of CONFIG_STATIC_ALLOC_ALIGNMENT. The returned node stays in the
* free list and must be moved with ezStaticAlloc_MoveBlock() or unlinked.
*
* @param[in]    free_list_head:     head of the free list
* @param[in]    block_size_byte:    number of byte to be reserved
* @return       node of the reserved block, NULL if free_list_head is NULL,
*               the size is 0 or too big, or no free block is big enough
*
* @pre free_list_head belongs to an initialized mem list
* @post None
* @isr_safe{no}
* @blocking{no}
*
* @see ezStaticAlloc_MoveBlock
*
*****************************************************************************/
struct Node* ezStaticAlloc_ReserveMemoryBlock(struct Node* free_list_head, size_t block_size_byte);


/*****************************************************************************
* Function : ezStaticAlloc_MoveBlock
*//** 
* @brief Move a block from one list to the tail of another list
*
* @details Advanced function, see ezStaticAlloc_ReserveMemoryBlock
*
* @param[in]    move_node:          node of the block
* @param[in]    from_list_head:     list that contains the node
* @param[in]    to_list_head:       destination list
* @return       true if the block is moved, false if an argument is NULL or
*               the node is not in from_list_head
*
* @pre None
* @post On success the node is at the tail of to_list_head
* @isr_safe{no}
* @blocking{no}
* @see ezStaticAlloc_ReserveMemoryBlock
*
*****************************************************************************/
bool ezStaticAlloc_MoveBlock(struct Node* move_node, struct Node* from_list_head, struct Node* to_list_head);


/*****************************************************************************
* Function : ezStaticAlloc_GetNumOfAllocBlock
*//** 
* @brief Return the number of allocated memory block
*
* @details The blocks of the allocated list are counted.
*
* @param[in]    mem_list: handle to manage memory buffer
* @return       Number of allocated block, 0 if mem_list is NULL
*
* @pre mem_list must initialized
* @post None
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* uint16_t num = ezStaticAlloc_GetNumOfAllocBlock(&mem_list);
* @endcode
*
* @see ezStaticAlloc_InitMemList
*
*****************************************************************************/
uint16_t ezStaticAlloc_GetNumOfAllocBlock(ezmMemList* mem_list);


/*****************************************************************************
* Function : ezStaticAlloc_GetNumOfFreeBlock
*//** 
* @brief Return the number of freed memory block
*
* @details The blocks of the free list are counted. Neighbouring free blocks
* are merged, so this is the number of separate free areas.
*
* @param[in]    mem_list: handle to manage memory buffer
* @return       Number of freed block, 0 if mem_list is NULL
*
* @pre mem_list must initialized
* @post None
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* uint16_t num = ezStaticAlloc_GetNumOfFreeBlock(&mem_list);
* @endcode
*
* @see ezStaticAlloc_InitMemList
*
*****************************************************************************/
uint16_t ezStaticAlloc_GetNumOfFreeBlock(ezmMemList* mem_list);


/*****************************************************************************
* Function : ezStaticAlloc_HexdumpBuffer
*//** 
* @brief This function prints the content of the memory list.
*
* @details For debugging purpose. The function prints only if the source is
* built with STCMEM_VERBOSE set to 1 and the module debug output enabled,
* otherwise it does nothing.
*
* @param[in]    mem_list: handle to manage memory buffer
*
* @pre mem_list must initialized
* @post None
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* ezStaticAlloc_HexdumpBuffer(&mem_list);
* @endcode
*
* @see ezStaticAlloc_InitMemList
*
*****************************************************************************/
void ezStaticAlloc_HexdumpBuffer (ezmMemList* mem_list);


/*****************************************************************************
* Function : ezStaticAlloc_PrintFreeList
*//** 
* @brief This function prints the memory headers of the free list.
*
* @details For debugging purpose. The function prints only if the source is
* built with STCMEM_VERBOSE set to 1 and the module debug output enabled,
* otherwise it does nothing.
*
* @param[in]    mem_list:  handle to manage memory buffer
*
* @pre  mem_list must initialized
* @post None
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* ezStaticAlloc_PrintFreeList(&mem_list);
* @endcode
*
* @see ezStaticAlloc_InitMemList
*
*****************************************************************************/
void ezStaticAlloc_PrintFreeList (ezmMemList* mem_list);


/*****************************************************************************
* Function : ezStaticAlloc_PrintAllocList
*//** 
* @brief This function prints the memory headers of the allocated list.
*
* @details For debugging purpose. The function prints only if the source is
* built with STCMEM_VERBOSE set to 1 and the module debug output enabled,
* otherwise it does nothing.
*
* @param[in]    mem_list:  handle to manage memory buffer
*
* @pre  mem_list must initialized
* @post None
* @isr_safe{no}
* @blocking{no}
*
* \b Example
* @code
* ezStaticAlloc_PrintAllocList(&mem_list);
* @endcode
*
* @see ezStaticAlloc_InitMemList
*
*****************************************************************************/
void ezStaticAlloc_PrintAllocList(ezmMemList* mem_list);

#ifdef __cplusplus
}
#endif

#endif /* EZ_STATIC_ALLOC == 1*/
#endif /* EZ_STATIC_ALLOC_H */

/* End of file*/
