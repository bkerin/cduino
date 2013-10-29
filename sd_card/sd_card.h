// Interface to SD card (via SPI controller).
//
// Test driver: sd_card_test.c    Implementation: sd_card.c

#ifndef SD_CARD_H
#define SD_CARD_H

// FIXME: shouldn't be draggin term_io.h in here in shipping code
#include <term_io.h>

// See the description of this define in the Makefile for this module.
#ifdef SD_CARD_USE_TIMER0_FOR_TIMEOUTS
#  include "timer0_stopwatch.h"
#endif // SD_CARD_USE_TIMER0_FOR_TIMEOUTS

#include "sd_card_info.h"

// WARNING: despite being ubiquitous, many SD cards are utter junk.
// They lack any underlying wear leveling for the flash memory and are
// horribly intolerant of asynchronous shutdown (power cuts).  If you're
// doing anything remotely serious you must invest in an "industrial"
// SD card.  I've used the Apacer AP-MSD04GCS4P-1TM with good results.
//
// This module assumes that communication with the SD card is reliable, and
// doesn't use the CRC functionality that may be available on the SD card.
// If this is not the case, perhaps you would like to use the hardware
// watchdog on the ATMega or perhaps add CRC support to this module :)
//
// This module has been tested with the SD card
// hardware on the official Arduino Ethernet/SD Card shield, Rev. 3
// (http://arduino.cc/en/Main/ArduinoEthernetShield).
//
// This module always checks for card support of supply voltages in the 2.7V
// - 3.6V range.  Its possible that incorrect support for a lower voltage
// might be indicated (though for the test hardware mentioned above this
// isn't an issue of course).
//
// This interface supports using the card simply as a large memory.
// FAT filesystem support is (FIXME) not done yet.
//
// Basic use looks about like this:
//
//   // Specify the IO pin which is being used for SD card SPI slave selection
//   #define SD_CARD_SPI_SLAVE_SELEC_PIN DIO_PIN_DIGITAL_4 
//
//   #define SAMPLE_COUNT 42
//   uint8_t buf[SAMPLE_COUNT];
//
//   get_samples_from_somewhere (buf);
//
//   uint8_t sentinel = sd_card_init (SD_CARD_FULL_SPEED);
//   assert (sentinel);
//
//   uint32_t some_block = 42;
//
//   sentinel = sd_card_write_partial_block (some_block, SAMPLE_COUNT, buf);
//   assert (sentinel);
//
//   // Time passes...
//
//   sentinel = sd_card_read_partial_block (some_block, SAMPLE_COUNT, buf);
//   assert (sentinel);
//
// For more details see the rest of this reader and the test/demo driver
// in sd_cart_test.c.


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

// All SDHC cards at least (I don't know about SD1 or SD2 types) always
// write blocks or this size at a time.
#define SD_CARD_BLOCK_SIZE 512

// Protect block zero from write if nonzero
#define SD_CARD_PROTECT_BLOCK_ZERO 1

// Very approximate timeouts for various SD card operations.  The actual
// time required for these operations to complete could actually be several
// times these values.  FIXXME: I'm not sure where these timeout values
// come from originally, they are inherited from the Arduino code.
#define SD_CARD_PRECMD_TIMEOUT ((uint16_t const) 300)     // Pre-cmd timeout ms
#define SD_CARD_INIT_TIMEOUT   ((uint16_t const) 2000)    // Init timeout ms
#define SD_CARD_ERASE_TIMEOUT  ((uint16_t const) 10000)   // Erase timeout ms
#define SD_CARD_READ_TIMEOUT   ((uint16_t const) 300)     // Read timeout ms
#define SD_CARD_WRITE_TIMEOUT  ((uint16_t const) 600)     // Write timeout ms

// Errors that can occur when trying to talk to the SD card.
typedef enum {
  // No error.
  SD_CARD_ERROR_NONE = 0x00,
  // Timeout error for command CMD0
  SD_CARD_ERROR_CMD0_TIMEOUT = 0x01,
  // CMD8 was not accepted - not valid SD card or supplied voltage unsupported
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
  SD_CARD_SPI_SPEED_UNSET   = 0,   // Speed hasn't been set yet.
  SD_CARD_SPI_SPEED_FULL    = 2,   // Maximum speed of F_CPU / 2.
  SD_CARD_SPI_SPEED_HALF    = 4,   // F_CPU / 4.
  SD_CARD_SPI_SPEED_QUARTER = 8    // F_CPU / 8.
} sd_card_spi_speed_t;

// SPI communication speed that we set (at sd_card_init()-time).
extern sd_card_spi_speed_t speed;

// Initialize an SD flash card and this interface.  The speed argument sets
// the SPI communcation rate between card and microcontroller.  Returns TRUE
// on success and zero on error (in which case sd_card_last_error() can
// be called).  If SD_CARD_USE_TIMER0_FOR_TIMEOUTS is defined, this calls
// time0_stopwatch_init() from the timer0_stopwatch.h interface, which
// uses an interrupt.  It also call spi_init() from the spi.h interface,
// with the SPI settings required for communicating with an SD card.
// If you're talking to multiple SPI devices, you may need to change the
// SPI settings to talk to them, then call this function again when you
// want to talk to the SD card more.  This should work fine.  Hopefully :)
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

// Read a block of data.  The block argument is the logical block to
// read, and the data read is stores at dst (which must be at least
// SD_CARD_BLOCK_SIZE bytes long).  On success, TRUE is returned, otherwise
// FALSE is returned (in which case sd_card_last_error() may be called.
// See also sd_card_read_partial_block().
uint8_t
sd_card_read_block (uint32_t block, uint8_t *dst);

// Write a block of data (but see SD_CARD_PROTECT_BLOCK_ZERO).  The block
// argument is the logical block to write, and the data to write is
// taken from location src (which must be at least SD_CARD_BLOCK_SIZE
// bytes long.  On success, TRUE is returned, otherwise FALSE
// is returned and sd_card_last_error() may be called.  See also
// sd_card_write_partial_block().
uint8_t
sd_card_write_block (uint32_t block, uint8_t const *src);

// Like sd_card_read_block(), but src only needs to be cnt bytes long,
// and the remainder of the data read from the SD card is thrown away.
// This function lets you trade storage efficiency on the SD card for RAM
// on the AVR (because the memory space pointed to by src can be smaller).
// NOTE: because SDHC cards *always* read/write (and send via SPI) full
// blocks, it will take just as long to retrieve a partial block as it does
// to retrieve a full one.
uint8_t
sd_card_read_partial_block (uint32_t block, uint16_t cnt, uint8_t *dst);

// Analagous to sd_card_read_partial_block().  Garbage data is written for
// (0-indexed) bytes cnt through SD_CARD_BLOCK_SIZE - 1.  NOTE: because SDHC
// cards *always* read/write (and send via SPI) full blocks, it will take
// just as long to retrieve a partial block as it does to retrieve a full one.
uint8_t
sd_card_write_partial_block (uint32_t block, uint16_t cnt, uint8_t const *src);

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
