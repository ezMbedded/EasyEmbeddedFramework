/*****************************************************************************
* Filename:         ez_i2c.h
* Author:           Hai Nguyen
* Original Date:    25.05.2025
*
* ----------------------------------------------------------------------------
* Contact:          Hai Nguyen
*                   hainguyen.eeit@gmail.com
*
* ----------------------------------------------------------------------------
* License: This file is published under the license described in LICENSE.md
*
*****************************************************************************/

/** @file   ez_i2c.h
 *  @author Hai Nguyen
 *  @date   25.05.2025
 *  @brief  One line description of the component
 *
 *  @details Detail description of the component
 */

#ifndef _EZ_I2C_H
#define _EZ_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
* Includes
*****************************************************************************/
#if (EZ_BUILD_WITH_CMAKE == 0U)
#include "ez_target_config.h"
#endif

#if (EZ_I2C == 1)
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
 * @brief Enumeration for I2C related constants or states.
 *
 * This enum is used to define various constants or states relevant to the I2C hardware abstraction layer.
 * The specific values and their descriptions should be provided below each enumerator.
 */
typedef enum
{
    EZ_I2C_MODE_MASTER,  /**< I2C master mode */
    EZ_I2C_MODE_SLAVE,   /**< I2C slave mode */
} EZ_I2C_MODE;


typedef enum
{
    EZ_I2C_SPEED_STANDARD,  /**< Standard speed (100 kHz) */
    EZ_I2C_SPEED_FAST,      /**< Fast speed (400 kHz) */
    EZ_I2C_SPEED_HIGH,      /**< High speed (1 MHz+), depending on HW support */
} EZ_I2C_SPEED;


typedef enum
{
    EZ_I2C_ADDRESSING_MODE_7BIT,  /**< 7-bit addressing mode */
    EZ_I2C_ADDRESSING_MODE_10BIT, /**< 10-bit addressing mode */
} EZ_I2C_ADDRESSING_MODE;

typedef struct
{
    EZ_I2C_MODE mode;                /**< I2C mode (master/slave) */
    EZ_I2C_SPEED speed;              /**< I2C speed (standard/fast/high) */
    EZ_I2C_ADDRESSING_MODE addressing_mode; /**< Addressing mode (7-bit/10-bit) */
} ezI2cConfig_t;


typedef EZ_DRV_STATUS (*ezHwI2c_Initialize)(void *driver_h, ezI2cConfig_t *config);
typedef EZ_DRV_STATUS (*ezHwI2c_TransmitSync)(void *driver_h,
                                              uint16_t address,
                                              const uint8_t *data,
                                              size_t length,
                                              bool send_stop,
                                              uint32_t timeout_millis);

typedef EZ_DRV_STATUS (*ezHwI2c_TransmitAsync)(void *driver_h,
                                               uint16_t address,
                                               const uint8_t *data,
                                               size_t length,
                                               bool send_stop);

typedef EZ_DRV_STATUS (*ezHwI2c_ReceiveSync)(void *driver_h,
                                             uint16_t address,
                                             uint8_t *data,
                                             size_t length,
                                             bool send_stop,
                                             uint32_t timeout_millis);

typedef EZ_DRV_STATUS (*ezHwI2c_ReceiveAsync)(void *driver_h,
                                              uint16_t address,
                                              uint8_t *data,
                                              size_t length,
                                              bool send_stop);

typedef EZ_DRV_STATUS (*ezHwI2c_Probe)(void *driver_h,
                                  uint16_t address,
                                  uint32_t timeout_millis);

struct ezHwI2cInterface
{
    void                   *driver_h;       /**< Pointer to the HW driver handle to identify which hw driver is used */
    ezHwI2c_Initialize     initialize;      /**< Initialize I2C interface */
    ezHwI2c_TransmitSync   transmit_sync;   /**< Transmit data synchronously */
    ezHwI2c_TransmitAsync  transmit_async;  /**< Transmit data asynchronously */
    ezHwI2c_ReceiveSync    receive_sync;    /**< Receive data synchronously */
    ezHwI2c_ReceiveAsync   receive_async;   /**< Receive data asynchronously */
    ezHwI2c_Probe          probe;           /**< Probe the I2C device at a specific address */
};

struct ezI2cDriver
{
    struct Node                 ll_node;    /* linked list node to link to list of hw driver implmentation */
    struct ezDriverCommon       common;     /* Common data of driver */
    struct ezHwI2cInterface     interface;  /* HW API */
    ezEventBus_t                i2c_event;  /* Subject for event notification */
    bool                        initialized; /* Flag to check if the driver is initialized */
};


typedef struct{
    struct ezDrvInstance    drv_instance;    /**< Driver instance */
    ezEventListener_t       event_subcriber; /**< Pointer to the event subscriber */
} ezI2cDrvInstance_t;

/*****************************************************************************
* Component Variable Definitions
*****************************************************************************/
/* None */

/*****************************************************************************
* Function Prototypes
*****************************************************************************/

EZ_DRV_STATUS ezI2c_SystemRegisterHwDriver(struct ezI2cDriver *hw_driver);

EZ_DRV_STATUS ezI2c_SystemUnregisterHwDriver(struct ezI2cDriver *hw_driver);

EZ_DRV_STATUS ezI2c_RegisterInstance(ezI2cDrvInstance_t *inst,
                                      const char *driver_name,
                                      EVENT_CALLBACK callback);

EZ_DRV_STATUS ezI2c_UnregisterInstance(ezI2cDrvInstance_t *inst);

EZ_DRV_STATUS ezI2c_Initialize(ezI2cDrvInstance_t *inst, ezI2cConfig_t *config);

EZ_DRV_STATUS ezI2c_TransmitSync(ezI2cDrvInstance_t *inst,
                                 uint16_t address,
                                 const uint8_t *data,
                                 size_t length,
                                 bool send_stop,
                                 uint32_t timeout_millis);

EZ_DRV_STATUS ezI2c_TransmitAsync(ezI2cDrvInstance_t *inst,
                                  uint16_t address,
                                  const uint8_t *data,
                                  size_t length,
                                  bool send_stop);

EZ_DRV_STATUS ezI2c_ReceiveSync(ezI2cDrvInstance_t *inst,
                                uint16_t address,
                                uint8_t *data,
                                size_t length,
                                bool send_stop,
                                uint32_t timeout_millis);

EZ_DRV_STATUS ezI2c_ReceiveAsync(ezI2cDrvInstance_t *inst,
                                 uint16_t address,
                                 uint8_t *data,
                                 size_t length,
                                 bool send_stop);

EZ_DRV_STATUS ezI2c_Probe(ezI2cDrvInstance_t *inst,
                          uint16_t address,
                          uint32_t timeout_millis);
#endif /* EZ_I2C == 1 */

#ifdef __cplusplus
}
#endif

#endif /* _EZ_I2C_H */


/* End of file */
