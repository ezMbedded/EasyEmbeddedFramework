/*****************************************************************************
* Filename:         ez_spi.h
* Author:           Hai Nguyen
* Original Date:    24.08.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_spi.h
 *  @author Hai Nguyen
 *  @date   24.08.2025
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 */

#ifndef _EZ_SPI_H
#define _EZ_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_SPI == 1)
#include <stdint.h>
#include <stdbool.h>

#include "ez_driver_def.h"
#include "ez_event_bus.h"

/*****************************************************************************
* Component Preprocessor Macros
*****************************************************************************/
#define A_MACRO     1   /**< a macro*/

/*****************************************************************************
* Component Typedefs
*****************************************************************************/

/**
 * @enum
 * @brief Enumeration for SPI related constants or states.
 *
 * This enum is used to define various constants or states relevant to the SPI hardware abstraction layer.
 */
typedef enum
{
    EZ_SPI_MODE_MASTER,  /**< SPI master mode */
    EZ_SPI_MODE_SLAVE,   /**< SPI slave mode */
} EZ_SPI_MODE;

typedef enum
{
    EZ_SPI_CPOL0_CPHA0,  /**< CPOL=0, CPHA=0 */
    EZ_SPI_CPOL0_CPHA1,  /**< CPOL=0, CPHA=1 */
    EZ_SPI_CPOL1_CPHA0,  /**< CPOL=1, CPHA=0 */
    EZ_SPI_CPOL1_CPHA1,  /**< CPOL=1, CPHA=1 */
} EZ_SPI_CLOCK_MODE;

typedef enum
{
    EZ_SPI_BIT_ORDER_MSB_FIRST,  /**< Most significant bit first */
    EZ_SPI_BIT_ORDER_LSB_FIRST,  /**< Least significant bit first */
} EZ_SPI_BIT_ORDER;

typedef struct
{
    EZ_SPI_MODE mode;                /**< SPI mode (master/slave) */
    EZ_SPI_CLOCK_MODE clock_mode;    /**< SPI clock mode */
    EZ_SPI_BIT_ORDER bit_order;      /**< SPI bit order (MSB/LSB first) */
    uint32_t clock_frequency;        /**< SPI clock frequency in Hz */
    uint8_t data_size;               /**< Data size in bits (usually 8 or 16) */
} ezSpiConfig_t;


typedef EZ_DRV_STATUS (*ezHwSpi_Initialize)(void *driver_h, ezSpiConfig_t *config);
typedef EZ_DRV_STATUS (*ezHwSpi_TransferSync)(void *driver_h,
                                             const uint8_t *tx_data,
                                             size_t tx_length,
                                             uint8_t *rx_data,
                                             size_t rx_length,
                                             uint32_t timeout_millis);

typedef EZ_DRV_STATUS (*ezHwSpi_TransferAsync)(void *driver_h,
                                              const uint8_t *tx_data,
                                              size_t tx_length,
                                              uint8_t *rx_data,
                                              size_t rx_length);

typedef EZ_DRV_STATUS (*ezHwSpi_TransmitSync)(void *driver_h,
                                             const uint8_t *tx_data,
                                             size_t length,
                                             uint32_t timeout_millis);

typedef EZ_DRV_STATUS (*ezHwSpi_TransmitAsync)(void *driver_h,
                                              const uint8_t *tx_data,
                                              size_t length);

typedef EZ_DRV_STATUS (*ezHwSpi_ReceiveSync)(void *driver_h,
                                            uint8_t *rx_data,
                                            size_t length,
                                            uint32_t timeout_millis);

typedef EZ_DRV_STATUS (*ezHwSpi_ReceiveAsync)(void *driver_h,
                                             uint8_t *rx_data,
                                             size_t length);

typedef EZ_DRV_STATUS (*ezHwSpi_ChipSelect)(void *driver_h,
                                           bool select);

struct ezHwSpiInterface
{
    void                   *driver_h;       /**< Pointer to the HW driver handle to identify which hw driver is used */
    ezHwSpi_Initialize     initialize;      /**< Initialize SPI interface */
    ezHwSpi_TransferSync   transfer_sync;   /**< Transfer data synchronously (transmit and receive simultaneously) */
    ezHwSpi_TransferAsync  transfer_async;  /**< Transfer data asynchronously (transmit and receive simultaneously) */
    ezHwSpi_TransmitSync   transmit_sync;   /**< Transmit data synchronously */
    ezHwSpi_TransmitAsync  transmit_async;  /**< Transmit data asynchronously */
    ezHwSpi_ReceiveSync    receive_sync;    /**< Receive data synchronously */
    ezHwSpi_ReceiveAsync   receive_async;   /**< Receive data asynchronously */
    ezHwSpi_ChipSelect     chip_select;     /**< Control the chip select line */
};

struct ezSpiDriver
{
    struct Node                 ll_node;    /* linked list node to link to list of hw driver implmentation */
    struct ezDriverCommon       common;     /* Common data of driver */
    struct ezHwSpiInterface     interface;  /* HW API */
    ezEventBus_t                spi_event;  /* Subject for event notification */
    bool                        initialized; /* Flag to check if the driver is initialized */
};


typedef struct{
    struct ezDrvInstance    drv_instance;    /**< Driver instance */
    ezEventListener_t       event_subcriber; /**< Pointer to the event subscriber */
} ezSpiDrvInstance_t;

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/

EZ_DRV_STATUS ezSpi_SystemRegisterHwDriver(struct ezSpiDriver *hw_driver);

EZ_DRV_STATUS ezSpi_SystemUnregisterHwDriver(struct ezSpiDriver *hw_driver);

EZ_DRV_STATUS ezSpi_RegisterInstance(ezSpiDrvInstance_t *inst,
                                    const char *driver_name,
                                    EVENT_CALLBACK callback);

EZ_DRV_STATUS ezSpi_UnregisterInstance(ezSpiDrvInstance_t *inst);

EZ_DRV_STATUS ezSpi_Initialize(ezSpiDrvInstance_t *inst, ezSpiConfig_t *config);

EZ_DRV_STATUS ezSpi_TransferSync(ezSpiDrvInstance_t *inst,
                                const uint8_t *tx_data,
                                size_t tx_length,
                                uint8_t *rx_data,
                                size_t rx_length,
                                uint32_t timeout_millis);

EZ_DRV_STATUS ezSpi_TransferAsync(ezSpiDrvInstance_t *inst,
                                const uint8_t *tx_data,
                                size_t tx_length,
                                uint8_t *rx_data,
                                size_t rx_length);

EZ_DRV_STATUS ezSpi_TransmitSync(ezSpiDrvInstance_t *inst,
                                const uint8_t *tx_data,
                                size_t length,
                                uint32_t timeout_millis);

EZ_DRV_STATUS ezSpi_TransmitAsync(ezSpiDrvInstance_t *inst,
                                const uint8_t *tx_data,
                                size_t length);

EZ_DRV_STATUS ezSpi_ReceiveSync(ezSpiDrvInstance_t *inst,
                               uint8_t *rx_data,
                               size_t length,
                               uint32_t timeout_millis);

EZ_DRV_STATUS ezSpi_ReceiveAsync(ezSpiDrvInstance_t *inst,
                                uint8_t *rx_data,
                                size_t length);

EZ_DRV_STATUS ezSpi_ChipSelect(ezSpiDrvInstance_t *inst, bool select);

#endif /* EZ_SPI == 1 */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_SPI_H */


/* End of file */
