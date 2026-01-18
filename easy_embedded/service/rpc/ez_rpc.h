/*****************************************************************************
* Filename:         ez_rpc.h
* Author:           Hai Nguyen
* Original Date:    10.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_rpc.h
 *  @author Hai Nguyen
 *  @date   10.03.2024
 *  @brief  Public API of remote procedure call (rpc) component
 *
 *  @details
 */

#ifndef _EZ_RPC_H
#define _EZ_RPC_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_RPC == 1)
#include "stdint.h"
#include "stdbool.h"
#include "ez_queue.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#ifndef CONFIG_NUM_OF_REQUEST
#define CONFIG_NUM_OF_REQUEST       4
#endif


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/** @brief Message type enumeration
 *
 */
typedef enum
{
    RPC_MSG_REQ,            /**< request */
    RPC_MSG_RESP,           /**< response */
    RPC_MSG_EVENT,          /**< event */
    RPC_MSG_NUM_OF_TYPE    /**< number of message type */
}RPC_MSG_TYPE;

typedef enum{
    RPC_ERROR_WRONG_SYNC_BYTES,     /**< wrong sync bytes */
    RPC_ERROR_WRONG_MSG_TYPE,       /**< wrong message type */
    RPC_ERROR_UNKNOWN_CMD,          /**< unknown command */
    RPC_ERROR_CRC_FAILED,           /**< CRC check failed */
    RPC_ERROR_QUEUE_RESERVE_FAILED, /**< queue reserve failed */
    RPC_ERROR_MAX,                  /**< maximum error code */
}RPC_ERROR;


struct ezRpcMsgHeader
{
    uint16_t        sync_bytes;     /**< sync bytes, must be 0xCAFE */
    uint16_t        cmd_id;         /**< Command ID, linked to a function which will be executed */
    uint8_t         is_encrypted;   /**< encrpypted flag */
    RPC_MSG_TYPE    type;           /**< RPC message type */
    uint16_t        uuid;           /**< UUID of the message */
    uint32_t        payload_size;   /**< Size of the payload */
};


/** @brief Record of the request. Keep track of sent requests
 *
 */
struct ezRpcRequestRecord
{
    uint16_t    uuid;           /**< UUID of the request */
    int         timestamp;      /**< Time stamp when the request is created */
    bool        is_available;   /**< Availalbe flag */
};

typedef uint32_t(*RpcTransmit)  (uint8_t *tx_data, uint32_t tx_size);
typedef uint32_t(*RpcReceive)   (uint8_t *rx_data, uint32_t rx_size);
typedef void(*CommandHandler)   (struct ezRpcMsgHeader *header, void *payload, uint32_t payload_size_byte);
typedef bool(*CrcVerify)        (uint8_t *input,
                                 uint32_t input_size,
                                 uint8_t *crc,
                                 uint32_t crc_size);

typedef void(*CrcCalculate)     (uint8_t *input, 
                                 uint32_t input_size,
                                 uint8_t *crc_output,
                                 uint32_t crc_output_size);
typedef void(*RpcErrorCallback) (RPC_ERROR error_code, void *context);


/** @brief Communication interface
 */
struct ezRpcCommInterface
{
    RpcTransmit   transmit; /**< Function to transmit data */
    RpcReceive    receive;  /**< Function to receive data */
};


/** @brief Rpc service structure
 *
 */
struct ezRpcCommandEntry
{
    uint16_t         id;                /**< Stores the command code*/
    CommandHandler   command_handler;   /**< pointer to function handling that command */
};


/** @brief Data structure holding deserializer related data
 *
 */
struct ezRpcUnmarshal
{
    uint8_t state;                      /**< Store the state of the binary parser statemachine */
    uint16_t sync_bytes;               /**< temporary storage for sync bytes */
    struct ezRpcMsgHeader *curr_hdr;    /**< pointer to the current header that the parser is working*/
    uint32_t byte_count;                /**< index for deserialize rpc message */
    uint8_t *payload;                   /**< */
    uint8_t *crc_val;                   /**< */
    ezReservedElement payload_elem;     /**< */
    ezReservedElement crc_elem;         /**< */
    ezReservedElement header_elem;      /**< */
};


/** @brief Data structure holding encryption related data
 *
 */
struct ezRpcEncrypt
{
    bool is_encrypted;  /**< Flag to indicate the rpc instance using encryption */
};


/** @brief Data structure holding crc related data
 *
 */
struct ezRpcCrcHandler
{
    CrcVerify           verify;     /**< Pointer to the CRC verification function */
    CrcCalculate        calculate;  /**< Pointer to the CRC calculation function */
    uint32_t            size;       /**< Size of the crc value, in bytes*/
};


/** @brief Define an RPC object, holding data to make an RPC instance working
 *  
 */
struct ezRpc
{
    uint16_t            num_of_commands;        /**< Size of the command table, how many commands are there in total */
    struct ezRpcCommandEntry *commands;         /**< Poiter to the command table */
    struct ezRpcUnmarshal unmarshal;            /**< Hold unmarshaler related data */
    struct ezRpcCrcHandler *crc_handler;        /**< Hold crc related data */
    struct ezRpcEncrypt encrypt;                /**< Hold encryption related data */
    ezQueue             tx_msg_queue;           /**< Queue to store request */
    ezQueue             rx_msg_queue;           /**< Queue to store request */
    uint16_t            next_uuid;              /**< Value of next uuid, assign this value to rpc message */
    struct ezRpcCommInterface *comm_interface;  /**< Communication interface */
    RpcErrorCallback    error_callback;         /**< Error callback function, optional */
    struct ezRpcRequestRecord records[CONFIG_NUM_OF_REQUEST]; /* Keep track of records */
};


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function: ezRpc_Initialization
*//** 
* @brief This function initializes RPC instance of the RPC module
*
* @details
*
* @param[in]    *rpc_inst: pointer to the rpc instance
* @param[in]    *buff: pointer to memory provided to this rpc instance
* @param[in]    buff_size: size of the buff
* @param[in]    *service_table: pointer to the table containing services, see ezRpcService
* @param[in]    service_table_size: size of the table
* @return   ezSUCCESS or ezFAIL
*
* @pre None
* @post None
*
*****************************************************************************/
ezSTATUS ezRpc_Initialization(struct ezRpc *rpc_inst,
                                uint8_t *buff,
                                uint32_t buff_size,
                                struct ezRpcCommandEntry *commands,
                                uint32_t num_of_commands);


/*****************************************************************************
* Function: ezRpc_SetCrcFunctions
*//** 
* @brief This function enables the CRC check capability of an RPC instance
*
* @details
*
* @param[in]    *rpc_inst: pointer to the rpc instance
* @param[in]    crc_size: size of the crc value
* @param[in]    verify_func:function to verify crc value
* @param[in]    cal_func: function to calculate crc value
* @return   ezSUCCESS or ezFAIL
*
* @pre ezRpc_Initialization() has been called
* @post None
*
*****************************************************************************/
ezSTATUS ezRpc_SetCrcHandler(
    struct ezRpc *rpc_inst,
    struct ezRpcCrcHandler *handler);


/*
* @brief This function set the interface for transmitting and receiving data
*
* @details
*
* @param[in]    *rpc_inst: pointer to the rpc instance
* @param[in]    tx_function: function to transmit data
* @param[in]    rx_function: function to receive data
* @return   ezSUCCESS or ezFAIL
*
* @pre ezRpc_Initialization() has been called
* @post None
*/
ezSTATUS ezRpc_SetCommFunctions(struct ezRpc *rpc_inst,
                                struct ezRpcCommInterface *comm_interface);

/*
* @brief This function set a callback function to report error events
*
* @details
*
* @param[in]    *rpc_inst: pointer to the rpc instance
* @param[in]    error_callback: function to transmit data
* @return       None 
*
* @pre ezRpc_Initialization() has been called
* @post None
*/
void ezRpc_SetEventCallback(struct ezRpc *rpc_inst,
                                RpcErrorCallback error_callback);


/*****************************************************************************
* Function: ezRPC_CreateRpcRequest
*//** 
* @brief This function creates an RPC request and put it in the transmit queue
*
* @details
*
* @param[in]    *rpc_inst: pointer to the rpc instance
* @param[in]    tag: tag value
* @param[in]    *payload: pointer to payload to send
* @param[in]    payload_size: siez of the payload
* @return       ezSUCCESS or ezFAIL
*
* @pre ezRpc_Initialization() and ezRpc_SetCommFunctions() has been called
* @post None
*
*****************************************************************************/
ezSTATUS ezRPC_CreateRpcRequest(struct ezRpc *rpc_inst,
    uint16_t cmd_id,
    uint8_t *payload,
    uint32_t payload_size);


/*****************************************************************************
* Function: ezRPC_CreateRpcResponse
*//** 
* @brief This function creates an RPC response and put it in the transmit queue
*
* @details
*
* @param[in]    *rpc_inst: pointer to the rpc instance
* @param[in]    uuid: uuid value, must match the request value
* @param[in]    tag: tag value
* @param[in]    *payload: pointer to payload to send
* @param[in]    payload_size: siez of the payload
* @return   ezSUCCESS or ezFAIL
*
* @pre ezRpc_Initialization() and ezRpc_SetCommFunctions() has been called
* @post None
*
*****************************************************************************/
ezSTATUS ezRPC_CreateRpcResponse(struct ezRpc *rpc_inst,
    uint16_t cmd_id,
    uint32_t uuid,
    uint8_t *payload,
    uint32_t payload_size);


/*****************************************************************************
* Function: ezRPC_Run
*//** 
* @brief Run the RPC instance
*
* @details  must be call in a tick function, a loop or a task to advance the
* internal state machine
*
* @param[in]    *rpc_inst: pointer to the rpc instance
* @return       None
*
* @pre ezRpc_Initialization() and ezRpc_SetCommFunctions() has been called
* @post None
*
*****************************************************************************/
void ezRPC_Run(struct ezRpc *rpc_inst);


/*****************************************************************************
* Function: ezRPC_NumOfTxPendingMsg
*//** 
* @brief Return the number of messages waiting to be transmitted.
* It is used for disagnostic or testing purpose
*
* @details
*
* @param[in]    *rpc_inst: ointer to the rpc instance
* @return       number of messages
*
* @pre None
* @post None
*
*****************************************************************************/
uint32_t ezRPC_NumOfTxPendingMsg(struct ezRpc *rpc_inst);


/*****************************************************************************
* Function: ezRPC_NumOfPendingRecords
*//** 
* @brief Return the number pending records, i.e. requests that is waiting
* for the responses
*
* @details
*
* @param[in]    *rpc_inst: pointer to the rpc instance
* @return       number of records
*
* @pre None
* @post None
*
*****************************************************************************/
uint32_t ezRPC_NumOfPendingRecords(struct ezRpc *rpc_inst);


/*****************************************************************************
* Function: ezRpc_IsRpcInstanceReady
*//** 
* @brief Return the status if the rpc instance is ready
*
* @details
*
* @param[in]    *rpc_inst: pointer to the rpc instance
* @return   true if ready, else false
*
* @pre None
* @post None
*
*****************************************************************************/
static inline bool ezRpc_IsRpcInstanceReady(struct ezRpc *rpc_inst)
{
    bool is_ready = false;

    if (rpc_inst != NULL)
    {
        return ((rpc_inst->commands != NULL)
                    && (rpc_inst->num_of_commands > 0)
                    && (ezQueue_IsQueueReady(&rpc_inst->rx_msg_queue))
                    && (ezQueue_IsQueueReady(&rpc_inst->tx_msg_queue))
                    && (rpc_inst->comm_interface != NULL)
                    && (rpc_inst->comm_interface->receive != NULL)
                    && (rpc_inst->comm_interface->transmit != NULL));
    }

    return false;
}

#ifdef __cplusplus
}
#endif

#endif /* EZ_RPC == 1 */
#endif /* _EZ_RPC_H */


/* End of file */
