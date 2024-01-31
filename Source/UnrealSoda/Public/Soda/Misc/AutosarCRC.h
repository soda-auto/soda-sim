// Copyright 2023 SODA.AUTO UK LTD. All Rights Reserved.

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


/**
 * @brief Calculate 8-bit SAE J1850 CRC
 * @areqby SWS_Crc_00030
 * @areqby SWS_Crc_00032
 * @param[in] DataPtr Pointer to start address of data block to be calculated.
 * @param[in] Length Length of data block to be calculated in bytes.
 * @param[in] StartValue8 Start value when the algorithm starts.
 * @param[in] IsFirstCall FALSE: Crc_StartValue8 is interpreted to be the return value of the previous function call. TRUE: start from initial value, ignore Crc_StartValue8.
 * @return For definitions for return values, see SWS_E2E_00047.
 * @retval 8-bit SAE J1850 CRC
 */
uint8_t UNREALSODA_API CalculateCRC8 (const uint8_t* DataPtr, uint32_t Length, uint8_t StartValue8, bool IsFirstCall);

/**
 * @brief Calculate 16-bit CCITT-FALSE CRC
 * @areqby SWS_Crc_00015
 * @param[in] DataPtr Pointer to start address of data block to be calculated.
 * @param[in] Length Length of data block to be calculated in bytes.
 * @param[in] StartValue16 Start value when the algorithm starts.
 * @param[in] IsFirstCall FALSE: Crc_StartValue16 is interpreted to be the return value of the previous function call. TRUE: start from initial value, ignore Crc_StartValue8.
 * @return For definitions for return values, see SWS_E2E_00047.
 * @retval 16-bit CCITT-FALSE CRC
 */
uint16_t UNREALSODA_API CalculateCRC16 (const uint8_t* DataPtr, uint32_t Length, uint16_t StartValue16, bool IsFirstCall);
