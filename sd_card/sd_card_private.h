// This header contains details of SD Card commands, responses, and registers.

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

// vim:foldmethod=marker

// FIXME: rename this header to sd_card_private.h or so?

#ifndef SD_CARD_INFO_H
#define SD_CARD_INFO_H

#include <stdint.h>

#include "util.h"

// Based on the document:
//
// SD Specifications
// Part 1
// Physical Layer
// Simplified Specification
// Version 4.10
// January 22, 2013
//
// References to document sections (e.g. "section 7.42.42") in this source
// file refer to this document.
//
// https://www.sdcard.org/downloads/pls/simplified_specs/part1_410.pdf
//
// NOTE: it may be easier to use one of the interface functions in sd_card.h,
// rather than using sd_card_read_csd() and interpreting its results.

// SD card commands {{{1

// FIXXME: we don't actually use all of these ourselves.  The unused ones
// are untested.

// GO_IDLE_STATE - Init card in spi mode if CS low
#define SD_CARD_CMD0 0x00
// SEND_IF_COND - Verify SD card interface operating condition
#define SD_CARD_CMD8 0x08
// SEND_CSD - Read the Card Specific Data (CSD register)
#define SD_CARD_CMD9 0x09
// SEND_CID - Read the Card Identification Data (CID register)
#define SD_CARD_CMD10 0x0A
// SEND_STATUS - Read the card status register
#define SD_CARD_CMD13 0x0D
// READ_BLOCK - Read a single data block from the card
#define SD_CARD_CMD17 0x11
// WRITE_BLOCK - Write a single data block to the card
#define SD_CARD_CMD24 0x18
// WRITE_MULTIPLE_BLOCK - Write blocks of data until a STOP_TRANSMISSION
#define SD_CARD_CMD25 0x19
// ERASE_WR_BLK_START - Sets the address of the first block to be erased
#define SD_CARD_CMD32 0x20
// ERASE_WR_BLK_END - Sets the address of the last block of the continuous
// range to be erased
#define SD_CARD_CMD33 0x21
// ERASE - Erase all previously selected blocks
#define SD_CARD_CMD38 0x26
// APP_SD_CARD_CMD - Escape for application specific command
#define SD_CARD_CMD55 0x37
// READ_OCR - Read the OCR register of a card
#define SD_CARD_CMD58 0x3A
// SET_WR_BLK_ERASE_COUNT - Set the number of write blocks to be pre-erased
// before writing
#define SD_CARD_ACMD23 0x17
// SD_SEND_OP_COMD - Sends host capacity support information and activates the
// card's initialization process
#define SD_CARD_ACMD41 0x29

// }}}1

// Card constants, status codes, masks, and other attributes {{{1

// All commands begin with bit values 0 followed by 1 (section 7.3.1.1).
#define SD_CARD_COMMAND_PREFIX_MASK B01000000

// Lengh of the argument part of commands (section 7.3.1.1).
#define SD_CARD_COMMAND_ARGUMENT_BYTES 4

// The SD card will hold its data out line low when busy programming
// (section 7.2.4).
#define SD_CARD_BUSY_SIGNAL_BYTE_VALUE 0x00

// The SD card will hold its data bus line high when no data is transmitted
// (section 4.3.3).
#define SD_CARD_NO_TRANSMISSION_BYTE_VALUE 0xFF

// What we send when we aren't sending actual commands or data.  We often
// don't care what the actual data is when this is used, but sometimes we
// might actually need to be sending this value to make it clear that we
// don't care :)
#define SD_CARD_DUMMY_BYTE_VALUE 0xFF

// The correct CRC value for CMD0 (a constant since CMD0 has no arguments,
// see section 7.2.2).
#define SD_CARD_CMD0_CRC 0x95
// We only support one particular argument value for CMD8.  Other argument
// values aren't needed.  See Physical Layer Specification sections 7.3.1.4
// and 4.3.13 for details.  The 0x01 byte indicates 2.7V to 3.6V range,
// and the AA byte is our check pattern.
#define SD_CARD_CMD8_SUPPORTED_ARGUMENT_VALUE 0x000001AA
// The correct CRC value for CMD8 with the argument we always use with it
#define SD_CARD_CMD8_CRC_FOR_SUPPORTED_ARGUMENT_VALUE 0x87

// The response to CMD8 is of format R7, which is this many bytes long
// (section 7.3.2.6).
#define SD_CARD_R7_BYTES 5
// This (zero-indexed) byte of the CMD8 response contains a field which if
// not all zeros indicates that the supplied voltage is ok (sections 7.3.2.6,
// 7.3.1.4, 4.9.6).
#define SD_CARD_CMD8_VOLTAGE_OK_BYTE 3
// Mask for the bits which must not all be zero if card supports supplied
// voltage (sections 7.3.2.6, 7.3.1.4).
#define SD_CARD_SUPPLIED_VOLTAGE_OK_MASK 0x0F
// The responst to CMD8 is R7, which is 5 bytes long.  This (zero-indexed)
// byte contains the bit pattern we supplied in the last byte of the CMD8
// argument, echoed back (sections 7.3.2.6, 7.3.1.4).
#define SD_CARD_CMD8_PATTERN_ECHO_BACK_BYTE 4
// This is the actual pattern that we supplied which should be echoed back
#define SD_CARD_CMD8_ECHOED_PATTERN 0xAA

// The HCS bit of the CMD41 argument is included to query for an SDHC type
// card All other bits of the CMD41 argument are currently reserved (and
// must be set to zero).  See Physical Layer Specification Table 7-3.
#define SD_CARD_ACMD41_HCS_MASK 0x40000000
#define SD_CARD_ACMD41_NOTHING_MASK 0x00000000

// The response to CMD58 is of format R3, which is this many bytes long
// (section 7.3.2.4).
#define SD_CARD_R3_BYTES 5
// This (zero-indexed) byte of R3 is the first byte of the OCR (section
// 7.3.2.4).
#define SD_CARD_R3_OCR_START_BYTE 1
// These bits of the first byte of the card OCR indicate conditions we care
// about (section 5.1).
#define SD_CARD_OCR_POWERED_UP_MASK B10000000
#define SD_CARD_OCR_CCS_MASK B01000000

// A valid R1 response token byte always has a 0 MSB (see the SD Physical
// Layer Simplified Specification Version 4.10, section 7.3.2.1).
#define SD_CARD_NOT_R1_RESPONSE_MASK B1000000

// Status for card in the ready state (section 7.3.2.1).
#define SD_CARD_R1_READY_STATE 0x00
// Status for card in the idle state (section 7.3.2.1).
#define SD_CARD_R1_IDLE_STATE 0x01
// Status bit for illegal command (section 7.3.2.1).
#define SD_CARD_R1_ILLEGAL_COMMAND 0x04

// Start data token for read or write single bloc (section 7.3.3.2).
#define SD_CARD_DATA_START_BLOCK 0xFE
// Mask for data response tokens after a write block operation (section
// 7.3.3.1).
#define SD_CARD_DATA_RES_MASK 0x1F
// Write data accepted token (section 7.3.3.1).
#define SD_CARD_DATA_RES_ACCEPTED 0x05

// }}}1

// Card CID and CSD register descriptions {{{1

// Card Identification (CID) register (section 5.2).
typedef struct sd_card_cid {
  // Byte 0
  uint8_t mid;  // Manufacturer ID
  // Byte 1-2
  char oid[2];  // OEM/Application ID
  // Byte 3-7
  char pnm[5];  // Product name
  // Byte 8
  unsigned prv_m : 4;  // Product revision n.m
  unsigned prv_n : 4;
  // Byte 9-12
  uint32_t psn;  // Product serial number
  // Byte 13
  unsigned mdt_year_high : 4;  // Manufacturing date
  unsigned reserved : 4;
  // Byte 14
  unsigned mdt_month : 4;
  unsigned mdt_year_low :4;
  // Byte 15
  unsigned always1 : 1;
  unsigned crc : 7;
} sd_card_cid_t;

// Card-Specific Data (CSD) register (section 5.3).  see also the higher-level
// (partial) interface functions in sd_card.h.  Note that there are two
// versions of this structure each used by a different SD card version,
// this is the version 1 form.
typedef struct sd_card_csd1 {
  // Byte 0
  unsigned reserved1 : 6;
  unsigned csd_ver : 2;
  // Byte 1
  uint8_t taac;
  // Byte 2
  uint8_t nsac;
  // Byte 3
  uint8_t tran_speed;
  // Byte 4
  uint8_t ccc_high;
  // Byte 5
  unsigned read_bl_len : 4;
  unsigned ccc_low : 4;
  // Byte 6
  unsigned c_size_high : 2;
  unsigned reserved2 : 2;
  unsigned dsr_imp : 1;
  unsigned read_blk_misalign :1;
  unsigned write_blk_misalign : 1;
  unsigned read_bl_partial : 1;
  // Byte 7
  uint8_t c_size_mid;
  // Byte 8
  unsigned vdd_r_curr_max : 3;
  unsigned vdd_r_curr_min : 3;
  unsigned c_size_low :2;
  // Byte 9
  unsigned c_size_mult_high : 2;
  unsigned vdd_w_cur_max : 3;
  unsigned vdd_w_curr_min : 3;
  // Byte 10
  unsigned sector_size_high : 6;
  unsigned erase_blk_en : 1;
  unsigned c_size_mult_low : 1;
  // Byte 11
  unsigned wp_grp_size : 7;
  unsigned sector_size_low : 1;
  // Byte 12
  unsigned write_bl_len_high : 2;
  unsigned r2w_factor : 3;
  unsigned reserved3 : 2;
  unsigned wp_grp_enable : 1;
  // Byte 13
  unsigned reserved4 : 5;
  unsigned write_partial : 1;
  unsigned write_bl_len_low : 2;
  // Byte 14
  unsigned reserved5: 2;
  unsigned file_format : 2;
  unsigned tmp_write_protect : 1;
  unsigned perm_write_protect : 1;
  unsigned copy : 1;
  unsigned file_format_grp : 1;
  // Byte 15
  unsigned always1 : 1;
  unsigned crc : 7;
} sd_card_csd1_t;

// Card-Specific Data (CSD) register (section 5.3).  see also the higher-level
// (partial) interface functions in sd_card.h.  Note that there are two
// versions of this structure each used by a different SD card version,
// this is the version 2 form.
typedef struct sd_card_csd2 {
  // Byte 0
  unsigned reserved1 : 6;
  unsigned csd_ver : 2;
  // Byte 1
  uint8_t taac;
  // Byte 2
  uint8_t nsac;
  // Byte 3
  uint8_t tran_speed;
  // Byte 4
  uint8_t ccc_high;
  // Byte 5
  unsigned read_bl_len : 4;
  unsigned ccc_low : 4;
  // Byte 6
  unsigned reserved2 : 4;
  unsigned dsr_imp : 1;
  unsigned read_blk_misalign :1;
  unsigned write_blk_misalign : 1;
  unsigned read_bl_partial : 1;
  // Byte 7
  unsigned reserved3 : 2;
  unsigned c_size_high : 6;
  // Byte 8
  uint8_t c_size_mid;
  // Byte 9
  uint8_t c_size_low;
  // Byte 10
  unsigned sector_size_high : 6;
  unsigned erase_blk_en : 1;
  unsigned reserved4 : 1;
  // Byte 11
  unsigned wp_grp_size : 7;
  unsigned sector_size_low : 1;
  // Byte 12
  unsigned write_bl_len_high : 2;
  unsigned r2w_factor : 3;
  unsigned reserved5 : 2;
  unsigned wp_grp_enable : 1;
  // Byte 13
  unsigned reserved6 : 5;
  unsigned write_partial : 1;
  unsigned write_bl_len_low : 2;
  // Byte 14
  unsigned reserved7: 2;
  unsigned file_format : 2;
  unsigned tmp_write_protect : 1;
  unsigned perm_write_protect : 1;
  unsigned copy : 1;
  unsigned file_format_grp : 1;
  // Byte 15
  unsigned always1 : 1;
  unsigned crc : 7;
} sd_card_csd2_t;

// Union of old and new style CSD register types.
typedef union {
  sd_card_csd1_t v1;
  sd_card_csd2_t v2;
} sd_card_csd_t;

// }}}1

#endif  // SD_CARD_INFO_H
