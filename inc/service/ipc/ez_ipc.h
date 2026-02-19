/*****************************************************************************
* Filename:         ez_ipc.h
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

/** @file   ez_ipc.h
 *  @author Hai Nguyen
 *  @date   11.03.2024
 *  @brief  Public API for ipc components
 *
 *  @details
 */

#ifndef _EZ_IPC_H
#define _EZ_IPC_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_IPC == 1)
#include "stdint.h"
#include "stdbool.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define IPC_INVALID         NUM_OF_IPC_INSTANCE


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
typedef uint32_t(*ezmIpc_MessageCallback)(void);
typedef uint32_t ezmMailBox;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function: ezIpc_InitModule
*//** 
* @brief This function initializes the IPC component.
*
* @details It reset all of ipc instances in the pools
*
* @param    None
* @return   None
*
* @pre None
* @post None
*
* \b Example
* @code
* TBD
* @endcode
*
* @see TBD
*
*****************************************************************************/
void ezIpc_InitModule(void);


/*****************************************************************************
* Function: ezIpc_GetInstance
*//** 
* @brief Get a free instance from the Ipc pool and init it according to the parameters
*
* @details
*
* @param[in]    *ipc_buffer: pointer to the providing buffer for the instance
* @param[in]    buffer_size: size of the buffer in byte
* @param[in]    fnCallback:  callback function, tell the owner that it receives a message
*
* @return   handle to the ipc instance
*
* @pre None
* @post None
*
* \b Example
* @code
* TBD
* @endcode
*
* @see TBD
*
*****************************************************************************/
ezmMailBox ezIpc_GetInstance(uint8_t* ipc_buffer,
                             uint16_t buffer_size,
                             ezmIpc_MessageCallback fnCallback);


/*****************************************************************************
* Function: ezIpc_InitMessage
*//** 
* @brief Init a message and return the address of the buffer for usage.
*
* @details It reseves a memory block in the buffer of the ipc instance and
* return it to the users so they can write data into the block.
*
* @param[in]    send_to: ipc handle, which the message will be sent to
* @param[in]    size_in_byte: size of the message in byte
* @return       address of the buffer
*               NULL error
*
* @pre IPC instance must be exsisting
* @post None
*
* \b Example
* @code
* TBD
* @endcode
*
* @see TBD
*
*****************************************************************************/
void *ezIpc_InitMessage(ezmMailBox send_to, uint16_t size_in_byte);


/*****************************************************************************
* Function: ezIpc_SendMessage
*//** 
* @brief "Send" the message to the module.
*
* @details Send action means in move the memory header from free list to the
* allocated list. If a callback is set, it will trigger the owner that a
* message is sent.
*
* @param[in]    send_to: ipc handle, which the message will be sent to
* @param[in]    *message: pointer the message
*
* @return   true: success
*           false: fail
*
* @pre None
* @post None
*
* \b Example
* @code
* TBD
* @endcode
*
* @see TBD
*
*****************************************************************************/
bool ezIpc_SendMessage(ezmMailBox send_to, void *message);


/*****************************************************************************
* Function: ezIpc_ReceiveMessage
*//** 
* @brief This function check the buffer and return the message if there is one.
*
* @details Note calling this function only return the message. After working
* with the message, ezIpc_ReleaseMessage must be called to actually free the
* message from the buffer
*
* @param[in]    receive_from: the handle, which message will be read out
* @param[in]    *message_size: size of the message
*
* @return   address of the message if there is one
*
* @pre instance must be exist
* @post None
*
* \b Example
* @code
* TBD
* @endcode
*
* @see TBD
*
*****************************************************************************/
void *ezIpc_ReceiveMessage(ezmMailBox receive_from, uint16_t *message_size);


/*****************************************************************************
* Function: ezIpc_ReleaseMessage
*//** 
* @brief Free the message in the buffer.
*
* @details
*
* @param[in]    receive_from: the handle, which message will be read out
* @param[in]    *message:   message to be free
*
* @return   true: success
*           false: fail
*
* @pre None
* @post None
*
* \b Example
* @code
* TBD
* @endcode
*
* @see TBD
*
*****************************************************************************/
bool ezIpc_ReleaseMessage(ezmMailBox receive_from, void *message);

#ifdef __cplusplus
}
#endif

#endif /* EZ_IPC == 1 */
#endif /* _EZ_IPC_H */


/* End of file */
