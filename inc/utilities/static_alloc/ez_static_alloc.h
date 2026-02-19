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

#ifndef _EZ_STATIC_ALLOC_H
#define _EZ_STATIC_ALLOC_H

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

#include "stdint.h"
#include "ez_linked_list.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */


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
    uint16_t buff_size;
    /**< Size of the buffer */
};


/**@brief Header of a memory block
 */
struct MemBlock
{
    struct Node node;   /* Linked list node */
    void* buff;         /* Pointer to the allocated memory */
    uint16_t buff_size; /* Size of the allocated memory */
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
* @param[in]    *mem_list:      handle to manage memory buffer
* @param[in]    *buffer:        buffer to be managed
* @param[in]    buffer_size:    size of the buffer
* @return       True is initialization is success
*
* @pre None
* @post None
*
* \b Example
* @code
* ezmMemList mem_list;
* uint8_t buff[32];
* bool success = ezStaticAlloc_InitMemList(&mem_list, buff, 32);
* @endcode
*
*****************************************************************************/
bool ezStaticAlloc_InitMemList(ezmMemList* mem_list, void* buff, uint16_t buff_size);


/*****************************************************************************
* Function : ezStaticAlloc_IsMemListReady
*//** 
* @brief Return the status if the mem list is ready 
*
* @details List is ready when it is initialized with the function
* ezStaticAlloc_InitMemList
*
* @param[in]    *mem_list:  handle to manage memory buffer
* @return       true if ready, else false
*
* @pre None
* @post None
*
* \b Example
* @code
* bool ready = ezStaticAlloc_IsMemListReady(&mem_list);
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
* @param[in]    *mem_list:  handle to manage memory buffer
* @param[in]    alloc_size: number of byte to be allocated
* @return       address of the allocated memory
*
* @pre mem_list must initialized
* @post None
*
* \b Example
* @code
* ezmMemList mem_list;
* uint8_t buff[32];
* bool success = ezStaticAlloc_InitMemList(&mem_list, buff, 32);
* uint32_t *foo = (uint32_t*)ezStaticAlloc_Malloc(&mem_list, sizeof(uint32_t));
* if(foo != NULL)
* {
*     printf("ok");
* }
* @endcode
*
* @see ezStaticAlloc_InitMemList
*
*****************************************************************************/
void *ezStaticAlloc_Malloc(ezmMemList* mem_list, uint16_t alloc_size);


/*****************************************************************************
* Function : ezStaticAlloc_Free
*//** 
* @brief This function frees the allocated memory in the memory buffer
*
* @details Implementation of the free() function
*
* @param[in]    *mem_list:  handle to manage memory buffer
* @param[in]    *alloc_addr: the address of the allocated memory
* @return       true if free is success, else false
*
* @pre mem_list must initialized
* @post None
*
* \b Example
* @code
* ezmMemList mem_list;
* uint8_t buff[32];
* bool success = ezStaticAlloc_InitMemList(&mem_list, buff, 32);
* uint32_t *foo = (uint32_t*)ezStaticAlloc_Malloc(&mem_list, sizeof(uint32_t));
* if(foo == NULL)
* {
*     printf("error");
* }
*
* if(ezStaticAlloc_Free(&mem_list, foo) == false)
* {
*     printf(error");
* }
* @endcode
*
* @see ezStaticAlloc_InitMemList
*
*****************************************************************************/
bool ezStaticAlloc_Free(ezmMemList *mem_list, void *alloc_addr);


/*****************************************************************************
* Function : ezStaticAlloc_GetNumOfAllocBlock
*//** 
* @brief Return the number of allocated memory block
*
* @details
*
* @param[in]    *mem_list: handle to manage memory buffer
* @return       Number of allocated block
*
* @pre mem_list must initialized
* @post None
*
* \b Example
* @code
* ezStaticAlloc_GetNumOfAllocBlock(&mem_list);
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
* @details
*
* @param[in]    *mem_list: handle to manage memory buffer
* @return       Number of freed block
*
* @pre mem_list must initialized
* @post None
*
* \b Example
* @code
* ezStaticAlloc_GetNumOfFreeBlock(&mem_list);
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
* @details For debugging purpose
*
* @param[in]    *mem_list: handle to manage memory buffer
* @return       None
*
* @pre mem_list must initialized
* @post None
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
* @details For debugging purpose
*
* @param[in]    *mem_list:  handle to manage memory buffer
* @return       None
*
* @pre  mem_list must initialized
* @post None
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
* @details For debugging purpose
*
* @param[in]    *mem_list:  handle to manage memory buffer
* @return       None
*
* @pre  mem_list must initialized
* @post None
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
#endif /* _EZ_STATIC_ALLOC_H */

/* End of file*/
