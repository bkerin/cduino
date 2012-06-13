/* Arduino Sd2Card Library
 * Copyright (C) 2009 by William Greiman
 *
 * This file is part of the Arduino Sd2Card Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino Sd2Card Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef SD_CARD_H
#define SD_CARD_H

#ifdef __cplusplus
  extern "C" {
#   include <term_io.h>
  }
#endif

/**
 * \file
 * Sd2Card class
 */
#include "Sd2PinMap.h"
#include "SdInfo.h"

// FIXME: clients using/returning the enum types should use this rather
// than uint8_t.

typedef enum {
  SPI_FULL_SPEED = 0,   // Maximum speed of F_CPU / 2.
  SPI_HALF_SPEED = 1,   // F_CPU / 4.
  SPI_QUARTER_SPEED = 2   // F_CPU / 8.
} sd_card_spi_speed_t;

#define OPTIMIZE_HARDWARE_SPI

//------------------------------------------------------------------------------
/** Protect block zero from write if nonzero */
#define SD_PROTECT_BLOCK_ZERO 1

/** init timeout ms */
uint16_t const SD_INIT_TIMEOUT = 2000;
/** erase timeout ms */
uint16_t const SD_ERASE_TIMEOUT = 10000;
/** read timeout ms */
uint16_t const SD_READ_TIMEOUT = 300;
/** write time out ms */
uint16_t const SD_WRITE_TIMEOUT = 600;

// Errors that can occur when trying to talk to the SD card.
typedef enum {
  // No error.
  SD_CARD_ERROR_NONE = 0x0,
  // Timeout error for command CMD0
  SD_CARD_ERROR_CMD0 = 0X1,
  // CMD8 was not accepted - not a valid SD card
  SD_CARD_ERROR_CMD8 = 0X2,
  // Card returned an error response for CMD17 (read block) 
  SD_CARD_ERROR_CMD17 = 0X3,
  // Card returned an error response for CMD24 (write block) 
  SD_CARD_ERROR_CMD24 = 0X4,
  // WRITE_MULTIPLE_BLOCKS command failed 
  SD_CARD_ERROR_CMD25 = 0X05,
  // Card returned an error response for CMD58 (read OCR) 
  SD_CARD_ERROR_CMD58 = 0X06,
  // SET_WR_BLK_ERASE_COUNT failed 
  SD_CARD_ERROR_ACMD23 = 0X07,
  // Card's ACMD41 initialization process timeout 
  SD_CARD_ERROR_ACMD41 = 0X08,
  // Card returned a bad CSR version field 
  SD_CARD_ERROR_BAD_CSD = 0X09,
  // Erase block group command failed 
  SD_CARD_ERROR_ERASE = 0X0A,
  // Card not capable of single block erase 
  SD_CARD_ERROR_ERASE_SINGLE_BLOCK = 0X0B,
  // Erase sequence timed out 
  SD_CARD_ERROR_ERASE_TIMEOUT = 0X0C,
  // Card returned an error token instead of read data 
  SD_CARD_ERROR_READ = 0X0D,
  // Read CID or CSD failed 
  SD_CARD_ERROR_READ_REG = 0X0E,
  // Timeout while waiting for start of read data 
  SD_CARD_ERROR_READ_TIMEOUT = 0X0F,
  // Card did not accept STOP_TRAN_TOKEN 
  SD_CARD_ERROR_STOP_TRAN = 0X10,
  // Card returned an error token as a response to a write operation 
  SD_CARD_ERROR_WRITE = 0X11,
  // Attempt to write protected block zero 
  SD_CARD_ERROR_WRITE_BLOCK_ZERO = 0X12,
  // Card did not go ready for a multiple block write 
  SD_CARD_ERROR_WRITE_MULTIPLE = 0X13,
  // Card returned an error to a CMD13 status check after a write 
  SD_CARD_ERROR_WRITE_PROGRAMMING = 0X14,
  // Timeout occurred during write programming 
  SD_CARD_ERROR_WRITE_TIMEOUT = 0X15,
  // Incorrect rate selected 
  SD_CARD_ERROR_SCK_RATE = 0X16
} sd_card_error_t;

// Card types.  FIXME: maybe using enums are int according to C, which would
// explain why Arduino libs like to use big lists of uint8_t const values
// instead.  But it could at least map a type to uint8_t for readability
// in the function that return this stuff.
typedef enum {
  SD_CARD_TYPE_INDETERMINATE = 0,   // Car type not known (yet).
  SD_CARD_TYPE_SD1 = 1,    // SD V1
  SD_CARD_TYPE_SD2 = 2,    // SD V2
  SD_CARD_TYPE_SDHC = 3,   // SDHC
} sd_card_type_t;

uint32_t
sd_card_size (void);

uint8_t
erase (uint32_t firstBlock, uint32_t lastBlock);

uint8_t
eraseSingleBlockEnable (void);

/**
 * \return error code for last error. See Sd2Card.h for a list of error codes.
 */
uint8_t
sd_card_error_code (void);

/** \return error data for last error. */
uint8_t
sd_card_error_data (void);

uint8_t
sd_card_init (uint8_t sckRateID, uint8_t chipSelectPin);

uint8_t
sd_card_read_block(uint32_t block, uint8_t* dst);

/**
 * Read a cards CID register. The CID contains card identification
 * information such as Manufacturer ID, Product name, Product serial
 * number and Manufacturing date. */
uint8_t
sd_card_read_cid (cid_t *cid);

/**
 * Read a cards CSD register. The CSD contains Card-Specific Data that
 * provides information regarding access to the card's contents. */
uint8_t
sd_card_read_csd (csd_t* csd);

// Return the card type.
sd_card_type_t
sd_card_type (void);

uint8_t
sd_card_write_block (uint32_t blockNumber, const uint8_t* src);

#endif  // SD_CARD_H
