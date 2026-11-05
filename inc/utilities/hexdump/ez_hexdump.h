/*****************************************************************************
* Filename:         ez_hexdump.h
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

/** @file   ez_hexdump.h
 *  @author Hai Nguyen
 *  @date   18.02.2024
 *  @brief  Public functions of hexdump component
 *
 *  @details The API is compiled only if EZ_HEXDUMP is 1U.
 */
#ifndef _HEXDUMP_H
#define _HEXDUMP_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
* Includes
*******************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if(EZ_HEXDUMP == 1U)
#include "stdint.h"


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
/* None */


/******************************************************************************
* Function Prototypes
*******************************************************************************/

/*****************************************************************************
* Function : ezHexdump
*//**
* @brief Prints a memory region as a hex and ASCII dump
*
* @details Prints the region in rows of 16 bytes. Each row shows the address
* of its first byte, the bytes in hex and the same bytes as ASCII. Printable
* characters (0x20 to 0x7E) are printed as they are, all other bytes as ".".
* The last row is padded with spaces when size is not a multiple of 16.
* A dump starts with a newline and a column header line. If size is 0, only
* those two lines are printed. Output goes through dbg_print, which defaults
* to printf (see ez_logging.h).
*
* @param[in] address: Starting address of the memory region to print
* @param[in] size: Size of the memory region in bytes
*
* @pre address points to at least size readable bytes, or size is 0
* @post None
* @warning address is not checked for NULL. A NULL address with size greater
* than 0 is dereferenced (undefined behaviour).
* @warning Each row is printed with several dbg_print calls and no lock, so
* output from concurrent callers can interleave.
* @isr_safe{no}
*
* \b Example
* @code
* uint8_t payload[8] = {0x01, 0x02, 0x03, 0x41, 0x42, 0x43, 0x0A, 0x00};
* ezHexdump(payload, sizeof(payload));
* @endcode
*
*****************************************************************************/
void ezHexdump(void *address, uint32_t size);

#endif /* EZ_HEXDUMP */

#ifdef __cplusplus
}
#endif

#endif /* _HEXDUMP_H */

/* End of file*/
