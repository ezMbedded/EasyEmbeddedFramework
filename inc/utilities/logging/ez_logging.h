
/*****************************************************************************
* Filename:         ez_logging.h
* Author:           Hai Nguyen
* Original Date:    26.02.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_logging.h
 *  @author Hai Nguyen
 *  @date   26.02.2024
 *  @brief  This is the header file for logging module
 *
 *  @details Logging module provides user with multi-level and multi-format
 *  logging. Originally, logging will use printf to print file on the console,
 *  but  it can be tailored to other IO
 */


#ifndef _EZ_LOGGING_H
#define _EZ_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* Includes
*******************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#include <stdio.h>
#include "ez_hexdump.h"

/******************************************************************************
* Module Preprocessor Macros
*******************************************************************************/
#define LVL_NO_LOG      0   /**< set no logging */
#define LVL_CRITICAL    1   /**< set logging to level critical */
#define LVL_ERROR       2   /**< set logging to level error */
#define LVL_WARNING     3   /**< set logging to level warning */
#define LVL_INFO        4   /**< set logging to level info */
#define LVL_DEBUG       5   /**< set logging to level debug */
#define LVL_TRACE       6   /**< set logging to level trace */

#define PRINT_MODULE_NAME       1U  /**< logging shows module name */
#define PRINT_FILENAME          0U  /**< logging shows file name */
#define PRINT_LINE              0U  /**< logging shows line number */
#define PRINT_FUNCTION_NAME     0U  /**< logging shows function name */

/* Print data on stdout by default */
#ifndef dbg_print
#define dbg_print(fmt, ...)     printf(fmt, ##__VA_ARGS__)
#endif

#if (EZ_LOGGING_COLOR == 1U)
#define EZ_BLACK       "\033[0;30m"
#define EZ_RED         "\033[0;31m"
#define EZ_GREEN       "\033[0;32m"
#define EZ_YELLOW      "\033[0;33m"
#define EZ_BLUE        "\033[0;34m"
#define EZ_PURPLE      "\033[0;35m"
#define EZ_CYAN        "\033[0;36m"
#define EZ_WHITE       "\033[0;37m"
#define EZ_RESET_COLOR "\033[0m"
#else
#define EZ_BLACK       
#define EZ_RED         
#define EZ_GREEN       
#define EZ_YELLOW      
#define EZ_BLUE        
#define EZ_PURPLE      
#define EZ_CYAN        
#define EZ_WHITE       
#define EZ_RESET_COLOR 
#endif

#if (PRINT_MODULE_NAME == 1U)
#define print_module(mod_name)      dbg_print("[%s] ", mod_name)
#else
#define print_module(mod_name)
#endif

#if (PRINT_FILENAME == 1U)
#define print_file()     dbg_print("%s ", __FILE__)
#else
#define print_file()
#endif

#if (PRINT_LINE == 1U)
#define print_line()     dbg_print("(%d) ", __LINE__)
#else
#define print_line()
#endif

#if (PRINT_FUNCTION_NAME == 1U)
#define print_func()     dbg_print("%s ", __func__)
#else
#define print_func()
#endif

#if (EZ_LOGGING == 1U)
#if DEBUG_LVL >= LVL_CRITICAL
#define EZCRITICAL(fmt, ...) do {\
                                dbg_print("\n" EZ_PURPLE "[CRITICAL] " EZ_RESET_COLOR);\
                                print_module(MOD_NAME);\
                                print_file();\
                                print_line();\
                                print_func();\
                                dbg_print(fmt, ##__VA_ARGS__);\
                            }while(0)
#else
#define EZCRITICAL(fmt, ...)
#endif

#if DEBUG_LVL >= LVL_ERROR
#define EZERROR(fmt, ...)   do {\
                                dbg_print("\n" EZ_RED "[   ERROR] " EZ_RESET_COLOR);\
                                print_module(MOD_NAME);\
                                print_file();\
                                print_line();\
                                print_func();\
                                dbg_print(fmt, ##__VA_ARGS__);\
                            }while(0)
#else
#define EZERROR(fmt, ...)
#endif

#if DEBUG_LVL >= LVL_WARNING
#define EZWARNING(fmt, ...) do {\
                                if (DEBUG_LVL >= LVL_WARNING)\
                                {\
                                    dbg_print("\n" EZ_YELLOW "[ WARNING] " EZ_RESET_COLOR);\
                                    print_module(MOD_NAME);\
                                    print_file();\
                                    print_line();\
                                    print_func();\
                                    dbg_print(fmt, ##__VA_ARGS__);\
                                }\
                            }while(0)
#else
#define EZWARNING(fmt, ...)
#endif

#if DEBUG_LVL >= LVL_INFO
#define EZINFO(fmt, ...)    do {\
                                dbg_print("\n" EZ_CYAN "[    INFO] " EZ_RESET_COLOR);\
                                print_module(MOD_NAME);\
                                print_file();\
                                print_line();\
                                print_func();\
                                dbg_print(fmt, ##__VA_ARGS__);\
                            }while(0)
#else
#define EZINFO(fmt, ...)
#endif

#if DEBUG_LVL >= LVL_DEBUG
#define EZDEBUG(fmt, ...)   do {\
                                dbg_print("\n" EZ_GREEN "[   DEBUG] " EZ_RESET_COLOR);\
                                print_module(MOD_NAME);\
                                print_file();\
                                print_line();\
                                print_func();\
                                dbg_print(fmt, ##__VA_ARGS__);\
                            }while(0)
#else
#define EZDEBUG(fmt, ...)
#endif

#if DEBUG_LVL >= LVL_TRACE
#define EZTRACE(fmt, ...)   do {\
                                dbg_print("\n");\
                                dbg_print(EZ_BLUE "[   TRACE] " EZ_RESET_COLOR);\
                                print_module(MOD_NAME);\
                                print_file();\
                                print_line();\
                                print_func();\
                                dbg_print(fmt, ##__VA_ARGS__);\
                            }while(0)
#else
#define EZTRACE(fmt, ...)
#endif

#if DEBUG_LVL >= LVL_DEBUG
#define EZHEXDUMP(x,y)  ezHexdump(x,y)
#else
#define EZHEXDUMP(x, y)
#endif

#else
#define EZCRITICAL(fmt, ...)
#define EZERROR(fmt, ...)
#define EZWARNING(fmt, ...)
#define EZINFO(fmt, ...)
#define EZTRACE(fmt, ...)
#define EZDEBUG(fmt, ...)
#define EZHEXDUMP(x, y)
#endif

/******************************************************************************
* Module Typedefs
*******************************************************************************/
/* None */

/******************************************************************************
* Module Variable Definitions
*******************************************************************************/
/* None */

/******************************************************************************
* Function Prototypes
*******************************************************************************/

/*****************************************************************************
* Function : Logging_DemoFeatures
*//** 
* @brief This function demostrates some features of logging module
*
* @details
*
* @param    None
* @return   None
*
* @pre None
* @post None
*
*****************************************************************************/
void Logging_DemoFeatures(void);

#ifdef __cplusplus
}
#endif

#endif /* _EZ_LOGGING_H */

/* End of file */

