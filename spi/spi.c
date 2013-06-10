/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 * SPI Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include "pins_arduino.h"
#include "SPI.h"

#define SPI_SS_INIT DIO_INIT_DIGITAL_10
#define SPI_SCK_INIT DIO_INIT_DIGITAL_13
#define SPI_MOSI_INIT DIO_INIT_DIGITAL_11

void
spi_init (void)
{
  // Initialize the SS pin for ouput with a HIGH value.  When the SS pin
  // is set as an output, it can be used as a general purpose output port
  // (it doesn't influence SPI overations).  WARNING: if the SS pin ever
  // becomes a LOW INPUT, then SPI automatically switches to slave mode,
  // so the data direction of the SS pin MUST be kept as OUTPUT.
  SPI_SS_INIT (DIO_OUTPUT, DIO_DONT_CARE, HIGH);

  SPCR |= _BV (MSTR);   // Set SPI master mode
  SPCR |= _BV (SPE);    // Enable SPI

  // Set the SCK and MOSI pins as OUTPUTS.  The MISO pin
  // automatically overrides to act as an input.  By doing this
  // AFTER enabling SPI, we avoid accidentally clocking in a single
  // bit since the lines go directly from "input" to SPI control.
  // http://code.google.com/p/arduino/issues/detail?id=888
  SPI_SCK_INIT (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  SPI_MOSI_INIT (DIO_OUTPUT, DIO_DONT_CARE, LOW);
}

void
spi_set_bit_order (spi_bit_order_t bit_order)
{
  assert (0);   // FIXME: untested

  switch ( bit_order ) {
    case SPI_BIT_ORER_LSB_FIRST:
      SPCR |= _BV (DORD);
      break;
    case SPI_BIT_ORDER_MSB_FIRST:
      SPCR &= ~(_BV (DORD));
      break;
    default:
      assert (0);   // Shouldn't be here
      break;
  }
}

#define SPI_MODE_MASK 0x0C  // CPOL = bit 3, CPHA = bit 2 on SPCR

void
spi_set_data_mode (spi_data_mode_t data_mode)
{
  assert (0);   // FIXME: untested
  
  SPCR = (SPCR & ~SPI_MODE_MASK) | ((uint8_t) mode);
}

#define SPI_CLOCK_MASK 0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR

void
spi_set_clock_divider (spi_clock_divider_t divider)
{
  assert (0);   // FIXME: untested

  SPCR = (SPCR & ~SPI_CLOCK_MASK) | (((uint8_t) divider) & SPI_CLOCK_MASK);
  SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | 
         (((uint8_t) rate) >> 2) & SPI_2XCLOCK_MASK);
}

void
spi_attach_interrupt (void)
{
  SPCR |= _BV(SPIE);
}

void
spi_detach_interrupt (void)
{
  SPCR &= ~_BV(SPIE);
}

uint8_t
spi_transfer (uint8_t data)
{
  SPDR = data;
  //loop_until_bit_clear (SPSR, SPIF);
  // FIXME: I like ot use the avr libc loop_until_bit_clear (above) to
  // replace this (below) but must verify it works:
  while (!(SPSR & _BV(SPIF)))
    ;
  return SPDR;
}


SPIClass SPI;

void SPIClass::begin() {

  // Set SS to high so a connected chip will be "deselected" by default
  digitalWrite(SS, HIGH);

  // When the SS pin is set as OUTPUT, it can be used as
  // a general purpose output port (it doesn't influence
  // SPI operations).
  pinMode(SS, OUTPUT);

  // Warning: if the SS pin ever becomes a LOW INPUT then SPI
  // automatically switches to Slave, so the data direction of
  // the SS pin MUST be kept as OUTPUT.
  SPCR |= _BV(MSTR);
  SPCR |= _BV(SPE);

  // Set direction register for SCK and MOSI pin.  MISO pin
  // automatically overrides to INPUT.  By doing this AFTER
  // enabling SPI, we avoid accidentally clocking in a single
  // bit since the lines go directly from "input" to SPI control.
  // http://code.google.com/p/arduino/issues/detail?id=888
  pinMode(SCK, OUTPUT);
  pinMode(MOSI, OUTPUT);
}

void SPIClass::end() {
  SPCR &= ~_BV(SPE);
}

void SPIClass::setBitOrder(uint8_t bitOrder)
{
  if(bitOrder == LSBFIRST) {
    SPCR |= _BV(DORD);
  } else {
    SPCR &= ~(_BV(DORD));
  }
}

void SPIClass::setDataMode(uint8_t mode)
{
  SPCR = (SPCR & ~SPI_MODE_MASK) | mode;
}

void SPIClass::setClockDivider(uint8_t rate)
{
  SPCR = (SPCR & ~SPI_CLOCK_MASK) | (rate & SPI_CLOCK_MASK);
  SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | ((rate >> 2) & SPI_2XCLOCK_MASK);
}

