#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
#include <limits.h>

/*
 *******************************************************************************
                             Private Includes
 *******************************************************************************
 */

#include "modbus_slave_user.h"

/*
 *******************************************************************************
                        Private Macros and Typedefs
 *******************************************************************************
 */


// Define access types for the registers
typedef enum
{
    RW, // Read/Write
    R   // Read Only
} AccessType_t;

// Define data types for the registers
typedef enum
{
    TYPE_UINT16,
    TYPE_UINT32,
    TYPE_UINT64,
    TYPE_INT16,
    TYPE_INT32,
    TYPE_INT64,
    TYPE_FLOAT,
    TYPE_DOUBLE
} DataType_t;

// Define Modbus register addresses
// MACRO DEFINITIONS START
#define MB_SP1_ADDR_MOD                         1
#define MB_SP2_ADDR_MOD                         2
#define RANGE_HIGH                              3
#define DISP_GAS_VAL                            4
#define TEST_DAC_CNT_MOD                        5
#define GAS_TYPE                                6
#define ACK_CONFIG                              7
#define COMM_ADDR                               8
#define COMM_SPEED                              9
#define ADC_DATA                               10
#define FACTORY_LOWER_LIMIT                    11
#define FACTORY_UPPER_LIMIT                    12
#define FACTORY_CALIBRATION                    13
#define APPLIED_GAS_VALUE                      14
#define DAC_CURRENT_THRESHOLD                  15
#define MB_LAST_REGISTER_ADDRESS               DAC_CURRENT_THRESHOLD
// MACRO DEFINITIONS END

// Define endianness macros
#define MB_LITTLE_ENDIAN                1
#define MB_BIG_ENDIAN                   2

// Select the platform's endianness format (STM32F is little endian)
#define MB_PLATFORM_ENDIANNESS          MB_LITTLE_ENDIAN

// Define the target endianness format
#define MB_TARGET_ENDIANNESS            MB_BIG_ENDIAN

/*
 *******************************************************************************
                        External Variable Declarations
 *******************************************************************************
 */

// Declare the external gas sensor database
extern gd_db_t gd_db;

/*
 *******************************************************************************
                        Private Variable Declaration
 *******************************************************************************
 */

// None

/*
 *******************************************************************************
                        Private Function Declaration
 *******************************************************************************
 */

// None

/*
 *******************************************************************************
                        Register Map Declaration
 *******************************************************************************
 */

// Structure to map Modbus register addresses to data pointers and sizes
typedef struct
{
    uint16_t address;    // Modbus register address
    void *dataPtr;       // Pointer to the data
    size_t dataSize;     // Size of the data in bytes
    AccessType_t access; // Access type (Read/Write or Read Only)
    DataType_t dataType; // Data type of the register
    const void *minLimPtr;     // Pointer to the Min Limit
    const void *maxLimPtr;     // Pointer to the Max Limit
} RegisterMap_t;

// Array to hold the register map
// REGISTER MAP ARRAY START
static const RegisterMap_t registerMap[] =
{
    {MB_SP1_ADDR_MOD, &gd_db.alarm_set.alarm1_lel, sizeof(&gd_db.alarm_set.alarm1_lel), RW, TYPE_UINT16, &LMT_ZERO_MIN, &gd_db.alarm_set.alarm2_lel},
    {MB_SP2_ADDR_MOD, &gd_db.alarm_set.alarm2_lel, sizeof(&gd_db.alarm_set.alarm2_lel), RW, TYPE_UINT16, &gd_db.alarm_set.alarm1_lel, NULL},
    {RANGE_HIGH, &gd_db.range_high, sizeof(&gd_db.range_high), RW, TYPE_UINT16, NULL, NULL},
    {DISP_GAS_VAL, &gd_db.disp_gas_val, sizeof(&gd_db.disp_gas_val), R, TYPE_FLOAT, NULL, NULL},
    {TEST_DAC_CNT_MOD, &gd_db.test_dac_cnt, sizeof(&gd_db.test_dac_cnt), RW, TYPE_UINT16, NULL, NULL},
    {GAS_TYPE, &gd_db.gas_type, sizeof(&gd_db.gas_type), RW, TYPE_UINT16, NULL, NULL},
    {ACK_CONFIG, &gd_db.ack_config, sizeof(&gd_db.ack_config), RW, TYPE_UINT16, NULL, NULL},
    {COMM_ADDR, &gd_db.comm_addr, sizeof(&gd_db.comm_addr), RW, TYPE_UINT16, NULL, NULL},
    {COMM_SPEED, &gd_db.comm_speed, sizeof(&gd_db.comm_speed), RW, TYPE_UINT16, NULL, NULL},
    {ADC_DATA, &gd_db.adc_data, sizeof(&gd_db.adc_data), R, TYPE_UINT32, NULL, NULL},
    {FACTORY_LOWER_LIMIT, &gd_db.factory_lower_limit, sizeof(&gd_db.factory_lower_limit), R, TYPE_UINT16, NULL, NULL},
    {FACTORY_UPPER_LIMIT, &gd_db.factory_upper_limit, sizeof(&gd_db.factory_upper_limit), R, TYPE_UINT16, NULL, NULL},
    {FACTORY_CALIBRATION, &gd_db.factory_calibration, sizeof(&gd_db.factory_calibration), R, TYPE_UINT16, NULL, NULL},
    {APPLIED_GAS_VALUE, &gd_db.applied_gas_value, sizeof(&gd_db.applied_gas_value), R, TYPE_UINT32, NULL, NULL},
    {DAC_CURRENT_THRESHOLD, &gd_db.dac_current_threshold, sizeof(&gd_db.dac_current_threshold), RW, TYPE_UINT16, NULL, NULL},
};
// REGISTER MAP ARRAY END

/*
 *******************************************************************************
 * Function Definitions
 *******************************************************************************
 */

// Swap bytes in a 16-bit value
static uint16_t SwapBytes16(uint16_t value)
{
    return (value >> 8) | (value << 8);
}

// Swap bytes in a 32-bit value
static uint32_t SwapBytes32(uint32_t value)
{
    return ((value >> 24) & 0x000000FF) |
           ((value >>  8) & 0x0000FF00) |
           ((value <<  8) & 0x00FF0000) |
           ((value << 24) & 0xFF000000);
}

// Swap bytes in a 64-bit value
static uint64_t SwapBytes64(uint64_t value)
{
    return ((value >> 56) & 0x00000000000000FF) |
           ((value >> 40) & 0x000000000000FF00) |
           ((value >> 24) & 0x0000000000FF0000) |
           ((value >>  8) & 0x00000000FF000000) |
           ((value <<  8) & 0x000000FF00000000) |
           ((value << 24) & 0x0000FF0000000000) |
           ((value << 40) & 0x00FF000000000000) |
           ((value << 56) & 0xFF00000000000000);
}

/**
 *******************************************************************************
 * @fn uint8_t MB_ReadHoldingReg(uint16_t reg_address, uint8_t *tx_buff_ptr)
 *******************************************************************************
 * @brief   This routine checks for the Modbus register address and reads data.
 * @note    Call from MB_ReadCmdPrcs in while loop.
 * @param   reg_address : Register address of the data to read
 * @param   tx_buff_ptr : Pointer to the buffer to store the read data
 * @retval  readRegCount: Returns the number of data registers read by the device.
 *******************************************************************************
 */
uint8_t MB_ReadHoldingReg(uint16_t reg_address, uint8_t *tx_buff_ptr)
{
    uint8_t tempBuff[8] = {0};
    size_t dataSize = 0;
    bool_t address_found = false;
    uint8_t readRegCount = 0;

    // Loop through the register map to find the register address
    for (size_t i = 0; i < sizeof(registerMap) / sizeof(registerMap[0]); i++)
    {
        if (registerMap[i].address == reg_address)
        {
            // Copy the data from the register to the temporary buffer
            memcpy(tempBuff, registerMap[i].dataPtr, registerMap[i].dataSize);
            dataSize = registerMap[i].dataSize;
            address_found = true;

            if (dataSize == 1)
            {
                dataSize = 2;
            }
            break;
        }
    }

    if (!address_found)
    {
        return 0; // Return 0 if the register address is invalid
    }

    // Adjust data based on endianness
    if (MB_PLATFORM_ENDIANNESS == MB_LITTLE_ENDIAN && MB_TARGET_ENDIANNESS == MB_BIG_ENDIAN)
    {
        switch (dataSize)
        {
            case 2:
                *(uint16_t *)tempBuff = SwapBytes16(*(uint16_t *)tempBuff);
                break;
            case 4:
                *(uint32_t *)tempBuff = SwapBytes32(*(uint32_t *)tempBuff);
                break;
            case 8:
                *(uint64_t *)tempBuff = SwapBytes64(*(uint64_t *)tempBuff);
                break;
        }
    }
    else if (MB_PLATFORM_ENDIANNESS == MB_BIG_ENDIAN && MB_TARGET_ENDIANNESS == MB_LITTLE_ENDIAN)
    {
        switch (dataSize)
        {
            case 2:
                *(uint16_t *)tempBuff = SwapBytes16(*(uint16_t *)tempBuff);
                break;
            case 4:
                *(uint32_t *)tempBuff = SwapBytes32(*(uint32_t *)tempBuff);
                break;
            case 8:
                *(uint64_t *)tempBuff = SwapBytes64(*(uint64_t *)tempBuff);
                break;
        }
    }

    readRegCount = dataSize / 2; // Each register is 2 bytes

    // Copy the data from the temporary buffer to the transmission buffer
    for (uint8_t i = 0; i < dataSize; i++)
    {
        *tx_buff_ptr++ = tempBuff[i];
    }

    return readRegCount;
}

/**
 *******************************************************************************
 * @fn uint8_t MB_WriteHoldingReg(uint16_t reg_address, uint8_t *rx_buff_ptr)
 *******************************************************************************
 * @brief   This routine checks for the Modbus register address and stores data.
 * @note    Call from MB_WriteCmdPrcs in while loop.
 * @param   reg_address : Register address of the data to write
 * @param   rx_buff_ptr : Pointer to the buffer containing the data to write
 * @retval  writeRegCount: Returns the number of data registers written to the device,
 *                          or 0xFF if the register address is invalid,
 *                          or 0xFE if the data is out of range.
 *******************************************************************************
 */
uint8_t MB_WriteHoldingReg(uint16_t reg_address, uint8_t *rx_buff_ptr)
{
    uint8_t tempBuff[8] = {0};
    size_t dataSize = 0;
    uint8_t writeRegCount = 0;

    // Loop through the register map to find the register address
    for (size_t i = 0; i < sizeof(registerMap) / sizeof(registerMap[0]); i++)
    {
        if (registerMap[i].address == reg_address)
        {
            // Check if the register is read-only
            if (registerMap[i].access == R)
            {
                return 0xFF; // Return an error code for read-only registers
            }

            dataSize = registerMap[i].dataSize;
            if(dataSize == 1)
            {
            	dataSize = 2;
            }
            // Copy the data from the receive buffer to the temporary buffer
            memcpy(tempBuff, rx_buff_ptr, dataSize);

            // Adjust data based on endianness
            if (MB_PLATFORM_ENDIANNESS == MB_LITTLE_ENDIAN && MB_TARGET_ENDIANNESS == MB_BIG_ENDIAN)
            {
                switch (dataSize)
                {
                    case 2:
                        *(uint16_t *)tempBuff = SwapBytes16(*(uint16_t *)tempBuff);
                        break;
                    case 4:
                        *(uint32_t *)tempBuff = SwapBytes32(*(uint32_t *)tempBuff);
                        break;
                    case 8:
                        *(uint64_t *)tempBuff = SwapBytes64(*(uint64_t *)tempBuff);
                        break;
                }
            }
            else if (MB_PLATFORM_ENDIANNESS == MB_BIG_ENDIAN && MB_TARGET_ENDIANNESS == MB_LITTLE_ENDIAN)
            {
                switch (dataSize)
                {
                    case 2:
                        *(uint16_t *)tempBuff = SwapBytes16(*(uint16_t *)tempBuff);
                        break;
                    case 4:
                        *(uint32_t *)tempBuff = SwapBytes32(*(uint32_t *)tempBuff);
                        break;
                    case 8:
                        *(uint64_t *)tempBuff = SwapBytes64(*(uint64_t *)tempBuff);
                        break;
                }
            }

            // Check min/max limits if applicable
            if (registerMap[i].minLimPtr != NULL && registerMap[i].maxLimPtr != NULL)
            {
                switch (registerMap[i].dataType)
                {
                    case TYPE_UINT16:
                    {
                        uint16_t value = *(uint16_t *)tempBuff;
                        uint16_t minLim = *(uint16_t *)registerMap[i].minLimPtr;
                        uint16_t maxLim = *(uint16_t *)registerMap[i].maxLimPtr;
                        if (value < minLim || value > maxLim)
                        {
                            return 0xFE; // Return an error code for value out of range
                        }
                        break;
                    }
                    case TYPE_UINT32:
                    {
                        uint32_t value = *(uint32_t *)tempBuff;
                        uint32_t minLim = *(uint32_t *)registerMap[i].minLimPtr;
                        uint32_t maxLim = *(uint32_t *)registerMap[i].maxLimPtr;
                        if (value < minLim || value > maxLim)
                        {
                            return 0xFE; // Return an error code for value out of range
                        }
                        break;
                    }
                    case TYPE_UINT64:
                    {
                        uint64_t value = *(uint64_t *)tempBuff;
                        uint64_t minLim = *(uint64_t *)registerMap[i].minLimPtr;
                        uint64_t maxLim = *(uint64_t *)registerMap[i].maxLimPtr;
                        if (value < minLim || value > maxLim)
                        {
                            return 0xFE; // Return an error code for value out of range
                        }
                        break;
                    }
                    case TYPE_INT16:
                    {
                        int16_t value = *(int16_t *)tempBuff;
                        int16_t minLim = *(int16_t *)registerMap[i].minLimPtr;
                        int16_t maxLim = *(int16_t *)registerMap[i].maxLimPtr;
                        if (value < minLim || value > maxLim)
                        {
                            return 0xFE; // Return an error code for value out of range
                        }
                        break;
                    }
                    case TYPE_INT32:
                    {
                        int32_t value = *(int32_t *)tempBuff;
                        int32_t minLim = *(int32_t *)registerMap[i].minLimPtr;
                        int32_t maxLim = *(int32_t *)registerMap[i].maxLimPtr;
                        if (value < minLim || value > maxLim)
                        {
                            return 0xFE; // Return an error code for value out of range
                        }
                        break;
                    }
                    case TYPE_INT64:
                    {
                        int64_t value = *(int64_t *)tempBuff;
                        int64_t minLim = *(int64_t *)registerMap[i].minLimPtr;
                        int64_t maxLim = *(int64_t *)registerMap[i].maxLimPtr;
                        if (value < minLim || value > maxLim)
                        {
                            return 0xFE; // Return an error code for value out of range
                        }
                        break;
                    }
                    case TYPE_FLOAT:
                    {
                        float value = *(float *)tempBuff;
                        float minLim = *(float *)registerMap[i].minLimPtr;
                        float maxLim = *(float *)registerMap[i].maxLimPtr;
                        if (value < minLim || value > maxLim)
                        {
                            return 0xFE; // Return an error code for value out of range
                        }
                        break;
                    }
                    case TYPE_DOUBLE:
                    {
                        double value = *(double *)tempBuff;
                        double minLim = *(double *)registerMap[i].minLimPtr;
                        double maxLim = *(double *)registerMap[i].maxLimPtr;
                        if (value < minLim || value > maxLim)
                        {
                            return 0xFE; // Return an error code for value out of range
                        }
                        break;
                    }
                }
            }

            // Copy the data from the temporary buffer to the register
            memcpy(registerMap[i].dataPtr, tempBuff, dataSize);
            StoreToMemory(); // Store the updated data to memory

            writeRegCount = dataSize / 2; // Each register is 2 bytes
            break;
        }
    }

    // Return an error code if the register address is invalid
    if (dataSize == 0)
    {
        return 0xFF; // Invalid Address
    }

    return writeRegCount; // Return the number of registers written
}


/*--------------------------- End Of File ------------------------------------*/
