/*****************************************************************************
* Filename:         ez_hexdump.c
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

/** @file   ez_hexdump.c
 *  @author Hai Nguyen
 *  @date   18.02.2024
 *  @brief  Implementation of the hexdump component
 *
 *  @details
 *
 */

/******************************************************************************
* Includes
*******************************************************************************/
#include "ez_hexdump.h"

#if(EZ_HEXDUMP == 1U)
#include "ez_logging.h"
#include "stdio.h"

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
* Function Definitions
*******************************************************************************/
static void PrintAscii(char c);


/*****************************************************************************
* External functions
*****************************************************************************/
void ezHexdump( void *address, uint16_t size)
{
    void * ulStartingAddress = address;

    dbg_print("\n\nAddress: %p - size: %d\n", address, size);
    dbg_print("00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
    while((uint8_t*)ulStartingAddress < (uint8_t*)address + size)
    {
        for(uint8_t i = 0; i < 16; i = i + 1)
        {
            if((uint8_t *)((uint8_t*)ulStartingAddress + i) - (uint8_t *) address < size)
            {
                dbg_print("%02x ", *((uint8_t*)ulStartingAddress + i));
            }
            else
            {
                dbg_print("%s","   ");
            }
        }

        dbg_print("%s", "| ");
        for(uint8_t i = 0; i < 16; i = i + 1)
        {
            if ((uint8_t*)((uint8_t*)ulStartingAddress + i) - (uint8_t*)address < size)
            {
                PrintAscii(*(char*)((uint8_t*)ulStartingAddress + i));
            }
        }

        dbg_print("\n");
        ulStartingAddress = (uint8_t*)ulStartingAddress + 16;
    }
    dbg_print("");
}


/*****************************************************************************
* Internal functions
*****************************************************************************/

/*****************************************************************************
* Function : PrintAscii
*//** 
* @brief Print readable ascii value
* @details Non-readable ascii will be printed as a dot "."
*
* @param    c: (IN)ascci will be printed
* @return   None
*
* @pre None
* @post None
*
* @code
* char a = 'a';
* PrintAscii(a);
* @endcode
*
*****************************************************************************/
static void PrintAscii(char c)
{
    /* Only print readable ascii character */
    if (c >= 33U && c <= 126U)
    {
        dbg_print("%c", c);
    }
    else
    {
        dbg_print("%s", ".");
    }
}

#endif /* (EZ_HEXDUMP == 1U) */
/* End of file*/