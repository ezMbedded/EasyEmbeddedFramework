/*****************************************************************************
* Filename:         ez_uart.h
* Author:           Hai Nguyen
* Original Date:    15.03.2024
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_uart.h
 *  @author Hai Nguyen
 *  @date   15.03.2024
 *  @brief  Public API of the UART component
 *
 *  @details This component provides the interfaces to interact with HW uart.
 *           It also provides the interface, which every HW UART implementation
 *           must follow.
 */

#ifndef _EZ_UART_H
#define _EZ_UART_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_UART == 1)
#include <stdint.h>
#include <stdbool.h>

#include "ez_driver_def.h"


/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
/* None */


/*****************************************************************************
* Component Typedefs
*****************************************************************************/
/** @brief UART event definitions
 */
typedef enum
{
    UART_EVENT_TX_CMPLT,    /**< Transmit operation is completed */
    UART_EVENT_TX_ERR,      /**< Transmit operation failed */
    UART_EVENT_RX_CMPLT,    /**< Receive operation is completed */
    UART_EVENT_RX_ERR,      /**< Receive operation failed */
    UART_EVENT_TIMEOUT,     /**< Operation is timeout */
    UART_NUM_EVENTS,        /**< Number of events */
}EZ_UART_EVENT;


/** @brief Parity value
 */
typedef enum
{
    PARITY_NONE,    /**< None */
    PARITY_ODD,     /**< Odd */
    PARITY_EVEN,    /**< Even */
    PARITY_MARK,    /**< Mark */
    PARITY_SPACE,   /**< Space */
}EZ_UART_PARITY_ENUM;


/** @brief Number of stop bit
 */
typedef enum
{
    ONE_BIT,            /**< 1 stop bit */
    ONE_AND_HALF_BIT,   /**< 1.5 stop bit */
    TWO_BITS,           /**< 2 stop bits */
}EZ_UART_NUM_STOP_BIT_ENUM;


/** @brief Number of stop bit
 */
struct ezUartConfiguration
{
    const char                  *port_name; /**< Name of the serial port */
    uint32_t                    baudrate;   /**< Baudrate*/
    EZ_UART_PARITY_ENUM         parity;     /**< Parity */
    EZ_UART_NUM_STOP_BIT_ENUM   stop_bit;   /**< Number of stop bit */
    uint8_t                     byte_size;  /**< Size of data in bit */
};


/** @brief Define Uart Driver Instance
 */
typedef struct ezDrvInstance ezUartDrvInstance_t;


/****************************************************************************/
/* List of API implemented by the HW driver. Users are not supposed to use
 * these API.
 */

/** @brief Initialize the Hw Uart
 *
 *  @param[in]  index: index of the HW Uart instance
 *  @return     STATUS_OK if success, otherwise STATUS_XXX. @see EZ_DRV_STATUS
 */
typedef EZ_DRV_STATUS(*ezHwUart_Initialize)(uint8_t index);


/** @brief Deinitialize the Hw Uart
 *
 *  @param[in]  index: index of the HW Uart instance
 *  @return     STATUS_OK if success, otherwise STATUS_XXX. @see EZ_DRV_STATUS
 */
typedef EZ_DRV_STATUS(*ezHwUart_Deinitialize)(uint8_t index);


/** @brief Transmit data asynchornously
 *
 *  @param[in]  index: index of the HW Uart instance
 *  @param[in]  tx_buff: point to transmited buffer
 *  @param[in]  buff_size: size of the buffer
 *  @return     STATUS_OK if success, otherwise STATUS_XXX. @see EZ_DRV_STATUS
 */
typedef EZ_DRV_STATUS(*ezHwUart_AsyncTransmit)(uint8_t index, const uint8_t *tx_buff, uint16_t buff_size);


/** @brief Receive data asynchornously
 *
 *  @param[in]  index: index of the HW Uart instance
 *  @param[in]  rx_buff: point to received buffer
 *  @param[in]  buff_size: size of the buffer
 *  @return     STATUS_OK if success, otherwise STATUS_XXX. @see EZ_DRV_STATUS
 */
typedef EZ_DRV_STATUS(*ezHwUart_AsyncReceive)(uint8_t index, uint8_t *rx_buff, uint16_t buff_size);


/** @brief Transmit data synchronously
 *
 *  @param[in]  index: index of the HW Uart instance
 *  @param[in]  tx_buff: point to transmited buffer
 *  @param[in]  buff_size: size of the buffer
 *  @param[in]  timeout_millis: timeout in millisecond
 *  @return     STATUS_OK if success, otherwise STATUS_XXX. @see EZ_DRV_STATUS
 */
typedef EZ_DRV_STATUS(*ezHwUart_SyncTransmit)(uint8_t index, const uint8_t *tx_buff, uint16_t buff_size, uint32_t timeout_millis);


/** @brief Receive data synchronously
 *
 *  @param[in]  index: index of the HW Uart instance
 *  @param[in]  rx_buff: point to received buffer
 *  @param[in]  buff_size: size of the buffer
 *  @param[in]  timeout_millis: timeout in millisecond
 *  @return     STATUS_OK if success, otherwise STATUS_XXX. @see EZ_DRV_STATUS
 */
typedef EZ_DRV_STATUS(*ezHwUart_SyncReceive)(uint8_t index, uint8_t *rx_buff, uint16_t buff_size, uint32_t timeout_millis);


/** @brief Update the configuration
 *
 *  @param[in]  index: index of the HW Uart instance
 *  @return     STATUS_OK if success, otherwise STATUS_ERR_XXX. @see EZ_DRV_STATUS
 */
typedef EZ_DRV_STATUS(*ezHwUart_UpdateConfig)(uint8_t index);


/** @brief List of API must be supported by the Hw implementation
 */
struct ezHwUartInterface
{
    uint8_t                 index;          /**< Index of the driver instance */
    ezHwUart_Initialize     initialize;     /**< Initialize function */
    ezHwUart_Deinitialize   deinitialize;   /**< Deinitialize function */
    ezHwUart_AsyncTransmit  async_transmit; /**< Transmit data asynchorously. Events are reported via callback in ezDriverCommon -> ezDrvInstance_t -> calback*/
    ezHwUart_AsyncReceive   async_receive;  /**< Receive data asynchorously. Events are reported via callback in ezDriverCommon -> ezDrvInstance_t -> calback*/
    ezHwUart_SyncTransmit   sync_transmit;  /**< Transmit data synchorously */
    ezHwUart_SyncReceive    sync_receive;   /**< Receive data synchorously */
    ezHwUart_UpdateConfig   update_conf;    /**< Update configuration function */
    /* Add more API if required */
};


/** @brief Number of stop bit
 */
struct ezUartDriver
{
    struct Node                 ll_node;    /* linked list node to link to list of hw driver implmentation */
    struct ezDriverCommon       common;     /* Common data of driver */
    struct ezUartConfiguration  config;     /* Uart related configuration */
    struct ezHwUartInterface    interface;  /* HW API */
};


/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */


/*****************************************************************************
* Function Prototypes
*****************************************************************************/

/*****************************************************************************
* Function: ezUart_SystemRegisterHwDriver
*//** 
* @brief Register a HW uart implementation to the HAL
*
* @details The implementation must follow the interfaces defined by ezUartDriver
*
* @param[in]    hw_uart_driver: (IN)pointer to the HW implementation
* @return       STATUS_OK: Success
*               STATUS_ERR_ARG: hw_uart_driver is NULL
*
* @pre None
* @post None
*
* \b Example
* @code
* EZ_DRV_STATUS status = ezUart_SystemRegisterHwDriver(&stm32_uart_driver);
* @endcode
*
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_SystemRegisterHwDriver(struct ezUartDriver *hw_uart_driver);


/*****************************************************************************
* Function: ezUart_SystemUnregisterHwDriver
*//** 
* @brief Unregister a HW driver implementation
*
* @details
*
* @param[in]    hw_uart_driver: (IN)pointer to the HW implementation
* @return       STATUS_OK: Success
*               STATUS_ERR_ARG: hw_uart_driver is NULL
*
* @pre None
* @post None
*
* \b Example
* @code
* EZ_DRV_STATUS status = ezUart_SystemUnregisterHwDriver(&stm32_uart_driver);
* @endcode
*
* @see sum
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_SystemUnregisterHwDriver(struct ezUartDriver *hw_uart_driver);


/*****************************************************************************
* Function: ezUart_RegisterInstance
*//** 
* @brief Register a driver instance to use the driver
*
* @details
*
* @param[in]    inst: Instance to be register. @see ezUartDrvInstance_t
* @param[in]    driver_name: Name of the driver we want to use.
* @param[in]    callback: callback function to receive event from the HW driver
* @return       STATUS_OK: Success
*               STATUS_XXX: Error code defined in EZ_DRV_STATUS
*
* @pre HW driver must be registered by using ezUart_SystemRegisterHwDriver
* @post None
*
* \b Example
* @code
* ezUartDrvInstance_t inst;
* EZ_DRV_STATUS status = ezUart_RegisterInstance(&inst, "stm32 uart", callback);
* @endcode
*
* @see ezUart_SystemRegisterHwDriver
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_RegisterInstance(ezUartDrvInstance_t *inst,
                                      const char *driver_name,
                                      ezDrvCallback callback);


/*****************************************************************************
* Function: ezUart_UnregisterInstance
*//** 
* @brief Unregister a driver instance
*
* @details
*
* @param[in]    inst: Driver instance
* @return       STATUS_OK: Success
*               STATUS_XXX: Error code defined in EZ_DRV_STATUS
*
* @pre None
* @post None
*
* \b Example
* @code
* EZ_DRV_STATUS status = ezUart_RegisterInstance(&inst);
* @endcode
*
* @see ezUart_RegisterInstance
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_UnregisterInstance(ezUartDrvInstance_t *inst);


/*****************************************************************************
* Function: ezUart_Initialize
*//** 
* @brief Initialize the UART driver
*
* @details Normall, user do not need to call this function. The HW uart
*          implementation take care of the initialization.
*
* @param[in]    iinst: Driver instance
* @return       STATUS_OK: Success
*               STATUS_XXX: Error code defined in EZ_DRV_STATUS
*
* @pre inst must be register using ezUart_RegisterInstance
* @post None
*
* \b Example
* @code
* EZ_DRV_STATUS status = ezUart_Initialize(&inst);
* @endcode
*
* @see ezUart_RegisterInstance
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_Initialize(ezUartDrvInstance_t *inst);


/*****************************************************************************
* Function: ezUart_Deinitialize
*//** 
* @brief Deintialize the UART driver
*
* @details
*
* @param[in]    inst: Driver instance
* @return       STATUS_OK: Success
*               STATUS_XXX: Error code defined in EZ_DRV_STATUS
*
* @pre inst must be register using ezUart_RegisterInstance
* @post None
*
* \b Example
* @code
* EZ_DRV_STATUS status = ezUart_Deinitialize(&inst);
* @endcode
*
* @see ezUart_RegisterInstance
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_Deinitialize(ezUartDrvInstance_t *inst);


/*****************************************************************************
* Function: ezUart_AsyncTransmit
*//** 
* @brief Transmit data asynchronously.
*
* @details The events or error code is reported via callback defined by
*          ezUart_RegisterInstance. Since it is an asynchrous function, the user
*          must ensure that the buffer for transmiting data must not be used by
*          other processes.
*
* @param[in]    inst: Driver instance
* @param[in]    tx_buff: pointer to transmit buffer
* @param[in]    buff_size: size of the buffer
* @return       STATUS_OK: Success
*               STATUS_XXX: Error code defined in EZ_DRV_STATUS
*
* @pre inst must be register using ezUart_RegisterInstance
* @post None
*
* \b Example
* @code
* EZ_DRV_STATUS status;
* uint8_t buff[3] = {1, 2, 3};
* status = ezUart_AsyncTransmit(&inst, buff, 3);
* @endcode
*
* @see ezUart_RegisterInstance
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_AsyncTransmit(ezUartDrvInstance_t *inst,
                                   const uint8_t *tx_buff,
                                   uint16_t buff_size);


/*****************************************************************************
* Function: ezUart_AsyncReceive
*//** 
* @brief Receive data asynchronously.
*
* @details The events or error code is reported via callback defined by
*          ezUart_RegisterInstance. Since it is an asynchrous function, the users
*          must ensure that the buffer for receiving data must not be used by
*          other processes.
*
* @param[in]    inst: Driver instance
* @param[in]    rx_buff: pointer to receive buffer
* @param[in]    buff_size: size of the buffer
* @return       STATUS_OK: Success
*               STATUS_XXX: Error code defined in EZ_DRV_STATUS
*
* @pre inst must be register using ezUart_RegisterInstance
* @post None
*
* \b Example
* @code
* EZ_DRV_STATUS status;
* uint8_t buff[3];
* status = ezUart_AsyncReceive(&inst, buff, 3);
* @endcode
*
* @see ezUart_RegisterInstance
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_AsyncReceive(ezUartDrvInstance_t *inst,
                                  uint8_t *rx_buff,
                                  uint16_t buff_size);


/*****************************************************************************
* Function: ezUart_SyncTransmit
*//** 
* @brief Transmit data synchronously.
*
* @details Since it is a synchrous function, the process is blocked for an
*          interval of millisecond defined by timeout_millis argument
*
* @param[in]    inst: Driver instance
* @param[in]    tx_buff: pointer to transmit buffer
* @param[in]    buff_size: size of the buffer
* @param[in]    timeout_millis: timeout in millisecond
* @return       STATUS_OK: Success
*               STATUS_XXX: Error code defined in EZ_DRV_STATUS
*
* @pre inst must be register using ezUart_RegisterInstance
* @post None
*
* \b Example
* @code
* EZ_DRV_STATUS status;
* uint8_t buff[3] = {1, 2, 3};
* status = ezUart_SyncTransmit(&inst, buff, 3, 1000);
* @endcode
*
* @see ezUart_RegisterInstance
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_SyncTransmit(ezUartDrvInstance_t *inst, 
                                  const uint8_t *tx_buff,
                                  uint16_t buff_size,
                                  uint32_t timeout_millis);


/*****************************************************************************
* Function: ezUart_SyncReceive
*//** 
* @brief Receive data synchronously.
*
* @details Since it is a synchrous function, the process is blocked for an
*          interval of millisecond defined by timeout_millis argument
*
* @param[in]    inst: Driver instance
* @param[in]    rx_buff: pointer to receive buffer
* @param[in]    buff_size: size of the buffer
* @param[in]    timeout_millis: timeout in millisecond
* @return       STATUS_OK: Success
*               STATUS_XXX: Error code defined in EZ_DRV_STATUS
*
* @pre inst must be register using ezUart_RegisterInstance
* @post None
*
* \b Example
* @code
* EZ_DRV_STATUS status;
* uint8_t buff[3] = {1, 2, 3};
* status = ezUart_SyncReceive(&inst, buff, 3, 1000);
* @endcode
*
* @see ezUart_RegisterInstance
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_SyncReceive(ezUartDrvInstance_t *inst,
                                 uint8_t *rx_buff,
                                 uint16_t buff_size,
                                 uint32_t timeout_millis);


/*****************************************************************************
* Function: ezUart_GetConfig
*//** 
* @brief Get the configuration of the Hw Uart implementation
*
* @details
*
* @param[in]    inst: Driver instance
* @param[in]    config: pointer to the configutation
* @return       STATUS_OK: Success
*               STATUS_XXX: Error code defined in EZ_DRV_STATUS
*
* @pre inst must be register using ezUart_RegisterInstance
* @post None
*
* \b Example
* @code
* EZ_DRV_STATUS status;
* ezUartConfiguration *config = NULL;
* status = ezUart_GetConfig(&inst, &config);
* @endcode
*
* @see ezUart_RegisterInstance
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_GetConfig(ezUartDrvInstance_t *inst,
                               struct ezUartConfiguration **config);


/*****************************************************************************
* Function: ezUart_UpdateConfig
*//** 
* @brief Update the configuration
*
* @details  Configuration is obtained by calling the function ezUart_GetConfig.
*           When this function is call, the HW implementation will internally
*           update its configuration and call the intialization function
*
* @param[in]    inst: Driver instance
* @return       STATUS_OK: Success
*               STATUS_XXX: Error code defined in EZ_DRV_STATUS
*
* @pre Configuration is obtained by ezUart_GetConfig.
* @post None
*
* \b Example
* @code
* EZ_DRV_STATUS status;
* ezUartConfiguration *config = NULL;
* status = ezUart_GetConfig(&inst, &config);
* config->baudrate = 115200;
* status = ezUart_UpdateConfig(&inst);
* @endcode
*
* @see ezUart_GetConfig
*
*****************************************************************************/
EZ_DRV_STATUS ezUart_UpdateConfig(ezUartDrvInstance_t *inst);

#ifdef __cplusplus
}
#endif

#endif /* EZ_UART_ENABLE == 1 */
#endif /* _EZ_UART_H */


/* End of file */
