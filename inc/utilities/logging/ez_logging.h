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

/* Configuration (defined by the build system or ez_target_config.h):
 * EZ_LOGGING       1U enables logging, otherwise all EZxxx macros expand to nothing.
 * EZ_LOGGING_COLOR 1U enables ANSI colors, otherwise the color macros are empty.
 * DEBUG_LVL        Per-file level, must be defined BEFORE including this header.
 * MOD_NAME         Per-file module name string, must be defined BEFORE including
 *                  this header (used when PRINT_MODULE_NAME is 1U).
 */

#define PRINT_MODULE_NAME       1U  /**< logging shows module name */
#define PRINT_FILENAME          0U  /**< logging shows file name */
#define PRINT_LINE              0U  /**< logging shows line number */
#define PRINT_FUNCTION_NAME     0U  /**< logging shows function name */

/** @brief Output function used by all logging macros. Defaults to printf on
 *  stdout. Define it before including this header to redirect the output.
 */
#ifndef dbg_print
#define dbg_print(fmt, ...)     printf(fmt, ##__VA_ARGS__)
#endif

#if (EZ_LOGGING_COLOR == 1U)
#define EZ_BLACK       "\033[0;30m"    /**< ANSI black, empty if EZ_LOGGING_COLOR is not 1U */
#define EZ_RED         "\033[0;31m"    /**< ANSI red, empty if EZ_LOGGING_COLOR is not 1U */
#define EZ_GREEN       "\033[0;32m"    /**< ANSI green, empty if EZ_LOGGING_COLOR is not 1U */
#define EZ_YELLOW      "\033[0;33m"    /**< ANSI yellow, empty if EZ_LOGGING_COLOR is not 1U */
#define EZ_BLUE        "\033[0;34m"    /**< ANSI blue, empty if EZ_LOGGING_COLOR is not 1U */
#define EZ_PURPLE      "\033[0;35m"    /**< ANSI purple, empty if EZ_LOGGING_COLOR is not 1U */
#define EZ_CYAN        "\033[0;36m"    /**< ANSI cyan, empty if EZ_LOGGING_COLOR is not 1U */
#define EZ_WHITE       "\033[0;37m"    /**< ANSI white, empty if EZ_LOGGING_COLOR is not 1U */
#define EZ_RESET_COLOR "\033[0m"       /**< ANSI reset, empty if EZ_LOGGING_COLOR is not 1U */
#else
#define EZ_BLACK        /**< no color: expands to nothing */
#define EZ_RED          /**< no color: expands to nothing */
#define EZ_GREEN        /**< no color: expands to nothing */
#define EZ_YELLOW       /**< no color: expands to nothing */
#define EZ_BLUE         /**< no color: expands to nothing */
#define EZ_PURPLE       /**< no color: expands to nothing */
#define EZ_CYAN         /**< no color: expands to nothing */
#define EZ_WHITE        /**< no color: expands to nothing */
#define EZ_RESET_COLOR  /**< no color: expands to nothing */
#endif

#if (PRINT_MODULE_NAME == 1U)
/** @brief Prints the module name as "[name] " if PRINT_MODULE_NAME is 1U, else nothing. */
#define print_module(mod_name)      dbg_print("::%s::", mod_name)
#else
#define print_module(mod_name)  /**< disabled: expands to nothing */
#endif

#if (PRINT_FILENAME == 1U)
/** @brief Prints the file name if PRINT_FILENAME is 1U, else nothing. */
#define print_file()     dbg_print("%s ", __FILE__)
#else
#define print_file()  /**< disabled: expands to nothing */
#endif

#if (PRINT_LINE == 1U)
/** @brief Prints the line number if PRINT_LINE is 1U, else nothing. */
#define print_line()     dbg_print("(%d) ", __LINE__)
#else
#define print_line()  /**< disabled: expands to nothing */
#endif

#if (PRINT_FUNCTION_NAME == 1U)
/** @brief Prints the function name if PRINT_FUNCTION_NAME is 1U, else nothing. */
#define print_func()     dbg_print("%s ", __func__)
#else
#define print_func()  /**< disabled: expands to nothing */
#endif

#if (EZ_LOGGING == 1U)
#if DEBUG_LVL >= LVL_CRITICAL
#define EZCRITICAL(fmt, ...) do {\
                                dbg_print("\n" EZ_PURPLE "C" EZ_RESET_COLOR);\
                                print_module(MOD_NAME);\
                                print_file();\
                                print_line();\
                                print_func();\
                                dbg_print(fmt, ##__VA_ARGS__);\
                            }while(0)
#else
/** @brief Logs a message at level critical
 *
 * @details Compiled in only if EZ_LOGGING is 1U and DEBUG_LVL >= LVL_CRITICAL;
 * otherwise it expands to nothing and the arguments are not evaluated.
 * DEBUG_LVL and MOD_NAME must be defined before including this header.
 *
 * @param fmt: printf-style format string
 * @param ... Optional printf-style arguments
 *
 * \b Example
 * @code
 * EZCRITICAL("fatal: code %d", code);
 * @endcode
 */
#define EZCRITICAL(fmt, ...)
#endif

#if DEBUG_LVL >= LVL_ERROR
#define EZERROR(fmt, ...)   do {\
                                dbg_print("\n" EZ_RED "E" EZ_RESET_COLOR);\
                                print_module(MOD_NAME);\
                                print_file();\
                                print_line();\
                                print_func();\
                                dbg_print(fmt, ##__VA_ARGS__);\
                            }while(0)
#else
/** @brief Logs a message at level error
 *
 * @details Compiled in only if EZ_LOGGING is 1U and DEBUG_LVL >= LVL_ERROR;
 * otherwise it expands to nothing and the arguments are not evaluated.
 * DEBUG_LVL and MOD_NAME must be defined before including this header.
 *
 * @param fmt: printf-style format string
 * @param ... Optional printf-style arguments
 *
 * \b Example
 * @code
 * EZERROR("init failed: %d", status);
 * @endcode
 */
#define EZERROR(fmt, ...)
#endif

#if DEBUG_LVL >= LVL_WARNING
#define EZWARNING(fmt, ...) do {\
                                if (DEBUG_LVL >= LVL_WARNING)\
                                {\
                                    dbg_print("\n" EZ_YELLOW "W" EZ_RESET_COLOR);\
                                    print_module(MOD_NAME);\
                                    print_file();\
                                    print_line();\
                                    print_func();\
                                    dbg_print(fmt, ##__VA_ARGS__);\
                                }\
                            }while(0)
#else
/** @brief Logs a message at level warning
 *
 * @details Compiled in only if EZ_LOGGING is 1U and DEBUG_LVL >= LVL_WARNING;
 * otherwise it expands to nothing and the arguments are not evaluated.
 * DEBUG_LVL and MOD_NAME must be defined before including this header.
 *
 * @param fmt: printf-style format string
 * @param ... Optional printf-style arguments
 *
 * \b Example
 * @code
 * EZWARNING("retry %d", count);
 * @endcode
 */
#define EZWARNING(fmt, ...)
#endif

#if DEBUG_LVL >= LVL_INFO
#define EZINFO(fmt, ...)    do {\
                                dbg_print("\n" EZ_CYAN "I" EZ_RESET_COLOR);\
                                print_module(MOD_NAME);\
                                print_file();\
                                print_line();\
                                print_func();\
                                dbg_print(fmt, ##__VA_ARGS__);\
                            }while(0)
#else
/** @brief Logs a message at level info
 *
 * @details Compiled in only if EZ_LOGGING is 1U and DEBUG_LVL >= LVL_INFO;
 * otherwise it expands to nothing and the arguments are not evaluated.
 * DEBUG_LVL and MOD_NAME must be defined before including this header.
 *
 * @param fmt: printf-style format string
 * @param ... Optional printf-style arguments
 *
 * \b Example
 * @code
 * EZINFO("Task 1 - count = %d", count);
 * @endcode
 */
#define EZINFO(fmt, ...)
#endif

#if DEBUG_LVL >= LVL_DEBUG
#define EZDEBUG(fmt, ...)   do {\
                                dbg_print("\n" EZ_GREEN "D" EZ_RESET_COLOR);\
                                print_module(MOD_NAME);\
                                print_file();\
                                print_line();\
                                print_func();\
                                dbg_print(fmt, ##__VA_ARGS__);\
                            }while(0)
#else
/** @brief Logs a message at level debug
 *
 * @details Compiled in only if EZ_LOGGING is 1U and DEBUG_LVL >= LVL_DEBUG;
 * otherwise it expands to nothing and the arguments are not evaluated.
 * DEBUG_LVL and MOD_NAME must be defined before including this header.
 *
 * @param fmt: printf-style format string
 * @param ... Optional printf-style arguments
 *
 * \b Example
 * @code
 * EZDEBUG("value = %d", value);
 * @endcode
 */
#define EZDEBUG(fmt, ...)
#endif

#if DEBUG_LVL >= LVL_TRACE
#define EZTRACE(fmt, ...)   do {\
                                dbg_print("\n");\
                                dbg_print(EZ_BLUE "T" EZ_RESET_COLOR);\
                                print_module(MOD_NAME);\
                                print_file();\
                                print_line();\
                                print_func();\
                                dbg_print(fmt, ##__VA_ARGS__);\
                            }while(0)
#else
/** @brief Logs a message at level trace
 *
 * @details Compiled in only if EZ_LOGGING is 1U and DEBUG_LVL >= LVL_TRACE;
 * otherwise it expands to nothing and the arguments are not evaluated.
 * DEBUG_LVL and MOD_NAME must be defined before including this header.
 *
 * @param fmt: printf-style format string
 * @param ... Optional printf-style arguments
 *
 * \b Example
 * @code
 * EZTRACE("enter");
 * @endcode
 */
#define EZTRACE(fmt, ...)
#endif

#if DEBUG_LVL >= LVL_DEBUG
#define EZHEXDUMP(x,y)  ezHexdump(x,y)
#else
/** @brief Prints a hex dump of a memory region using ezHexdump
 *
 * @details Compiled in only if EZ_LOGGING is 1U and DEBUG_LVL >= LVL_DEBUG;
 * otherwise it expands to nothing. Unlike the EZxxx log macros it does not
 * print a level or module prefix.
 *
 * @param x: Start address of the memory region (void pointer, see ezHexdump)
 * @param y: Size of the region in bytes
 * @see ezHexdump
 *
 * \b Example
 * @code
 * EZHEXDUMP(&buff[0], payload_size);
 * @endcode
 */
#define EZHEXDUMP(x, y)
#endif

#else
#define EZCRITICAL(fmt, ...)  /**< Logs a message at level critical; no-op if EZ_LOGGING != 1U or DEBUG_LVL < LVL_CRITICAL */
#define EZERROR(fmt, ...)  /**< Logs a message at level error; no-op if EZ_LOGGING != 1U or DEBUG_LVL < LVL_ERROR */
#define EZWARNING(fmt, ...)  /**< Logs a message at level warning; no-op if EZ_LOGGING != 1U or DEBUG_LVL < LVL_WARNING */
#define EZINFO(fmt, ...)  /**< Logs a message at level info; no-op if EZ_LOGGING != 1U or DEBUG_LVL < LVL_INFO */
#define EZTRACE(fmt, ...)  /**< Logs a message at level trace; no-op if EZ_LOGGING != 1U or DEBUG_LVL < LVL_TRACE */
#define EZDEBUG(fmt, ...)  /**< Logs a message at level debug; no-op if EZ_LOGGING != 1U or DEBUG_LVL < LVL_DEBUG */
#define EZHEXDUMP(x, y)  /**< Hex dump of memory region x (address) of y bytes via ezHexdump; no-op if EZ_LOGGING != 1U or DEBUG_LVL < LVL_DEBUG */
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
* @details Prints one test message per level (trace, debug, info, warning,
* error, critical) and a hex dump, using the EZxxx macros. Its source file
* defines DEBUG_LVL as LVL_ERROR, so only the error and critical messages are
* printed. The body exists only if EZ_LOGGING is 1U.
*
* @pre None
* @post None
* @see EZINFO
* @see EZHEXDUMP
*
*****************************************************************************/
void Logging_DemoFeatures(void);

#ifdef __cplusplus
}
#endif

#endif /* _EZ_LOGGING_H */

/* End of file */

