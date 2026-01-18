/*****************************************************************************
* Filename:         ez_rpc.c
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

/** @file   ez_rpc.c
 *  @author Hai Nguyen
 *  @date   10.03.2024
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include <string.h>
#include "ez_rpc.h"

#if (EZ_RPC == 1)
#include "ez_default_logging_level.h"

#define DEBUG_LVL   EZ_RPC_LOGGING_LEVEL   /**< logging level */
#define MOD_NAME    "ez_rpc"       /**< module name */
#include "ez_logging.h"


/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define SYNC_BYTES          0xCAFE  /**< start of frame, for syncronisation */
#define NUM_OF_BYTE_READ    1U      /**< number of byte being read by RPC */
#define WAIT_TIME           3000U   /**< time a request waiting for its response in millisec*/
#define SYNC_SIZE           2U
#define UUID_SIZE           2U
#define TYPE_SIZE           1U
#define ENC_SIZE            1U
#define CMD_ID_SIZE         2U
#define LEN_SIZE            4U
#define CRC_SIZE            2U
#define HEADER_SIZE         (SYNC_SIZE + UUID_SIZE + TYPE_SIZE + ENC_SIZE + CMD_ID_SIZE + LEN_SIZE)

/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/** @brief Message type enumeration
 */
typedef enum
{
    STATE_SYNC,         /**< State parsing SOF */
    STATE_UUID,         /**< State parsing UUID */
    STATE_MSG_TYPE,     /**< State parsing message type */
    STATE_CMD_ID,       /**< State parsing tag */
    STATE_ENCRYPT_FLAG, /**< State parsing encryption flag */
    STATE_PAYLOAD_SIZE, /**< State parsing payload size */
    STATE_PAYLOAD,      /**< State parsing payload */
    STATE_CRC,          /**< State parsing crc */
}RPC_DESERIALIZE_STATES;


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */


/*****************************************************************************
* Function Definitions
*****************************************************************************/
static void ezRpc_ResetAllRecords(struct ezRpcRequestRecord *records);

static ezSTATUS ezRPC_MarshalMessage(struct ezRpc *rpc_inst,
                                       struct ezRpcMsgHeader *header,
                                       uint8_t *payload,
                                       uint32_t payload_size);

static ezSTATUS ezRpc_MarshalHeader(uint8_t *buff,
                                         struct ezRpcMsgHeader *header);
static bool ezRpc_IsCrcActivated(struct ezRpc *rpc_inst);
static struct ezRpcRequestRecord *ezRpc_GetAvailRecord(struct ezRpc *rpc_inst);
static void ezRpc_UnmarshalData(struct ezRpc *rpc_inst, uint8_t rx_byte);
static void ezRpc_HandleReceivedMsg(struct ezRpc *rpc_inst);
static void ezRpc_CheckTimeoutRecords(struct ezRpc *rpc_inst);
static inline void ezRpc_ResetRecord(struct ezRpcRequestRecord *record);

/*Helper functions for debugging */
#if (DEBUG_LVL == LVL_TRACE)
void ezRpc_PrintHeader(struct ezRpcMsgHeader *header);
void ezRpc_PrintPayload(uint8_t *payload, uint32_t size);
void ezRpc_PrintCrc(uint8_t *crc, uint32_t size);
#endif /* DEBUG_LVL == LVL_TRACE */


/*****************************************************************************
* Public functions
*****************************************************************************/

ezSTATUS ezRpc_Initialization(struct ezRpc *rpc_inst,
                                uint8_t *buff,
                                uint32_t buff_size,
                                struct ezRpcCommandEntry *commands,
                                uint32_t num_of_commands)
{
    ezSTATUS status = ezSUCCESS;

    EZTRACE("ezRpc_Initialization()");

    if (rpc_inst != NULL
        && buff != NULL
        && buff_size > 0
        && commands != NULL
        && num_of_commands > 0)
    {
        /* clean the struct */
        memset(rpc_inst, 0, sizeof(struct ezRpc));

        ezRpc_ResetAllRecords(rpc_inst->records);


        status = ezQueue_CreateQueue(&rpc_inst->tx_msg_queue, buff, buff_size/2);
        if (status == ezSUCCESS)
        {
            status = ezQueue_CreateQueue(
                &rpc_inst->rx_msg_queue,
                (buff + buff_size/2),
                buff_size/2);
        }

        if (status == ezSUCCESS)
        {
            rpc_inst->commands = commands;
            rpc_inst->num_of_commands = num_of_commands;

            rpc_inst->unmarshal.state = STATE_SYNC;
            rpc_inst->unmarshal.byte_count = 0;

            rpc_inst->encrypt.is_encrypted = 0;
            rpc_inst->error_callback = NULL;
        }
    }
    else
    {
        status = ezFAIL;
    }

    return status;
}


ezSTATUS ezRpc_SetCrcHandler(struct ezRpc *rpc_inst,
                                struct ezRpcCrcHandler *handler)
{
    EZTRACE("ezRpc_SetCrcHandler()");

    if (rpc_inst != NULL
        && handler != NULL)
    {
        rpc_inst->crc_handler = handler;
        return ezSUCCESS;
    }

    return ezFAIL;
}


ezSTATUS ezRpc_SetCommFunctions(struct ezRpc *rpc_inst,
    struct ezRpcCommInterface *comm_interface)
{
    if (rpc_inst == NULL || comm_interface == NULL)
    {
        return ezFAIL;   
    }
    rpc_inst->comm_interface = comm_interface;
    return ezSUCCESS;
}

void ezRpc_SetEventCallback(struct ezRpc *rpc_inst,
                            RpcErrorCallback error_callback)
{
    if (rpc_inst != NULL)
    {
        rpc_inst->error_callback = error_callback;
    }
}


ezSTATUS ezRPC_CreateRpcRequest(struct ezRpc *rpc_inst,
                                uint16_t cmd_id,
                                uint8_t *payload,
                                uint32_t payload_size)
{
    ezSTATUS status = ezFAIL;

    struct ezRpcMsgHeader temp_header = { 0 };

    if (rpc_inst != NULL)
    {
        temp_header.cmd_id = cmd_id;
        temp_header.type = RPC_MSG_REQ;
        temp_header.payload_size = payload_size;
        temp_header.uuid = ++rpc_inst->next_uuid;
        temp_header.is_encrypted = rpc_inst->encrypt.is_encrypted;

        status = ezRPC_MarshalMessage(
            rpc_inst,
            &temp_header,
            payload,
            payload_size);
    }

    return status;
}


ezSTATUS ezRPC_CreateRpcResponse(struct ezRpc *rpc_inst,
    uint16_t cmd_id,
    uint32_t uuid,
    uint8_t *payload,
    uint32_t payload_size)
{
    ezSTATUS status = ezFAIL;

    struct ezRpcMsgHeader temp_header = { 0 };

    if (rpc_inst != NULL)
    {
        temp_header.cmd_id = cmd_id;
        temp_header.type = RPC_MSG_RESP;
        temp_header.payload_size = payload_size;
        temp_header.uuid = uuid;
        temp_header.is_encrypted = rpc_inst->encrypt.is_encrypted;

        status = ezRPC_MarshalMessage(
            rpc_inst,
            &temp_header,
            payload,
            payload_size);
    }

    return status;
}


void ezRPC_Run(struct ezRpc *rpc_inst)
{
    ezSTATUS status = ezSUCCESS;
    uint8_t one_byte = 0U;
    uint8_t *req = NULL;
    uint32_t req_size = 0U; 

    if (rpc_inst != NULL && ezRpc_IsRpcInstanceReady(rpc_inst) == true)
    {
        /* Try to read all available bytes */
        while (rpc_inst->comm_interface->receive(&one_byte, NUM_OF_BYTE_READ) == NUM_OF_BYTE_READ)
        {
            ezRpc_UnmarshalData(rpc_inst, one_byte);
        }
        

        /* Handle the received message */
        ezRpc_HandleReceivedMsg(rpc_inst);

        /* Transmit message */
        if (ezQueue_GetNumOfElement(&rpc_inst->tx_msg_queue) > 0)
        {
            if (ezQueue_GetFront(&rpc_inst->tx_msg_queue,
                (void *)&req,
                &req_size) == ezSUCCESS)
            {
                rpc_inst->comm_interface->transmit(req, req_size);
            }
            (void)ezQueue_PopFront(&rpc_inst->tx_msg_queue);
        }

        ezRpc_CheckTimeoutRecords(rpc_inst);
    }
}


uint32_t ezRPC_NumOfTxPendingMsg(struct ezRpc *rpc_inst)
{
    uint32_t num_of_msg = 0;
    if (rpc_inst != NULL)
    {
        num_of_msg = ezQueue_GetNumOfElement(&rpc_inst->tx_msg_queue);
    }

    return num_of_msg;
}


uint32_t ezRPC_NumOfPendingRecords(struct ezRpc *rpc_inst)
{
    uint32_t num_of_records = 0;

    if (rpc_inst != NULL)
    {
        for (uint32_t i = 0; i < CONFIG_NUM_OF_REQUEST; i++)
        {
            if (rpc_inst->records[i].is_available == false)
            {
                num_of_records++;
            }
        }
    }

    return num_of_records;
}


/*****************************************************************************
* Local functions
*****************************************************************************/

/******************************************************************************
* Function : ezRpc_ResetAllRecords
*//**
* @Description: Reset all records of the rpc module
*
* @param    *records: (IN)pointer to the record
* @return   None
*
*******************************************************************************/
static void ezRpc_ResetAllRecords(struct ezRpcRequestRecord *records)
{
    if (records != NULL)
    {
        for (uint32_t i = 0; i < CONFIG_NUM_OF_REQUEST; i++)
        {
            records[i].is_available = true;
            records[i].timestamp = 0;
            records[i].uuid = 0;
        }
    }
}


/******************************************************************************
* Function : ezRpc_MarshalHeader
*//**
* @Description: serialize the header of a RPC message
*
* @param    *buff: (IN)Buffer containing the serialized header
* @param    *header: (IN)Pointer to the header
* @return   ezSUCCESS: success
*           ezFAIL: fail
*
*******************************************************************************/
static ezSTATUS ezRpc_MarshalHeader(uint8_t *buff,
                                        struct ezRpcMsgHeader *header)
{
    if (buff != NULL && header != NULL)
    {
        *(buff++) = (SYNC_BYTES >> 8);
        *(buff++) = (SYNC_BYTES & 0xFF);

        *(buff++) = (header->uuid >> 8);
        *(buff++) = (header->uuid & 0xFF);

        *(buff++) = (uint8_t)header->type;
        *(buff++) = header->is_encrypted;
        *(buff++) = (header->cmd_id >> 8);
        *(buff++) = (header->cmd_id & 0xFF);
        
        *(buff++) = (header->payload_size >24);
        *(buff++) = ((header->payload_size >> 16 & 0xFF));
        *(buff++) = ((header->payload_size >>8) & 0xFF);
        *(buff++) = (header->payload_size & 0xFF);
        return ezSUCCESS;
    }
    return ezFAIL;
}


/******************************************************************************
* Function : ezRpc_IsCrcActivated
*//**
* @Description: Return to status if CRC calculation is activated
*
* @param    *rpc_inst: (IN)Rpc instance
* @return   true if CRC calculation is activated
*
*******************************************************************************/
static bool ezRpc_IsCrcActivated(struct ezRpc *rpc_inst)
{
    return ( rpc_inst != NULL
        && rpc_inst->crc_handler != NULL
        && rpc_inst->crc_handler->size > 0
        && rpc_inst->crc_handler->calculate != NULL
        && rpc_inst->crc_handler->verify != NULL);
}


/******************************************************************************
* Function : ezRpc_GetAvailRecord
*//**
* @Description: Get an available record from the records array
*
* @param    *rpc_inst: (IN)Rpc instance
* @return   a record or NULL if no record is available
*
*******************************************************************************/
static struct ezRpcRequestRecord *ezRpc_GetAvailRecord(struct ezRpc *rpc_inst)
{
    struct ezRpcRequestRecord *ret_record = NULL;

    if (rpc_inst != NULL)
    {
        for (uint32_t i = 0; i < CONFIG_NUM_OF_REQUEST; i++)
        {
            if (rpc_inst->records[i].is_available == true)
            {
                ret_record = &rpc_inst->records[i];
                rpc_inst->records[i].is_available = false;
                break;
            }
        }
    }

    return ret_record;
}


/******************************************************************************
* Function : ezRpc_UnmarshalData
*//**
* @Description: Unmarshal data from the communication interface
*
* @param    *rpc_inst: (IN)pointer to rpc instance
* @param    rx_byte: (IN)byte receive from the communication interface
* @return   None
*
*******************************************************************************/
static void ezRpc_UnmarshalData(struct ezRpc *rpc_inst, uint8_t rx_byte)
{
    ezSTATUS status = ezSUCCESS;

    if (rpc_inst != NULL)
    {
        switch (rpc_inst->unmarshal.state)
        {
        case STATE_SYNC:
            EZTRACE("STATE_SOF");
            rpc_inst->unmarshal.sync_bytes = (
                (rpc_inst->unmarshal.sync_bytes << 8) | rx_byte);
            rpc_inst->unmarshal.byte_count++;

            if(rpc_inst->unmarshal.byte_count >= SYNC_SIZE)
            {
                if(rpc_inst->unmarshal.sync_bytes == SYNC_BYTES)
                {
                    EZDEBUG("Got SYNC_BYTES");
                    rpc_inst->unmarshal.header_elem = ezQueue_ReserveElement(
                        &rpc_inst->rx_msg_queue,
                        (void*)&rpc_inst->unmarshal.curr_hdr,
                        sizeof(struct ezRpcMsgHeader));
                    
                    if (rpc_inst->unmarshal.header_elem == NULL)
                    {
                        EZERROR("Cannot get queue item");
                        if(rpc_inst->error_callback != NULL)
                        {
                            rpc_inst->error_callback(RPC_ERROR_QUEUE_RESERVE_FAILED, NULL);
                        }
                    }
                    else
                    {
                        rpc_inst->unmarshal.curr_hdr->uuid = 0;
                        rpc_inst->unmarshal.curr_hdr->payload_size = 0;
                        rpc_inst->unmarshal.state = STATE_UUID;
                    }
                }
                else
                {
                    EZDEBUG("Get wrong SYNC_BYTES");
                    if(rpc_inst->error_callback != NULL)
                    {
                        rpc_inst->error_callback(RPC_ERROR_WRONG_SYNC_BYTES, NULL);
                    }
                }
                rpc_inst->unmarshal.byte_count = 0;
            }
            break;

        case STATE_UUID:
            EZTRACE("STATE_UUID");
            if (rpc_inst->unmarshal.curr_hdr != NULL)
            {
                rpc_inst->unmarshal.curr_hdr->uuid = 
                    (rpc_inst->unmarshal.curr_hdr->uuid << 8) | rx_byte;

                rpc_inst->unmarshal.byte_count++;
                if (rpc_inst->unmarshal.byte_count >= sizeof(uint16_t))
                {
                    rpc_inst->unmarshal.byte_count = 0;
                    rpc_inst->unmarshal.state = STATE_MSG_TYPE;
                    EZDEBUG("UUID parsed: uuid = %d", rpc_inst->unmarshal.curr_hdr->uuid);
                }
            }
            else
            {
                rpc_inst->unmarshal.state = STATE_SYNC;
                ezQueue_ReleaseReservedElement(
                    &rpc_inst->rx_msg_queue,
                    rpc_inst->unmarshal.header_elem);
            }
            break;

        case STATE_MSG_TYPE:
            EZTRACE("STATE_MSG_TYPE");
            if (rx_byte < RPC_MSG_NUM_OF_TYPE)
            {
                rpc_inst->unmarshal.curr_hdr->type = rx_byte;
                rpc_inst->unmarshal.state = STATE_ENCRYPT_FLAG;
                EZDEBUG("Message type parsed: type = %d", rpc_inst->unmarshal.curr_hdr->type);
            }
            else
            {
                EZDEBUG("wrong message type");
                if(rpc_inst->error_callback != NULL)
                {
                    rpc_inst->error_callback(RPC_ERROR_WRONG_MSG_TYPE, NULL);
                }
                rpc_inst->unmarshal.state = STATE_SYNC;
                ezQueue_ReleaseReservedElement(
                    &rpc_inst->rx_msg_queue,
                    rpc_inst->unmarshal.header_elem);
            }
            break;

        case STATE_ENCRYPT_FLAG:
            EZTRACE("STATE_ENCRYPT_FLAG");
            rpc_inst->unmarshal.curr_hdr->is_encrypted = rx_byte;
            rpc_inst->unmarshal.state = STATE_CMD_ID;
            EZDEBUG("Encryption flag parsed: is_encrypted = %d", rpc_inst->unmarshal.curr_hdr->is_encrypted);
            break;

        case STATE_CMD_ID:
            EZTRACE("STATE_CMD_ID");
            rpc_inst->unmarshal.curr_hdr->cmd_id = 
                    (rpc_inst->unmarshal.curr_hdr->cmd_id << 8) | rx_byte;
            rpc_inst->unmarshal.byte_count++;
            if (rpc_inst->unmarshal.byte_count >= sizeof(uint16_t))
            {
                rpc_inst->unmarshal.state = STATE_PAYLOAD_SIZE;
                rpc_inst->unmarshal.byte_count = 0;
                EZDEBUG("Command ID parsed: cmd_id = %d", rpc_inst->unmarshal.curr_hdr->cmd_id);
            }
            break;

        case STATE_PAYLOAD_SIZE:
            EZTRACE("STATE_PAYLOAD_SIZE");
            rpc_inst->unmarshal.curr_hdr->payload_size = 
                (rpc_inst->unmarshal.curr_hdr->payload_size << 8) | rx_byte;

            rpc_inst->unmarshal.byte_count++;

            if (rpc_inst->unmarshal.byte_count >= sizeof(uint32_t))
            {
                rpc_inst->unmarshal.payload_elem = ezQueue_ReserveElement(
                    &rpc_inst->rx_msg_queue,
                    (void*)&rpc_inst->unmarshal.payload,
                    rpc_inst->unmarshal.curr_hdr->payload_size);
            

                if (rpc_inst->unmarshal.payload_elem != NULL)
                {
                    rpc_inst->unmarshal.byte_count = 0;
                    rpc_inst->unmarshal.state = STATE_PAYLOAD;
                    EZDEBUG("Payload size parsed: payload_size = %d",
                        rpc_inst->unmarshal.curr_hdr->payload_size);

#if(DEBUG_LVL == LVL_TRACE)
                    ezRpc_PrintHeader(rpc_inst->unmarshal.curr_hdr);
#endif /* DEBUG_LVL == LVL_TRACE */
                }
                else
                {
                    (void)ezQueue_ReleaseReservedElement(
                        &rpc_inst->rx_msg_queue,
                        rpc_inst->unmarshal.header_elem);
                    if(rpc_inst->error_callback != NULL)
                    {
                        rpc_inst->error_callback(RPC_ERROR_QUEUE_RESERVE_FAILED, NULL);
                    }
                    rpc_inst->unmarshal.state = STATE_SYNC;
                    EZDEBUG("Queue operation error");
                }
            }
            break;

        case STATE_PAYLOAD:
            EZTRACE("STATE_PAYLOAD");

            rpc_inst->unmarshal.payload[rpc_inst->unmarshal.byte_count] = rx_byte;
            rpc_inst->unmarshal.byte_count++;

            if (rpc_inst->unmarshal.byte_count >= rpc_inst->unmarshal.curr_hdr->payload_size)
            {
#if(DEBUG_LVL == LVL_TRACE)
                ezRpc_PrintPayload(
                    rpc_inst->unmarshal.payload,
                    rpc_inst->unmarshal.curr_hdr->payload_size);
#endif /* DEBUG_LVL == LVL_TRACE */

                if (ezRpc_IsCrcActivated(rpc_inst))
                {
                    rpc_inst->unmarshal.crc_elem = ezQueue_ReserveElement(
                        &rpc_inst->rx_msg_queue,
                        (void *)&rpc_inst->unmarshal.crc_val,
                        rpc_inst->crc_handler->size);

                    if (rpc_inst->unmarshal.crc_elem != NULL)
                    {
                        rpc_inst->unmarshal.byte_count = 0;
                        rpc_inst->unmarshal.state = STATE_CRC;
                    }
                    else
                    {
                        EZDEBUG("Queue operation error");
                        if(rpc_inst->error_callback != NULL)
                        {
                            rpc_inst->error_callback(RPC_ERROR_QUEUE_RESERVE_FAILED, NULL);
                        }
                        rpc_inst->unmarshal.state = STATE_SYNC;
                        (void)ezQueue_ReleaseReservedElement(
                            &rpc_inst->rx_msg_queue,
                            rpc_inst->unmarshal.header_elem);

                        (void)ezQueue_ReleaseReservedElement(
                            &rpc_inst->rx_msg_queue,
                            rpc_inst->unmarshal.payload_elem);
                    }
                }
                else
                {
                    status = ezQueue_PushReservedElement(
                        &rpc_inst->rx_msg_queue,
                        rpc_inst->unmarshal.header_elem);

                    status = ezQueue_PushReservedElement(
                        &rpc_inst->rx_msg_queue,
                        rpc_inst->unmarshal.payload_elem);

                    rpc_inst->unmarshal.state = STATE_SYNC;
                }
            }
            break;

        case STATE_CRC:
            EZTRACE("STATE_CRC");

            rpc_inst->unmarshal.crc_val[rpc_inst->unmarshal.byte_count] = rx_byte;
            rpc_inst->unmarshal.byte_count++;

            if (rpc_inst->unmarshal.byte_count >= rpc_inst->crc_handler->size)
            {
#if(DEBUG_LVL == LVL_TRACE)
                ezRpc_PrintCrc(rpc_inst->unmarshal.crc_val, rpc_inst->crc_handler->size);
#endif /* DEBUG_LVL == LVL_TRACE */
                if (rpc_inst->crc_handler->verify != NULL)
                {
                    if (rpc_inst->crc_handler->verify(
                            rpc_inst->unmarshal.payload,
                            rpc_inst->unmarshal.curr_hdr->payload_size,
                            rpc_inst->unmarshal.crc_val,
                            rpc_inst->crc_handler->size) == true)
                    {
                        EZDEBUG("crc correct");
                        status = ezQueue_PushReservedElement(
                            &rpc_inst->rx_msg_queue,
                            rpc_inst->unmarshal.header_elem);

                        status = ezQueue_PushReservedElement(
                            &rpc_inst->rx_msg_queue,
                            rpc_inst->unmarshal.payload_elem);
                    }
                    else
                    {
                        EZDEBUG("crc wrong");
                        (void)ezQueue_ReleaseReservedElement(
                            &rpc_inst->rx_msg_queue,
                            rpc_inst->unmarshal.header_elem);

                        (void)ezQueue_ReleaseReservedElement(
                            &rpc_inst->rx_msg_queue,
                            rpc_inst->unmarshal.payload_elem);
                        if(rpc_inst->error_callback != NULL)
                        {
                            rpc_inst->error_callback(RPC_ERROR_CRC_FAILED, NULL);
                        }
                    }
                }

                (void)ezQueue_ReleaseReservedElement(
                    &rpc_inst->rx_msg_queue,
                    rpc_inst->unmarshal.crc_elem);

                rpc_inst->unmarshal.state = STATE_SYNC;
            }
            break;
        
        default:
            rpc_inst->unmarshal.state = STATE_SYNC;
            break;
        }
    }
}


/******************************************************************************
* Function : ezRPC_MarshalMessage
*//**
* @Description:
*
* This function marshals an RPC message into byte stream and put it in the transmit queue
*
* @param    *rpc_inst:      (IN)pointer to the rpc instance
* @param    type:           (IN)message type
* @param    tag:            (IN)tag value
* @param    *payload:       (IN)pointer to payload to send
* @param    payload_size:   (IN)siez of the payload
* @return   ezSUCCESS or ezFAIL
*
*******************************************************************************/
static ezSTATUS ezRPC_MarshalMessage(struct ezRpc *rpc_inst,
                                       struct ezRpcMsgHeader *header,
                                       uint8_t *payload,
                                       uint32_t payload_size)
{
    uint32_t alloc_size = HEADER_SIZE + payload_size;
    uint8_t *buff = NULL;
    ezSTATUS status = ezSUCCESS;
    struct ezRpcRequestRecord *record = NULL;
    ezReservedElement elem = NULL;

    EZTRACE("ezRPC_CreateRpcMessage()");

    if(payload == NULL || payload_size == 0 || header == NULL || rpc_inst == NULL)
    {
        return ezFAIL;
    }

    if(ezRpc_IsCrcActivated(rpc_inst))
    {
        alloc_size += rpc_inst->crc_handler->size;
    }
    EZDEBUG("[ total size = %d bytes]", alloc_size);

    if(header->type == RPC_MSG_REQ)
    {
        record = ezRpc_GetAvailRecord(rpc_inst);
        if(record == NULL)
        {
            EZDEBUG("no available record for new request");
            return ezFAIL;
        }
        record->uuid = header->uuid;
    }

    elem = ezQueue_ReserveElement(&rpc_inst->tx_msg_queue, (void**)&buff, alloc_size);
    if(elem == NULL)
    {
        EZDEBUG("cannot reserve queue element");
        if(record != NULL)
        {
            record->is_available = true;
        }
        return ezFAIL;
    }

    status = ezRpc_MarshalHeader(buff, header);
    if(status != ezSUCCESS)
    {
        ezQueue_ReleaseReservedElement(&rpc_inst->tx_msg_queue, elem);
        ezRpc_ResetRecord(record);
        return ezFAIL;
    }

    memcpy(&buff[HEADER_SIZE], payload, payload_size);
    EZDEBUG("payload value:");
    EZHEXDUMP(&buff[HEADER_SIZE], payload_size);

    if (ezRpc_IsCrcActivated(rpc_inst) == true)
    {
        rpc_inst->crc_handler->calculate(
            payload,
            payload_size,
            &buff[HEADER_SIZE + payload_size],
            rpc_inst->crc_handler->size);

        EZDEBUG("crc value:");
        EZHEXDUMP(&buff[HEADER_SIZE + payload_size], rpc_inst->crc_handler->size);
    }

    status = ezQueue_PushReservedElement(
        &rpc_inst->tx_msg_queue,
        elem);

    if(status != ezSUCCESS)
    {
        ezQueue_ReleaseReservedElement(&rpc_inst->tx_msg_queue, elem);
        ezRpc_ResetRecord(record);
        return ezFAIL;
    }

#if (DEBUG_LVL == LVL_TRACE)
    EZTRACE("serialized data:");
    EZHEXDUMP(buff, alloc_size);
#endif /* DEBUG_LVL == LVL_TRACE */

    return ezSUCCESS;
}


/******************************************************************************
* Function : ezRpc_HandleReceivedMsg
*//**
* @Description: this function checks the receive message in the rx_msg_queue 
* and handles them
*
* @param    *rpc_inst: (IN)rpc instance
* @return   None
*
*******************************************************************************/
static void ezRpc_HandleReceivedMsg(struct ezRpc *rpc_inst)
{
    struct ezRpcMsgHeader *header_ptr = NULL;
    struct ezRpcMsgHeader header;
    uint32_t header_size = 0U;
    uint8_t *payload = NULL;
    uint32_t payload_size = 0U;
    ezSTATUS status = ezFAIL;

    if(rpc_inst == NULL || ezRpc_IsRpcInstanceReady(rpc_inst) == false)
    {
        return;
    }

    if (ezQueue_GetNumOfElement(&rpc_inst->rx_msg_queue) == 0)
    {
        return;
    }

    if(ezQueue_GetFront(&rpc_inst->rx_msg_queue,
        (void *)&header_ptr,
        &header_size) != ezSUCCESS)
    {
        return;
    }

#if(DEBUG_LVL == LVL_TRACE)
    ezRpc_PrintHeader(header_ptr);
#endif /* DEBUG_LVL == LVL_TRACE */

    if (header_ptr->type == RPC_MSG_RESP)
    {
        status = ezFAIL;
        /* check in the records if we sent it */
        for (uint32_t i = 0; i < CONFIG_NUM_OF_REQUEST; i++)
        {
            if (rpc_inst->records[i].uuid == header_ptr->uuid)
            {
                status = ezSUCCESS;
                /* record found, so we clear it */
                rpc_inst->records[i].is_available = true;
                EZDEBUG("found request in record [uuid = %d]",
                    rpc_inst->records[i].uuid);
                break;
            }
        }

        if (status == ezFAIL)
        {
            EZDEBUG("no record found, discard message");
            /* discard header */
            (void)ezQueue_PopFront(&rpc_inst->rx_msg_queue);
            /* discard payload */
            (void)ezQueue_PopFront(&rpc_inst->rx_msg_queue);
            return;
        }
    }

    for (uint32_t i = 0; i < rpc_inst->num_of_commands; i++)
    {
        if (rpc_inst->commands[i].id == header_ptr->cmd_id)
        {
            EZDEBUG("service supported [cmd_id = %d]",
                rpc_inst->commands[i].id);

            memcpy(&header, header_ptr, sizeof(struct ezRpcMsgHeader));

            /* pop header to read payload */
            (void)ezQueue_PopFront(&rpc_inst->rx_msg_queue);

            /* get payload data */
            status = ezQueue_GetFront(&rpc_inst->rx_msg_queue,
                (void *)&payload,
                &payload_size);

            if ((status == ezSUCCESS)
                && (rpc_inst->commands[i].command_handler != NULL)
                && (header.payload_size == payload_size)) 
            {
#if(DEBUG_LVL == LVL_TRACE)
                ezRpc_PrintPayload(payload, payload_size);
#endif /* DEBUG_LVL == LVL_TRACE */
                rpc_inst->commands[i].command_handler(&header, payload, payload_size);
            }
            break;
        }
    }

    if(status != ezSUCCESS && rpc_inst->error_callback != NULL)
    {
        rpc_inst->error_callback(RPC_ERROR_UNKNOWN_CMD, NULL);
    }

    /* done, pop payload */
    (void)ezQueue_PopFront(&rpc_inst->rx_msg_queue);
}


/******************************************************************************
* Function : ezRpc_CheckTimeoutRecords
*//**
* @Description: this function checks if a used record is timeout. If timeout,
* we clear it (dont wait for the response of that request)
*
* @param    *rpc_inst: (IN)rpc instance
* @return   None
*
*******************************************************************************/
static void ezRpc_CheckTimeoutRecords(struct ezRpc *rpc_inst)
{
    if (rpc_inst != NULL)
    {
        for (uint32_t i = 0; i < CONFIG_NUM_OF_REQUEST; i++)
        {
#if 0 /* TODO */
            if (rpc_inst->records[i].is_available == false
                && rpc_inst->records[i].timestamp + (int)WAIT_TIME < ezmKernel_GetTickMillis())
            {
                rpc_inst->records[i].is_available = true;
                rpc_inst->records[i].timestamp = 0;
                rpc_inst->records[i].uuid = 0;
                rpc_inst->records[i].name = NULL;

                EZDEBUG("record [i = %d] [uuid = %d] is time out",
                    i,
                    rpc_inst->records[i].uuid);
            }
#endif
        }
    }
}


static inline void ezRpc_ResetRecord(struct ezRpcRequestRecord *record)
{
    if (record != NULL)
    {
        record->is_available = true;
        record->timestamp = 0;
        record->uuid = 0;
    }
}

#if(DEBUG_LVL == LVL_TRACE)
/******************************************************************************
* Function : ezRpc_PrintHeader
*//**
* @Description: Print the header on the terminal
*
* @param    *header: (IN)pointer to rpc message header
* @return   None
*
*******************************************************************************/
void ezRpc_PrintHeader(struct ezRpcMsgHeader *header)
{
    if (header != NULL)
    {
        dbg_print("\nRPC header\n");
        dbg_print("------------------------\n");
        dbg_print("uuid:\t\t %d\n", header->uuid);

        if (header->type == RPC_MSG_REQ)
        {
            dbg_print("type:\t\t request\n");
        }
        else if (header->type == RPC_MSG_RESP)
        {
            dbg_print("type:\t\t response\n");
        }
        else
        {
            dbg_print("type:\t\t unknown\n");
        }

        dbg_print("tag:\t\t %d\n", header->tag);
        dbg_print("encrypt:\t %d\n", header->is_encrypted);
        dbg_print("size:\t\t %d\n", header->payload_size);
        dbg_print("\n");
    }
}


/******************************************************************************
* Function : ezRpc_PrintPayload
*//**
* @Description: Print the payload on the terminal
*
* @param    *payload: (IN)pointer to rpc message payload
* @param    size: (IN)size of the payload
* @return   None
*
*******************************************************************************/
void ezRpc_PrintPayload(uint8_t *payload, uint32_t size)
{
    if (payload != NULL)
    {
        dbg_print("\nRPC payload\n");
        dbg_print("------------------------\n");
        EZHEXDUMP(payload, size);
        dbg_print("\n");
    }
}


/******************************************************************************
* Function : ezRpc_PrintCrc
*//**
* @Description: Print the crc on the terminal
*
* @param    *crc: (IN)pointer to rpc message crc
* @param    size: (IN)size of the crc
* @return   None
*
*******************************************************************************/
void ezRpc_PrintCrc(uint8_t *crc, uint32_t size)
{
    if (crc != NULL)
    {
        dbg_print("\nRPC CRC\n");
        dbg_print("------------------------\n");
        EZHEXDUMP(crc, size);
        dbg_print("\n");
    }
}
#endif /* DEBUG_LVL == LVL_TRACE */


#endif /* EZ_RPC == 1 */
/* End of file*/
