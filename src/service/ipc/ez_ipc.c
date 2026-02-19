/*****************************************************************************
* Filename:         ez_ipc.c
* Author:           Hai Nguyen
* Original Date:    11.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_ipc.c
 *  @author Hai Nguyen
 *  @date   11.03.2024
 *  @brief  Implementation of ipc component
 *
 *  @details
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_ipc.h"

#if (EZ_IPC == 1)
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_IPC_LOGGING_LEVEL   /**< logging level */
#define MOD_NAME    "ez_ipc"       /**< module name */
#include "ez_logging.h"

#include <string.h>
#include "ez_linked_list.h"
#include "ez_static_alloc.h"


/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#ifndef CONFIG_NUM_OF_IPC_INSTANCE
#define CONFIG_NUM_OF_IPC_INSTANCE      5U
#endif /* CONFIG_NUM_OF_IPC_INSTANCE */

#ifndef CONFIG_USING_MODULE_NAME
#define CONFIG_USING_MODULE_NAME     1U
#endif /* CONFIG_USING_MODULE_NAME */

/**@brief Get the MemBlock structure from the node
 *
 */
#define GET_BLOCK(node_ptr)\
    (EZ_LINKEDLIST_GET_PARENT_OF(node_ptr, node, struct MemBlock))

/**@brief Get the instance from the ezmIpc type
 *
 */
#define GET_INSTANCE(ipc)\
    ((ipc<CONFIG_NUM_OF_IPC_INSTANCE) ? &instance_pool[ipc] : NULL)


/*****************************************************************************
* Component Typedefs
/**@brief structure define an IPC instance
 *
 */
typedef struct
{
    bool        is_busy;                /**< Store the id of the owner of the instance */
    ezmMemList  memory_list;            /**< Memory list to manage the buffer of the ipc instance*/
    struct Node pending_list_head;      /**< list contains message pending to be sent*/
    ezmIpc_MessageCallback fnCallback;  /**< Callback function */
}IpcInstance;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
static IpcInstance instance_pool[CONFIG_NUM_OF_IPC_INSTANCE] = {0};    /**< pool of ipc intance*/


/*****************************************************************************
* Function Definitions
*****************************************************************************/
/**Function to manipulate the buffer of the ipc. Keyword extern is used because these 
 * functions are not meant to be shared with the user
 */
extern struct Node*      ezmStcMem_ReserveMemoryBlock(struct Node* free_list_head, uint16_t block_size_byte);
extern bool              ezmStcMem_MoveBlock(struct Node* move_node, struct Node* from_list_head, struct Node* to_list_head);
extern struct MemBlock*  GetFreeBlock(void);
extern void              ReleaseBlock(struct MemBlock* block);
static void              ezIpc_ResetInstance    (uint8_t instance_index);


/*****************************************************************************
* Public functions
*****************************************************************************/
void ezIpc_InitModule(void)
{
    for (uint8_t i = 0; i < CONFIG_NUM_OF_IPC_INSTANCE; i++)
    {
        ezIpc_ResetInstance(i);
    }
}


ezmMailBox ezIpc_GetInstance(uint8_t* ipc_buffer, uint16_t buffer_size, ezmIpc_MessageCallback fnCallback)
{
    ezmMailBox free_instance = CONFIG_NUM_OF_IPC_INSTANCE;
    for (uint8_t i = 0; i < CONFIG_NUM_OF_IPC_INSTANCE; i++)
    {
        if (instance_pool[i].is_busy == false)
        {
            instance_pool[i].is_busy = true;
            instance_pool[i].fnCallback = fnCallback;
            instance_pool[i].memory_list.buff = ipc_buffer;
            instance_pool[i].memory_list.buff_size = buffer_size;
            (void)ezStaticAlloc_InitMemList(&instance_pool[i].memory_list, ipc_buffer, buffer_size);
            free_instance = (ezmMailBox)i;
            break;
        }
    }
    return free_instance;
}


void *ezIpc_InitMessage(ezmMailBox send_to, uint16_t size_in_byte)
{
    void            *buffer_address = NULL;
    IpcInstance     *send_to_instance = NULL;
    struct Node     *pending_node = NULL;

    if (size_in_byte > 0 && send_to < CONFIG_NUM_OF_IPC_INSTANCE)
    {
        send_to_instance = GET_INSTANCE(send_to);
        pending_node = ezmStcMem_ReserveMemoryBlock(&send_to_instance->memory_list.free_list_head, size_in_byte);
        if (NULL != pending_node)
        {
            buffer_address = GET_BLOCK(pending_node)->buff;
            EZ_LINKEDLIST_UNLINK_NODE(pending_node);
            EZ_LINKEDLIST_ADD_TAIL(&send_to_instance->pending_list_head, pending_node);
        }
    }

    return buffer_address;
}


bool ezIpc_SendMessage(ezmMailBox send_to, void *message)
{
    bool        is_success = false;
    IpcInstance *send_to_instance = NULL;
    struct Node *it_node = NULL;

    if (NULL != message && send_to < CONFIG_NUM_OF_IPC_INSTANCE)
    {
        send_to_instance = GET_INSTANCE(send_to);
        EZ_LINKEDLIST_FOR_EACH(it_node, &send_to_instance->pending_list_head)
        {
            if (GET_BLOCK(it_node)->buff == message)
            {
                ezmStcMem_MoveBlock(it_node, &send_to_instance->pending_list_head, &send_to_instance->memory_list.alloc_list_head);
                if (NULL != send_to_instance->fnCallback)
                {
                    send_to_instance->fnCallback();
                }
                is_success = true;
                break;
            }
        }
    }

    return is_success;
}


void* ezIpc_ReceiveMessage(ezmMailBox receive_from, uint16_t *message_size)
{
    void        *buffer_address = NULL;
    IpcInstance *instance = NULL;

    if (receive_from < CONFIG_NUM_OF_IPC_INSTANCE)
    {
        instance = GET_INSTANCE(receive_from);
   
        if (!IS_LIST_EMPTY(&instance->memory_list.alloc_list_head))
        {
            buffer_address = GET_BLOCK(instance->memory_list.alloc_list_head.next)->buff;
            *message_size = GET_BLOCK(instance->memory_list.alloc_list_head.next)->buff_size;
        }
    }
    return buffer_address;
}


bool ezIpc_ReleaseMessage(ezmMailBox receive_from, void* message)
{
    bool        is_success = true;
    IpcInstance *instance = NULL;

    if (message != NULL && receive_from < CONFIG_NUM_OF_IPC_INSTANCE)
    {
        instance = GET_INSTANCE(receive_from);
        is_success = is_success && ezStaticAlloc_Free(&instance->memory_list, message);
    }

    return is_success;
}

/*****************************************************************************
* Local functions
*****************************************************************************/
/******************************************************************************
* Function : ezmIpc_ResetInstance
*//**
* \b Description:
*
* Reset a ipc instance to init state
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
*
* @param    instance_index; index of the instance
* @return   None
*
*******************************************************************************/
static void ezIpc_ResetInstance(uint8_t instance_index)
{
    if (instance_index < CONFIG_NUM_OF_IPC_INSTANCE)
    {
        instance_pool[instance_index].is_busy = false;
        instance_pool[instance_index].fnCallback = NULL;
 
        instance_pool[instance_index].pending_list_head.next = &instance_pool[instance_index].pending_list_head;
        instance_pool[instance_index].pending_list_head.prev = &instance_pool[instance_index].pending_list_head;

        memset(&instance_pool[instance_index].memory_list, 0, sizeof(instance_pool[instance_index].memory_list));
    }
}

#endif /* EZ_IPC == 1 */
/* End of file*/
