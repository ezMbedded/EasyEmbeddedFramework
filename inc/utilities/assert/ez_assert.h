/*****************************************************************************
* Filename:         ez_assert.h
* Author:           Hai Nguyen
* Original Date:    18.02.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_assert.h
 *  @author Hai Nguyen
 *  @date   18.02.2024
 *  @brief  Public API of the assert component
 *
 *  @details Provide implementation of the assert function. This
 *  implementation also shows function and line where the assert failure
 *  occurs
 * 
 */

#ifndef _EZ_ASSERT_H
#define _EZ_ASSERT_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* Includes
*******************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif


/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/

#if (EZ_ASSERT == 1U)
#include "logging/ez_logging.h"
#define ASSERT(expr) \
            if(!(expr)) {dbg_print("\nassert:%s:%d\n", __func__ , __LINE__);while(1);}

#define ASSERT_MSG(expr, msg)                                                           \
            do                                                                          \
            {                                                                           \
                if(!(expr))                                                             \
                {dbg_print("\nassert:%s:%d - %s\n", __func__ , __LINE__, msg);while(1);} \
            }while(0);                                                                  \

#define ASSERT_CUST_MSG(expr, fmt, ...) \
            do                                                                                      \
            {                                                                                       \
                if(!(expr))                                                                         \
                {printf("\nassert:%s:%d - " fmt "\n", __func__ , __LINE__, ##__VA_ARGS__);while(1);}\
            }while(0);                                                                              \

#else
#define ASSERT(expr)
#define ASSERT_MSG(expr, msg)
#define ASSERT_CUST_MSG(expr, msg, ...)
#endif /* HELPER_ASSERT */


/******************************************************************************
* Module Typedefs
*******************************************************************************/
/*None*/

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
/*None*/

/******************************************************************************
* Function Prototypes
*******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* _EZ_ASSERT_H */

/* End of file*/
