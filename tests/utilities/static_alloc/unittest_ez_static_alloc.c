/*****************************************************************************
* Filename:         unittest_ez_static_alloc.c
* Author:           Hai Nguyen
* Original Date:    06.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   unittest_ez_static_alloc.c
 *  @author Hai Nguyen
 *  @date   06.03.2024
 *  @brief  Unit test for static allocation component
 *
 *  @details Detail description of the component
 * 
 */

/******************************************************************************
* Includes
*******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "unity.h"
#include "unity_fixture.h"
#include "ez_static_alloc.h"

TEST_GROUP(ez_static_alloc);

/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
/* None */


/******************************************************************************
* Module Typedefs
*******************************************************************************/
/* None */


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
/* the buffers are aligned so the tests do not depend on the address of the arrays */
static _Alignas(32) uint8_t au8Buffer[512] = {0};
static _Alignas(32) uint8_t au8HugeBuffer[70000] = {0};

#define PERM_NUM_BLOCK      5U
#define PERM_BLOCK_SIZE     16U
#define PERM_BUFFER_SIZE    128U

#define ALIGNMENT           ((size_t)CONFIG_STATIC_ALLOC_ALIGNMENT)
/* offset of the next block after a block of the given size */
#define ALIGN_UP(x)         ((((size_t)(x)) + ALIGNMENT - 1U) & ~(ALIGNMENT - 1U))


/******************************************************************************
* Function Definitions
*******************************************************************************/
static void RunAllTests(void);
static bool NextPermutation(uint8_t *arr, uint8_t len);

/******************************************************************************
* External functions
*******************************************************************************/
int main(int argc, const char *argv[])
{
    return UnityMain(argc, argv, RunAllTests);
}


TEST_SETUP(ez_static_alloc)
{
}


TEST_TEAR_DOWN(ez_static_alloc)
{
}


TEST_GROUP_RUNNER(ez_static_alloc)
{
    RUN_TEST_CASE(ez_static_alloc, u8_var);
    RUN_TEST_CASE(ez_static_alloc, u16_var);
    RUN_TEST_CASE(ez_static_alloc, u32_var);
    RUN_TEST_CASE(ez_static_alloc, array_1);
    RUN_TEST_CASE(ez_static_alloc, array_2);
    RUN_TEST_CASE(ez_static_alloc, free_out_of_order_merge);
    RUN_TEST_CASE(ez_static_alloc, free_all_permutations);
    RUN_TEST_CASE(ez_static_alloc, block_pool_exhausted_no_memory_lost);
    RUN_TEST_CASE(ez_static_alloc, init_invalid_argument);
    RUN_TEST_CASE(ez_static_alloc, null_handle_is_safe);
    RUN_TEST_CASE(ez_static_alloc, malloc_invalid_argument);
    RUN_TEST_CASE(ez_static_alloc, free_invalid_address);
    RUN_TEST_CASE(ez_static_alloc, malloc_exact_fit_and_full);
    RUN_TEST_CASE(ez_static_alloc, init_fails_when_block_pool_exhausted);
    RUN_TEST_CASE(ez_static_alloc, deinit_returns_blocks_to_pool);
    RUN_TEST_CASE(ez_static_alloc, address_and_size_are_aligned);
    RUN_TEST_CASE(ez_static_alloc, init_with_unaligned_buffer);
    RUN_TEST_CASE(ez_static_alloc, init_buffer_too_small_after_alignment);
    RUN_TEST_CASE(ez_static_alloc, size_bigger_than_16_bit);
    RUN_TEST_CASE(ez_static_alloc, size_overflow_is_rejected);
}


TEST(ez_static_alloc, u8_var)
{
    ezmMemList stMemList = {0};
    
    ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 512);

    uint8_t* pu8TestU8Var1 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, sizeof(uint8_t));
    *pu8TestU8Var1 = 100;
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(*pu8TestU8Var1, au8Buffer[0]);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);

    uint8_t* pu8TestU8Var2 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, sizeof(uint8_t));
    *pu8TestU8Var2 = 123;
    TEST_ASSERT_EQUAL(*pu8TestU8Var2, au8Buffer[ALIGN_UP(sizeof(uint8_t))]);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 2U);
    ezStaticAlloc_HexdumpBuffer(&stMemList); 

    ezStaticAlloc_Free(&stMemList, pu8TestU8Var1);
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 2U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 1U);

    ezStaticAlloc_Free(&stMemList, pu8TestU8Var2);
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 0U);
    }

TEST(ez_static_alloc, u16_var)
{
    ezmMemList stMemList;

    ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 512);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    uint16_t* pu16TestU16Var1 = (uint16_t*)ezStaticAlloc_Malloc(&stMemList, sizeof(uint16_t));
    *pu16TestU16Var1 = 0xABCD;
    TEST_ASSERT_EQUAL(*pu16TestU16Var1, *(uint16_t*)&au8Buffer[0]);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    uint16_t* pu16TestU16Var2 = (uint16_t*)ezStaticAlloc_Malloc(&stMemList, sizeof(uint16_t));
    *pu16TestU16Var2 = 0xCDEF;
    TEST_ASSERT_EQUAL(*pu16TestU16Var2, *(uint16_t*)&au8Buffer[ALIGN_UP(sizeof(uint16_t))]);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 2U);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    ezStaticAlloc_Free(&stMemList, pu16TestU16Var1);
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 2U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 1U);

    ezStaticAlloc_Free(&stMemList, pu16TestU16Var2);
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 0U);
}

TEST(ez_static_alloc, u32_var)
{
    ezmMemList stMemList;

    ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 512);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    uint32_t* pu32TestU32Var1 = (uint32_t*)ezStaticAlloc_Malloc(&stMemList, sizeof(uint32_t));
    *pu32TestU32Var1 = 0xABCDCDEF;
    TEST_ASSERT_EQUAL(*pu32TestU32Var1, *(uint32_t*)&au8Buffer[0]);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    uint32_t* pu32TestU32Var2 = (uint32_t*)ezStaticAlloc_Malloc(&stMemList, sizeof(uint32_t));
    *pu32TestU32Var2 = 0xCDEFEFAB;
    TEST_ASSERT_EQUAL(*pu32TestU32Var2, *(uint32_t*)&au8Buffer[ALIGN_UP(sizeof(uint32_t))]);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 2U);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    ezStaticAlloc_Free(&stMemList, pu32TestU32Var1);
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 2U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 1U);

    ezStaticAlloc_Free(&stMemList, pu32TestU32Var2);
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 0U);
}

TEST(ez_static_alloc, array_1)
{
    ezmMemList stMemList;
    bool bIsEqual;

    ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 512);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    /* alloc 256 bytes */
    uint8_t* au8Array256_1 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 256 * sizeof(uint8_t));
    memset(au8Array256_1, 1, 256 * sizeof(uint8_t));
    bIsEqual = memcmp(au8Array256_1, &au8Buffer[0], 256 * sizeof(uint8_t));

    TEST_ASSERT_EQUAL(bIsEqual, 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 1U);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    /* alloc another 256 bytes, buffer is fulled */
    uint8_t* au8Array256_2 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 256 * sizeof(uint8_t));
    memset(au8Array256_2, 2, 256 * sizeof(uint8_t));
    bIsEqual = memcmp(au8Array256_2, &au8Buffer[256], 256 * sizeof(uint8_t));

    TEST_ASSERT_EQUAL(bIsEqual, 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList),2U);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    /* Free first half 256 bytes */
    ezStaticAlloc_Free(&stMemList, au8Array256_1);
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 1U);

    /* Allocate 128 bytes */
    uint8_t* au8Array128_1 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 128 * sizeof(uint8_t));
    TEST_ASSERT_NOT_EQUAL(au8Array128_1, NULL);
    memset(au8Array128_1, 3, 128 * sizeof(uint8_t));
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    bIsEqual = memcmp(au8Array128_1, &au8Buffer[0], 128 * sizeof(uint8_t));
    TEST_ASSERT_EQUAL(bIsEqual, 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(2U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    /* Allocate another 128 bytes */
    uint8_t* au8Array128_2 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 128 * sizeof(uint8_t));
    TEST_ASSERT_NOT_EQUAL(au8Array128_2, NULL);
    memset(au8Array128_2, 4, 128 * sizeof(uint8_t));
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    bIsEqual = memcmp(au8Array128_2, &au8Buffer[128], 128 * sizeof(uint8_t));
    TEST_ASSERT_EQUAL(bIsEqual, 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 3U);

    /* Free second 128 byte */
    ezStaticAlloc_Free(&stMemList, au8Array128_2);
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 2U);

    /* Allocate another 64 bytes */
    uint8_t* au8Array64_1 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 64 * sizeof(uint8_t));
    TEST_ASSERT_NOT_EQUAL(au8Array64_1, NULL);
    memset(au8Array64_1, 5, 64 * sizeof(uint8_t));
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    bIsEqual = memcmp(au8Array64_1, &au8Buffer[128], 64 * sizeof(uint8_t));
    TEST_ASSERT_EQUAL(bIsEqual, 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 3U);

    /* Allocate another 64 bytes */
    uint8_t* au8Array64_2 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 64 * sizeof(uint8_t));
    TEST_ASSERT_NOT_EQUAL(au8Array64_2, NULL);
    memset(au8Array64_2, 6, 64 * sizeof(uint8_t));
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    bIsEqual = memcmp(au8Array64_2, &au8Buffer[192], 64 * sizeof(uint8_t));
    TEST_ASSERT_EQUAL(bIsEqual, 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 4U);

    /* Free everything let see if it breaks */
    ezStaticAlloc_Free(&stMemList, au8Array256_2);
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 3U);

    ezStaticAlloc_Free(&stMemList, au8Array128_1);
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 2U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 2U);

    ezStaticAlloc_Free(&stMemList, au8Array64_1);
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 2U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 1U);

    ezStaticAlloc_Free(&stMemList, au8Array64_2);
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 0U);

}

TEST(ez_static_alloc, array_2)
{
    ezmMemList stMemList;
    bool bIsEqual;

    ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 512);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    /* alloc 256 bytes */
    uint8_t* au8Array256_1 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 256 * sizeof(uint8_t));
    memset(au8Array256_1, 1, 256 * sizeof(uint8_t));
    bIsEqual = memcmp(au8Array256_1, &au8Buffer[0], 256 * sizeof(uint8_t));

    TEST_ASSERT_EQUAL(bIsEqual, 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 1U);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    /* alloc another 256 bytes, buffer is fulled */
    uint8_t* au8Array256_2 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 256 * sizeof(uint8_t));
    memset(au8Array256_2, 2, 256 * sizeof(uint8_t));
    bIsEqual = memcmp(au8Array256_2, &au8Buffer[256], 256 * sizeof(uint8_t));

    TEST_ASSERT_EQUAL(bIsEqual, 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 2U);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    /* Free first half 256 bytes */
    ezStaticAlloc_Free(&stMemList, au8Array256_1);
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 1U);

    /* Allocate 128 bytes */
    uint8_t* au8Array128_1 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 128 * sizeof(uint8_t));
    TEST_ASSERT_NOT_EQUAL(au8Array128_1, NULL);
    memset(au8Array128_1, 3, 128 * sizeof(uint8_t));
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    bIsEqual = memcmp(au8Array128_1, &au8Buffer[0], 128 * sizeof(uint8_t));
    TEST_ASSERT_EQUAL(bIsEqual, 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(2U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    /* Allocate another 128 bytes */
    uint8_t* au8Array128_2 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 128 * sizeof(uint8_t));
    TEST_ASSERT_NOT_EQUAL(au8Array128_2, NULL);
    memset(au8Array128_2, 4, 128 * sizeof(uint8_t));
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    bIsEqual = memcmp(au8Array128_2, &au8Buffer[128], 128 * sizeof(uint8_t));
    TEST_ASSERT_EQUAL(bIsEqual, 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 3U);

    /* Free second 128 byte */
    ezStaticAlloc_Free(&stMemList, au8Array128_2);
    ezStaticAlloc_HexdumpBuffer(&stMemList);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 2U);

    /* Allocate another 64 bytes */
    uint8_t* au8Array64_1 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 64 * sizeof(uint8_t));
    TEST_ASSERT_NOT_EQUAL(au8Array64_1, NULL);
    memset(au8Array64_1, 5, 64 * sizeof(uint8_t));
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    bIsEqual = memcmp(au8Array64_1, &au8Buffer[128], 64 * sizeof(uint8_t));
    TEST_ASSERT_EQUAL(bIsEqual, 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 3U);

    /* Allocate another 64 bytes */
    uint8_t* au8Array64_2 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 64 * sizeof(uint8_t));
    TEST_ASSERT_NOT_EQUAL(au8Array64_2, NULL);
    memset(au8Array64_2, 6, 64 * sizeof(uint8_t));
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    bIsEqual = memcmp(au8Array64_2, &au8Buffer[192], 64 * sizeof(uint8_t));
    TEST_ASSERT_EQUAL(bIsEqual, 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 0U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 4U);

    /* Free everything let see if it breaks */
    ezStaticAlloc_Free(&stMemList, au8Array64_1);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    ezStaticAlloc_Free(&stMemList, au8Array64_2);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    ezStaticAlloc_Free(&stMemList, au8Array128_1);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    ezStaticAlloc_Free(&stMemList, au8Array256_2);
    ezStaticAlloc_HexdumpBuffer(&stMemList);

    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfFreeBlock(&stMemList), 1U);
    TEST_ASSERT_EQUAL(ezStaticAlloc_GetNumOfAllocBlock(&stMemList), 0U);

}

TEST(ez_static_alloc, free_out_of_order_merge)
{
    ezmMemList stMemList;
    uint8_t* blocks[PERM_NUM_BLOCK];

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, PERM_BUFFER_SIZE));

    /* A..E, 10 bytes each, the remaining 50 bytes are the free tail T */
    for (uint8_t i = 0; i < PERM_NUM_BLOCK; i++)
    {
        blocks[i] = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, PERM_BLOCK_SIZE);
        TEST_ASSERT_NOT_NULL(blocks[i]);
        TEST_ASSERT_EQUAL_PTR(&au8Buffer[i * PERM_BLOCK_SIZE], blocks[i]);
    }
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(5U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    /* free A: [A, T] */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[0]));
    TEST_ASSERT_EQUAL_UINT16(2U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    /* free C: [A, C, T] */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[2]));
    TEST_ASSERT_EQUAL_UINT16(3U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    /* free E: merges with T -> [A, C, E+T]. The old code put E behind A */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[4]));
    TEST_ASSERT_EQUAL_UINT16(3U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    /* free D: merges with C and E+T -> [A, C+D+E+T] */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[3]));
    TEST_ASSERT_EQUAL_UINT16(2U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    /* free B: everything is merged into one block */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[1]));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    /* the whole buffer must be allocatable again */
    uint8_t* whole = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, PERM_BUFFER_SIZE);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[0], whole);
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, whole));
}


TEST(ez_static_alloc, free_all_permutations)
{
    ezmMemList stMemList;
    uint8_t* blocks[PERM_NUM_BLOCK];
    uint8_t order[PERM_NUM_BLOCK] = {0, 1, 2, 3, 4};
    uint16_t num_perm = 0U;

    /* Init once: a list which is fully merged goes back to its initial state,
     * so it can be reused (the block pool has a fixed size). */
    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, PERM_BUFFER_SIZE));

    do
    {
        for (uint8_t i = 0; i < PERM_NUM_BLOCK; i++)
        {
            blocks[i] = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, PERM_BLOCK_SIZE);
            TEST_ASSERT_NOT_NULL(blocks[i]);
        }

        for (uint8_t i = 0; i < PERM_NUM_BLOCK; i++)
        {
            TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[order[i]]));
        }

        TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
        TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

        uint8_t* whole = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, PERM_BUFFER_SIZE);
        TEST_ASSERT_EQUAL_PTR(&au8Buffer[0], whole);
        TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, whole));

        num_perm++;
    } while (NextPermutation(order, PERM_NUM_BLOCK));

    TEST_ASSERT_EQUAL_UINT16(120U, num_perm);
}


TEST(ez_static_alloc, block_pool_exhausted_no_memory_lost)
{
    ezmMemList stMemList;
    uint8_t* blocks[256U] = {NULL};
    uint16_t count = 0U;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8HugeBuffer, (sizeof(au8HugeBuffer) & ~(ALIGNMENT - 1U))));

    /* every split needs a block descriptor, so the pool runs dry at some point */
    while (count < (sizeof(blocks) / sizeof(blocks[0])))
    {
        blocks[count] = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 1U);
        if (blocks[count] == NULL)
        {
            break;
        }
        count++;
    }

    TEST_ASSERT_LESS_THAN_UINT16(sizeof(blocks) / sizeof(blocks[0]), count);

    for (uint16_t i = 0U; i < count; i++)
    {
        TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[i]));
    }

    /* nothing was lost when the split was not possible */
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));
    uint8_t* whole = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, (sizeof(au8HugeBuffer) & ~(ALIGNMENT - 1U)));
    TEST_ASSERT_EQUAL_PTR(&au8HugeBuffer[0], whole);
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, whole));
}

TEST(ez_static_alloc, init_invalid_argument)
{
    ezmMemList stMemList;

    memset(&stMemList, 0xFF, sizeof(stMemList));
    TEST_ASSERT_FALSE(ezStaticAlloc_InitMemList(NULL, au8Buffer, 64U));
    TEST_ASSERT_FALSE(ezStaticAlloc_InitMemList(&stMemList, NULL, 64U));
    TEST_ASSERT_FALSE(ezStaticAlloc_IsMemListReady(&stMemList));

    memset(&stMemList, 0xFF, sizeof(stMemList));
    TEST_ASSERT_FALSE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 0U));
    TEST_ASSERT_FALSE(ezStaticAlloc_IsMemListReady(&stMemList));
}


TEST(ez_static_alloc, null_handle_is_safe)
{
    TEST_ASSERT_FALSE(ezStaticAlloc_IsMemListReady(NULL));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(NULL));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfFreeBlock(NULL));
    TEST_ASSERT_FALSE(ezStaticAlloc_DeinitMemList(NULL));
    ezStaticAlloc_HexdumpBuffer(NULL);
    ezStaticAlloc_PrintFreeList(NULL);
    ezStaticAlloc_PrintAllocList(NULL);
}


TEST(ez_static_alloc, malloc_invalid_argument)
{
    ezmMemList stMemList;

    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(NULL, 4U));

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 64U));
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 0U));
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 65U));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));

    /* a released list can not allocate */
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 4U));
}


TEST(ez_static_alloc, free_invalid_address)
{
    ezmMemList stMemList;
    uint8_t dummy;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 64U));
    uint8_t* block = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 16U);
    TEST_ASSERT_NOT_NULL(block);

    TEST_ASSERT_FALSE(ezStaticAlloc_Free(NULL, block));
    TEST_ASSERT_FALSE(ezStaticAlloc_Free(&stMemList, NULL));
    TEST_ASSERT_FALSE(ezStaticAlloc_Free(&stMemList, &dummy));
    /* the address must be the start of the block */
    TEST_ASSERT_FALSE(ezStaticAlloc_Free(&stMemList, block + 1));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, block));
    /* double free */
    TEST_ASSERT_FALSE(ezStaticAlloc_Free(&stMemList, block));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, malloc_exact_fit_and_full)
{
    ezmMemList stMemList;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 64U));

    uint8_t* block = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 64U);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[0], block);
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    /* full */
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 1U));

    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, block));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, init_fails_when_block_pool_exhausted)
{
    ezmMemList stBigList;
    ezmMemList stSmallList;
    uint16_t count = 0U;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stBigList, au8HugeBuffer, (sizeof(au8HugeBuffer) & ~(ALIGNMENT - 1U))));

    /* use up all block descriptors */
    while (count < 256U && ezStaticAlloc_Malloc(&stBigList, 1U) != NULL)
    {
        count++;
    }
    TEST_ASSERT_LESS_THAN_UINT16(256U, count);

    /* init fails and the handle must not look ready */
    memset(&stSmallList, 0xFF, sizeof(stSmallList));
    TEST_ASSERT_FALSE(ezStaticAlloc_InitMemList(&stSmallList, au8Buffer, 64U));
    TEST_ASSERT_FALSE(ezStaticAlloc_IsMemListReady(&stSmallList));

    /* after the release the init works again */
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stBigList));
    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stSmallList, au8Buffer, 64U));
    TEST_ASSERT_TRUE(ezStaticAlloc_IsMemListReady(&stSmallList));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stSmallList));
}


TEST(ez_static_alloc, deinit_returns_blocks_to_pool)
{
    ezmMemList stMemList;

    /* more cycles than block descriptors in the pool: any leak would show up */
    for (uint16_t i = 0U; i < 400U; i++)
    {
        TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 64U));
        TEST_ASSERT_NOT_NULL(ezStaticAlloc_Malloc(&stMemList, 8U));
        TEST_ASSERT_NOT_NULL(ezStaticAlloc_Malloc(&stMemList, 8U));
        TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
        TEST_ASSERT_FALSE(ezStaticAlloc_IsMemListReady(&stMemList));
        /* second deinit has nothing to do */
        TEST_ASSERT_FALSE(ezStaticAlloc_DeinitMemList(&stMemList));
    }
}

TEST(ez_static_alloc, address_and_size_are_aligned)
{
    ezmMemList stMemList;
    uint8_t* blocks[4];
    const size_t sizes[4] = {1U, 3U, ALIGNMENT + 1U, 7U};

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 256U));

    for (uint8_t i = 0U; i < 4U; i++)
    {
        blocks[i] = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, sizes[i]);
        TEST_ASSERT_NOT_NULL(blocks[i]);
        TEST_ASSERT_EQUAL_UINT(0U, (uintptr_t)blocks[i] % ALIGNMENT);
        /* the requested bytes are usable and blocks do not overlap */
        memset(blocks[i], (int)(i + 1U), sizes[i]);
    }

    for (uint8_t i = 1U; i < 4U; i++)
    {
        TEST_ASSERT_EQUAL_PTR(blocks[i - 1U] + ALIGN_UP(sizes[i - 1U]), blocks[i]);
    }
    for (uint8_t i = 0U; i < 4U; i++)
    {
        for (size_t j = 0U; j < sizes[i]; j++)
        {
            TEST_ASSERT_EQUAL_UINT8(i + 1U, blocks[i][j]);
        }
    }

    /* the rounding is also applied to the whole buffer */
    for (uint8_t i = 0U; i < 4U; i++)
    {
        TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[i]));
    }
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_NOT_NULL(ezStaticAlloc_Malloc(&stMemList, 256U));
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 1U));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, init_with_unaligned_buffer)
{
    ezmMemList stMemList;
    uint8_t* block;

    /* au8Buffer + 1 is not aligned (the alignment is bigger than 1) */
    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, &au8Buffer[1], 64U));
    TEST_ASSERT_EQUAL_UINT(0U, (uintptr_t)stMemList.buff % ALIGNMENT);
    TEST_ASSERT_TRUE(stMemList.buff >= &au8Buffer[1]);
    TEST_ASSERT_TRUE(stMemList.buff + stMemList.buff_size <= &au8Buffer[1 + 64]);
    TEST_ASSERT_EQUAL_UINT(0U, stMemList.buff_size % ALIGNMENT);

    block = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 1U);
    TEST_ASSERT_NOT_NULL(block);
    TEST_ASSERT_EQUAL_UINT(0U, (uintptr_t)block % ALIGNMENT);
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, block));

    /* everything that is managed can be allocated at once */
    block = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, stMemList.buff_size);
    TEST_ASSERT_EQUAL_PTR(stMemList.buff, block);
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 1U));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, init_buffer_too_small_after_alignment)
{
    ezmMemList stMemList;

    memset(&stMemList, 0xFF, sizeof(stMemList));
    /* less than one aligned unit can not be managed */
    TEST_ASSERT_FALSE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, ALIGNMENT - 1U));
    TEST_ASSERT_FALSE(ezStaticAlloc_IsMemListReady(&stMemList));

    /* an unaligned start eats the whole buffer */
    memset(&stMemList, 0xFF, sizeof(stMemList));
    TEST_ASSERT_FALSE(ezStaticAlloc_InitMemList(&stMemList, &au8Buffer[1], ALIGNMENT - 1U));
    TEST_ASSERT_FALSE(ezStaticAlloc_IsMemListReady(&stMemList));

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, ALIGNMENT));
    TEST_ASSERT_NOT_NULL(ezStaticAlloc_Malloc(&stMemList, 1U));
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 1U));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, size_bigger_than_16_bit)
{
    ezmMemList stMemList;
    const size_t big = 66000U;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8HugeBuffer, sizeof(au8HugeBuffer)));
    TEST_ASSERT_EQUAL_UINT(sizeof(au8HugeBuffer) & ~(ALIGNMENT - 1U), stMemList.buff_size);

    /* 66000 would have been truncated to 464 with 16 bit sizes */
    uint8_t* block1 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, big);
    TEST_ASSERT_EQUAL_PTR(&au8HugeBuffer[0], block1);
    memset(block1, 0xA5, big);

    uint8_t* block2 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 1000U);
    TEST_ASSERT_EQUAL_PTR(&au8HugeBuffer[ALIGN_UP(big)], block2);
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 4000U));

    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, block1));
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, block2));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    uint8_t* whole = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, sizeof(au8HugeBuffer) & ~(ALIGNMENT - 1U));
    TEST_ASSERT_EQUAL_PTR(&au8HugeBuffer[0], whole);
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, size_overflow_is_rejected)
{
    ezmMemList stMemList;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 64U));

    /* rounding up to the alignment would wrap around to a small size */
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, SIZE_MAX));
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, SIZE_MAX - 1U));
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, (size_t)UINT16_MAX + 1U));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}

/******************************************************************************
* Internal functions
*******************************************************************************/
/* Lexicographic next permutation, returns false after the last one */
static bool NextPermutation(uint8_t *arr, uint8_t len)
{
    int8_t i = (int8_t)len - 2;
    while (i >= 0 && arr[i] >= arr[i + 1])
    {
        i--;
    }
    if (i < 0)
    {
        return false;
    }

    int8_t j = (int8_t)len - 1;
    while (arr[j] <= arr[i])
    {
        j--;
    }

    uint8_t tmp = arr[i];
    arr[i] = arr[j];
    arr[j] = tmp;

    for (int8_t l = i + 1, r = (int8_t)len - 1; l < r; l++, r--)
    {
        tmp = arr[l];
        arr[l] = arr[r];
        arr[r] = tmp;
    }
    return true;
}

static void RunAllTests(void)
{
    RUN_TEST_GROUP(ez_static_alloc);
}


/* End of file */
