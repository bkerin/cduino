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
//#include <Arduino.h>
#include "sd_card.h"

#include "dio.h"
#include "spi.h"
#include "timer0_stopwatch.h"

#define SD_CARD_SPI_SLAVE_SELECT_INIT(for_input, enable_pullup, initial_value) \
  DIO_INIT ( \
      SD_CARD_SPI_SLAVE_SELECT_PIN, for_input, enable_pullup, initial_value )
#define SD_CARD_SPI_SLAVE_SELECT_SET_LOW() \
  DIO_SET_LOW (SD_CARD_SPI_SLAVE_SELECT_PIN)
#define SD_CARD_SPI_SLAVE_SELECT_SET_HIGH() \
  DIO_SET_HIGH (SD_CARD_SPI_SLAVE_SELECT_PIN)

static uint32_t cur_block;   // Current block
static sd_card_error_t cur_error;   // Current error (might be none)
static uint8_t in_block;   // True iff we are in the process of reading a block
static uint16_t cur_offset;   // Offset within current block
// FIXME: the interface fctn to enable this mode currently doesn't exist, could
// be included from model easily enough.
static uint8_t partial_block_read_mode;   // Mode supporting partai block reads
static uint8_t status;   // SD controller status
static sd_card_type_t card_type;   // Type of installed SD card

static void
set_type (sd_card_type_t type)
{
  card_type = type;
}

//------------------------------------------------------------------------------
// functions for hardware SPI
/** Send a byte to the card */
static
void spiSend (uint8_t b)
{
  SPDR = b;
  while ( ! (SPSR & (1 << SPIF)) ) {
    ;
  }
}
/** Receive a byte from the card */
static
uint8_t spiRec (void)
{
  spiSend(0XFF);
  return SPDR;
}

static void
error (sd_card_error_t code)
{
  cur_error = code;
}

// Elapsed milliseconds since most recent timer0_stopwatch_reset().
#define EMILLIS() (timer0_stopwatch_microseconds () / 1000)

static uint8_t
wait_not_busy (uint16_t timeoutMillis)
{
  // Wait for card to go not busy.

  timer0_stopwatch_reset ();
  do {
    if (spiRec() == 0XFF) {
      return TRUE;
    }
    else {
      ;
    }
  } while ( EMILLIS () < timeoutMillis );
  
  return FALSE;
}

static void
read_end (void) {
    // Read any remaining block data and checksum, set chip select high,
    // and clear the in_block flag.

  if ( in_block ) {
      // Skip data and crc
#ifdef OPTIMIZE_HARDWARE_SPI
    // optimize skip for hardware
    SPDR = 0XFF;
    while ( cur_offset++ < 513 ) {
      while ( ! (SPSR & (1 << SPIF)) ) {
        ;
      }
      SPDR = 0XFF;
    }
    // Wait for last crc byte
    while ( ! (SPSR & (1 << SPIF)) ) {
      ;
    }
#else  // OPTIMIZE_HARDWARE_SPI
    while ( cur_offset++ < 514 ) {
      spiRec ();
    }
#endif  // OPTIMIZE_HARDWARE_SPI
    SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();
    in_block = 0;
  }
}

//------------------------------------------------------------------------------
static uint8_t
card_command (uint8_t cmd, uint32_t arg) {
  // Send command and return error code, or zero for OK.

  // Ensure read is done (in case we're in partial_block_read_mode mode)
  read_end ();

  // Select card
  SD_CARD_SPI_SLAVE_SELECT_SET_LOW ();

  // Wait up to 300 ms if busy
  wait_not_busy (300);

  // Send command
  spiSend (cmd | 0x40);

  // Send argument
  for ( int8_t s = 24 ; s >= 0 ; s -= 8 ) {
    spiSend (arg >> s);
  }

  // Send CRC
  uint8_t crc = 0XFF;
  if (cmd == CMD0) crc = 0X95;  // correct crc for CMD0 with arg 0
  if (cmd == CMD8) crc = 0X87;  // correct crc for CMD8 with arg 0X1AA
  spiSend(crc);

  // Wait for response
  for ( uint8_t ii = 0 ; ((status = spiRec()) & 0X80) && ii != 0XFF ; ii++ ) {
    ;
  }

  return status;
}

static uint8_t
write_data_private (uint8_t token, const uint8_t* src)
{
  // Send one block of data for write block or write multiple blocks.

#ifdef OPTIMIZE_HARDWARE_SPI

  // Send data - optimized loop
  SPDR = token;

  // Send two byte per iteration
  for ( uint16_t ii = 0; ii < 512; ii += 2 ) {
    while ( ! (SPSR & (1 << SPIF)) ) {
      ;
    }
    SPDR = src[ii];
    while ( ! (SPSR & (1 << SPIF)) ) {
      ;
    }
    SPDR = src[ii + 1];
  }

  // wait for last data byte
  while ( ! (SPSR & (1 << SPIF)) ) {
    ;
  }

#else  // ! OPTIMIZE_HARDWARE_SPI

  spiSend(token);
  for (uint16_t i = 0; i < 512; i++) {
    spiSend(src[i]);
  }

#endif  // end of if-else on OPTIMIZE_HARDWARE_SPI

  spiSend(0xff);  // dummy crc
  spiSend(0xff);  // dummy crc

  status = spiRec();
  if ( (status & DATA_RES_MASK) != DATA_RES_ACCEPTED ) {
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

  timer0_stopwatch_reset ();
  while ( (status = spiRec()) == 0XFF ) {
    if ( EMILLIS () > SD_READ_TIMEOUT ) {
      error (SD_CARD_ERROR_READ_TIMEOUT);
      goto fail;
    }
  }
  if ( status != DATA_START_BLOCK ) {
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
  // transfer data
  for (uint16_t i = 0; i < 16; i++) dst[i] = spiRec();
  spiRec();  // get first crc byte
  spiRec();  // get second crc byte
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
  csd_t csd;
  if (!sd_card_read_csd (&csd)) {
    return 0;
  }
  else {
  }
  if (csd.v1.csd_ver == 0) {
    uint8_t read_bl_len = csd.v1.read_bl_len;
    uint16_t c_size = (csd.v1.c_size_high << 10)
                      | (csd.v1.c_size_mid << 2) | csd.v1.c_size_low;
    uint8_t c_size_mult = (csd.v1.c_size_mult_high << 1)
                          | csd.v1.c_size_mult_low;
    return (uint32_t)(c_size + 1) << (c_size_mult + read_bl_len - 7);
  } else if (csd.v2.csd_ver == 1) {
    uint32_t c_size = ((uint32_t)csd.v2.c_size_high << 16)
                      | (csd.v2.c_size_mid << 8) | csd.v2.c_size_low;
    return (c_size + 1) << 10;
  } else {
    error(SD_CARD_ERROR_BAD_CSD);
    return 0;
  }
}

uint8_t
sd_card_single_block_erase_supported (void)
{
  csd_t csd;
  return sd_card_read_csd (&csd) ? csd.v1.erase_blk_en : 0;
}

uint8_t
sd_card_erase_blocks (uint32_t firstBlock, uint32_t lastBlock) {
  if ( ! sd_card_single_block_erase_supported () ) {
    error (SD_CARD_ERROR_ERASE_SINGLE_BLOCK);
    goto fail;
  }
  if (card_type != SD_CARD_TYPE_SDHC) {
    firstBlock <<= 9;
    lastBlock <<= 9;
  }
  if (card_command (CMD32, firstBlock)
    || card_command (CMD33, lastBlock)
    || card_command (CMD38, 0)) {
      error(SD_CARD_ERROR_ERASE);
      goto fail;
  }
  if ( ! wait_not_busy (SD_ERASE_TIMEOUT) ) {
    error(SD_CARD_ERROR_ERASE_TIMEOUT);
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
  // There is a class of so-call "application commands" that apparently
  // require an escape command to be sent first.

  card_command (CMD55, 0);
  return card_command (cmd, arg);
}

//------------------------------------------------------------------------------
/**
 * Set the SPI clock rate.
 *
 * \param[in] sckRateID A value in the range [0, 6].
 *
 * The SPI clock will be set to F_CPU/pow(2, 1 + sckRateID). The maximum
 * SPI rate is F_CPU/2 for \a sckRateID = 0 and the minimum rate is F_CPU/128
 * for \a scsRateID = 6.
 *
 * \return The value one, true, is returned for success and the value zero,
 * false, is returned for an invalid value of \a sckRateID.
 */
static uint8_t
setSckRate(uint8_t sckRateID)
{
  if (sckRateID > 6) {
    error(SD_CARD_ERROR_SCK_RATE);
    return FALSE;
  }
  // see avr processor datasheet for SPI register bit definitions
  if ((sckRateID & 1) || sckRateID == 6) {
    SPSR &= ~(1 << SPI2X);
  } else {
    SPSR |= (1 << SPI2X);
  }
  SPCR &= ~((1 <<SPR1) | (1 << SPR0));
  SPCR |= (sckRateID & 4 ? (1 << SPR1) : 0)
    | (sckRateID & 2 ? (1 << SPR0) : 0);
  return TRUE;
}

//------------------------------------------------------------------------------
/**
 * Initialize an SD flash memory card.
 *
 * \param[in] sckRateID SPI clock rate selector. See setSckRate().
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.  The reason for failure
 * can be determined by calling errorCode() and errorData().
 */
uint8_t
sd_card_init (sd_card_spi_speed_t speed) 
{
  timer0_stopwatch_init ();

  cur_error = SD_CARD_ERROR_NONE;
  card_type = SD_CARD_TYPE_INDETERMINATE;
  in_block = partial_block_read_mode = 0;

  timer0_stopwatch_reset ();

  uint32_t arg;

  // FIXME: where exactly is the equivalent of spi_set_data_mode happening,
  // or is the default getting used, or what?  same question for bit order.

  SD_CARD_SPI_SLAVE_SELECT_INIT (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
  // Set pin modes FIXME: WORK POINT: these should go away when we switch
  // to using the spi.h interface entirely.
  SPI_MISO_INIT (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
  SPI_MOSI_INIT (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  SPI_SCK_INIT (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  // SS must be in output mode even if it is not chip select
  SPI_SS_INIT (DIO_OUTPUT, DIO_DONT_CARE, HIGH);

  // Enable SPI, master mode, clock rate f_osc/128
  SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
  // Clear double speed
  SPSR &= ~(1 << SPI2X);

  // Must supply min of 74 clock cycles with CS high
  for ( uint8_t i = 0; i < 10; i++ ) {
    spiSend(0XFF);
  }

  SD_CARD_SPI_SLAVE_SELECT_SET_LOW ();

  // Command to go idle in SPI mode
  while ( (status = card_command (CMD0, 0)) != R1_IDLE_STATE ) {
    if ( EMILLIS () > SD_INIT_TIMEOUT ) {
      error (SD_CARD_ERROR_CMD0);
      goto fail;
    }
  }

  // Check SD version
  if ( (card_command (CMD8, 0x1AA) & R1_ILLEGAL_COMMAND) ) {
    set_type (SD_CARD_TYPE_SD1);
  } else {
    // Only need last byte of r7 response
    for ( uint8_t ii = 0; ii < 4; ii++ ) {
      status = spiRec();
    }
    if ( status != 0XAA ) {
      error (SD_CARD_ERROR_CMD8);
      goto fail;
    }
    set_type (SD_CARD_TYPE_SD2);
  }

  // Initialize card and send host supports SDHC if SD2
  arg = sd_card_type () == SD_CARD_TYPE_SD2 ? 0X40000000 : 0;

  while ( (status = card_application_command (ACMD41, arg))
          != R1_READY_STATE ) {
    if ( EMILLIS () > SD_INIT_TIMEOUT ) {
      error (SD_CARD_ERROR_ACMD41);
      goto fail;
    }
  }
  // If SD2, read OCR register to check for SDHC card
  if ( sd_card_type () == SD_CARD_TYPE_SD2 ) {
    if ( card_command (CMD58, 0) ) {
      error (SD_CARD_ERROR_CMD58);
      goto fail;
    }
    if ( (spiRec () & 0XC0) == 0XC0 ) {
      set_type (SD_CARD_TYPE_SDHC);
    }
    // Discard rest of ocr - contains allowed voltage range
    for ( uint8_t i = 0; i < 3; i++ ) {
      spiRec ();
    }
  }

  SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();

  return setSckRate (speed);

 fail:
  SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();
  return FALSE;
}

//------------------------------------------------------------------------------
/**
 * Read part of a 512 byte block from an SD card.
 *
 * \param[in] block Logical block to be read.
 * \param[in] offset Number of bytes to skip at start of block
 * \param[out] dst Pointer to the location that will receive the data.
 * \param[in] count Number of bytes to read
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
static uint8_t
read_data (uint32_t block, uint16_t offset, uint16_t count, uint8_t *dst)
{
  if (count == 0) {
    return TRUE;
  }
  if ( (count + offset) > 512 ) {
    goto fail;
  }
  if ( ! in_block || block != cur_block || offset < cur_offset ) {
    cur_block = block;
    // Use address if not SDHC card
    if ( sd_card_type () != SD_CARD_TYPE_SDHC ) {
      block <<= 9;
    }
    if ( card_command (CMD17, block) ) {
      error (SD_CARD_ERROR_CMD17);
      goto fail;
    }
    if ( ! wait_start_block () ) {
      goto fail;
    }
    cur_offset = 0;
    in_block = 1;
  }

#ifdef OPTIMIZE_HARDWARE_SPI
  // start first spi transfer
  SPDR = 0XFF;

  // skip data before offset
  for ( ; cur_offset < offset ; cur_offset++ ) {
    while ( ! (SPSR & (1 << SPIF)) ) {
      ;
    }
    SPDR = 0XFF;
  }
  // transfer data
  uint16_t n = count - 1;
  for ( uint16_t ii = 0; ii < n; ii++ ) {
    // FIXME: can these all be replaced with loop_until_bit_set() calls?
    while ( ! (SPSR & (1 << SPIF)) ) {
      ;
    }
    dst[i] = SPDR;
    SPDR = 0XFF;
  }
  // wait for last byte
  while ( ! (SPSR & (1 << SPIF)) ) {
    ;
  }
  dst[n] = SPDR;

#else  // OPTIMIZE_HARDWARE_SPI

  // Skip data before offset
  for ( ; cur_offset < offset ; cur_offset++ ) {
    spiRec();
  }
  // Transfer data
  for ( uint16_t ii = 0; ii < count; ii++ ) {
    dst[ii] = spiRec();
  }
#endif  // OPTIMIZE_HARDWARE_SPI

  cur_offset += count;
  if ( ! partial_block_read_mode || cur_offset >= 512 ) {
    read_end ();
  }
  return TRUE;

 fail:
  SD_CARD_SPI_SLAVE_SELECT_SET_HIGH ();
  return FALSE;
}

//------------------------------------------------------------------------------
/**
 * Read a 512 byte block from an SD card device.
 *
 * \param[in] block Logical block to be read.
 * \param[out] dst Pointer to the location that will receive the data.

 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t
sd_card_read_block (uint32_t block, uint8_t* dst)
{
  return read_data (block, 0, 512, dst);
}

//------------------------------------------------------------------------------
/**
 * Writes a 512 byte block to an SD card.
 *
 * \param[in] blockNumber Logical block to be written.
 * \param[in] src Pointer to the location of the data to be written.
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.
 */
uint8_t
sd_card_write_block (uint32_t blockNumber, const uint8_t* src)
{
#if SD_PROTECT_BLOCK_ZERO
  // don't allow write to first block
  if ( blockNumber == 0 ) {
    error (SD_CARD_ERROR_WRITE_BLOCK_ZERO);
    goto fail;
  }
#endif  // SD_PROTECT_BLOCK_ZERO

  // Use address if not SDHC card
  if ( sd_card_type () != SD_CARD_TYPE_SDHC ) {
    blockNumber <<= 9;
  }
  if ( card_command (CMD24, blockNumber) ) {
    error (SD_CARD_ERROR_CMD24);
    goto fail;
  }
  if ( ! write_data_private (DATA_START_BLOCK, src) ) {
    goto fail;
  }

  // Wait for flash programming to complete
  if ( ! wait_not_busy (SD_WRITE_TIMEOUT) ) {
    error (SD_CARD_ERROR_WRITE_TIMEOUT);
    goto fail;
  }

  // Response is r2, so get and check two bytes for nonzero
  if ( card_command (CMD13, 0) || spiRec () ) {
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
sd_card_read_cid (cid_t *cid)
{
  return read_register (CMD10, cid);
}

uint8_t
sd_card_read_csd (csd_t *csd)
{
  return read_register (CMD9, csd);
}
