/*****************************************************************************
* Filename:         unittest_ez_rpc.cpp
* Author:           Hai Nguyen
* Original Date:    15.03.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   unittest_ez_rpc.cpp
 *  @author Hai Nguyen
 *  @date   15.03.2025
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 * 
 */

/******************************************************************************
* Includes
*******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "ez_rpc.h"
#include "fff.h"
#include <catch2/catch_test_macros.hpp>
#include "ez_endian.h"


/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
#define BUFF_SIZE       1024
#define SUM_FUNC        0x01
DEFINE_FFF_GLOBALS;


/******************************************************************************
* Module Typedefs
*******************************************************************************/


class RpcTestFixture {
private:
protected:
public:
    RpcTestFixture();
    ~RpcTestFixture(){}
protected:
};


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
static ezRpc client;
static ezRpc server;
static uint8_t client_buff[BUFF_SIZE] = {0};
static uint8_t server_buff[BUFF_SIZE] = {0};
static uint8_t client_txrx_buff[BUFF_SIZE] = {0};
static uint8_t server_txrx_buff[BUFF_SIZE] = {0};
static size_t client_txrx_buff_size = 0;
static size_t client_txrx_buff_idx = 0;
static size_t server_txrx_buff_size = 0;
static size_t server_txrx_buff_idx = 0;
static bool server_func_called = false;
static bool client_func_called = false;
static uint32_t sum_val = 0;
RPC_ERROR last_server_error = RPC_ERROR_MAX;

void ClientSum(struct ezRpcMsgHeader *header, void *payload, uint32_t payload_size_byte);
void ServerSum(struct ezRpcMsgHeader *header, void *payload, uint32_t payload_size_byte);
static uint32_t Sum(uint32_t a, uint32_t b);

static uint32_t ClientTx(uint8_t *tx_data, uint32_t tx_size);
static uint32_t ClientRx(uint8_t *rx_data, uint32_t tx_size);

static uint32_t ServerTx(uint8_t *tx_data, uint32_t tx_size);
static uint32_t ServerRx(uint8_t *rx_data, uint32_t tx_size);

void ServerErrorCallback(RPC_ERROR error_code, void *context);

bool VerifyCrC(
    uint8_t *input,
    uint32_t input_size,
    uint8_t *crc,
    uint32_t crc_size);

void CalculateCrC(
    uint8_t *input, 
    uint32_t input_size,
    uint8_t *crc_output,
    uint32_t crc_output_size);
    
ezRpcCommandEntry client_cmds[1] = {
    {
        .id = SUM_FUNC,
        .command_handler = ClientSum,
    }
};


ezRpcCommandEntry server_cmds[1] = {
    {
        .id = SUM_FUNC,
        .command_handler = ServerSum,
    }
};

ezRpcCrcHandler crc_config = {
    .verify = VerifyCrC,
    .calculate = CalculateCrC,
    .size = 2,
};

struct ezRpcCommInterface client_comm_interface = {
    .transmit = ClientTx,
    .receive = ClientRx,
};

struct ezRpcCommInterface server_comm_interface = {
    .transmit = ServerTx,
    .receive = ServerRx,
};

/******************************************************************************
* Function Definitions
*******************************************************************************/


/******************************************************************************
* External functions
*******************************************************************************/
TEST_CASE_METHOD(RpcTestFixture, "Test initialization", "[service][rpc]")
{
    CHECK(ezRpc_IsRpcInstanceReady(&client) == true);
    CHECK(ezRpc_IsRpcInstanceReady(&server) == true);
}

TEST_CASE_METHOD(RpcTestFixture, "Test serialize request", "[service][rpc]")
{
    uint32_t args[2] = {2, 3};
    args[0] = EZHTON32(args[0]);
    args[1] = EZHTON32(args[1]);
    ezRPC_CreateRpcRequest(&client, SUM_FUNC, (uint8_t*)args, sizeof(args));
    ezRPC_Run(&client);

    /* Check if header OK */
    CHECK(server_txrx_buff[0] == 0xCA);
    CHECK(server_txrx_buff[1] == 0xFE);
    CHECK(server_txrx_buff[2] == 0x00);
    CHECK(server_txrx_buff[3] == 0x01);
    CHECK(server_txrx_buff[4] == RPC_MSG_REQ);
    CHECK(server_txrx_buff[5] == 0x00);
    CHECK(server_txrx_buff[6] == 0x00);
    CHECK(server_txrx_buff[7] == 0x01);
    CHECK(server_txrx_buff[8] == 0x00);
    CHECK(server_txrx_buff[9] == 0x00);
    CHECK(server_txrx_buff[10] == 0x00);
    CHECK(server_txrx_buff[11] == 0x08);
}


TEST_CASE_METHOD(RpcTestFixture, "Test parse request", "[service][rpc]")
{
    uint32_t args[2] = {2, 3};
    args[0] = EZHTON32(args[0]);
    args[1] = EZHTON32(args[1]);
    ezRPC_CreateRpcRequest(&client, SUM_FUNC, (uint8_t*)args, sizeof(args));
    ezRPC_Run(&client);

    for(uint32_t i = 0; i < 0xFF; i++)
    {
        ezRPC_Run(&server);
    }
    CHECK(server_func_called == true);


    for(uint32_t i = 0; i < 0xFF; i++)
    {
        ezRPC_Run(&client);
    }
    CHECK(client_func_called == true);
    CHECK(sum_val == 5);
}


TEST_CASE_METHOD(RpcTestFixture, "Test unsupported command", "[service][rpc]")
{
    uint32_t args[2] = {2, 3};
    args[0] = EZHTON32(args[0]);
    args[1] = EZHTON32(args[1]);
    ezRPC_CreateRpcRequest(&client, SUM_FUNC + 1, (uint8_t*)args, sizeof(args));
    ezRPC_Run(&client);

    for(uint32_t i = 0; i < 0xFF; i++)
    {
        ezRPC_Run(&server);
    }
    CHECK(server_func_called == false);


    for(uint32_t i = 0; i < 0xFF; i++)
    {
        ezRPC_Run(&client);
    }
    CHECK(client_func_called == false);
}

TEST_CASE_METHOD(RpcTestFixture, "Test parse request with CRC", "[service][rpc]")
{
    uint32_t args[2] = {2, 3};
    args[0] = EZHTON32(args[0]);
    args[1] = EZHTON32(args[1]);
    ezRpc_SetCrcHandler(&client, &crc_config);
    ezRpc_SetCrcHandler(&server, &crc_config);
    ezRPC_CreateRpcRequest(&client, SUM_FUNC, (uint8_t*)args, sizeof(args));
    ezRPC_Run(&client);

    for(uint32_t i = 0; i < 0xFF; i++)
    {
        ezRPC_Run(&server);
    }
    CHECK(server_func_called == true);


    for(uint32_t i = 0; i < 0xFF; i++)
    {
        ezRPC_Run(&client);
    }
    CHECK(client_func_called == true);
    CHECK(sum_val == 5);
}

TEST_CASE_METHOD(RpcTestFixture, "Test parse request with wrong crc", "[service][rpc]")
{
    uint32_t args[2] = {2, 3};
    args[0] = EZHTON32(args[0]);
    args[1] = EZHTON32(args[1]);
    ezRpc_SetCrcHandler(&client, &crc_config);
    ezRpc_SetCrcHandler(&server, &crc_config);
    ezRPC_CreateRpcRequest(&client, SUM_FUNC, (uint8_t*)args, sizeof(args));
    
    ezRPC_Run(&client);
    server_txrx_buff[12] = 0xFF; /* corrupt the data */

    for(uint32_t i = 0; i < 0xFF; i++)
    {
        ezRPC_Run(&server);
    }
    CHECK(server_func_called == false);

    for(uint32_t i = 0; i < 0xFF; i++)
    {
        ezRPC_Run(&client);
    }
    CHECK(client_func_called == false);
}

/******************************************************************************
* Internal functions
*******************************************************************************/
RpcTestFixture::RpcTestFixture()
{
    client_txrx_buff_size = 0;
    server_txrx_buff_size = 0;
    server_txrx_buff_idx = 0;
    client_txrx_buff_idx = 0;
    server_func_called = false;
    client_func_called = false;
    sum_val = 0;
    memset(client_txrx_buff, 0, BUFF_SIZE);
    memset(server_txrx_buff, 0, BUFF_SIZE);
    ezRpc_Initialization(&client, client_buff, BUFF_SIZE, client_cmds, 1);
    ezRpc_Initialization(&server, server_buff, BUFF_SIZE, server_cmds, 1);
    ezRpc_SetCommFunctions(&client, &client_comm_interface);
    ezRpc_SetCommFunctions(&server, &server_comm_interface);
}

void ClientSum(struct ezRpcMsgHeader *header, void *payload, uint32_t payload_size_byte)
{
    client_func_called = true;
    sum_val = *(uint32_t*)payload;
    sum_val = EZNTOH32(sum_val);
}


void ServerSum(struct ezRpcMsgHeader *header, void *payload, uint32_t payload_size_byte)
{
    uint8_t *arg = (uint8_t*)payload;
    uint32_t a = *(uint32_t *)arg;
    arg += 4;
    uint32_t b = *(uint32_t*)arg;
    
    a = EZNTOH32(a);
    b = EZNTOH32(b);
    uint32_t ret = Sum(a, b);
    ret = EZHTON32(ret);
    server_func_called = true;
    ezRPC_CreateRpcResponse(&server, SUM_FUNC, header->uuid, (uint8_t*)&ret, sizeof(sum_val));
}

static uint32_t ClientTx(uint8_t *tx_data, uint32_t tx_size)
{
    memcpy(server_txrx_buff, tx_data, tx_size);
    server_txrx_buff_size = tx_size;

    return 0;
}

static uint32_t ClientRx(uint8_t *rx_data, uint32_t tx_size)
{
    if(client_txrx_buff_size > 0)
    {
        memcpy(rx_data, &client_txrx_buff[client_txrx_buff_idx], tx_size);
        client_txrx_buff_size -= tx_size;
        client_txrx_buff_idx += tx_size;
        return 1;
    }
    return 0;
}

static uint32_t ServerTx(uint8_t *tx_data, uint32_t tx_size)
{
    memcpy(client_txrx_buff, tx_data, tx_size);
    client_txrx_buff_size = tx_size;
    return 0;
}

static uint32_t ServerRx(uint8_t *rx_data, uint32_t tx_size)
{
    if(server_txrx_buff_size > 0)
    {
        memcpy(rx_data, &server_txrx_buff[server_txrx_buff_idx], tx_size);
        server_txrx_buff_size -= tx_size;
        server_txrx_buff_idx += tx_size;
        return 1;
    }
    return 0;
}

static uint32_t Sum(uint32_t a, uint32_t b)
{
    return a+b;
}


bool VerifyCrC(
    uint8_t *input,
    uint32_t input_size,
    uint8_t *crc,
    uint32_t crc_size)
{
    uint32_t crc_val = 0;

    if(input_size == 0 || crc_size == 0 || input == NULL || crc == NULL)
    {
        return false;
    }

    for(uint32_t i = 0; i < input_size; i++)
    {
        crc_val += input[i];
    }

    if(crc[0] == (uint8_t)(crc_val & 0xFF)
        && crc[1] == (uint8_t)((crc_val >> 8) & 0xFF))
    {
        return true;
    }
    return false;
}

void CalculateCrC(
    uint8_t *input, 
    uint32_t input_size,
    uint8_t *crc_output,
    uint32_t crc_output_size)
{
    uint32_t crc_val = 0;
    if(input_size == 0 || crc_output_size == 0 || input == NULL || crc_output == NULL)
    {
        return;
    }

    for(uint32_t i = 0; i < input_size; i++)
    {
        crc_val += input[i];
    }

    crc_output[0] = (uint8_t)(crc_val & 0xFF);
    crc_output[1] = (uint8_t)((crc_val >> 8) & 0xFF);
}

void ServerErrorCallback(RPC_ERROR error_code, void *context)
{
    last_server_error = error_code;
}

/* End of file */
