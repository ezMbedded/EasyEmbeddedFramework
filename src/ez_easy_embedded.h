/*****************************************************************************
* Filename:         ez_easy_embedded.h
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

/** @file   ez_easy_embedded.h
 *  @author Hai Nguyen
 *  @date   09.03.2024
 *  @brief  Public API of the easy embedded component
 *
 *  @details This component is responsible for initialize the activated
 *           sub-components
 */

#ifndef _EZ_EASY_EMBEDDED_H
#define _EZ_EASY_EMBEDDED_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
/* None */


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
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function: ezEasyEmbedded_Initialize
*//** 
* @brief Initialize the sub components of the Easy Embedded framework
*
* @details Must be called once before using other components
*
* @param    None
* @return   None
*
* @pre None
* @post None
*
* \b Example
* @code
* ezEasyEmbedded_Initialize();
* @endcode
*
* @see
*
*****************************************************************************/
void ezEasyEmbedded_Initialize(void);

#ifdef __cplusplus
}
#endif

#endif /* _EZ_EASY_EMBEDDED_H */
/* End of file */
