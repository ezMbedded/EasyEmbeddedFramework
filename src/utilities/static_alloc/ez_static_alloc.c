/*****************************************************************************
* Filename:         ez_static_alloc.c
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


/** @file   ez_static_alloc.c
 *  @author Hai Nguyen
 *  @date   26.02.2024
 *  @brief  Implementation of the static memory allocation
 *
 *  @details The managed buffer is split into blocks. The block descriptors
 *  come from a pool shared by all memory lists. The free list is always sorted
 *  by address, so a returned block only has to be merged with its direct
 *  neighbours.
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_static_alloc.h"

#if (EZ_STATIC_ALLOC == 1U)
#include "ez_hexdump.h"

#include "stdbool.h"
#include <string.h>

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#ifndef STCMEM_VERBOSE
#define STCMEM_VERBOSE  0U
#endif

#define MOD_NAME        "STATIC_MEM"

#if (MODULE_DEBUG == 1U) && (STCMEM_DEBUG == 1U)
    #define STCMEMPRINT(a)            PRINT_DEBUG(MOD_NAME,a)
    #define STCMEMPRINT1(a,b)         PRINT_DEBUG1(MOD_NAME,a,b)
    #define STCMEMPRINT2(a,b,c)       PRINT_DEBUG2(MOD_NAME,a,b,c)
    #define STCMEMPRINT3(a,b,c,d)     PRINT_DEBUG3(MOD_NAME,a,b,c,d)
    #define STCMEMPRINT4(a,b,c,d,e)   PRINT_DEBUG4(MOD_NAME,a,b,c,d,e)
    #define STCMEMHEXDUMP(a,b)        ezHexdump(a,b)
#else 
    #define STCMEMPRINT(a)
    #define STCMEMPRINT1(a,b)
    #define STCMEMPRINT2(a,b,c)
    #define STCMEMPRINT3(a,b,c,d)
    #define STCMEMPRINT4(a,b,c,d,e)
    #define STCMEMHEXDUMP(a,b)
#endif

#ifndef CONFIG_NUM_OF_MEM_BLOCK
#define CONFIG_NUM_OF_MEM_BLOCK 128
#endif /*CONFIG_NUM_OF_MEM_BLOCK*/

/* the alignment must be a power of two, otherwise the array size is negative */
typedef char ezStaticAlloc_AlignmentCheck[((CONFIG_STATIC_ALLOC_ALIGNMENT) > 0U
    && (((CONFIG_STATIC_ALLOC_ALIGNMENT) & ((CONFIG_STATIC_ALLOC_ALIGNMENT) - 1U)) == 0U)) ? 1 : -1];

#define INIT_BLOCK(block, buff_ptr, size) do { ezLinkedList_InitNode(&(block)->node); (block)->buff = (buff_ptr); (block)->buff_size = (size); } while (0)
#define GET_LIST(x) ((struct MemList*)(x))
#define GET_BLOCK(node_ptr) (EZ_LINKEDLIST_GET_PARENT_OF((node_ptr), node, struct MemBlock))

/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
static struct MemBlock block_pool[CONFIG_NUM_OF_MEM_BLOCK] = { 0U };


/*****************************************************************************
* Function Definitions
*****************************************************************************/
static void ezStaticAlloc_ReturnHeaderToFreeList(struct Node *free_list_head, struct Node *free_node);
static void ezStaticAlloc_MergeAround(struct Node *free_list_head, struct Node *free_node);

static size_t ezStaticAlloc_GetPadding(size_t value);
static struct MemBlock* ezStaticAlloc_GetFreeBlock(void);
static void ezStaticAlloc_ReleaseBlock(struct MemBlock* block);


/*****************************************************************************
* Public functions
*****************************************************************************/
bool ezStaticAlloc_InitMemList(ezmMemList* mem_list, void* buff, size_t buff_size)
{
    bool    is_success = false;
    struct MemBlock *free_block = NULL;
    uint8_t *aligned_buff = NULL;
    size_t  skipped_size = 0U;
    size_t  usable_size = 0U;
    uintptr_t buff_addr = (uintptr_t)buff;
    const size_t alignment = CONFIG_STATIC_ALLOC_ALIGNMENT;

    STCMEMPRINT("ezStaticAlloc_InitMemList()");
    STCMEMPRINT1("size = %u", (unsigned int)sizeof(struct MemList));

    if (mem_list != NULL)
    {
        /* the handle is not ready until everything below succeeded */
        GET_LIST(mem_list)->buff = NULL;
        GET_LIST(mem_list)->buff_size = 0U;

        /* The managed area starts at the first aligned address inside the
         * buffer and its size is a multiple of the alignment, so every block
         * start (and end) is aligned.
         *
         * Example with alignment = 8:
         *   buff_addr    = 0x1003
         *   skipped_size = 5       (padding to the aligned address 0x1008)
         *   buff_size    = 100  ->  usable_size = (100 - 5) rounded down to 88 */
        if (buff != NULL)
        {
            skipped_size = ezStaticAlloc_GetPadding((size_t)buff_addr);
        }

        /* the aligned address must not wrap around the address space */
        if (buff != NULL && buff_addr <= (UINTPTR_MAX - skipped_size))
        {
            if (buff_size > skipped_size)
            {
                /* round down: drop the tail bytes that do not fill a whole alignment unit */
                usable_size = buff_size - skipped_size;
                usable_size -= usable_size % alignment;
                aligned_buff = (uint8_t*)buff + skipped_size;
            }
        }

        if (aligned_buff != NULL && usable_size > 0U)
        {
            free_block = ezStaticAlloc_GetFreeBlock();

            if (NULL != free_block)
            {
                ezLinkedList_InitNode(&GET_LIST(mem_list)->alloc_list_head);
                ezLinkedList_InitNode(&GET_LIST(mem_list)->free_list_head);
                INIT_BLOCK(free_block, aligned_buff, usable_size);

                is_success = EZ_LINKEDLIST_ADD_HEAD(&GET_LIST(mem_list)->free_list_head, &free_block->node);

                if (is_success)
                {
                    GET_LIST(mem_list)->buff = aligned_buff;
                    GET_LIST(mem_list)->buff_size = usable_size;
                }
                else
                {
                    ezStaticAlloc_ReleaseBlock(free_block);
                }
            }
        }
    }

    ezStaticAlloc_PrintAllocList(mem_list);
    ezStaticAlloc_PrintFreeList(mem_list);
    return is_success;
}


bool ezStaticAlloc_DeinitMemList(ezmMemList* mem_list)
{
    bool            is_success = false;
    struct Node*    it_node = NULL;

    STCMEMPRINT("ezStaticAlloc_DeinitMemList()");

    if (ezStaticAlloc_IsMemListReady(mem_list))
    {
        /* give all block descriptors back to the pool */
        while ((it_node = ezLinkedList_RemoveFirst(&GET_LIST(mem_list)->free_list_head)) != NULL)
        {
            ezStaticAlloc_ReleaseBlock(GET_BLOCK(it_node));
        }

        while ((it_node = ezLinkedList_RemoveFirst(&GET_LIST(mem_list)->alloc_list_head)) != NULL)
        {
            ezStaticAlloc_ReleaseBlock(GET_BLOCK(it_node));
        }

        GET_LIST(mem_list)->buff = NULL;
        GET_LIST(mem_list)->buff_size = 0U;
        is_success = true;
    }

    return is_success;
}


void *ezStaticAlloc_Malloc(ezmMemList *mem_list, size_t alloc_size)
{
    void    *alloc_addr = NULL;
    bool    is_success = true;
    struct Node  *reserved_node = NULL;

    STCMEMPRINT("ezStaticAlloc_Malloc()");

    if (!ezStaticAlloc_IsMemListReady(mem_list) || 0U == alloc_size)
    {
        is_success = false;
    }

    if (is_success)
    {
        reserved_node = ezStaticAlloc_ReserveMemoryBlock(&GET_LIST(mem_list)->free_list_head, alloc_size);
    }

    if (NULL != reserved_node)
    {
        is_success = ezStaticAlloc_MoveBlock(reserved_node, &GET_LIST(mem_list)->free_list_head, &GET_LIST(mem_list)->alloc_list_head);

        if (is_success)
        {
            alloc_addr = GET_BLOCK(reserved_node)->buff;
        }
    }

    ezStaticAlloc_PrintFreeList(mem_list);
    ezStaticAlloc_PrintAllocList(mem_list);

    return alloc_addr;
}


bool ezStaticAlloc_Free(ezmMemList *mem_list, void *alloc_addr)
{
    bool        is_success = false;
    struct Node* it_node = NULL;

    STCMEMPRINT1("ezStaticAlloc_Free() - [address = %p]", alloc_addr);

    if (ezStaticAlloc_IsMemListReady(mem_list) && alloc_addr != NULL)
    {
        EZ_LINKEDLIST_FOR_EACH(it_node, &GET_LIST(mem_list)->alloc_list_head)
        {
            if (GET_BLOCK(it_node)->buff == (uint8_t*)alloc_addr)
            {
                EZ_LINKEDLIST_UNLINK_NODE(it_node);
                ezStaticAlloc_ReturnHeaderToFreeList(&GET_LIST(mem_list)->free_list_head, it_node);
                ezStaticAlloc_MergeAround(&GET_LIST(mem_list)->free_list_head, it_node);
                is_success = true;
                STCMEMPRINT("Free OK");
                break;
            }
        }
    }

    ezStaticAlloc_PrintFreeList(mem_list);
    ezStaticAlloc_PrintAllocList(mem_list);

    return is_success;
}


bool ezStaticAlloc_IsMemListReady(ezmMemList *mem_list)
{
    bool is_ready = false;

    if (mem_list != NULL && mem_list->buff != NULL && mem_list->buff_size > 0)
    {
        is_ready = true;
    }

    return is_ready;
}


void ezStaticAlloc_HexdumpBuffer(ezmMemList *mem_list)
{
#if (STCMEM_VERBOSE == 1U)
    if (mem_list)
    {
        struct MemList* list = GET_LIST(mem_list);
        STCMEMPRINT("mem list info");
        STCMEMPRINT1("[addr = %p]", list);
        STCMEMPRINT1("[alloc = %p]", (void*)&list->alloc_list_head);
        STCMEMPRINT1("[free = %p]", (void*)&list->free_list_head);
        STCMEMPRINT1("[buff = %p]", list->buff);
        STCMEMPRINT1("[size = %lu]", (unsigned long)list->buff_size);
        STCMEMHEXDUMP(list->buff, list->buff_size);
    }
#endif /* STCMEM_VERBOSE */
    (void)mem_list;
}


void ezStaticAlloc_PrintFreeList(ezmMemList *mem_list)
{
#if (STCMEM_VERBOSE == 1U)
    struct Node* it_node = NULL;

    if (!ezStaticAlloc_IsMemListReady(mem_list))
    {
        return;
    }

    STCMEMPRINT("*****************************************");
    STCMEMPRINT("free list");
    EZ_LINKEDLIST_FOR_EACH(it_node, &GET_LIST(mem_list)->free_list_head)
    {
        STCMEMPRINT1("[addr = %p]", it_node);
        STCMEMPRINT1("[next = %p]", it_node->next);
        STCMEMPRINT1("[prev = %p]", it_node->prev);
        STCMEMPRINT1("[buff = %p]", GET_BLOCK(it_node)->buff);
        STCMEMPRINT1("[size = %lu]", (unsigned long)GET_BLOCK(it_node)->buff_size);
        STCMEMPRINT("<======>");
    }
    STCMEMPRINT("*****************************************\n");
#endif
    (void)mem_list;
}


void ezStaticAlloc_PrintAllocList(ezmMemList * mem_list)
{
#if (STCMEM_VERBOSE == 1U)
    struct Node* it_node = NULL;

    if (!ezStaticAlloc_IsMemListReady(mem_list))
    {
        return;
    }

    STCMEMPRINT("*****************************************");
    STCMEMPRINT("allocated list");
    EZ_LINKEDLIST_FOR_EACH(it_node, &GET_LIST(mem_list)->alloc_list_head)
    {
        STCMEMPRINT1("[addr = %p]", it_node);
        STCMEMPRINT1("[next = %p]", it_node->next);
        STCMEMPRINT1("[prev = %p]", it_node->prev);
        STCMEMPRINT1("[buff = %p]", GET_BLOCK(it_node)->buff);
        STCMEMPRINT1("[size = %lu]", (unsigned long)GET_BLOCK(it_node)->buff_size);
        STCMEMPRINT("<======>");
    }
    STCMEMPRINT("*****************************************\n");
#endif
    (void)mem_list;
}


uint16_t ezStaticAlloc_GetNumOfAllocBlock(ezmMemList* mem_list)
{
    return (mem_list != NULL) ? ezLinkedList_GetListSize(&GET_LIST(mem_list)->alloc_list_head) : 0U;
}


uint16_t ezStaticAlloc_GetNumOfFreeBlock(ezmMemList* mem_list)
{
    return (mem_list != NULL) ? ezLinkedList_GetListSize(&GET_LIST(mem_list)->free_list_head) : 0U;
}
/**************************** Private function *******************************/

/******************************************************************************
* Function : ezStaticAlloc_ReturnHeaderToFreeList
*//**
* \b Description:
*
* This function returns the free memory header (and its buffer) to the free list
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
*
* @param    *free_list      free list the the header will be returned
* @param    *free_header    the header to be moved
*
* @return   None
*
*******************************************************************************/
static void ezStaticAlloc_ReturnHeaderToFreeList(struct Node* free_list_head, struct Node* free_node)
{
    struct Node* it_node = NULL;

    if (free_list_head != NULL && free_node != NULL)
    {
        memset(GET_BLOCK(free_node)->buff, 0, GET_BLOCK(free_node)->buff_size);

        /* The free list is sorted by address so merging only needs to look at
         * the direct neighbours. Stop at the first block located behind the
         * returned block, or at the head when there is none (append to tail). */
        EZ_LINKEDLIST_FOR_EACH(it_node, free_list_head)
        {
            if (GET_BLOCK(it_node)->buff > GET_BLOCK(free_node)->buff)
            {
                break;
            }
        }

        (void)ezLinkedList_AppendNode(free_node, it_node->prev);
    }
}

/******************************************************************************
* Function : ezStaticAlloc_MergeAround
*//**
* \b Description:
*
* This function tries to merge a block, which was just returned to the sorted
* free list, with its adjacent free blocks (next and previous)
*
* PRE-CONDITION: free list is sorted by address and was fully merged before
* free_node was inserted
*
* POST-CONDITION: None
*
* @param    *free_list_head     head of the free list
* @param    *free_node          the node which was just inserted
*
* @return   None
*
*******************************************************************************/
static void ezStaticAlloc_MergeAround(struct Node* free_list_head, struct Node* free_node)
{
    struct Node* next_node = NULL;
    struct Node* prev_node = NULL;

    if (free_list_head != NULL && free_node != NULL)
    {
        /* merge with the next block first so the previous block sees the full size */
        next_node = free_node->next;
        if (next_node != free_list_head &&
            ((uint8_t*)GET_BLOCK(free_node)->buff + GET_BLOCK(free_node)->buff_size) == (uint8_t*)GET_BLOCK(next_node)->buff)
        {
            STCMEMPRINT("Next adjacent block is free");
            GET_BLOCK(free_node)->buff_size = GET_BLOCK(free_node)->buff_size + GET_BLOCK(next_node)->buff_size;
            EZ_LINKEDLIST_UNLINK_NODE(next_node);
            ezStaticAlloc_ReleaseBlock(GET_BLOCK(next_node));
        }

        prev_node = free_node->prev;
        if (prev_node != free_list_head &&
            ((uint8_t*)GET_BLOCK(prev_node)->buff + GET_BLOCK(prev_node)->buff_size) == (uint8_t*)GET_BLOCK(free_node)->buff)
        {
            STCMEMPRINT("Previous adjacent block is free");
            GET_BLOCK(prev_node)->buff_size = GET_BLOCK(prev_node)->buff_size + GET_BLOCK(free_node)->buff_size;
            EZ_LINKEDLIST_UNLINK_NODE(free_node);
            ezStaticAlloc_ReleaseBlock(GET_BLOCK(free_node));
        }
    }
}

static struct MemBlock* ezStaticAlloc_GetFreeBlock(void)
{
    struct MemBlock* free_block = NULL;
    for (uint16_t i = 0; i < CONFIG_NUM_OF_MEM_BLOCK; i++)
    {
        if (block_pool[i].buff == NULL)
        {
            free_block = &block_pool[i];
            ezLinkedList_InitNode(&free_block->node);
            break;
        }
    }
    return free_block;
}

static void ezStaticAlloc_ReleaseBlock(struct MemBlock* block)
{
    INIT_BLOCK(block, NULL, 0U);
}

struct Node* ezStaticAlloc_ReserveMemoryBlock(struct Node* free_list_head, size_t block_size_byte)
{
    struct MemBlock* remain_block = NULL;
    struct Node* iterate_Node = NULL;
    struct Node* reserved_node = NULL;
    size_t       reserve_size = 0U;
    size_t       padding = 0U;

    STCMEMPRINT("ezStaticAlloc_ReserveMemoryBlock()");

    /* every block has a size which is a multiple of the alignment */
    padding = ezStaticAlloc_GetPadding(block_size_byte);

    /* the rounded size must not overflow */
    if (NULL != free_list_head && block_size_byte > 0U && block_size_byte <= (SIZE_MAX - padding))
    {
        reserve_size = block_size_byte + padding;

        EZ_LINKEDLIST_FOR_EACH(iterate_Node, free_list_head)
        {
            if (GET_BLOCK(iterate_Node)->buff_size >= reserve_size)
            {
                remain_block = NULL;
                if (GET_BLOCK(iterate_Node)->buff_size > reserve_size)
                {
                    remain_block = ezStaticAlloc_GetFreeBlock();
                }

                /* If no block descriptor is left, hand out the whole block
                 * instead of shrinking it, otherwise the remainder is lost. */
                if (remain_block)
                {
                    remain_block->buff_size = GET_BLOCK(iterate_Node)->buff_size - reserve_size;
                    remain_block->buff = (uint8_t*)GET_BLOCK(iterate_Node)->buff + reserve_size;
                    /* keep the free list sorted by address */
                    (void)ezLinkedList_AppendNode(&remain_block->node, iterate_Node);
                    GET_BLOCK(iterate_Node)->buff_size = reserve_size;
                }

                reserved_node = iterate_Node;
                break;
            }
        }
    }

    return reserved_node;
}

bool ezStaticAlloc_MoveBlock(struct Node* move_node, struct Node* from_list_head, struct Node* to_list_head)
{
    bool is_success = false;

    if (NULL != move_node 
        && NULL != from_list_head 
        && NULL != to_list_head
        && ezLinkedList_IsNodeInList(from_list_head, move_node))
    {
        EZ_LINKEDLIST_UNLINK_NODE(move_node);
        is_success = EZ_LINKEDLIST_ADD_TAIL(to_list_head, move_node);
    }

    return is_success;
}

/******************************************************************************
* Function : ezStaticAlloc_GetPadding
*//**
* \b Description:
*
* Number of bytes to add to a size or an address to reach the next multiple of
* CONFIG_STATIC_ALLOC_ALIGNMENT. 0 if the value is already aligned.
*
* For an address, pass the address cast to size_t: only the low bits matter and
* those survive the cast.
*
* @param    value:  size or address to be aligned
*
* @return   padding in bytes, always smaller than the alignment
*
*******************************************************************************/
static size_t ezStaticAlloc_GetPadding(size_t value)
{
    const size_t alignment = CONFIG_STATIC_ALLOC_ALIGNMENT;

    /* bytes past the last multiple of the alignment, e.g. 0x1003 % 8 = 3 */
    const size_t overshoot = value % alignment;

    /* bytes needed to reach the next multiple, e.g. 8 - 3 = 5 */
    return (overshoot == 0U) ? 0U : (alignment - overshoot);
}

#endif /* EZ_STATIC_ALLOC */
/* End of file */
