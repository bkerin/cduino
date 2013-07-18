// Implementation of the interface described in spi.h

/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 * SPI Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#include <assert.h>
#include <inttypes.h>
#include <stdlib.h>

#include "dio.h"
#include "spi.h"

void
spi_init (void)
{
  // Initialize the SS pin for ouput with a HIGH value
  SPI_SS_INIT (DIO_OUTPUT, DIO_DONT_CARE, HIGH);

  // FIXME: WORK POINT: SSPR1 and SPR0 setst the clock divider to f_osc/128,
  // is that a sensible default?

  // NOTE: setting SPR1 and SPR0 results in the largest clocke divider,
  // i.e. the slowest possible operation, which is probably a sensible
  // default.  The other bits set SPI master mode and enable SPI.
  SPCR |= _BV (MSTR) | _BV (SPE) | _BV (SPR1) | _BV (SPR0);

  // Set the SCK and MOSI pins as OUTPUTS.  The MISO pin automatically
  // overrides to act as an input, but according to the ATMega328P datasheet
  // we still control the status of the pull-up resistor.  I believe we
  // probably never want this pull-up enabled for SPI operation, so we
  // go ahead and call SPI_MISO_INIT for the pull-up disabling effect.
  // By doing this AFTER enabling SPI, we avoid accidentally clocking in
  // a single bit since the lines go directly from "input" to SPI control.
  // http://code.google.com/p/arduino/issues/detail?id=888
  SPI_SCK_INIT (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  SPI_MOSI_INIT (DIO_OUTPUT, DIO_DONT_CARE, LOW);
  SPI_MISO_INIT (DIO_INPUT, DIO_DISABLE_PULLUP, DIO_DONT_CARE);
}

void
spi_set_bit_order (spi_bit_order_t bit_order)
{
  switch ( bit_order ) {
    case SPI_BIT_ORDER_LSB_FIRST:
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
  SPCR = (SPCR & ~SPI_MODE_MASK) | ((uint8_t) data_mode);
}

#define SPI_CLOCK_MASK 0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR

void
spi_set_clock_divider (spi_clock_divider_t divider)
{
  SPCR = (SPCR & ~SPI_CLOCK_MASK) | (((uint8_t) divider) & SPI_CLOCK_MASK);
  SPSR = (SPSR & ~SPI_2XCLOCK_MASK) | 
         ((((uint8_t) divider) >> 2) & SPI_2XCLOCK_MASK);
}

void
spi_attach_interrupt (void)
{
  SPCR |= _BV (SPIE);
}

void
spi_detach_interrupt (void)
{
  SPCR &= ~_BV (SPIE);
}

uint8_t
spi_transfer (uint8_t data)
{
  SPDR = data;
  //loop_until_bit_clear (SPSR, SPIF);
  // FIXME: I like ot use the avr libc loop_until_bit_clear (above) to
  // replace this (below) but must verify it works:
  while ( ! (SPSR & _BV (SPIF)) ) {
    ;
  }
  return SPDR;
}

void
spi_shutdown (void)
{
  SPCR &= ~_BV (SPE);
}
