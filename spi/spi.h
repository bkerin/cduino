// Interface to hardware SPI controller (master mode only).
//
// Test Driver: spi_test.c    Implementation: spi.c
//
// There is a particular call order which should probably always be used,
// which will look something like this:
//
//   spi_init ();
//   spi_set_bit_order (SPI_BIT_ORDER_LSB_FIRST);
//   spi_set_data_mode (SPI_DATA_MODE_0);
//   spi_set_clock_divider (SPI_CLOCK_DIVIDER_DIV4);
//   SPI_SS_LOW ();
//   uint8_t input_byte1 = spi_transfer (output_byte1);
//   uint8_t input_byte2 = spi_transfer (output_byte2);
//   ...
//   SPI_SS_HIGH ();
//   spi_shutdown ();   // Possibly
//
// See spi_test.c for an example.
//
/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 * SPI Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

#ifndef SPI_H
#define SPI_H

#include <stdio.h>
#include <avr/pgmspace.h>

#include "dio.h"

#ifndef UNTESTEDNESS_ACKNOWLEDGED
#  error This module not fully tested.  Remove this warning trap and try it! \
         I have tested output with SPI_BIT_ORDER_MSB_FIRST and SPI_DATA_MODE_0 \
         with all SPI_CLOCK_DIVIDER_* settings.  The other bit orders and \
         modes are only trivially different and should work fine, but I have \
         not personally tried them.
#endif

// Bit order expected by the connected device
typedef enum {
  SPI_BIT_ORDER_LSB_FIRST,
  SPI_BIT_ORDER_MSB_FIRST
} spi_bit_order_t;

// Clock divider to use for communication
typedef enum {
  SPI_CLOCK_DIVIDER_DIV4 = 0x00,
  SPI_CLOCK_DIVIDER_DIV16 = 0x01,
  SPI_CLOCK_DIVIDER_DIV64 = 0x02,
  SPI_CLOCK_DIVIDER_DIV128 = 0x03,
  SPI_CLOCK_DIVIDER_DIV2 = 0x04,
  SPI_CLOCK_DIVIDER_DIV8 = 0x05,
  SPI_CLOCK_DIVIDER_DIV32 = 0x06
} spi_clock_divider_t;

// Clock polarity and phase (often called CPOL and CPHA) expected
// by the connected device.  It may be necessary to look at the
// device timing diagram to determine these, since they don't always
// explicitly mention the mode number, CPOL or CPHA values.  See
// wikipedia.org/wiki/Serial_Peripheral_Interface_Bus#Clock_polarity_and_phase.
typedef enum {
  SPI_DATA_MODE_0 = 0x00,   // CPOL == 0, CPHA == 1
  SPI_DATA_MODE_1 = 0x04,   // CPOL == 0, CPHA == 1
  SPI_DATA_MODE_2 = 0x08,   // CPOL == 1, CPHA == 0
  SPI_DATA_MODE_3 = 0x0C,   // CPOL == 1, CPHA == 1
} spi_data_mode_t;

// We require clients to set some macros at compile time to specify which
// pins are being used for SPI communication.  The Makefile in the spi
// module direcory shows one way to do this.
//
// NOTE: SPI_SCK_INIT and SPI_MOSI_INIT shouldn't be changed.
//
// NOTE: The MISO pin (aka PB4, aka DIGITAL_12) will automatically override
// to act as an input when spi_init() is called.
//
// NOTE: spi_init() will automatically initialize the SS pin (aka PB2, aka
// DIGITAL_10) for output.  This interface also contains macros SPI_SS_LOW()
// and SPI_SS_HIGH() to select this device.  This SS pin is usually a logical
// one to use to control the first SPI slave device, and is the only one
// you'll need to use if you're talking to just one slave.  If there are
// multiple slaves, you'll want to use a different output pin for each
// of them.  All that is required is that the output pin be initialized
// for output, and that you take the pin for the device you want to talk
// to low before talking.  The example given at the top of this file could
// change to look like this:
//
//   DIO_INIT_DIGITAL_4 (DIO_OUTPUT, DIO_DONT_CARE, HIGH);
//   spi_init ();
//   spi_set_bit_order (SPI_BIT_ORDER_LSB_FIRST);
//   spi_set_data_mode (SPI_DATA_MODE_0);
//   spi_set_clock_divider (SPI_CLOCK_DIVIDER_DIV4);
//
//   // Talk to first slave device
//   SPI_SS_LOW ();
//   uint8_t input_byte1 = spi_transfer (output_byte1);
//   uint8_t input_byte2 = spi_transfer (output_byte2);
//   ...
//   SPI_SS_HIGH ();
//
//   // Talk to second slave device
//   DIO_SET_DIGITAL_4_LOW ();
//   uint8_t input_byte1 = spi_transfer (output_byte1);
//   uint8_t input_byte2 = spi_transfer (output_byte2);
//   ...
//   DIO_SET_DIGITAL_4_HIGH ();
//
//   spi_shutdown ();   // Possibly
//
// Of course, it might also be necessary to change SPI bit order, data
// mode, and/or clock rate settings to talk to other slaves (which should
// be possible).
#if ! (defined (SPI_SS_INIT) && \
       defined (SPI_SS_SET_LOW) && \
       defined (SPI_SS_SET_HIGH) && \
       defined (SPI_SCK_INIT) && \
       defined (SPI_MOSI_INIT))
#  error The macros which specify which pins should be used for SPI \
         communication are not set.  Please see the example in the Makefile \
         in the spi module directory.
#endif

// Initialize hardware SPI interface.  This function initialized the SS
// pin for control of the first SPI slave device.  Additional devices may
// be used as well, see the comments near the first mention of SPI_SS_INIT
// in this file for details.
void
spi_init (void);

// Set bit order to use.
void
spi_set_bit_order (spi_bit_order_t bit_order);

// Set data mode to use.
void
spi_set_data_mode (spi_data_mode_t data_mode);

// Set clock divider to use.
void
spi_set_clock_divider (spi_clock_divider_t divider);

// FIXME: lose this guy?
void
spi_attach_interrupt (void);

// FIXME: lose this guy?
void
spi_detach_interrupt (void);

// FIXME: some stuff needs to be turned into macros.  Heck, maybe everything.

// Set the first SS line low.  It is required to set the slave select line
// for a slave low before a sequence of spi_transfer() calls intented to
// communicate with that slave.  See comments near spi_init() for a pointer
// to details about how to communicate with multiple slave devices.
#define SPI_SS_LOW() SPI_SS_SET_LOW ()

// Set the first SS line high.  This should be done at the end of a
// sequence of spi_transfer() calls communicating with a SPI slave device.
// See comments near spi_init() for a pointer to details about how to
// communicate with multiple slave devices.
#define SPI_SS_HIGH() SPI_SS_SET_HIGH ()

// Transfer data (in both directions, either of which might be meaningless).
uint8_t
spi_transfer (uint8_t data);

// Shut down hardware SPI interface.
void
spi_shutdown (void);

#endif // SPI_H
