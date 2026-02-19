
/*******************************************************************************
* Filename:         system_error_table.h
* Author:           Hai Nguyen
* Original Date:    06.04.2022 - 22:48:51
* Last Update:      06.04.2022 - 22:48:51
*
* -----------------------------------------------------------------------------
* Comany:           Easy Embedded
*                   Address Line 1
*                   Address Line 2
*
* -----------------------------------------------------------------------------
* Contact:          Easy Embedded
*                   hainguyen.ezm@gmail.com
*
* -----------------------------------------------------------------------------
* Copyright (C) 2022 by Easy Embedded
*  
*******************************************************************************/

/** @file   system_error_table.h
 *  @author Hai Nguyen
 *  @date   06.04.2022 - 22:48:51
 *  @brief  this file contains the system errors. All error must be managed 
 *          centralize here
 */

#ifndef _SYSTEM_ERROR_TABLE_H
#define _SYSTEM_ERROR_TABLE_H

#include "ez_system_error.h"

#if (EZ_SYS_ERROR == 1U)

 /*******************************************************************************
* Includes
*******************************************************************************/


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

ErrorObject error_table[] =
{
    { 0x00, WARNING, SystemError_HandleTestWarningError },
    /**< Default system error, testing purpose*/
    { 0x01, ATTENTION, SystemError_HandleTestAttentionError },
    /**< Default system error, testing purpose*/
    { 0x02, CRITICAL, SystemError_HandleTestCriticalError },
    /**< Default system error, testing purpose*/
    { 0x03, CASTATROPHIC, SystemError_HandleTestCastatrophicError },
    /**< Default system error, testing purpose*/
};

/******************************************************************************
* Function Prototypes
*******************************************************************************/
/* None */

#endif /* EZ_SYS_ERROR == 1U */

#endif /* _SYSTEM_ERROR_TABLEH */

/* End of file */

