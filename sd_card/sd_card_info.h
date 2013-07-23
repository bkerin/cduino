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

#ifndef SD_CARD_INFO_H
#define SD_CARD_INFO_H

#include <stdint.h>

// Based on the document:
//
// SD Specifications
// Part 1
// Physical Layer
// Simplified Specification
// Version 2.00
// September 25, 2006
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
// SEND_IF_COND - Verify SD Memory Card interface operating condition
#define SD_CARD_CMD8 0x08
// SEND_CSD - Read the Card Specific Data (CSD register)
#define SD_CARD_CMD9 0x09
// SEND_CID - Read the card identification information (CID register)
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

// WORK POINT: FIXME: all these codes need namespace prefixes

// Card status codes and masks {{{1

// Status for card in the ready state
#define SD_CARD_R1_READY_STATE 0x00
// Status for card in the idle state
#define SD_CARD_R1_IDLE_STATE 0x01
// Status bit for illegal command
#define SD_CARD_R1_ILLEGAL_COMMAND 0x04
// Start data token for read or write single bloc
#define SD_CARD_DATA_START_BLOCK 0xFE
// Stop token for write multiple block
#define SD_CARD_STOP_TRAN_TOKEN 0xFD
// Start data token for write multiple block
#define SD_CARD_WRITE_MULTIPLE_TOKEN 0xFC
// Mask for data response tokens after a write block operation
#define SD_CARD_DATA_RES_MASK 0x1F
// Write data accepted token
#define SD_CARD_DATA_RES_ACCEPTED 0x05

// }}}1

// Im not sure what CID stands for exactly.  But everyone calls it that :)
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

// I'm not sure what CSD stands for exactly.  But everyone calls it that:) See
// also the interface functions in sd_card.h.  There are two version of this
// structure depending on the SD card version, this is the version 1 form.
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

// I'm not sure what CSD stands for exactly.  But everyone calls it that:) See
// also the interface functions in sd_card.h.  There are two version of this
// structure depending on the SD card version, this is the version 2 form.
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

#endif  // SD_CARD_INFO_H
