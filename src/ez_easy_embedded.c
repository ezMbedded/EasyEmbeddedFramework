/*****************************************************************************
* Filename:         ez_easy_embedded.c
* Author:           Hai Nguyen
* Original Date:    09.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_easy_embedded.c
 *  @author Hai Nguyen
 *  @date   09.03.2024
 *  @brief  Implementation of the easy embedded component
 *
 *  @details
 */

/*****************************************************************************
* Includes
*****************************************************************************/
#include "ez_easy_embedded.h"

#define DEBUG_LVL   LVL_ERROR   /**< logging level */
#define MOD_NAME    "ez_easy_embedded"       /**< module name */
#include "ez_logging.h"


/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */


/*****************************************************************************
* Function Definitions
*****************************************************************************/
static void ezEasyEmbedded_PrintHeader(void);


/*****************************************************************************
* Public functions
*****************************************************************************/
void ezEasyEmbedded_Initialize(void)
{
    ezEasyEmbedded_PrintHeader();
}


/*****************************************************************************
* Local functions
*****************************************************************************/
/******************************************************************************
* Function : ezEasyEmbedded_PrintHeader
*//**
* \b Description:
*
* Show fancy header whenever the framework is started
*
* PRE-CONDITION: None
*
* POST-CONDITION: None
*
* @param    None
* @return   None
*
*******************************************************************************/
static void ezEasyEmbedded_PrintHeader(void)
{
    EZINFO("******************************************************************************");
    EZINFO("* EASY EMBEDDED FRAMEWORK");
    EZINFO("* Author: Quang Hai Nguyen");
    EZINFO("* Contact: hainguyen.eeit@gmail.com");
    EZINFO("******************************************************************************\n\n");
}

/* End of file*/
