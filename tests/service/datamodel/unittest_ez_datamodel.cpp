/*****************************************************************************
* Filename:         unittest_ez_osal.c
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

/** @file   unittest_ez_osal.c
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
#include <stddef.h>
#include <string>
#include "ez_data_model.h"
#include "fff.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ez_event_bus.h"


/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
#define BUFFER_SIZE    256
#define DATA_POINT_BOOL_IDX         0U
#define DATA_POINT_UINT8_IDX        1U
#define DATA_POINT_UINT16_IDX       2U
#define DATA_POINT_UINT32_IDX       3U
#define DATA_POINT_INT8_IDX         4U
#define DATA_POINT_INT16_IDX        5U
#define DATA_POINT_INT32_IDX        6U
#define DATA_POINT_FLOAT_IDX        7U
#define DATA_POINT_DOUBLE_IDX       8U
#define DATA_POINT_STRING_IDX       9U
#define DATA_POINT_BLOB_IDX         10U
#define DATA_POINT_STRING_SIZE      64U
#define DATA_POINT_BLOB_SIZE        128U

/******************************************************************************
* Module Typedefs
*******************************************************************************/
static uint8_t buff[BUFFER_SIZE];

static ezDataPoint_t data_points[] = {
    {DATA_POINT_BOOL_IDX, sizeof(bool), NULL, false},
    {DATA_POINT_UINT8_IDX, sizeof(uint8_t), NULL, false},
    {DATA_POINT_UINT16_IDX, sizeof(uint16_t), NULL, false},
    {DATA_POINT_UINT32_IDX, sizeof(uint32_t), NULL, false},
    {DATA_POINT_INT8_IDX, sizeof(int8_t), NULL, false},
    {DATA_POINT_INT16_IDX, sizeof(int16_t), NULL, false},
    {DATA_POINT_INT32_IDX, sizeof(int32_t), NULL, false},
    {DATA_POINT_FLOAT_IDX, sizeof(float), NULL, false},
    {DATA_POINT_DOUBLE_IDX, sizeof(double), NULL, false},
    {DATA_POINT_STRING_IDX, DATA_POINT_STRING_SIZE, NULL, false},
    {DATA_POINT_BLOB_IDX, DATA_POINT_BLOB_SIZE, NULL, false},
};

class DatamodelTestsFixture {
private:
protected:
    ezDataModel_t data_model;
public:
    DatamodelTestsFixture();
    ~DatamodelTestsFixture(){}
    
protected:
};


/******************************************************************************
* Module Variable Definitions
*******************************************************************************/


/******************************************************************************
* Function Definitions
*******************************************************************************/


/******************************************************************************
* External functions
*******************************************************************************/

TEST_CASE_METHOD(DatamodelTestsFixture, "Set data", "[service][datamodel]")
{
    SECTION("Set UINT8 data point")
    {
        const uint8_t *value = 0;
        size_t data_size = 0U;
        uint8_t expected_value = 42;
        ezSTATUS status = ezDataModel_SetDataPoint(
            &data_model,
            DATA_POINT_UINT8_IDX,
            &expected_value,
            sizeof(expected_value));
        REQUIRE(status == ezSUCCESS);

        value = (const uint8_t*)ezDataModel_GetDataPoint(
            &data_model,
            DATA_POINT_UINT8_IDX,
            &data_size);
        REQUIRE(value != NULL);
        REQUIRE(data_size == sizeof(expected_value));
        CHECK(*value == expected_value);

        uint32_t index = ezDataModel_GetFirstDirty(&data_model);
        CHECK(index == DATA_POINT_UINT8_IDX);
        ezDataModel_ClearDirtyFlags(&data_model, DATA_POINT_UINT8_IDX);
        index = ezDataModel_GetFirstDirty(&data_model);
        CHECK(index == DATA_POINT_INVALID);
    }

    SECTION("Set BOOL data point")
    {
        const bool *value = NULL;
        bool expected_value = true;
        ezSTATUS status = ezDataModel_SetDataPoint(
            &data_model,
            DATA_POINT_BOOL_IDX,
            &expected_value,
            sizeof(expected_value));
        REQUIRE(status == ezSUCCESS);

        size_t data_size = 0U;
        value = (const bool*)ezDataModel_GetDataPoint(
            &data_model,
            DATA_POINT_BOOL_IDX,
            &data_size);
        REQUIRE(value != NULL);
        REQUIRE(data_size == sizeof(expected_value));
        CHECK(*value == expected_value);

        uint32_t index = ezDataModel_GetFirstDirty(&data_model);
        CHECK(index == DATA_POINT_BOOL_IDX);
        ezDataModel_ClearDirtyFlags(&data_model, DATA_POINT_BOOL_IDX);
        index = ezDataModel_GetFirstDirty(&data_model);
        CHECK(index == DATA_POINT_INVALID);
    }

    SECTION("Set INT16 data point")
    {
        const int16_t *value = NULL;
        int16_t expected_value = -1234;
        ezSTATUS status = ezDataModel_SetDataPoint(
            &data_model,
            DATA_POINT_INT16_IDX,
            &expected_value,
            sizeof(expected_value));
        REQUIRE(status == ezSUCCESS);

        size_t data_size = 0U;
        value = (const int16_t*)ezDataModel_GetDataPoint(
            &data_model,
            DATA_POINT_INT16_IDX,
            &data_size);

        REQUIRE(value != NULL);
        REQUIRE(data_size == sizeof(expected_value));
        CHECK(*value == expected_value);

        uint32_t index = ezDataModel_GetFirstDirty(&data_model);
        CHECK(index == DATA_POINT_INT16_IDX);
        ezDataModel_ClearDirtyFlags(&data_model, DATA_POINT_INT16_IDX);
        index = ezDataModel_GetFirstDirty(&data_model);
        CHECK(index == DATA_POINT_INVALID);
    }

    SECTION("Set UINT32 data point")
    {
        const uint32_t *value = 0;
        uint32_t expected_value = 0xDEADBEEF;
        ezSTATUS status = ezDataModel_SetDataPoint(
            &data_model,
            DATA_POINT_UINT32_IDX,
            &expected_value,
            sizeof(expected_value));
        REQUIRE(status == ezSUCCESS);

        size_t data_size = 0U;
        value = (const uint32_t*)ezDataModel_GetDataPoint(
            &data_model,
            DATA_POINT_UINT32_IDX,
            &data_size);
        REQUIRE(value != NULL);
        REQUIRE(data_size == sizeof(expected_value));
        CHECK(*value == expected_value);

        uint32_t index = ezDataModel_GetFirstDirty(&data_model);
        CHECK(index == DATA_POINT_UINT32_IDX);
        ezDataModel_ClearDirtyFlags(&data_model, DATA_POINT_UINT32_IDX);
        index = ezDataModel_GetFirstDirty(&data_model);
        CHECK(index == DATA_POINT_INVALID);
    }

    SECTION("Set FLOAT data point")
    {
        const float *value = 0;
        float expected_value = 3.14159f;
        ezSTATUS status = ezDataModel_SetDataPoint(
            &data_model,
            DATA_POINT_FLOAT_IDX,
            &expected_value,
            sizeof(expected_value));
        REQUIRE(status == ezSUCCESS);

        size_t data_size = 0U;
        value = (const float*)ezDataModel_GetDataPoint(
            &data_model,
            DATA_POINT_FLOAT_IDX,
            &data_size);
        REQUIRE(value != NULL);
        REQUIRE(data_size == sizeof(expected_value));
        CHECK(*value == Catch::Approx(expected_value));

        uint32_t index = ezDataModel_GetFirstDirty(&data_model);
        CHECK(index == DATA_POINT_FLOAT_IDX);
        ezDataModel_ClearDirtyFlags(&data_model, DATA_POINT_FLOAT_IDX);
        index = ezDataModel_GetFirstDirty(&data_model);
        CHECK(index == DATA_POINT_INVALID);
    }

    SECTION("Set STRING data point")
    {
        const char *value = NULL;
        char expected_value[DATA_POINT_STRING_SIZE] = "hello world";
        ezSTATUS status = ezDataModel_SetDataPoint(
            &data_model,
            DATA_POINT_STRING_IDX,
            expected_value,
            sizeof(expected_value));
        REQUIRE(status == ezSUCCESS);

        size_t data_size = 0U;
        value = (const char*)ezDataModel_GetDataPoint(
            &data_model,
            DATA_POINT_STRING_IDX,
            &data_size);
        REQUIRE(value != NULL);
        REQUIRE(data_size == DATA_POINT_STRING_SIZE);
        CHECK(std::string(value) == std::string(expected_value));

        uint32_t index = ezDataModel_GetFirstDirty(&data_model);
        CHECK(index == DATA_POINT_STRING_IDX);
        ezDataModel_ClearDirtyFlags(&data_model, DATA_POINT_STRING_IDX);
        index = ezDataModel_GetFirstDirty(&data_model);
        CHECK(index == DATA_POINT_INVALID);
    }
}


TEST_CASE_METHOD(DatamodelTestsFixture, "Data is not overflown", "[service][datamodel]")
{
    bool bool_value = true;
    uint8_t uint8_value = 255;
    uint16_t uint16_value = 65535;
    ezSTATUS status;

    WHEN("Data is set")
    {
        status = ezDataModel_SetDataPoint(
            &data_model,
            DATA_POINT_BOOL_IDX,
            &bool_value,
            sizeof(bool_value));
        REQUIRE(status == ezSUCCESS);

        status = ezDataModel_SetDataPoint(
            &data_model,
            DATA_POINT_UINT8_IDX,
            &uint8_value,
            sizeof(uint8_value));
        REQUIRE(status == ezSUCCESS);

        status = ezDataModel_SetDataPoint(
            &data_model,
            DATA_POINT_UINT16_IDX,
            &uint16_value,
            sizeof(uint16_value));
        REQUIRE(status == ezSUCCESS);

        THEN("Data is not overflown")
        {
            const bool* read_bool_value = NULL;
            size_t data_size = 0U;
            read_bool_value = (const bool*)ezDataModel_GetDataPoint(
                &data_model,
                DATA_POINT_BOOL_IDX,
                &data_size);
            REQUIRE(read_bool_value != NULL);
            REQUIRE(data_size == sizeof(bool_value));
            CHECK(*read_bool_value == bool_value);

            const uint8_t* read_uint8_value = NULL;
            read_uint8_value = (const uint8_t*)ezDataModel_GetDataPoint(
                &data_model,
                DATA_POINT_UINT8_IDX,
                &data_size);
            REQUIRE(read_uint8_value != NULL);
            REQUIRE(data_size == sizeof(uint8_value));
            CHECK(*read_uint8_value == uint8_value);

            const uint16_t* read_uint16_value = NULL;
            read_uint16_value = (const uint16_t*)ezDataModel_GetDataPoint(
                &data_model,
                DATA_POINT_UINT16_IDX,
                &data_size);
            REQUIRE(read_uint16_value != NULL);
            REQUIRE(data_size == sizeof(uint16_value));
            CHECK(*read_uint16_value == uint16_value);
        }
    }
}


TEST_CASE_METHOD(DatamodelTestsFixture, "Set data is validated", "[service][datamodel]")
{
    SECTION("Size mismatch is rejected and nothing is written")
    {
        uint32_t value = 0xDEADBEEF;
        ezSTATUS status = ezDataModel_SetDataPoint(
            &data_model,
            DATA_POINT_UINT16_IDX,
            &value,
            sizeof(value));
        CHECK(status == ezSTATUS_ARG_INVALID);
        CHECK(ezDataModel_GetFirstDirty(&data_model) == DATA_POINT_INVALID);
    }

    SECTION("Unknown index is rejected")
    {
        uint8_t value = 1;
        CHECK(ezDataModel_SetDataPoint(&data_model, 1000U, &value, sizeof(value)) == ezFAIL);
        size_t data_size = 1U;
        CHECK(ezDataModel_GetDataPoint(&data_model, 1000U, &data_size) == NULL);
        CHECK(data_size == 0U);
    }

    SECTION("NULL arguments are rejected")
    {
        uint8_t value = 1;
        CHECK(ezDataModel_SetDataPoint(NULL, DATA_POINT_UINT8_IDX, &value, sizeof(value)) == ezSTATUS_ARG_INVALID);
        CHECK(ezDataModel_SetDataPoint(&data_model, DATA_POINT_UINT8_IDX, NULL, sizeof(value)) == ezSTATUS_ARG_INVALID);
    }
}


TEST_CASE_METHOD(DatamodelTestsFixture, "Data points are zeroed and aligned", "[service][datamodel]")
{
    size_t data_size = 0U;

    for(size_t i = 0U; i < sizeof(data_points)/sizeof(data_points[0]); i++)
    {
        const uint8_t *value = (const uint8_t*)ezDataModel_GetDataPoint(
            &data_model,
            data_points[i].index,
            &data_size);
        REQUIRE(value != NULL);
        REQUIRE(data_size == data_points[i].size);

        for(size_t j = 0U; j < data_size; j++)
        {
            CHECK(value[j] == 0U);
        }
    }

    const void *ptr = ezDataModel_GetDataPoint(&data_model, DATA_POINT_UINT16_IDX, &data_size);
    CHECK((uintptr_t)ptr % alignof(uint16_t) == 0U);
    ptr = ezDataModel_GetDataPoint(&data_model, DATA_POINT_UINT32_IDX, &data_size);
    CHECK((uintptr_t)ptr % alignof(uint32_t) == 0U);
    ptr = ezDataModel_GetDataPoint(&data_model, DATA_POINT_FLOAT_IDX, &data_size);
    CHECK((uintptr_t)ptr % alignof(float) == 0U);
    ptr = ezDataModel_GetDataPoint(&data_model, DATA_POINT_DOUBLE_IDX, &data_size);
    CHECK((uintptr_t)ptr % alignof(double) == 0U);
}


TEST_CASE("Initialize rejects invalid configuration", "[service][datamodel]")
{
    static uint8_t local_buff[64];
    ezDataModel_t model;

    SECTION("NULL or empty arguments")
    {
        ezDataPoint_t points[] = {{0U, 4U, NULL, false}};
        CHECK_FALSE(ezDataModel_Initialize(NULL, points, 1U, local_buff, sizeof(local_buff)));
        CHECK_FALSE(ezDataModel_Initialize(&model, NULL, 1U, local_buff, sizeof(local_buff)));
        CHECK_FALSE(ezDataModel_Initialize(&model, points, 0U, local_buff, sizeof(local_buff)));
        CHECK_FALSE(ezDataModel_Initialize(&model, points, 1U, NULL, sizeof(local_buff)));
        CHECK_FALSE(ezDataModel_Initialize(&model, points, 1U, local_buff, 0U));
    }

    SECTION("Buffer too small, model stays unusable")
    {
        ezDataPoint_t points[] = {{0U, 32U, NULL, false}, {1U, 64U, NULL, false}};
        CHECK_FALSE(ezDataModel_Initialize(&model, points, 2U, local_buff, sizeof(local_buff)));

        uint8_t value = 1;
        CHECK(ezDataModel_SetDataPoint(&model, 0U, &value, 32U) != ezSUCCESS);
        CHECK(ezDataModel_GetFirstDirty(&model) == DATA_POINT_INVALID);
    }

    SECTION("Zero size")
    {
        ezDataPoint_t points[] = {{0U, 0U, NULL, false}};
        CHECK_FALSE(ezDataModel_Initialize(&model, points, 1U, local_buff, sizeof(local_buff)));
    }

    SECTION("Indexes not strictly increasing")
    {
        ezDataPoint_t unsorted[] = {{2U, 4U, NULL, false}, {1U, 4U, NULL, false}};
        CHECK_FALSE(ezDataModel_Initialize(&model, unsorted, 2U, local_buff, sizeof(local_buff)));

        ezDataPoint_t duplicated[] = {{1U, 4U, NULL, false}, {1U, 4U, NULL, false}};
        CHECK_FALSE(ezDataModel_Initialize(&model, duplicated, 2U, local_buff, sizeof(local_buff)));
    }

    SECTION("Reserved invalid index")
    {
        ezDataPoint_t points[] = {{DATA_POINT_INVALID, 4U, NULL, false}};
        CHECK_FALSE(ezDataModel_Initialize(&model, points, 1U, local_buff, sizeof(local_buff)));
    }
}


/******************************************************************************
* Internal functions
*******************************************************************************/
DatamodelTestsFixture::DatamodelTestsFixture()
{
    ezDataModel_Initialize(
        &data_model,
        data_points,
        sizeof(data_points)/sizeof(data_points[0]),
        buff,
        BUFFER_SIZE);
}


/* End of file */
