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
#include <Arduino.h>
#include "Sd2Card.h"

#include "dio.h"

static uint32_t block_;
static uint8_t chipSelectPin_;
static sd_card_error_t errorCode_;
static uint8_t inBlock_;
static uint16_t offset_;
static uint8_t partialBlockRead_;
static uint8_t status_;
static sd_card_type_t type_;

static void
set_type (sd_card_type_t type)
{
  type_ = type;
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

//------------------------------------------------------------------------------
static void
chipSelectHigh(void) {
  digitalWrite(chipSelectPin_, HIGH);
}

//------------------------------------------------------------------------------
static void
chipSelectLow(void) {
  digitalWrite(chipSelectPin_, LOW);
}

static void
error (sd_card_error_t code)
{
  errorCode_ = code;
}

//------------------------------------------------------------------------------
// wait for card to go not busy
static uint8_t
waitNotBusy(uint16_t timeoutMillis) {
  uint16_t t0 = millis();
  do {
    if (spiRec() == 0XFF) {
      return true;
    }
    else {
    }
  }
  while (((uint16_t)millis() - t0) < timeoutMillis);
  return false;
}

//------------------------------------------------------------------------------
/** Skip remaining data in a block when in partial block read mode. */
static void
readEnd(void) {
  if (inBlock_) {
      // skip data and crc
#ifdef OPTIMIZE_HARDWARE_SPI
    // optimize skip for hardware
    SPDR = 0XFF;
    while (offset_++ < 513) {
      while ( ! (SPSR & (1 << SPIF)) ) {
        ;
      }
      SPDR = 0XFF;
    }
    // wait for last crc byte
    while ( ! (SPSR & (1 << SPIF)) ) {
      ;
    }
#else  // OPTIMIZE_HARDWARE_SPI
    while ( offset_++ < 514 ) {
      spiRec();
    }
#endif  // OPTIMIZE_HARDWARE_SPI
    chipSelectHigh();
    inBlock_ = 0;
  }
}

//------------------------------------------------------------------------------
// send command and return error code.  Return zero for OK
static uint8_t
cardCommand(uint8_t cmd, uint32_t arg) {


  // end read if in partialBlockRead mode
  readEnd();


  // select card
  chipSelectLow();


  // wait up to 300 ms if busy
  waitNotBusy(300);


  // send command
  spiSend(cmd | 0x40);


  // send argument
  for (int8_t s = 24; s >= 0; s -= 8) spiSend(arg >> s);


  // send CRC
  uint8_t crc = 0XFF;
  if (cmd == CMD0) crc = 0X95;  // correct crc for CMD0 with arg 0
  if (cmd == CMD8) crc = 0X87;  // correct crc for CMD8 with arg 0X1AA
  spiSend(crc);

  // wait for response
  for ( uint8_t i = 0 ; ((status_ = spiRec()) & 0X80) && i != 0XFF ; i++ ) {
    ;
  }

  return status_;
}

//------------------------------------------------------------------------------
// send one block of data for write block or write multiple blocks
static uint8_t
writeData_private (uint8_t token, const uint8_t* src)
{
#ifdef OPTIMIZE_HARDWARE_SPI

  // send data - optimized loop
  SPDR = token;

  // send two byte per iteration
  for (uint16_t i = 0; i < 512; i += 2) {
    while ( ! (SPSR & (1 << SPIF)) ) {
      ;
    }
    SPDR = src[i];
    while ( ! (SPSR & (1 << SPIF)) ) {
      ;
    }
    SPDR = src[i+1];
  }

  // wait for last data byte
  while ( ! (SPSR & (1 << SPIF))){
    ;
  }

#else  // OPTIMIZE_HARDWARE_SPI
  spiSend(token);
  for (uint16_t i = 0; i < 512; i++) {
    spiSend(src[i]);
  }
#endif  // OPTIMIZE_HARDWARE_SPI
  spiSend(0xff);  // dummy crc
  spiSend(0xff);  // dummy crc

  status_ = spiRec();
  if ((status_ & DATA_RES_MASK) != DATA_RES_ACCEPTED) {
    error(SD_CARD_ERROR_WRITE);
    chipSelectHigh();
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
/** Wait for start block token */
static uint8_t
waitStartBlock (void)
{
  uint16_t t0 = millis();
  while ((status_ = spiRec()) == 0XFF) {
    if (((uint16_t)millis() - t0) > SD_READ_TIMEOUT) {
      error(SD_CARD_ERROR_READ_TIMEOUT);
      goto fail;
    }
  }
  if (status_ != DATA_START_BLOCK) {
    error(SD_CARD_ERROR_READ);
    goto fail;
  }
  return true;

 fail:
  chipSelectHigh();
  return false;
}

//------------------------------------------------------------------------------
/** read CID or CSR register */
static uint8_t
readRegister (uint8_t cmd, void* buf)
{
  uint8_t* dst = reinterpret_cast<uint8_t*>(buf);
  if (cardCommand(cmd, 0)) {
    error(SD_CARD_ERROR_READ_REG);
    goto fail;
  }
  if (!waitStartBlock()) goto fail;
  // transfer data
  for (uint16_t i = 0; i < 16; i++) dst[i] = spiRec();
  spiRec();  // get first crc byte
  spiRec();  // get second crc byte
  chipSelectHigh();
  return true;

 fail:
  chipSelectHigh();
  return false;
}

sd_card_error_t
sd_card_last_error (void)
{
  return errorCode_;
}

uint8_t
sd_card_last_error_data (void)
{
  return status_;
}

sd_card_type_t
sd_card_type (void)
{
  return type_;
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
  if (type_ != SD_CARD_TYPE_SDHC) {
    firstBlock <<= 9;
    lastBlock <<= 9;
  }
  if (cardCommand(CMD32, firstBlock)
    || cardCommand(CMD33, lastBlock)
    || cardCommand(CMD38, 0)) {
      error(SD_CARD_ERROR_ERASE);
      goto fail;
  }
  if (!waitNotBusy(SD_ERASE_TIMEOUT)) {
    error(SD_CARD_ERROR_ERASE_TIMEOUT);
    goto fail;
  }
  chipSelectHigh();
  return true;

 fail:
  chipSelectHigh();
  return false;
}

static uint8_t
cardAcmd (uint8_t cmd, uint32_t arg)
{
  cardCommand (CMD55, 0);
  return cardCommand (cmd, arg);
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
setSckRate(uint8_t sckRateID) {
  if (sckRateID > 6) {
    error(SD_CARD_ERROR_SCK_RATE);
    return false;
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
  return true;
}

//------------------------------------------------------------------------------
/**
 * Initialize an SD flash memory card.
 *
 * \param[in] sckRateID SPI clock rate selector. See setSckRate().
 * \param[in] chipSelectPin SD chip select pin number.
 *
 * \return The value one, true, is returned for success and
 * the value zero, false, is returned for failure.  The reason for failure
 * can be determined by calling errorCode() and errorData().
 */
uint8_t
sd_card_init (sd_card_spi_speed_t speed, uint8_t chipSelectPin) 
{
  errorCode_ = SD_CARD_ERROR_NONE;
  type_ = SD_CARD_TYPE_INDETERMINATE;
  inBlock_ = partialBlockRead_ = 0;
  chipSelectPin_ = chipSelectPin;
  // 16-bit init start time allows over a minute
  uint16_t t0 = (uint16_t)millis();
  uint32_t arg;

  // Apparently SS doesn't have to be the same as the chip select pin.  Is
  // there any reason for us to support this, given that we won't support
  // software SPI?
  // set pin modes
  pinMode(chipSelectPin_, OUTPUT);
  chipSelectHigh();

  SPI_MISO_INIT (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);

  SPI_MOSI_INIT (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  SPI_SCK_INIT (DIO_OUTPUT, DIO_DONT_CARE, LOW);

  // SS must be in output mode even it is not chip select
  SPI_SS_INIT (DIO_OUTPUT, DIO_DONT_CARE, HIGH);

  // Enable SPI, Master, clock rate f_osc/128
  SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
  // clear double speed
  SPSR &= ~(1 << SPI2X);

  // must supply min of 74 clock cycles with CS high.
  for (uint8_t i = 0; i < 10; i++) spiSend(0XFF);

  chipSelectLow();

  // command to go idle in SPI mode
  while ((status_ = cardCommand(CMD0, 0)) != R1_IDLE_STATE) {
    if (((uint16_t)millis() - t0) > SD_INIT_TIMEOUT) {
      error(SD_CARD_ERROR_CMD0);
      goto fail;
    }
  }
  // check SD version
  if ((cardCommand(CMD8, 0x1AA) & R1_ILLEGAL_COMMAND)) {
    set_type(SD_CARD_TYPE_SD1);
  } else {
    // only need last byte of r7 response
    for (uint8_t i = 0; i < 4; i++) status_ = spiRec();
    if (status_ != 0XAA) {
      error(SD_CARD_ERROR_CMD8);
      goto fail;
    }
    set_type(SD_CARD_TYPE_SD2);
  }
  // initialize card and send host supports SDHC if SD2
  arg = sd_card_type () == SD_CARD_TYPE_SD2 ? 0X40000000 : 0;

  while ((status_ = cardAcmd(ACMD41, arg)) != R1_READY_STATE) {
    // check for timeout
    if (((uint16_t)millis() - t0) > SD_INIT_TIMEOUT) {
      error(SD_CARD_ERROR_ACMD41);
      goto fail;
    }
  }
  // if SD2 read OCR register to check for SDHC card
  if (sd_card_type () == SD_CARD_TYPE_SD2) {
    if (cardCommand(CMD58, 0)) {
      error(SD_CARD_ERROR_CMD58);
      goto fail;
    }
    if ((spiRec() & 0XC0) == 0XC0) set_type(SD_CARD_TYPE_SDHC);
    // discard rest of ocr - contains allowed voltage range
    for (uint8_t i = 0; i < 3; i++) spiRec();
  }
  chipSelectHigh();

  return setSckRate (speed);

 fail:
  chipSelectHigh();
  return false;
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
readData(uint32_t block, uint16_t offset, uint16_t count, uint8_t* dst)
{
  uint16_t n;
  if (count == 0) return true;
  if ((count + offset) > 512) {
    goto fail;
  }
  if (!inBlock_ || block != block_ || offset < offset_) {
    block_ = block;
    // use address if not SDHC card
    if ( sd_card_type () != SD_CARD_TYPE_SDHC ) {
      block <<= 9;
    }
    if (cardCommand(CMD17, block)) {
      error(SD_CARD_ERROR_CMD17);
      goto fail;
    }
    if (!waitStartBlock()) {
      goto fail;
    }
    offset_ = 0;
    inBlock_ = 1;
  }

#ifdef OPTIMIZE_HARDWARE_SPI
  // start first spi transfer
  SPDR = 0XFF;

  // skip data before offset
  for (;offset_ < offset; offset_++) {
    while ( ! (SPSR & (1 << SPIF)) ) {
      ;
    }
    SPDR = 0XFF;
  }
  // transfer data
  n = count - 1;
  for (uint16_t i = 0; i < n; i++) {
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

  // skip data before offset
  for (;offset_ < offset; offset_++) {
    spiRec();
  }
  // transfer data
  for (uint16_t i = 0; i < count; i++) {
    dst[i] = spiRec();
  }
#endif  // OPTIMIZE_HARDWARE_SPI

  offset_ += count;
  if (!partialBlockRead_ || offset_ >= 512) {
    // read rest of data, checksum and set chip select high
    readEnd();
  }
  return true;

 fail:
  chipSelectHigh();
  return false;
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
  return readData(block, 0, 512, dst);
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
  if (blockNumber == 0) {
    error(SD_CARD_ERROR_WRITE_BLOCK_ZERO);
    goto fail;
  }
#endif  // SD_PROTECT_BLOCK_ZERO

  // use address if not SDHC card
  if (sd_card_type () != SD_CARD_TYPE_SDHC) blockNumber <<= 9;
  if (cardCommand(CMD24, blockNumber)) {
    error(SD_CARD_ERROR_CMD24);
    goto fail;
  }
  if (!writeData_private (DATA_START_BLOCK, src)) goto fail;

  // wait for flash programming to complete
  if (!waitNotBusy(SD_WRITE_TIMEOUT)) {
    error(SD_CARD_ERROR_WRITE_TIMEOUT);
    goto fail;
  }
  // response is r2 so get and check two bytes for nonzero
  if (cardCommand(CMD13, 0) || spiRec()) {
    error(SD_CARD_ERROR_WRITE_PROGRAMMING);
    goto fail;
  }
  chipSelectHigh();
  return true;

 fail:
  chipSelectHigh();
  return false;
}

uint8_t
sd_card_read_cid (cid_t *cid)
{
  return readRegister(CMD10, cid);
}

uint8_t
sd_card_read_csd (csd_t *csd)
{
  return readRegister(CMD9, csd);
}
