// Interface to SD card (via SPI controller).
//
// Test Driver: sd_card_test.c    Implementation: sd_card.c

#ifndef SD_CARD_H
#define SD_CARD_H

#include <term_io.h>

#include "sd_card_info.h"

// WARNING: despite being ubiquitous, many SD cards are utter junk.
// They lack any underlying wear leveling for the flash memory and are
// horribly intolerant of asynchronous shutdown (power cuts).  If you're
// doing anything remotely serious you must invest in an "industrial"
// SD card.  I've used the Apacer AP-MSD04GCS4P-1TM with good results.
//
// This interface has been tested with the SD card
// hardware on the official Arduino Ethernet/SD Card shield, Rev. 3
// (http://arduino.cc/en/Main/ArduinoEthernetShield).
//
// This interface supports using the card simply as a large memory.
// FAT filesystem support is (FIXME) not done yet.

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

// Clients must specify the SPI slave select pin being used at compile
// time, but the other SPI lines (MISO, MOSI, and SCK) are fixed by the
// spi module implementation.
#ifndef SD_CARD_SPI_SLAVE_SELECT_PIN
#  error The SD_CARD_SPI_SLAVE_SELEC_PIN which is supposed to specify the \
         pin to use to select the SD card controller SPI slave is not set. \
         See the example in the Makefile in the sd_card module directory.
#endif

// FIXXME: Optimized hardware SPI isn't currently supported.  The point
// of this in the origianl Arduino libs was to make the SD card interface
// go fast enough to keep up with audio data rates, but I don't need that
// and have never tested it.  Also, modern SDHC cards seem to support write
// rates fast enough for audio support with out it (depending on the audio
// buffering I guess).  I think all the code enabled by this should work
// as it is, and it would probably be pretty easy to tidy it up and put it
// in the spi.h interface.
//#define OPTIMIZE_HARDWARE_SPI

// Protect block zero from write if nonzero
#define SD_PROTECT_BLOCK_ZERO 1

#define SD_INIT_TIMEOUT  ((uint16_t const) 2000)    // Init timeout ms
#define SD_ERASE_TIMEOUT ((uint16_t const) 10000)   // Erase timeout ms
#define SD_READ_TIMEOUT  ((uint16_t const) 300)     // Read timeout ms
#define SD_WRITE_TIMEOUT ((uint16_t const) 600)     // Write timeout ms

// Errors that can occur when trying to talk to the SD card.
typedef enum {
  // No error.
  SD_CARD_ERROR_NONE = 0x00,
  // Timeout error for command CMD0
  SD_CARD_ERROR_CMD0 = 0x01,
  // CMD8 was not accepted - not a valid SD card
  SD_CARD_ERROR_CMD8 = 0x02,
  // Card returned an error response for CMD17 (read block) 
  SD_CARD_ERROR_CMD17 = 0x03,
  // Card returned an error response for CMD24 (write block) 
  SD_CARD_ERROR_CMD24 = 0x04,
  // WRITE_MULTIPLE_BLOCKS command failed 
  SD_CARD_ERROR_CMD25 = 0x05,
  // Card returned an error response for CMD58 (read OCR) 
  SD_CARD_ERROR_CMD58 = 0x06,
  // SET_WR_BLK_ERASE_COUNT failed 
  SD_CARD_ERROR_ACMD23 = 0x07,
  // Card's ACMD41 initialization process timeout 
  SD_CARD_ERROR_ACMD41 = 0x08,
  // Card returned a bad CSR version field 
  SD_CARD_ERROR_BAD_CSD = 0x09,
  // Erase block group command failed 
  SD_CARD_ERROR_ERASE = 0x0A,
  // Card not capable of single block erase 
  SD_CARD_ERROR_ERASE_SINGLE_BLOCK = 0x0B,
  // Erase sequence timed out 
  SD_CARD_ERROR_ERASE_TIMEOUT = 0x0C,
  // Card returned an error token instead of read data 
  SD_CARD_ERROR_READ = 0x0D,
  // Read CID or CSD failed 
  SD_CARD_ERROR_READ_REG = 0x0E,
  // Timeout while waiting for start of read data 
  SD_CARD_ERROR_READ_TIMEOUT = 0x0F,
  // Card did not accept STOP_TRAN_TOKEN 
  SD_CARD_ERROR_STOP_TRAN = 0x10,
  // Card returned an error token as a response to a write operation 
  SD_CARD_ERROR_WRITE = 0x11,
  // Attempt to write protected block zero 
  SD_CARD_ERROR_WRITE_BLOCK_ZERO = 0x12,
  // Card did not go ready for a multiple block write 
  SD_CARD_ERROR_WRITE_MULTIPLE = 0x13,
  // Card returned an error to a CMD13 status check after a write 
  SD_CARD_ERROR_WRITE_PROGRAMMING = 0x14,
  // Timeout occurred during write programming 
  SD_CARD_ERROR_WRITE_TIMEOUT = 0x15,
  // Incorrect rate selected 
  SD_CARD_ERROR_SCK_RATE = 0x16
} sd_card_error_t;

// Return error code for last error.  Many other functions in this interface
// set an internal error code but only return a generic "failure" sentinel
// value on error.  This method will return a code that describes the most
// recent error more precisely.
sd_card_error_t
sd_card_last_error (void);

// Return any error data associated with the last error.  In practice
// I believe this always means the most recent byte received from the
// SD card controller.  This may not be anything relevant, depending on
// the error, and will probably require inspection of the source code to
// interpret usefully.  The status codes are defined in sd_card_info.h.
uint8_t
sd_card_last_error_data (void);

// Communication speed between microcontroller and SD card.
typedef enum {
  SD_CARD_SPI_FULL_SPEED    = 0,   // Maximum speed of F_CPU / 2.
  SD_CARD_SPI_HALF_SPEED    = 1,   // F_CPU / 4.
  SD_CARD_SPI_QUARTER_SPEED = 2    // F_CPU / 8.
} sd_card_spi_speed_t;

// Initialize an SD flash card and this interface.  The speed argument sets
// the SPI communcation rate between card and microcontroller.  Returns TRUE
// on success and zero on error (in which case sd_card_last_error() can
// be called).  This calls time0_stopwatch_init() and spi_init().
uint8_t
sd_card_init (sd_card_spi_speed_t speed);

// Return the size of the card in SD_CARD_BLOCK_SIZE byte blocks, or zero
// if an error occurs.
uint32_t
sd_card_size (void);

// Card types. 
typedef enum {
  SD_CARD_TYPE_INDETERMINATE = 0,   // Car type not known (yet).
  SD_CARD_TYPE_SD1           = 1,   // SD V1
  SD_CARD_TYPE_SD2           = 2,   // SD V2
  SD_CARD_TYPE_SDHC          = 3,   // SDHC
} sd_card_type_t;

// Return the card type.
sd_card_type_t
sd_card_type (void);

// Read a cards CID register. The CID contains card identification
// information such as manufacturer ID, product name, product serial number
// and manufacturing date.  Returns TRUE on success, and FALSE on failure
// (in which case sd_card_last_error() can be called).
uint8_t
sd_card_read_cid (sd_card_cid_t *cid);

// Read a cards CSD register. The CSD contains card-specific data that
// provides information regarding access to the card's contents.  Returne TRUE
// on success, and FALSE on failure (in which case sd_card_last_error()
// can be called).
uint8_t
sd_card_read_csd (sd_card_csd_t *csd);

// Read a block of data.  The block argument is the logical block to read, and
// the data read is stores at dst.  On success, TRUE is returned, otherwise
// FALSE is returned (in which case sd_card_last_error() may be called.
uint8_t
sd_card_read_block (uint32_t block, uint8_t *dst);

// Write a block of data (but see SD_PROTECT_BLOCK_ZERO).  The block
// argument is the logical block to write, and the data to write is taken
// from location src.  On success, TRUE is returned, otherwise FALSE is
// returned and sd_card_last_error() may be called.
uint8_t
sd_card_write_block (uint32_t block, const uint8_t *src);

// Returns TRUE iff the SD card provides an erase operation for individual
// blocks.  Note that its always possible to simply overwrite blocks.
uint8_t
sd_card_single_block_erase_supported (void);

// Erase a range of blocks.  This method requires that
// sd_card_single_block_erase_supported() return TRUE.  The data on the
// card after this operation may be either zeros or ones, depending on the
// card vendor.
uint8_t
sd_card_erase_blocks (uint32_t first_block, uint32_t last_block);

#endif  // SD_CARD_H
