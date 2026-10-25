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
static _Alignas(32) uint8_t au8Buffer[1024] = {0};
static _Alignas(32) uint8_t au8HugeBuffer[70000] = {0};

#define ALIGNMENT           ((size_t)CONFIG_STATIC_ALLOC_ALIGNMENT)
/* offset of the next block after a block of the given size */
#define ALIGN_UP(x)         ((((size_t)(x)) + ALIGNMENT - 1U) & ~(ALIGNMENT - 1U))

/* size of the record which is stored in front of the data of every chunk */
#define REC_SIZE            ALIGN_UP(sizeof(struct MemBlock))

#define PERM_NUM_BLOCK      5U
#define PERM_BLOCK_SIZE     16U
/* record + data of one block */
#define PERM_CHUNK_SIZE     (REC_SIZE + ALIGN_UP(PERM_BLOCK_SIZE))
/* five chunks and a free tail with 48 bytes of data */
#define PERM_BUFFER_SIZE    (PERM_NUM_BLOCK * PERM_CHUNK_SIZE + REC_SIZE + 48U)


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
    RUN_TEST_CASE(ez_static_alloc, record_is_stored_in_front_of_data);
    RUN_TEST_CASE(ez_static_alloc, free_releases_record_and_data);
    RUN_TEST_CASE(ez_static_alloc, record_of_merged_chunk_becomes_data);
    RUN_TEST_CASE(ez_static_alloc, no_limit_on_number_of_chunks);
    RUN_TEST_CASE(ez_static_alloc, remainder_too_small_for_record_is_not_lost);
    RUN_TEST_CASE(ez_static_alloc, malloc_fails_if_record_and_data_do_not_fit);
    RUN_TEST_CASE(ez_static_alloc, lists_do_not_share_records);
    RUN_TEST_CASE(ez_static_alloc, init_invalid_argument);
    RUN_TEST_CASE(ez_static_alloc, null_handle_is_safe);
    RUN_TEST_CASE(ez_static_alloc, malloc_invalid_argument);
    RUN_TEST_CASE(ez_static_alloc, free_invalid_address);
    RUN_TEST_CASE(ez_static_alloc, malloc_exact_fit_and_full);
    RUN_TEST_CASE(ez_static_alloc, deinit_and_reinit);
    RUN_TEST_CASE(ez_static_alloc, address_and_size_are_aligned);
    RUN_TEST_CASE(ez_static_alloc, init_with_unaligned_buffer);
    RUN_TEST_CASE(ez_static_alloc, init_buffer_too_small_after_alignment);
    RUN_TEST_CASE(ez_static_alloc, size_bigger_than_16_bit);
    RUN_TEST_CASE(ez_static_alloc, size_overflow_is_rejected);
}


/* Two variables of the given size, allocated and freed in order */
static void CheckTwoVars(size_t var_size)
{
    ezmMemList stMemList = {0};
    const size_t chunk = REC_SIZE + ALIGN_UP(var_size);

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 512));

    uint8_t* var1 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, var_size);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[REC_SIZE], var1);
    memset(var1, 0xAB, var_size);
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    /* the second record follows the data of the first chunk */
    uint8_t* var2 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, var_size);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[chunk + REC_SIZE], var2);
    memset(var2, 0xCD, var_size);
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(2U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    /* the first chunk is not adjacent to the free tail */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, var1));
    TEST_ASSERT_EQUAL_UINT16(2U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT8(0xCD, var2[0]);

    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, var2));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}

TEST(ez_static_alloc, u8_var)
{
    CheckTwoVars(sizeof(uint8_t));
}

TEST(ez_static_alloc, u16_var)
{
    CheckTwoVars(sizeof(uint16_t));
}

TEST(ez_static_alloc, u32_var)
{
    CheckTwoVars(sizeof(uint32_t));
}


TEST(ez_static_alloc, array_1)
{
    ezmMemList stMemList;
    const size_t total = 1024U;
    /* A takes 256 bytes of data, B takes everything behind it */
    const size_t b_size = total - 2U * REC_SIZE - 256U;
    uint8_t *a, *b, *c, *d, *e;
    size_t e_size;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, total));

    a = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 256U);
    memset(a, 1, 256U);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[REC_SIZE], a);
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    /* the buffer is full */
    b = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, b_size);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[2U * REC_SIZE + 256U], b);
    memset(b, 2, b_size);
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(2U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 1U));

    /* free A, then split its chunk: C (128) + record + remainder */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, a));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    c = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 128U);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[REC_SIZE], c);
    memset(c, 3, 128U);
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(2U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    /* D takes the remainder of A's chunk but leaves room for E */
    d = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 64U);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[2U * REC_SIZE + 128U], d);
    memset(d, 4, 64U);
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(3U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    e_size = 256U - 128U - REC_SIZE - 64U - REC_SIZE;
    e = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, e_size);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[3U * REC_SIZE + 128U + 64U], e);
    memset(e, 5, e_size);
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(4U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    /* the data of the neighbours must be untouched by the records */
    for (size_t i = 0U; i < 128U; i++) { TEST_ASSERT_EQUAL_UINT8(3, c[i]); }
    for (size_t i = 0U; i < 64U; i++)  { TEST_ASSERT_EQUAL_UINT8(4, d[i]); }
    for (size_t i = 0U; i < e_size; i++) { TEST_ASSERT_EQUAL_UINT8(5, e[i]); }
    for (size_t i = 0U; i < b_size; i++) { TEST_ASSERT_EQUAL_UINT8(2, b[i]); }

    /* free everything, the chunks merge back to one */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, b));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, c));
    TEST_ASSERT_EQUAL_UINT16(2U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, e));
    TEST_ASSERT_EQUAL_UINT16(2U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, d));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    a = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, total - REC_SIZE);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[REC_SIZE], a);
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}

TEST(ez_static_alloc, array_2)
{
    ezmMemList stMemList;
    const size_t total = 1024U;
    const size_t b_size = total - 2U * REC_SIZE - 256U;
    uint8_t *a, *b, *c, *d;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, total));

    a = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 256U);
    b = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, b_size);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    /* freeing the last chunk first, then reusing it */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, b));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    c = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 100U);
    d = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 100U);
    TEST_ASSERT_EQUAL_PTR(b, c);
    TEST_ASSERT_EQUAL_PTR(c + ALIGN_UP(100U) + REC_SIZE, d);
    TEST_ASSERT_EQUAL_UINT16(3U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    /* c is next to a (allocated) and d (allocated): no merge */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, c));
    TEST_ASSERT_EQUAL_UINT16(2U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    /* freeing d merges with c and with the tail */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, d));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, a));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[REC_SIZE], ezStaticAlloc_Malloc(&stMemList, total - REC_SIZE));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}

TEST(ez_static_alloc, free_out_of_order_merge)
{
    ezmMemList stMemList;
    uint8_t* blocks[PERM_NUM_BLOCK];

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, PERM_BUFFER_SIZE));

    /* A..E, 16 bytes of data each, the rest is the free tail T */
    for (uint8_t i = 0; i < PERM_NUM_BLOCK; i++)
    {
        blocks[i] = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, PERM_BLOCK_SIZE);
        TEST_ASSERT_NOT_NULL(blocks[i]);
        TEST_ASSERT_EQUAL_PTR(&au8Buffer[i * PERM_CHUNK_SIZE + REC_SIZE], blocks[i]);
    }
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(5U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    /* free A: [A, T] */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[0]));
    TEST_ASSERT_EQUAL_UINT16(2U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    /* free C: [A, C, T] */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[2]));
    TEST_ASSERT_EQUAL_UINT16(3U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    /* free E: merges with T -> [A, C, E+T] */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[4]));
    TEST_ASSERT_EQUAL_UINT16(3U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    /* free D: merges with C and E+T -> [A, C+D+E+T] */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[3]));
    TEST_ASSERT_EQUAL_UINT16(2U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    /* free B: everything is merged into one block */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[1]));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    /* the whole buffer (minus one record) must be allocatable again */
    uint8_t* whole = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, PERM_BUFFER_SIZE - REC_SIZE);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[REC_SIZE], whole);
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, whole));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, free_all_permutations)
{
    ezmMemList stMemList;
    uint8_t* blocks[PERM_NUM_BLOCK];
    uint8_t order[PERM_NUM_BLOCK] = {0, 1, 2, 3, 4};
    uint16_t num_perm = 0U;

    /* A list which is fully merged goes back to its initial state, so it can be reused. */
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

        uint8_t* whole = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, PERM_BUFFER_SIZE - REC_SIZE);
        TEST_ASSERT_EQUAL_PTR(&au8Buffer[REC_SIZE], whole);
        TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, whole));

        num_perm++;
    } while (NextPermutation(order, PERM_NUM_BLOCK));

    TEST_ASSERT_EQUAL_UINT16(120U, num_perm);
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, record_is_stored_in_front_of_data)
{
    ezmMemList stMemList;
    const size_t sizes[3] = {1U, 20U, 64U};
    uint8_t* blocks[3];

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 512U));

    for (uint8_t i = 0U; i < 3U; i++)
    {
        blocks[i] = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, sizes[i]);
        TEST_ASSERT_NOT_NULL(blocks[i]);

        /* the record sits directly in front of the data, inside the buffer */
        struct MemBlock* record = (struct MemBlock*)(blocks[i] - REC_SIZE);
        TEST_ASSERT_TRUE((uint8_t*)record >= au8Buffer);
        TEST_ASSERT_TRUE(blocks[i] + record->buff_size <= au8Buffer + 512U);
        TEST_ASSERT_EQUAL_PTR(blocks[i], record->buff);
        TEST_ASSERT_EQUAL_UINT(ALIGN_UP(sizes[i]), record->buff_size);
        TEST_ASSERT_TRUE(ezLinkedList_IsNodeInList(&stMemList.alloc_list_head, &record->node));
    }

    /* the free chunk has its own record behind the last data */
    struct Node* free_node = stMemList.free_list_head.next;
    TEST_ASSERT_EQUAL_PTR(blocks[2] + ALIGN_UP(sizes[2]), (uint8_t*)free_node);
    TEST_ASSERT_EQUAL_UINT(512U - 3U * REC_SIZE - ALIGN_UP(1U) - ALIGN_UP(20U) - ALIGN_UP(64U) - REC_SIZE,
                           ((struct MemBlock*)free_node)->buff_size);

    for (uint8_t i = 0U; i < 3U; i++)
    {
        TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[i]));
    }
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, free_releases_record_and_data)
{
    ezmMemList stMemList;
    const size_t usable = 256U - REC_SIZE;
    uint8_t *a, *b;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 256U));

    a = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 32U);
    b = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 32U);
    memset(a, 0xAA, 32U);
    memset(b, 0xBB, 32U);

    /* an allocation costs its data plus one record */
    TEST_ASSERT_EQUAL_PTR(a + 32U + REC_SIZE, b);

    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, a));
    for (size_t i = 0U; i < 32U; i++)
    {
        TEST_ASSERT_EQUAL_UINT8(0U, a[i]);
    }
    /* the neighbour is not touched */
    TEST_ASSERT_EQUAL_UINT8(0xBB, b[0]);
    TEST_ASSERT_EQUAL_UINT8(0xBB, b[31]);

    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, b));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    /* both records and both data areas are free again: the full payload fits */
    a = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, usable);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[REC_SIZE], a);
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 1U));
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, a));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, record_of_merged_chunk_becomes_data)
{
    ezmMemList stMemList;
    uint8_t *a, *b;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 256U));
    a = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 16U);
    b = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 16U);

    /* free b first: merges into the tail. Then a: merges with it, the record of b is
     * absorbed, so a block bigger than a's and b's data together can be allocated */
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, b));
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, a));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    struct MemBlock* record = (struct MemBlock*)stMemList.free_list_head.next;
    TEST_ASSERT_EQUAL_PTR(au8Buffer, record);
    TEST_ASSERT_EQUAL_UINT(256U - REC_SIZE, record->buff_size);
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, no_limit_on_number_of_chunks)
{
    ezmMemList stMemList;
    static uint8_t* blocks[1500U];
    const size_t buff_size = sizeof(au8HugeBuffer) & ~(ALIGNMENT - 1U);
    uint16_t count = 0U;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8HugeBuffer, buff_size));

    /* there is no pool of records: only the buffer limits the number of chunks */
    while (count < (sizeof(blocks) / sizeof(blocks[0])))
    {
        blocks[count] = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 1U);
        TEST_ASSERT_NOT_NULL(blocks[count]);
        count++;
    }
    TEST_ASSERT_EQUAL_UINT16(count, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    for (uint16_t i = 0U; i < count; i++)
    {
        TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[i]));
    }

    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));
    uint8_t* whole = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, buff_size - REC_SIZE);
    TEST_ASSERT_EQUAL_PTR(&au8HugeBuffer[REC_SIZE], whole);
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, remainder_too_small_for_record_is_not_lost)
{
    ezmMemList stMemList;
    /* 16 bytes of data and a remainder which can not hold a record plus one unit */
    const size_t remainder = REC_SIZE;
    const size_t buff_size = REC_SIZE + 16U + remainder;
    uint8_t* block;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, buff_size));

    block = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 16U);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[REC_SIZE], block);
    /* the whole chunk is handed out, no free chunk is left */
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT(16U + remainder, ((struct MemBlock*)(block - REC_SIZE))->buff_size);

    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, block));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    block = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 16U + remainder);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[REC_SIZE], block);
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, block));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, malloc_fails_if_record_and_data_do_not_fit)
{
    ezmMemList stMemList;
    const size_t payload = 64U;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, REC_SIZE + payload));

    /* the data alone would fit into the buffer, but not together with the record */
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, payload + ALIGNMENT));
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, REC_SIZE + payload));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    TEST_ASSERT_NOT_NULL(ezStaticAlloc_Malloc(&stMemList, payload));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, lists_do_not_share_records)
{
    ezmMemList stList1;
    ezmMemList stList2;
    uint8_t *a, *b;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stList1, &au8Buffer[0], 128U));
    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stList2, &au8Buffer[256], 128U));

    a = (uint8_t*)ezStaticAlloc_Malloc(&stList1, 8U);
    b = (uint8_t*)ezStaticAlloc_Malloc(&stList2, 8U);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[REC_SIZE], a);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[256U + REC_SIZE], b);

    /* an address of another list is not accepted */
    TEST_ASSERT_FALSE(ezStaticAlloc_Free(&stList1, b));
    TEST_ASSERT_FALSE(ezStaticAlloc_Free(&stList2, a));
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stList1, a));
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stList2, b));

    /* releasing one list does not influence the other */
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stList1));
    TEST_ASSERT_NOT_NULL(ezStaticAlloc_Malloc(&stList2, 8U));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stList2));
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

    /* the buffer must hold one record and one unit of data */
    memset(&stMemList, 0xFF, sizeof(stMemList));
    TEST_ASSERT_FALSE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, REC_SIZE));
    TEST_ASSERT_FALSE(ezStaticAlloc_IsMemListReady(&stMemList));

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, REC_SIZE + ALIGNMENT));
    TEST_ASSERT_TRUE(ezStaticAlloc_IsMemListReady(&stMemList));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
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

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 128U));
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 0U));
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 129U));
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

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 128U));
    uint8_t* block = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 16U);
    TEST_ASSERT_NOT_NULL(block);

    TEST_ASSERT_FALSE(ezStaticAlloc_Free(NULL, block));
    TEST_ASSERT_FALSE(ezStaticAlloc_Free(&stMemList, NULL));
    TEST_ASSERT_FALSE(ezStaticAlloc_Free(&stMemList, &dummy));
    /* the address must be the start of the data, not the record */
    TEST_ASSERT_FALSE(ezStaticAlloc_Free(&stMemList, block + 1));
    TEST_ASSERT_FALSE(ezStaticAlloc_Free(&stMemList, block - REC_SIZE));
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

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 128U));

    uint8_t* block = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 128U - REC_SIZE);
    TEST_ASSERT_EQUAL_PTR(&au8Buffer[REC_SIZE], block);
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));

    /* full */
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 1U));

    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, block));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, deinit_and_reinit)
{
    ezmMemList stMemList;

    for (uint16_t i = 0U; i < 400U; i++)
    {
        TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 128U));
        TEST_ASSERT_NOT_NULL(ezStaticAlloc_Malloc(&stMemList, 8U));
        TEST_ASSERT_NOT_NULL(ezStaticAlloc_Malloc(&stMemList, 8U));
        TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
        TEST_ASSERT_FALSE(ezStaticAlloc_IsMemListReady(&stMemList));
        /* second deinit has nothing to do */
        TEST_ASSERT_FALSE(ezStaticAlloc_DeinitMemList(&stMemList));
    }

    /* a re-initialized list starts empty, even if chunks were still allocated */
    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 128U));
    TEST_ASSERT_EQUAL_UINT16(0U, ezStaticAlloc_GetNumOfAllocBlock(&stMemList));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}

TEST(ez_static_alloc, address_and_size_are_aligned)
{
    ezmMemList stMemList;
    uint8_t* blocks[4];
    const size_t sizes[4] = {1U, 3U, ALIGNMENT + 1U, 7U};

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 512U));

    for (uint8_t i = 0U; i < 4U; i++)
    {
        blocks[i] = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, sizes[i]);
        TEST_ASSERT_NOT_NULL(blocks[i]);
        TEST_ASSERT_EQUAL_UINT(0U, (uintptr_t)blocks[i] % ALIGNMENT);
        TEST_ASSERT_EQUAL_UINT(0U, (uintptr_t)(blocks[i] - REC_SIZE) % ALIGNMENT);
        /* the requested bytes are usable and blocks do not overlap */
        memset(blocks[i], (int)(i + 1U), sizes[i]);
    }

    for (uint8_t i = 1U; i < 4U; i++)
    {
        TEST_ASSERT_EQUAL_PTR(blocks[i - 1U] + ALIGN_UP(sizes[i - 1U]) + REC_SIZE, blocks[i]);
    }
    for (uint8_t i = 0U; i < 4U; i++)
    {
        for (size_t j = 0U; j < sizes[i]; j++)
        {
            TEST_ASSERT_EQUAL_UINT8(i + 1U, blocks[i][j]);
        }
    }

    for (uint8_t i = 0U; i < 4U; i++)
    {
        TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, blocks[i]));
    }
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));
    TEST_ASSERT_NOT_NULL(ezStaticAlloc_Malloc(&stMemList, 512U - REC_SIZE));
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 1U));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, init_with_unaligned_buffer)
{
    ezmMemList stMemList;
    uint8_t* block;

    /* au8Buffer + 1 is not aligned (the alignment is bigger than 1) */
    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, &au8Buffer[1], 128U));
    TEST_ASSERT_EQUAL_UINT(0U, (uintptr_t)stMemList.buff % ALIGNMENT);
    TEST_ASSERT_TRUE(stMemList.buff >= &au8Buffer[1]);
    TEST_ASSERT_TRUE(stMemList.buff + stMemList.buff_size <= &au8Buffer[1 + 128]);
    TEST_ASSERT_EQUAL_UINT(0U, stMemList.buff_size % ALIGNMENT);

    block = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 1U);
    TEST_ASSERT_NOT_NULL(block);
    TEST_ASSERT_EQUAL_UINT(0U, (uintptr_t)block % ALIGNMENT);
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, block));

    /* everything that is managed, minus the record, can be allocated at once */
    block = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, stMemList.buff_size - REC_SIZE);
    TEST_ASSERT_EQUAL_PTR(stMemList.buff + REC_SIZE, block);
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
    TEST_ASSERT_FALSE(ezStaticAlloc_InitMemList(&stMemList, &au8Buffer[1], REC_SIZE + ALIGNMENT - 1U));
    TEST_ASSERT_FALSE(ezStaticAlloc_IsMemListReady(&stMemList));

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, REC_SIZE + ALIGNMENT));
    TEST_ASSERT_NOT_NULL(ezStaticAlloc_Malloc(&stMemList, 1U));
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 1U));
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, size_bigger_than_16_bit)
{
    ezmMemList stMemList;
    const size_t big = 66000U;
    const size_t buff_size = sizeof(au8HugeBuffer) & ~(ALIGNMENT - 1U);

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8HugeBuffer, sizeof(au8HugeBuffer)));
    TEST_ASSERT_EQUAL_UINT(buff_size, stMemList.buff_size);

    /* 66000 would have been truncated to 464 with 16 bit sizes */
    uint8_t* block1 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, big);
    TEST_ASSERT_EQUAL_PTR(&au8HugeBuffer[REC_SIZE], block1);
    memset(block1, 0xA5, big);

    uint8_t* block2 = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, 1000U);
    TEST_ASSERT_EQUAL_PTR(&au8HugeBuffer[REC_SIZE + ALIGN_UP(big) + REC_SIZE], block2);
    TEST_ASSERT_NULL(ezStaticAlloc_Malloc(&stMemList, 4000U));

    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, block1));
    TEST_ASSERT_TRUE(ezStaticAlloc_Free(&stMemList, block2));
    TEST_ASSERT_EQUAL_UINT16(1U, ezStaticAlloc_GetNumOfFreeBlock(&stMemList));

    uint8_t* whole = (uint8_t*)ezStaticAlloc_Malloc(&stMemList, buff_size - REC_SIZE);
    TEST_ASSERT_EQUAL_PTR(&au8HugeBuffer[REC_SIZE], whole);
    TEST_ASSERT_TRUE(ezStaticAlloc_DeinitMemList(&stMemList));
}


TEST(ez_static_alloc, size_overflow_is_rejected)
{
    ezmMemList stMemList;

    TEST_ASSERT_TRUE(ezStaticAlloc_InitMemList(&stMemList, au8Buffer, 128U));

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
