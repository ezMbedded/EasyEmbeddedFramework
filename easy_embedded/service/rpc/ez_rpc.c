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
#define SOF                 0x80    /**< start of frame, for syncronisation */
#define RPC_BYTE_READ       1U      /**< number of byte being read by RPC */
#define WAIT_TIME           3000U   /**< time a request waiting for its response in millisec*/


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/** @brief Message type enumeration
 */
typedef enum
{
    STATE_SOF,          /**< State parsing SOF */
    STATE_UUID,         /**< State parsing UUID */
    STATE_MSG_TYPE,     /**< State parsing message type */
    STATE_TAG,          /**< State parsing tag */
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

static ezSTATUS ezRPC_CreateRpcMessage(struct ezRpc *rpc_inst,
                                       struct ezRpcMsgHeader *header,
                                       uint8_t *payload,
                                       uint32_t payload_size);

static ezSTATUS ezRpc_SerializeRpcHeader(uint8_t *buff,
                                         uint32_t buff_size,
                                         struct ezRpcMsgHeader *header);
static bool ezRpc_IsCrcActivated(struct ezRpc *rpc_inst);
static struct ezRpcRequestRecord *ezRpc_GetAvailRecord(struct ezRpc *rpc_inst);
static void ezRpc_DeserializedData(struct ezRpc *rpc_inst, uint8_t rx_byte);
static void ezRpc_HandleReceivedMsg(struct ezRpc *rpc_inst);
static void ezRpc_CheckTimeoutRecords(struct ezRpc *rpc_inst);

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

            rpc_inst->deserializer.state = STATE_SOF;
            rpc_inst->deserializer.byte_count = 0;

            rpc_inst->encrypt.is_encrypted = 0;
        }
    }
    else
    {
        status = ezFAIL;
    }

    return status;
}


ezSTATUS ezRpc_SetCrcFunctions(struct ezRpc *rpc_inst,
                                uint32_t crc_size,
                                CrcVerify verify_func,
                                CrcCalculate cal_func)
{
    ezSTATUS status = ezSUCCESS;

    EZTRACE("ezRpc_SetCrcFunctions()");

    if (rpc_inst != NULL
        && verify_func != NULL
        && cal_func != NULL
        && crc_size > 0)
    {
        rpc_inst->crc.size = crc_size;
        rpc_inst->crc.IsCorrect = verify_func;
        rpc_inst->crc.Calculate = cal_func;
    }
    else
    {
        status = ezFAIL;
    }

    return status;
}


ezSTATUS ezRpc_SetTxRxFunctions(struct ezRpc *rpc_inst,
                                RpcTransmit tx_function,
                                RpcReceive rx_function)
{
    ezSTATUS status = ezSUCCESS;

    if (rpc_inst != NULL && tx_function != NULL && rx_function != NULL)
    {
        rpc_inst->RpcTransmit = tx_function;
        rpc_inst->RpcReceive = rx_function;
    }

    return status;
}


ezSTATUS ezRPC_CreateRpcRequest(struct ezRpc *rpc_inst,
                                uint8_t tag,
                                uint8_t *payload,
                                uint32_t payload_size)
{
    ezSTATUS status = ezFAIL;

    struct ezRpcMsgHeader temp_header = { 0 };

    if (rpc_inst != NULL)
    {
        temp_header.cmd_id = tag;
        temp_header.type = RPC_MSG_REQ;
        temp_header.payload_size = payload_size;
        temp_header.uuid = ++rpc_inst->next_uuid;
        temp_header.is_encrypted = rpc_inst->encrypt.is_encrypted;

        status = ezRPC_CreateRpcMessage(rpc_inst,
                                        &temp_header,
                                        payload,
                                        payload_size);
    }

    return status;
}


ezSTATUS ezRPC_CreateRpcResponse(struct ezRpc *rpc_inst,
    uint8_t tag,
    uint32_t uuid,
    uint8_t *payload,
    uint32_t payload_size)
{
    ezSTATUS status = ezFAIL;

    struct ezRpcMsgHeader temp_header = { 0 };

    if (rpc_inst != NULL)
    {
        temp_header.cmd_id = tag;
        temp_header.type = RPC_MSG_REQ;
        temp_header.payload_size = payload_size;
        temp_header.uuid = uuid;
        temp_header.is_encrypted = rpc_inst->encrypt.is_encrypted;

        status = ezRPC_CreateRpcMessage(rpc_inst,
            &temp_header,
            payload,
            payload_size);
    }

    return status;
}


void ezRPC_Run(struct ezRpc *rpc_inst)
{
    ezSTATUS status = ezSUCCESS; 

    if (rpc_inst != NULL && ezRpc_IsRpcInstanceReady(rpc_inst) == true)
    {
        /* receive bytes from communication interface */
        if (rpc_inst->RpcReceive)
        {
            uint8_t one_byte = 0U;

            /**Try to read all available bytes */
            while (rpc_inst->RpcReceive(&one_byte, RPC_BYTE_READ) == RPC_BYTE_READ)
            {
                ezRpc_DeserializedData(rpc_inst, one_byte);
            }
        }

        /* handle the received message */
        ezRpc_HandleReceivedMsg(rpc_inst);

        /* Transmit message */
        if (ezQueue_GetNumOfElement(&rpc_inst->tx_msg_queue) > 0)
        {
            uint8_t *req;
            uint32_t req_size;

            if (ezQueue_GetFront(&rpc_inst->tx_msg_queue,
                (void *)&req,
                &req_size) == ezSUCCESS)
            {
                rpc_inst->RpcTransmit(req, req_size);
            }

            (void) ezQueue_PopFront(&rpc_inst->tx_msg_queue);
        }

        ezRpc_CheckTimeoutRecords(rpc_inst);
    }
}


bool ezRpc_IsRpcInstanceReady(struct ezRpc *rpc_inst)
{
    bool is_ready = false;

    if (rpc_inst != NULL)
    {
        is_ready = ((rpc_inst->commands != NULL)
                    && (rpc_inst->num_of_commands > 0)
                    && (ezQueue_IsQueueReady(&rpc_inst->rx_msg_queue))
                    && (ezQueue_IsQueueReady(&rpc_inst->tx_msg_queue))
                    && (rpc_inst->RpcTransmit != NULL)
                    && (rpc_inst->RpcReceive));
    }

    return is_ready;
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
            records[i].name = NULL;
            records[i].timestamp = 0;
            records[i].uuid = 0;
        }
    }
}


/******************************************************************************
* Function : ezRpc_SerializeRpcHeader
*//**
* @Description: serialize the header of a RPC message
*
* @param    *buff: (IN)Buffer containing the serialized header
* @param    buff_size: (IN)Size of the header
* @param    *header: (IN)Pointer to the header
* @return   ezSUCCESS: success
*           ezFAIL: fail
*
*******************************************************************************/
static ezSTATUS ezRpc_SerializeRpcHeader(uint8_t *buff,
                                        uint32_t buff_size,
                                        struct ezRpcMsgHeader *header)
{
    ezSTATUS status = ezSUCCESS;

    if (buff != NULL && buff_size >= sizeof(struct ezRpcMsgHeader))
    {
        *(buff++) = SOF;

        *(uint32_t *)buff = header->uuid;
        buff += sizeof(uint32_t);

        *(buff++) = (uint8_t)header->type;
        *(buff++) = header->cmd_id;
        *(buff++) = header->is_encrypted;

        *(uint32_t *)buff = header->payload_size;
    }
    else
    {
        status = ezFAIL;
    }

    return status;
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
    return (rpc_inst->crc.size > 0
        && rpc_inst->crc.Calculate != NULL
        && rpc_inst->crc.IsCorrect != NULL);
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
* Function : ezRpc_DeserializedData
*//**
* @Description: Deserialize data from the communoication interface
*
* @param    *rpc_inst: (IN)pointer to rpc instance
* @param    rx_byte: (IN)byte receive from the communication interface
* @return   None
*
*******************************************************************************/
static void ezRpc_DeserializedData(struct ezRpc *rpc_inst, uint8_t rx_byte)
{
    ezSTATUS status = ezSUCCESS;

    if (rpc_inst != NULL)
    {
        switch (rpc_inst->deserializer.state)
        {
        case STATE_SOF:
            EZTRACE("STATE_SOF");

            if (rx_byte == SOF)
            {
                rpc_inst->deserializer.header_elem = ezQueue_ReserveElement(
                    &rpc_inst->rx_msg_queue,
                    (void*)&rpc_inst->deserializer.curr_hdr,
                    sizeof(struct ezRpcMsgHeader));

                if (rpc_inst->deserializer.header_elem != NULL)
                {
                    rpc_inst->deserializer.byte_count = 0;
                    rpc_inst->deserializer.curr_hdr->uuid = 0;
                    rpc_inst->deserializer.curr_hdr->payload_size = 0;

                    rpc_inst->deserializer.state = STATE_UUID;
                    EZDEBUG("Got SOF");
                }
            }
            break;

        case STATE_UUID:
            EZTRACE("STATE_UUID");
            if (rpc_inst->deserializer.curr_hdr != NULL)
            {
                rpc_inst->deserializer.curr_hdr->uuid = 
                    (rpc_inst->deserializer.curr_hdr->uuid << 8) | rx_byte;

                rpc_inst->deserializer.byte_count++;
                if (rpc_inst->deserializer.byte_count >= sizeof(uint32_t))
                {
                    rpc_inst->deserializer.byte_count = 0;
                    rpc_inst->deserializer.state = STATE_MSG_TYPE;
                }
            }
            else
            {
                rpc_inst->deserializer.state = STATE_SOF;
            }
            
            break;

        case STATE_MSG_TYPE:
            EZTRACE("STATE_MSG_TYPE");
            if (rx_byte == RPC_MSG_REQ || rx_byte == RPC_MSG_RESP)
            {
                rpc_inst->deserializer.curr_hdr->type = rx_byte;
                rpc_inst->deserializer.state = STATE_TAG;
            }
            else
            {
                EZDEBUG("wrong message type");
                rpc_inst->deserializer.state = STATE_SOF;
            }
            break;

        case STATE_TAG:
            EZTRACE("STATE_TAG");
            rpc_inst->deserializer.curr_hdr->cmd_id = rx_byte;
            rpc_inst->deserializer.state = STATE_ENCRYPT_FLAG;
            break;

        case STATE_ENCRYPT_FLAG:
            EZTRACE("STATE_ENCRYPT_FLAG");
            rpc_inst->deserializer.curr_hdr->is_encrypted = rx_byte;
            rpc_inst->deserializer.state = STATE_PAYLOAD_SIZE;
            break;

        case STATE_PAYLOAD_SIZE:
            EZTRACE("STATE_PAYLOAD_SIZE");
            rpc_inst->deserializer.curr_hdr->payload_size = 
                (rpc_inst->deserializer.curr_hdr->payload_size << 8) | rx_byte;

            rpc_inst->deserializer.byte_count++;

            if (rpc_inst->deserializer.byte_count >=  sizeof(uint32_t))
            {
                if (status == ezSUCCESS)
                {
                    rpc_inst->deserializer.payload = NULL;

                    rpc_inst->deserializer.payload_elem = ezQueue_ReserveElement(
                        &rpc_inst->rx_msg_queue,
                        (void*)&rpc_inst->deserializer.payload,
                        rpc_inst->deserializer.curr_hdr->payload_size);
                }

                if (rpc_inst->deserializer.payload_elem != NULL 
                    && rpc_inst->deserializer.payload != NULL)
                {
                    rpc_inst->deserializer.byte_count = 0;
                    rpc_inst->deserializer.state = STATE_PAYLOAD;

#if(DEBUG_LVL == LVL_TRACE)
                    ezRpc_PrintHeader(rpc_inst->deserializer.curr_hdr);
#endif /* DEBUG_LVL == LVL_TRACE */
                }
                else
                {
                    (void)ezQueue_ReleaseReservedElement(
                        &rpc_inst->rx_msg_queue,
                        rpc_inst->deserializer.header_elem);

                    rpc_inst->deserializer.state = STATE_SOF;
                    EZDEBUG("Queue operation error");
                }

            }
            break;

        case STATE_PAYLOAD:
            EZTRACE("STATE_PAYLOAD");

            *(rpc_inst->deserializer.payload + rpc_inst->deserializer.byte_count) = rx_byte;
            rpc_inst->deserializer.byte_count++;

            if (rpc_inst->deserializer.byte_count >= rpc_inst->deserializer.curr_hdr->payload_size)
            {
#if(DEBUG_LVL == LVL_TRACE)
                ezRpc_PrintPayload(rpc_inst->deserializer.payload, rpc_inst->deserializer.curr_hdr->payload_size);
#endif /* DEBUG_LVL == LVL_TRACE */

                if (ezRpc_IsCrcActivated(rpc_inst))
                {
                    rpc_inst->deserializer.crc = NULL;
                    rpc_inst->deserializer.crc_elem = ezQueue_ReserveElement(
                        &rpc_inst->rx_msg_queue,
                        (void *)&rpc_inst->deserializer.crc,
                        rpc_inst->crc.size);

                    if (rpc_inst->deserializer.crc_elem != NULL 
                        && rpc_inst->deserializer.crc != NULL)
                    {
                        rpc_inst->deserializer.byte_count = 0;
                        rpc_inst->deserializer.state = STATE_CRC;
                    }
                    else
                    {
                        (void)ezQueue_ReleaseReservedElement(
                            &rpc_inst->rx_msg_queue,
                            rpc_inst->deserializer.header_elem);

                        (void)ezQueue_ReleaseReservedElement(
                            &rpc_inst->rx_msg_queue,
                            rpc_inst->deserializer.payload_elem);

                        rpc_inst->deserializer.state = STATE_SOF;
                        EZDEBUG("Queue operation error");
                    }
                }
                else
                {
                    status = ezQueue_PushReservedElement(
                        &rpc_inst->rx_msg_queue,
                        rpc_inst->deserializer.header_elem);

                    status = ezQueue_PushReservedElement(
                        &rpc_inst->rx_msg_queue,
                        rpc_inst->deserializer.payload_elem);

                    rpc_inst->deserializer.state = STATE_SOF;
                }
            }
            break;

        case STATE_CRC:
            EZTRACE("STATE_CRC");

            *(rpc_inst->deserializer.crc + rpc_inst->deserializer.byte_count) = rx_byte;
            rpc_inst->deserializer.byte_count++;

            if (rpc_inst->deserializer.byte_count >= rpc_inst->crc.size)
            {
#if(DEBUG_LVL == LVL_TRACE)
                ezRpc_PrintCrc(rpc_inst->deserializer.crc, rpc_inst->crc.size);
#endif /* DEBUG_LVL == LVL_TRACE */
                
                if (rpc_inst->crc.IsCorrect)
                {
                    if (rpc_inst->crc.IsCorrect(rpc_inst->deserializer.payload,
                            rpc_inst->deserializer.curr_hdr->payload_size,
                            rpc_inst->deserializer.crc,
                            rpc_inst->crc.size))
                    {
                        EZDEBUG("crc correct");
                        status = ezQueue_PushReservedElement(
                            &rpc_inst->rx_msg_queue,
                            rpc_inst->deserializer.header_elem);

                        status = ezQueue_PushReservedElement(
                            &rpc_inst->rx_msg_queue,
                            rpc_inst->deserializer.payload_elem);
                    }
                    else
                    {
                        EZDEBUG("crc wrong");
                        (void)ezQueue_ReleaseReservedElement(
                            &rpc_inst->rx_msg_queue,
                            rpc_inst->deserializer.header_elem);

                        (void)ezQueue_ReleaseReservedElement(
                            &rpc_inst->rx_msg_queue,
                            rpc_inst->deserializer.payload_elem);
                    }
                }

                (void)ezQueue_ReleaseReservedElement(
                    &rpc_inst->rx_msg_queue,
                    rpc_inst->deserializer.crc_elem);

                rpc_inst->deserializer.state = STATE_SOF;

            }
            break;
        
        default:
            rpc_inst->deserializer.state = STATE_SOF;
            break;
        }
    }
}


/******************************************************************************
* Function : ezRPC_CreateRpcMessage
*//**
* @Description:
*
* This function creates an RPC message and put it in the transmit queue
*
* @param    *rpc_inst:      (IN)pointer to the rpc instance
* @param    type:           (IN)message type
* @param    tag:            (IN)tag value
* @param    *payload:       (IN)pointer to payload to send
* @param    payload_size:   (IN)siez of the payload
* @return   ezSUCCESS or ezFAIL
*
*******************************************************************************/
static ezSTATUS ezRPC_CreateRpcMessage(struct ezRpc *rpc_inst,
                                       struct ezRpcMsgHeader *header,
                                       uint8_t *payload,
                                       uint32_t payload_size)
{
    uint32_t alloc_size = sizeof(struct ezRpcMsgHeader) + payload_size;
    uint8_t *buff = NULL;
    ezSTATUS status = ezSUCCESS;
    struct ezRpcRequestRecord *record = NULL;
    ezReservedElement elem = NULL;

    EZTRACE("ezRPC_CreateRpcMessage()");

    if (rpc_inst != NULL && header != NULL)
    {
        if (rpc_inst->encrypt.is_encrypted)
        {
            /* TODO must allocate data according to encryption algo */
            alloc_size += rpc_inst->crc.size;
        }
        else
        {
            alloc_size += rpc_inst->crc.size;
        }

        EZDEBUG("[ total size = %d bytes]", alloc_size);

        record = ezRpc_GetAvailRecord(rpc_inst);

        if (record != NULL)
        {
            elem = ezQueue_ReserveElement(
                &rpc_inst->tx_msg_queue,
                (void**)&buff,
                alloc_size);

            if (elem != NULL)
            {
                status = ezRpc_SerializeRpcHeader(buff,
                    alloc_size,
                    header);
            }
            else
            {
                status = ezFAIL;
            }
        }
        else
        {
            status = ezFAIL;
        }

        if (status == ezSUCCESS && payload != NULL && payload_size > 0)
        {
            buff += sizeof(struct ezRpcMsgHeader);
            alloc_size -= sizeof(struct ezRpcMsgHeader);

            memcpy(buff, payload, payload_size);

            EZDEBUG("payload value:");
            EZHEXDUMP(buff, payload_size);
        }

        if (status == ezSUCCESS && ezRpc_IsCrcActivated(rpc_inst))
        {
            alloc_size -= payload_size;

            if (alloc_size >= rpc_inst->crc.size)
            {
                buff += sizeof(struct ezRpcMsgHeader) + payload_size;

                rpc_inst->crc.Calculate(payload,
                    payload_size,
                    buff,
                    rpc_inst->crc.size);

                EZDEBUG("crc value:");
                EZHEXDUMP(buff, rpc_inst->crc.size);
            }
            else
            {
                status = ezFAIL;
            }
        }

        if (record != NULL)
        {
            if (status == ezSUCCESS)
            {
                record->uuid = header->uuid;
                record->name = NULL;
#if 0 /* TODO */
                record->timestamp = ezmKernel_GetTickMillis();
#endif
            }
            else
            {
                record->is_available = true;
            }
        }

        if (status == ezSUCCESS)
        {
            status = ezQueue_PushReservedElement(
                &rpc_inst->tx_msg_queue,
                elem);
#if (DEBUG_LVL == LVL_TRACE)
            EZTRACE("serialized data:");
            EZHEXDUMP(buff, alloc_size);
#endif /* DEBUG_LVL == LVL_TRACE */
        }
        else
        {
            status = ezQueue_ReleaseReservedElement(
                &rpc_inst->tx_msg_queue,
                elem);
        }
    }
    else
    {
        status = ezFAIL;
    }

    return status;
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
    struct ezRpcMsgHeader *header = NULL;
    uint32_t header_size = 0U;
    uint8_t *payload = NULL;
    uint32_t payload_size = 0U;
    ezSTATUS status = ezFAIL;
    bool service_found = false;

    if (rpc_inst != NULL 
        && ezRpc_IsRpcInstanceReady(rpc_inst) == true
        && ezQueue_GetNumOfElement(&rpc_inst->rx_msg_queue) > 0)
    {
        status = ezQueue_GetFront(&rpc_inst->rx_msg_queue,
            (void *)&header,
            &header_size);

        if (status == ezSUCCESS)
        {
#if(DEBUG_LVL == LVL_TRACE)
            ezRpc_PrintHeader(header);
#endif /* DEBUG_LVL == LVL_TRACE */

            if (header->type == RPC_MSG_RESP)
            {
                status = ezFAIL;

                /* check in the records if we sent it */
                for (uint32_t i = 0; i < CONFIG_NUM_OF_REQUEST; i++)
                {
                    if (rpc_inst->records[i].uuid == header->uuid)
                    {
                        status = ezSUCCESS;

                        /* record found, so we clear it */
                        rpc_inst->records[i].is_available = true;
                        EZDEBUG("found request in record [uuid = %d]",
                            rpc_inst->records[i].uuid);
                    }
                }

                if (status == ezFAIL)
                {
                    /* discard header */
                    (void)ezQueue_PopFront(&rpc_inst->rx_msg_queue);

                    /* discard payload */
                    (void)ezQueue_PopFront(&rpc_inst->rx_msg_queue);

                    EZDEBUG("no record found, discard message");
                }
            }
        }

        if (status == ezSUCCESS)
        {
            for (uint32_t i = 0; i < rpc_inst->num_of_commands; i++)
            {
                if (rpc_inst->commands[i].id == header->cmd_id)
                {
                    EZDEBUG("service supported [cmd_id = %d]",
                        rpc_inst->commands[i].id);

                    /* pop header to read payload */
                    (void)ezQueue_PopFront(&rpc_inst->rx_msg_queue);

                    /* get payload data */
                    status = ezQueue_GetFront(&rpc_inst->rx_msg_queue,
                        (void *)&payload,
                        &payload_size);

#if(DEBUG_LVL == LVL_TRACE)
                    if (status == ezSUCCESS)
                    {
                        ezRpc_PrintPayload(payload, payload_size);
                    }
#endif /* DEBUG_LVL == LVL_TRACE */

                    if (status == ezSUCCESS &&
                        rpc_inst->commands[i].command_handler != NULL)
                    {
                        rpc_inst->commands[i].command_handler(payload, payload_size);
                    }

                    service_found = true;
                    break;
                }
            }

            if (service_found == false)
            {
                /* no service found for the tag, pop header */
                (void)ezQueue_PopFront(&rpc_inst->rx_msg_queue);
            }

            /* done pop payload */
            (void)ezQueue_PopFront(&rpc_inst->rx_msg_queue);
        }
    }
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
