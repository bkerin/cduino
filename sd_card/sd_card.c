// Implementation of the interface described in sd_card.h.

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

#include <assert.h>

#include "sd_card.h"

#include "dio.h"
#include "spi.h"

#define SD_CARD_SPI_SLAVE_SELECT_INIT(for_input, enable_pullup, initial_value) \
  DIO_INIT ( \
      SD_CARD_SPI_SLAVE_SELECT_PIN, for_input, enable_pullup, initial_value )
#define SD_CARD_SPI_SLAVE_SELECT_SET_LOW() \
  DIO_SET_LOW (SD_CARD_SPI_SLAVE_SELECT_PIN)
#define SD_CARD_SPI_SLAVE_SELECT_SET_HIGH() \
  DIO_SET_HIGH (SD_CARD_SPI_SLAVE_SELECT_PIN)

// SD Card Bytes Per Millisecond, Very Approximately.  Note that this
// has nothing to do with the actual data rate the card can accept when
// programming or reading data, but is just the bus rate at which it
// communicates.  Note also that the numeric interpretation of the speed
// enumerated type value is a bit counterintuitive.  We need the fairly
// wide uint32_t here because some of timeouts (erase for example) are
// quite long).  There is some untested support for the low-poer on-chip
// 16 kHz oscillator here as well.
#if F_CPU == 16000
#  error This situation is untested.  Should work, but needs testing
#  define SDCBPMSVA \
  ((float) F_CPU / ((uint16_t) BITS_PER_BYTE * MS_PER_S * speed))
#else // This is the branch where F_CPU != 16000
#  define SDCBPMSVA \
  (F_CPU / ((uint32_t) BITS_PER_BYTE * MS_PER_S * speed))
#endif // F_CPU != 16000

// Timeouts, expressed in terms of bytes transferred on the SPI bus.
// FIXME: WORK POINT: sanity check these values
#define SD_CARD_PRECMD_TIMEOUT_BYTES (SD_CARD_PRECMD_TIMEOUT * SDCBPMSVA)
#define SD_CARD_INIT_TIMEOUT_BYTES   (SD_CARD_INIT_TIMEOUT   * SDCBPMSVA)
#define SD_CARD_ERASE_TIMEOUT_BYTES  (SD_CARD_ERASE_TIMEOUT  * SDCBPMSVA)
#define SD_CARD_READ_TIMEOUT_BYTES   (SD_CARD_READ_TIMEOUT   * SDCBPMSVA)
#define SD_CARD_WRITE_TIMEOUT_BYTES  (SD_CARD_WRITE_TIMEOUT  * SDCBPMSVA)

static uint32_t cur_block;   // Current block
static sd_card_error_t cur_error;   // Current error (might be none)
static uint8_t in_block;   // True iff we are in the process of reading a block
static uint16_t cur_offset;   // Offset within current block
// FIXXME: the interface fctn to enable this mode currently doesn't
// exist, could be included from model easily enough.  I don't need
// it though.  Also, I'm not sure it's even supported by SDHC cards, it
// may be an SD1 and SD2 only thing.  NOTE: this it *not* related to the
// sd_card_read/write_partial_block() functions.
static uint8_t partial_block_read_mode;   // Mode supporting partai block reads
static uint8_t status;   // SD controller status
static sd_card_type_t card_type;   // Type of installed SD card

sd_card_spi_speed_t speed = SD_CARD_SPI_SPEED_UNSET;

static
void send_byte (uint8_t bts)
{
  // Send byte bts (Byte To Send) to the SD controller

  spi_transfer (bts);
}

static
uint8_t receive_byte (void)
{
  // Receive a byte from the SD controller

  return spi_transfer (SD_CARD_DUMMY_BYTE_VALUE);
}

static void
error (sd_card_error_t code)
{
  // Set the current error code

  cur_error = code;
}

// Elapsed milliseconds since most recent timer0_stopwatch_reset().
#define EMILLIS() (timer0_stopwatch_microseconds () / 1000)

static uint8_t
wait_not_busy (uint16_t timeout_ms)
{
  // Wait timeout_ms milliseconds for card to go not busy.

#ifdef SD_CARD_USE_TIMER0_FOR_TIMEOUTS
  timer0_stopwatch_reset ();
  do {
    // Wait for card to go non-busy (to get done programming).
    if ( receive_byte () != SD_CARD_BUSY_SIGNAL_BYTE_VALUE ) {
      return TRUE;
    }
    else {
      ;
    }
  } while ( EMILLIS () < timeout_ms );
#else // The case where SD_CARD_USE_TIMER0_FOR_TIMEOUTS isn't defined
  uint32_t byte_count = 0;
  do {
    // Wait for card to go non-busy (to get done programming).
    if ( receive_byte () != SD_CARD_BUSY_SIGNAL_BYTE_VALUE ) {
      return TRUE;
    }
    else {
      byte_count++;
    }
  } while ( byte_count < timeout_ms * SDCBPMSVA );
#endif // SD_CARD_USE_TIMER0_FOR_TIMEOUTS
  
  return FALSE;
}

static void
read_end (void)
{
  // Read any remaining block data and checksum, set chip select high,
  // and clear the in_block flag.

  if ( in_block ) {
    // Skip data (hence +1) and CRC (hence other +1) bytes
    while ( cur_offset++ < SD_CARD_BLOCK_SIZE + 1 + 1 ) {
      receive_byte ();
    }

    SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();
    in_block = FALSE;
  }
}

// FIXME: get rid of this debug storage (and use of it) when all response
// codes are sorted (we might just have got there already, I think).
uint8_t sp = 0;
uint8_t byte_stream[UINT8_MAX];
uint8_t lc;

//------------------------------------------------------------------------------
static uint8_t
card_command (uint8_t cmd, uint32_t arg)
{
  // WARNING: CMD8 is a special case.  Send command and argument and return
  // error code, or zero for OK.  If cmd is SD_CARD_CMD8, then arg is required
  // to be SD_CARD_CMD8_SUPPORTED_ARGUMENT_VALUE.

  // Ensure read is done (in case we're in partial_block_read_mode mode)
  read_end ();

  // Select card
  SD_CARD_SPI_SLAVE_SELECT_SET_LOW ();

  // Wait for a bit if the card is busy.  Presumably some subsequent command
  // will fail and we'll get an error of some sort if this wait is ever
  // actually required and the card doesn't ever go non-buys.
  wait_not_busy (SD_CARD_PRECMD_TIMEOUT);

  // Send command
  send_byte (SD_CARD_COMMAND_PREFIX_MASK | cmd);

  // Send argument bytes
  uint8_t const bpp = 8;   // Bits per byte
  for ( int8_t shift = (SD_CARD_COMMAND_ARGUMENT_BYTES - 1) * bpp ;
        shift >= 0 ;
        shift -= bpp ) {
    send_byte (arg >> shift);
  }

  // Due to CRC requirements and the fact that CMD8 is a funny beast that
  // only needs a single particular argument, this function requires exactly
  // that argument to be supplied.
  if ( cmd == SD_CARD_CMD8 ) {
    assert (arg == SD_CARD_CMD8_SUPPORTED_ARGUMENT_VALUE);
  }

  // Send CRC bytes.  Normally we don't bother to send real CRC values (we
  // assume reliable SPI bus operation).  However the SD Physical Layer
  // Simplified Specification section 7.2.2 says that correct CRC values
  // must always be sent for CMD0 and CMD8.
  uint8_t crc = SD_CARD_DUMMY_BYTE_VALUE;
  if ( cmd == SD_CARD_CMD0 ) {
    crc = SD_CARD_CMD0_CRC;  // Correct CRC for CMD0 with arg 0
  }
  if ( cmd == SD_CARD_CMD8 ) {
    crc = SD_CARD_CMD8_CRC_FOR_SUPPORTED_ARGUMENT_VALUE;
  }
  send_byte (crc);

  // Wait for response (checking a maximum of Maximum Response Checks times)
  sp = 0;
  lc = cmd;
  uint8_t const mrc = UINT8_MAX;   // Maximum Response Checks
  for ( uint8_t ii = 0 ;
        ((status = receive_byte ()) & SD_CARD_NOT_R1_RESPONSE_MASK) &&
        ii != mrc ;
        ii++ ) {
    byte_stream[sp++] = status;
    ;
  }
  byte_stream[sp++] = status;

  return status;
}

static uint8_t
write_data_private (uint8_t token, uint16_t cnt, uint8_t const *src)
{
  // Send one block of data for write block or write multiple blocks.

  send_byte (token);

  // Send the real data
  uint16_t ii;   // Byte index
  for ( ii = 0 ; ii < cnt ; ii++ ) {
    send_byte (src[ii]);
  }
  // Send dummy data for the remainder of the block.  FIXXME: is there really
  // no way with SDHC SPI to specify that the we don't want to send the rest
  // of the block?
  for ( ; ii < SD_CARD_BLOCK_SIZE ; ii++ ) {
    send_byte (SD_CARD_DUMMY_BYTE_VALUE);
  }

  send_byte (SD_CARD_DUMMY_BYTE_VALUE);  // Dummy CRC
  send_byte (SD_CARD_DUMMY_BYTE_VALUE);  // Dummy CRC

  status = receive_byte ();
  if ( (status & SD_CARD_DATA_RES_MASK) != SD_CARD_DATA_RES_ACCEPTED ) {
    error (SD_CARD_ERROR_WRITE);
    SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();
    return FALSE;
  }
  return TRUE;
}

static uint8_t
wait_start_block (void)
{
  // Wait for start block token.
#ifdef SD_CARD_USE_TIMER0_FOR_TIMEOUTS
  timer0_stopwatch_reset ();
  while ( (status = receive_byte ()) == SD_CARD_NO_TRANSMISSION_BYTE_VALUE ) {
    if ( EMILLIS () > SD_CARD_READ_TIMEOUT ) {
      error (SD_CARD_ERROR_READ_TIMEOUT);
      goto fail;
    }
  }
#else // The case where SD_CARD_USE_TIMER0_FOR_TIMEOUTS isn't defined
  uint32_t byte_count = 0;
  while ( (status = receive_byte ()) == SD_CARD_NO_TRANSMISSION_BYTE_VALUE ) {
    if ( byte_count >= SD_CARD_READ_TIMEOUT_BYTES ) {
      error (SD_CARD_ERROR_READ_TIMEOUT);
      goto fail;
    }    
    byte_count++;
  }
#endif // SD_CARD_USE_TIMER0_FOR_TIMEOUTS

  if ( status != SD_CARD_DATA_START_BLOCK ) {
    error (SD_CARD_ERROR_READ);
    goto fail;
  }

  return TRUE;

  fail:
  SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();
  return FALSE;
}

static uint8_t
read_register (uint8_t cmd, void* buf)
{
  // Read CID or CSR register.

  uint8_t* dst = (uint8_t *) buf;
  if ( card_command (cmd, 0) ) {
    error (SD_CARD_ERROR_READ_REG);
    goto fail;
  }

  if ( ! wait_start_block () ) {
    goto fail;
  }

  // Transfer data
  for ( uint16_t ii = 0; ii < 16; ii++ ) {
    dst[ii] = receive_byte ();
  }

  receive_byte ();  // Get first CRC byte
  receive_byte ();  // Get second CRC byte

  SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();
  return TRUE;

  fail:
  SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();
  return FALSE;
}

sd_card_error_t
sd_card_last_error (void)
{
  return cur_error;
}

uint8_t
sd_card_last_error_data (void)
{
  return status;
}

sd_card_type_t
sd_card_type (void)
{
  return card_type;
}

uint32_t
sd_card_size (void)
{
  sd_card_csd_t csd;

  if ( ! sd_card_read_csd (&csd) ) {
    return 0;
  }

  if (csd.v1.csd_ver == 0) {
    uint8_t read_bl_len = csd.v1.read_bl_len;
    uint16_t c_size =
      (csd.v1.c_size_high << 10) |
      (csd.v1.c_size_mid << 2) |
      csd.v1.c_size_low;
    uint8_t c_size_mult =
      (csd.v1.c_size_mult_high << 1) |
      csd.v1.c_size_mult_low;
    return (uint32_t) (c_size + 1) << (c_size_mult + read_bl_len - 7);
  }
  else if ( csd.v2.csd_ver == 1 ) {
    uint32_t c_size =
      ((uint32_t) csd.v2.c_size_high << 16) |
      (csd.v2.c_size_mid << 8) | csd.v2.c_size_low;
    return (c_size + 1) << 10;
  }
  else {
    error (SD_CARD_ERROR_BAD_CSD);
    return 0;
  }
}

uint8_t
sd_card_single_block_erase_supported (void)
{
  sd_card_csd_t csd;
  return sd_card_read_csd (&csd) ? csd.v1.erase_blk_en : FALSE;
}

uint8_t
sd_card_erase_blocks (uint32_t first_block, uint32_t last_block)
{
  if ( ! sd_card_single_block_erase_supported () ) {
    error (SD_CARD_ERROR_ERASE_SINGLE_BLOCK);
    goto fail;
  }

  if ( card_type != SD_CARD_TYPE_SDHC ) {
    first_block <<= 9;
    last_block <<= 9;
  }

  if ( card_command (SD_CARD_CMD32, first_block) ||
       card_command (SD_CARD_CMD33, last_block) ||
       card_command (SD_CARD_CMD38, 0)) {
      error (SD_CARD_ERROR_ERASE);
      goto fail;
  }

  if ( ! wait_not_busy (SD_CARD_ERASE_TIMEOUT) ) {
    error (SD_CARD_ERROR_ERASE_TIMEOUT);
    goto fail;
  }

  SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();
  return TRUE;

  fail:
  SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();
  return FALSE;
}

static uint8_t
card_application_command (uint8_t cmd, uint32_t arg)
{
  // There is a class of so-call "application commands" that require an
  // escape command to be sent first.

  card_command (SD_CARD_CMD55, 0);
  return card_command (cmd, arg);
}

uint8_t
sd_card_init (sd_card_spi_speed_t speed) 
{
#ifdef SD_CARD_USE_TIMER0_FOR_TIMEOUTS
  timer0_stopwatch_init ();
#endif // SD_CARD_USE_TIMER0_FOR_TIMEOUTS

  cur_error = SD_CARD_ERROR_NONE;
  card_type = SD_CARD_TYPE_INDETERMINATE;
  in_block = FALSE;
  partial_block_read_mode = FALSE;

  // Initialize SPI interface to the SD card controller.
  SD_CARD_SPI_SLAVE_SELECT_INIT (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  spi_init ();
  spi_set_data_order (SPI_DATA_ORDER_MSB_FIRST);
  spi_set_data_mode (SPI_DATA_MODE_0);
  // We start out talking slow to the SD card.  I'm not sure if we need to,
  // but thats what the Arduino libs do and it seems like a safe choice.
  spi_set_clock_divider (SPI_CLOCK_DIVIDER_DIV128);

  // We must supply a minimum of 74 clock cycles with CS high as per SD
  // Physical Layer Simplified Specification Version 4.10 section 6.4.1.1.
  uint8_t const dbts = 10;   // Dummy Bytes To Send (for >74 cycles, see above)
  for ( uint8_t ii = 0 ; ii < dbts ; ii++ ) {
    send_byte (SD_CARD_DUMMY_BYTE_VALUE);
  }

  SD_CARD_SPI_SLAVE_SELECT_SET_LOW ();

  // Command to go idle in SPI mode.  SD cards go into SPI mode when their
  // CS line is low while CMD0 is performed, and remain in SPI mode until
  // power cycled.
#ifdef SD_CARD_USE_TIMER0_FOR_TIMEOUTS
  while ( (status = card_command (SD_CARD_CMD0, 0)) != SD_CARD_R1_IDLE_STATE ) {
    if ( EMILLIS () > SD_CARD_INIT_TIMEOUT ) {
      error (SD_CARD_ERROR_CMD0_TIMEOUT);
      goto fail;
    }
  }
#else // The case where SD_CARD_USE_TIMER0_FOR_TIMEOUTS isn't defined
  uint32_t byte_count = 0;
  while ( (status = card_command (SD_CARD_CMD0, 0)) != SD_CARD_R1_IDLE_STATE ) {
    if ( byte_count >= SD_CARD_INIT_TIMEOUT_BYTES ) {
      error (SD_CARD_ERROR_CMD0_TIMEOUT);
      goto fail;
    }
    byte_count++;
  }
#endif // SD_CARD_USE_TIMER0_FOR_TIMEOUTS

  // Check SD version
  status = card_command (SD_CARD_CMD8, SD_CARD_CMD8_SUPPORTED_ARGUMENT_VALUE);
  if ( status & SD_CARD_R1_ILLEGAL_COMMAND ) {
    card_type = SD_CARD_TYPE_SD1;
  } else {
    // FIXXME: I think we could get only R1 back if we get a CRC error after
    // CMD8, but since we generally assume those don't happen, and the bit
    // pattern below would probably blow up anyway we probably dont care.
    // And the client should use the watchdog if they are scared of
    // lock-ups :)
    //
    // CMD8 results in a 5 byte R7 response.  The first byte of the response
    // is identical to response type R1 and is consumed by the card_command()
    // function, so we start at byte 1 here.  See table 7.3.1.4 and section
    // 7.3.2.6 from the SD Physical Layer Simplified Specification Version
    // 4.10 for details.
    for ( uint8_t ii = 1; ii < SD_CARD_R7_BYTES; ii++ ) {
      status = receive_byte ();
      if ( ii == SD_CARD_CMD8_VOLTAGE_OK_BYTE ) {
        if ( ! (status && SD_CARD_SUPPLIED_VOLTAGE_OK_MASK) ) {
          error (SD_CARD_ERROR_CMD8);
          goto fail;
        }
      }
      if ( ii == SD_CARD_CMD8_PATTERN_ECHO_BACK_BYTE ) {
        if ( status != SD_CARD_CMD8_ECHOED_PATTERN ) {
          error (SD_CARD_ERROR_CMD8);
          goto fail;
        }
      }
    }
    card_type = SD_CARD_TYPE_SD2;
  }
  
  uint32_t arg;   // Argument to pass to card commands

  // We will initialize the card with ACMD41.  If we have an SD2 card, we want
  // to set a bit in the ACMD41 argument to determine if we have SDHC support.
  if ( sd_card_type () == SD_CARD_TYPE_SD2 ) {
    arg = SD_CARD_ACMD41_HCS_MASK;
  }
  else {
    arg = SD_CARD_ACMD41_NOTHING_MASK;
  }

  // Wait for card to say its ready FIXXME: section 7.2.2 of the SD Physical
  // Layer Simplified Specification Version 4.10 says we should ensure that
  // CRC is on (using CMD59 CRC_ON_OFF) before issuing ACMD41.  We don't.
  // I suppose this makes it possible or more likely that we get an early
  // false ready which somehow ends up locking up the card.  But if we're
  // getting noise on the line we're going to have problems anyway, since
  // there isn't any other error checking going on anywhere (unless the
  // client is doing it themselves, in which case they should also be using
  // the hardware watchdog :).
#ifdef SD_CARD_USE_TIMER0_FOR_TIMEOUTS
  while ( (status = card_application_command (SD_CARD_ACMD41, arg))
          != SD_CARD_R1_READY_STATE ) {
    if ( EMILLIS () > SD_CARD_INIT_TIMEOUT ) {
      error (SD_CARD_ERROR_ACMD41);
      goto fail;
    }
  }
#else // The case where SD_CARD_USE_TIMER0_FOR_TIMEOUTS isn't defined
  byte_count = 0;
  while ( (status = card_application_command (SD_CARD_ACMD41, arg))
          != SD_CARD_R1_READY_STATE ) {
    if ( byte_count >= SD_CARD_INIT_TIMEOUT_BYTES ) {
      error (SD_CARD_ERROR_ACMD41);
      goto fail;
    }
    byte_count++;
  }
#endif // SD_CARD_USE_TIMER0_FOR_TIMEOUTS

  // If SD2, read OCR register to check for SDHC card
  if ( sd_card_type () == SD_CARD_TYPE_SD2 ) {
    if ( card_command (SD_CARD_CMD58, 0) ) {
      error (SD_CARD_ERROR_CMD58);
      goto fail;
    }
    // NOTE: The card_command() functions consumes the first byte
    // of the R3 response, so the next byte recieved will be the
    // SD_CARD_R3_OCR_START_BYTE.
    uint8_t const expected_high_bits
      = SD_CARD_OCR_POWERED_UP_MASK | SD_CARD_OCR_CCS_MASK;
    if ( (receive_byte () & expected_high_bits) == expected_high_bits ) {
      card_type = SD_CARD_TYPE_SDHC;
    }
    // Discard rest of OCR - contains allowed voltage range.  Either we've
    // already checked this in the CMD8 response, or we've got a legacy card
    // and we don't support checking it.  We don't support switching to 1.8
    // volt operation.
    for ( uint8_t ii = SD_CARD_R3_OCR_START_BYTE + 1 ;
          ii < SD_CARD_R3_BYTES ;
          ii++ ) {
      receive_byte ();
    }
  }

  SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();

  switch ( speed ) {
    case SD_CARD_SPI_SPEED_FULL:
      spi_set_clock_divider (SPI_CLOCK_DIVIDER_DIV2);
      speed = SPI_CLOCK_DIVIDER_DIV2;
      break;
    case SD_CARD_SPI_SPEED_HALF:
      spi_set_clock_divider (SPI_CLOCK_DIVIDER_DIV4);
      speed = SPI_CLOCK_DIVIDER_DIV4;
      break;
    case SD_CARD_SPI_SPEED_QUARTER:
      spi_set_clock_divider (SPI_CLOCK_DIVIDER_DIV8);
      speed = SPI_CLOCK_DIVIDER_DIV8;
      break;
    default:
      // This is really a client error and not an SD card problem, but since
      // this interface doesn't use assert() yet it seems like a shame to start
      // doing so.
      error (SD_CARD_ERROR_SCK_RATE);
      return FALSE;
      break;
  }

  return TRUE;

  fail:
  SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();
  return FALSE;
}

static uint8_t
read_data (uint32_t block, uint16_t offset, uint16_t cnt, uint8_t *dst)
{
  // Read part of a block from an SD card.  Parameters:
  //
  //   * block    Logical block to be read.
  //   * offset   Number of bytes to skip at start of block
  //   * dst      Pointer to the location that will receive the data
  //   * cnt    Number of bytes to read
  //
  //  Return value: TRUE for success, FALSE for failure.

  if ( cnt == 0 ) {
    return TRUE;
  }

  if ( (cnt + offset) > SD_CARD_BLOCK_SIZE ) {
    goto fail;
  }

  if ( ! in_block || block != cur_block || offset < cur_offset ) {
    cur_block = block;
    // Use address if not SDHC card
    if ( sd_card_type () != SD_CARD_TYPE_SDHC ) {
      block <<= 9;
    }
    if ( card_command (SD_CARD_CMD17, block) ) {
      error (SD_CARD_ERROR_CMD17);
      goto fail;
    }
    if ( ! wait_start_block () ) {
      goto fail;
    }
    cur_offset = 0;
    in_block = TRUE;
  }

  // Skip data before offset
  for ( ; cur_offset < offset ; cur_offset++ ) {
    receive_byte ();
  }

  // Transfer data
  for ( uint16_t ii = 0; ii < cnt; ii++ ) {
    dst[ii] = receive_byte ();
  }

  cur_offset += cnt;

  if ( ! partial_block_read_mode || cur_offset >= SD_CARD_BLOCK_SIZE ) {
    read_end ();
  }

  return TRUE;

  fail:
  SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();
  return FALSE;
}

uint8_t
sd_card_read_block (uint32_t block, uint8_t *dst)
{
  return read_data (block, 0, SD_CARD_BLOCK_SIZE, dst);
}

uint8_t
sd_card_write_block (uint32_t block, uint8_t const *src)
{
  return sd_card_write_partial_block (block, SD_CARD_BLOCK_SIZE, src);
}

uint8_t
sd_card_read_partial_block (uint32_t block, uint16_t cnt, uint8_t *dst)
{
  return read_data (block, 0, cnt, dst);
}

uint8_t
sd_card_write_partial_block (uint32_t block, uint16_t cnt, uint8_t const *src)
{
  // NOTE: if cnt is SD_CARD_BLOCK_SIZE the entire block is written.

#if SD_CARD_PROTECT_BLOCK_ZERO
  // Don't allow write to first block
  if ( block == 0 ) {
    error (SD_CARD_ERROR_WRITE_BLOCK_ZERO);
    goto fail;
  }
#endif  // SD_CARD_PROTECT_BLOCK_ZERO

  // Use address if not SDHC card
  if ( sd_card_type () != SD_CARD_TYPE_SDHC ) {
    block <<= 9;
  }
  if ( card_command (SD_CARD_CMD24, block) ) {
    error (SD_CARD_ERROR_CMD24);
    goto fail;
  }
  if ( ! write_data_private (SD_CARD_DATA_START_BLOCK, cnt, src) ) {
    goto fail;
  }

  // Wait for flash programming to complete
  if ( ! wait_not_busy (SD_CARD_WRITE_TIMEOUT) ) {
    error (SD_CARD_ERROR_WRITE_TIMEOUT);
    goto fail;
  }

  // Read the status register to verify that the write worked.  The response
  // to this command is in format R2, so get and check two bytes for nonzero.
  if ( card_command (SD_CARD_CMD13, 0) || receive_byte () ) {
    error (SD_CARD_ERROR_WRITE_PROGRAMMING);
    goto fail;
  }

  SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();
  return TRUE;

  fail:
  SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();
  return FALSE;
}

uint8_t
sd_card_read_cid (sd_card_cid_t *cid)
{
  // Read the SD card CID register into *cid.  Return TRUE on success,
  // FALSE otherwise.

  return read_register (SD_CARD_CMD10, cid);
}

uint8_t
sd_card_read_csd (sd_card_csd_t *csd)
{
  // Read the SD card CSD register into *cid.  Return TRUE on success,
  // FALSE otherwise.

  return read_register (SD_CARD_CMD9, csd);
}
