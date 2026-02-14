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
 *  @brief  Implementation of the static memory alloction
 *
 *  @details
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
#define VERBOSE         0U

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

#define INIT_BLOCK(block, buff_ptr, size) {ezLinkedList_InitNode(&block->node);block->buff = buff_ptr;block->buff_size = size; }
#define GET_LIST(x) ((struct MemList*)x)
#define GET_BLOCK(node_ptr) (EZ_LINKEDLIST_GET_PARENT_OF(node_ptr, node, struct MemBlock))

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
static void ezmSmalloc_Merge(struct Node *free_list_head);

struct Node* ezStaticAlloc_ReserveMemoryBlock(struct Node* free_list_head, uint16_t block_size_byte);
bool ezStaticAlloc_MoveBlock(struct Node* move_node, struct Node* from_list_head, struct Node* to_list_head);
struct MemBlock* GetFreeBlock(void);
void ReleaseBlock(struct MemBlock* block);


/*****************************************************************************
* Public functions
*****************************************************************************/
bool ezStaticAlloc_InitMemList(ezmMemList* mem_list, void* buff, uint16_t buff_size)
{
    bool    is_success = true;
    struct MemBlock *free_block = NULL;

    STCMEMPRINT("ezStaticAlloc_InitMemList()");
    STCMEMPRINT1("size = %d", sizeof(struct MemList));
    if (mem_list == NULL || buff == NULL || buff_size == 0)
    {
        is_success = false;
    }

    if (is_success)
    {
        GET_LIST(mem_list)->buff = buff;
        GET_LIST(mem_list)->buff_size = buff_size;
        ezLinkedList_InitNode(&GET_LIST(mem_list)->alloc_list_head);
        ezLinkedList_InitNode(&GET_LIST(mem_list)->free_list_head);

        free_block = GetFreeBlock();
        
        if (NULL != free_block)
        {
            INIT_BLOCK(free_block, buff, buff_size);

            is_success = is_success && EZ_LINKEDLIST_ADD_HEAD(&GET_LIST(mem_list)->free_list_head, &free_block->node);
        }
        else
        {
            is_success = false;
        }
    }

    ezStaticAlloc_PrintAllocList(mem_list);
    ezStaticAlloc_PrintFreeList(mem_list);
    return is_success;
}


void *ezStaticAlloc_Malloc(ezmMemList *mem_list, uint16_t alloc_size)
{
    void    *alloc_addr = NULL;
    bool    is_success = true;
    struct Node  *reserved_node = NULL;

    STCMEMPRINT("ezStaticAlloc_Malloc()");

    if (NULL == mem_list || 0U == alloc_size)
    {
        is_success = false;
    }

    if (is_success)
    {
        reserved_node = ezStaticAlloc_ReserveMemoryBlock(&GET_LIST(mem_list)->free_list_head, alloc_size);
    }

    if (NULL != reserved_node)
    {
        is_success = is_success && ezStaticAlloc_MoveBlock(reserved_node, &GET_LIST(mem_list)->free_list_head, &GET_LIST(mem_list)->alloc_list_head);

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

    if (mem_list != NULL && alloc_addr != NULL)
    {
        EZ_LINKEDLIST_FOR_EACH(it_node, &GET_LIST(mem_list)->alloc_list_head)
        {
            if (GET_BLOCK(it_node)->buff == (uint8_t*)alloc_addr)
            {
                EZ_LINKEDLIST_UNLINK_NODE(it_node);
                ezStaticAlloc_ReturnHeaderToFreeList(&GET_LIST(mem_list)->free_list_head, it_node);
                ezmSmalloc_Merge(&GET_LIST(mem_list)->free_list_head);
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

    if (mem_list->buff != NULL && mem_list->buff_size > 0)
    {
        is_ready = true;
    }

    return is_ready;
}


void ezStaticAlloc_HexdumpBuffer(ezmMemList *mem_list)
{
#if (VERBOSE == 1U)
    if (mem_list)
    {
        struct MemList* list = GET_LIST(mem_list);
        STCMEMPRINT("mem list info");
        STCMEMPRINT1("[addr = %p]", list);
        STCMEMPRINT1("[alloc = %p]", (void*)&list->alloc_list_head);
        STCMEMPRINT1("[free = %p]", (void*)&list->free_list_head);
        STCMEMPRINT1("[buff = %p]", list->buff);
        STCMEMPRINT1("[size = %d]", list->buff_size);
        STCMEMHEXDUMP(list->buff, list->buff_size);
    }
#endif /* VERBOSE */
    (void)mem_list;
}


void ezStaticAlloc_PrintFreeList(ezmMemList *mem_list)
{
#if (VERBOSE == 1U)
    struct Node* it_node = NULL;

    STCMEMPRINT("*****************************************");
    STCMEMPRINT("free list");
    EZ_LINKEDLIST_FOR_EACH(it_node, &GET_LIST(mem_list)->free_list_head)
    {
        STCMEMPRINT1("[addr = %p]", it_node);
        STCMEMPRINT1("[next = %p]", it_node->next);
        STCMEMPRINT1("[prev = %p]", it_node->prev);
        STCMEMPRINT1("[buff = %p]", GET_BLOCK(it_node)->buff);
        STCMEMPRINT1("[size = %d]", GET_BLOCK(it_node)->buff_size);
        STCMEMPRINT("<======>");
    }
    STCMEMPRINT("*****************************************\n");
#endif
    (void)mem_list;
}


void ezStaticAlloc_PrintAllocList(ezmMemList * mem_list)
{
#if (VERBOSE == 1U)
    struct Node* it_node = NULL;

    STCMEMPRINT("*****************************************");
    STCMEMPRINT("allocated list");
    EZ_LINKEDLIST_FOR_EACH(it_node, &GET_LIST(mem_list)->alloc_list_head)
    {
        STCMEMPRINT1("[addr = %p]", it_node);
        STCMEMPRINT1("[next = %p]", it_node->next);
        STCMEMPRINT1("[prev = %p]", it_node->prev);
        STCMEMPRINT1("[buff = %p]", GET_BLOCK(it_node)->buff);
        STCMEMPRINT1("[size = %d]", GET_BLOCK(it_node)->buff_size);
        STCMEMPRINT("<======>");
    }
    STCMEMPRINT("*****************************************\n");
#endif
    (void)mem_list;
}


uint16_t ezStaticAlloc_GetNumOfAllocBlock(ezmMemList* mem_list)
{
    return ezLinkedList_GetListSize(&GET_LIST(mem_list)->alloc_list_head);
}


uint16_t ezStaticAlloc_GetNumOfFreeBlock(ezmMemList* mem_list)
{
    return ezLinkedList_GetListSize(&GET_LIST(mem_list)->free_list_head);
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
    memset(GET_BLOCK(free_node)->buff, 0, GET_BLOCK(free_node)->buff_size);
    struct Node* it_node = NULL;

    if (free_list_head != NULL && free_node != NULL)
    {
        if (IS_LIST_EMPTY(free_list_head))
        {
            EZ_LINKEDLIST_ADD_HEAD(free_list_head, free_node);
        }
        else
        {
            /* tranverse the list to add the node, we aort the address in the order so merge operation will be easier*/
            EZ_LINKEDLIST_FOR_EACH(it_node, free_list_head)
            {
                if (GET_BLOCK(free_node)->buff < GET_BLOCK(it_node)->buff)
                {
                    ezLinkedList_AppendNode(free_node, it_node->prev);
                    break;
                }
                else if (GET_BLOCK(free_node)->buff > GET_BLOCK(it_node)->buff)
                {
                    ezLinkedList_AppendNode(free_node, it_node);
                    break;
                }
                else
                {
                    /* do nothing, advance pointer */
                }
            }
        }
    }
}

/******************************************************************************
* Function : ezmSmalloc_Merge
*//**
* \b Description:
*
* This function tries to merge the adjacent free blocks into a bigger block
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
*
* @param    *free_list      free list the the header will be returned
*
* @return   None
*
*******************************************************************************/
static void ezmSmalloc_Merge(struct Node* free_list_head)
{
    //MemHdr* head = free_list->pstHead;
    //MemHdr* next = head->pstNextNode;

    struct Node* it_node = free_list_head->next;
    struct Node* it_next = it_node->next;
    while (it_next != free_list_head &&
        ((uint8_t*)GET_BLOCK(it_node)->buff + GET_BLOCK(it_node)->buff_size) == (uint8_t*)GET_BLOCK(it_next)->buff)
    {

        STCMEMPRINT("Next adjacent block is free");
        GET_BLOCK(it_node)->buff_size += GET_BLOCK(it_next)->buff_size;
        EZ_LINKEDLIST_UNLINK_NODE(it_next);
        ReleaseBlock(GET_BLOCK(it_next));
        it_next = it_node->next;
    }
}

struct MemBlock* GetFreeBlock(void)
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

void ReleaseBlock(struct MemBlock* block)
{
    INIT_BLOCK(block, NULL, 0U);
}

struct Node* ezStaticAlloc_ReserveMemoryBlock(struct Node* free_list_head, uint16_t block_size_byte)
{
    struct MemBlock* remain_block = NULL;
    struct Node* iterate_Node = NULL;
    bool success = false;

    STCMEMPRINT("ezStaticAlloc_ReserveMemoryBlock()");

    if (NULL != free_list_head &&  block_size_byte > 0)
    {
        EZ_LINKEDLIST_FOR_EACH(iterate_Node, free_list_head)
        {
            if(iterate_Node == NULL)
            {
                break;
            }
            
            if (GET_BLOCK(iterate_Node)->buff_size >= block_size_byte)
            {
                if (GET_BLOCK(iterate_Node)->buff_size > block_size_byte)
                {
                    remain_block = GetFreeBlock();
                }

                if (remain_block)
                {
                    remain_block->buff_size = GET_BLOCK(iterate_Node)->buff_size - block_size_byte;
                    remain_block->buff = (uint8_t*)GET_BLOCK(iterate_Node)->buff + block_size_byte;
                    EZ_LINKEDLIST_ADD_TAIL(free_list_head, &remain_block->node);
                }

                GET_BLOCK(iterate_Node)->buff_size = block_size_byte;

                success = true;
                break;
            }
        }
    }

    if (!success)
    {
        iterate_Node = NULL;
    }

    return iterate_Node;
}

bool ezStaticAlloc_MoveBlock(struct Node* move_node, struct Node* from_list_head, struct Node* to_list_head)
{
    bool is_success = true;

    if (NULL != move_node 
        && NULL != from_list_head 
        && NULL != to_list_head
        && ezLinkedList_IsNodeInList(from_list_head, move_node))
    {
        EZ_LINKEDLIST_UNLINK_NODE(move_node);
        EZ_LINKEDLIST_ADD_TAIL(to_list_head, move_node);
    }

    return is_success;
}
#endif /* CONFIG_STCMEM */
/* End of file */
