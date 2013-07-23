// Interface to hardware SPI controller (master mode only).
//
// Test Driver: spi_test.c    Implementation: spi.c

#ifndef SPI_H
#define SPI_H

#include <stdio.h>
#include <avr/pgmspace.h>

#include "dio.h"

////////////////////////////////////////////////////////////////////////////////
// 
// How This Interface Works
//
// You have to ensure that the slave select line for the device you want to
// talk to is brought low before you talk to it (of course this line should
// normally be high).  It may be convenient to define macros like this:
/*
//   #define MY_SPI_SLAVE_1_SELECT_INIT() \
//     SPI_SS_INIT(DIO_OUTPUT, DIO_DONT_CARE, HIGH)
//   #define MY_SPI_SLAVE_1_SELECT_SET_LOW SPI_SS_SET_LOW
//   #define MY_SPI_SLAVE_1_SELECT_SET_HIGH SPI_SS_SET_HIGH
*/
//   MY_SPI_SLAVE_1_SELECT_INIT ();
//   spi_init ();
//   spi_set_data_order (SPI_BIT_ORDER_LSB_FIRST);
//   spi_set_data_mode (SPI_DATA_MODE_0);
//   spi_set_clock_divider (SPI_CLOCK_DIVIDER_DIV4);
//
//   MY_SPI_SLAVE_1_SELECT_SET_LOW ();
//   uint8_t input_byte1 = spi_transfer (output_byte1);
//   uint8_t input_byte2 = spi_transfer (output_byte2);
//   //...
//   MY_SPI_SLAVE_1_SELECT_SET_HIGH ();
//
//   spi_shutdown ();   // Possibly
//
// See spi_test.c for a complete example using a single slave.
//
// The spi_init() function will automatically initialize the SS pin (aka
// PB2, aka DIGITAL_10) for output.  The ATMega requires this for correct
// SPI master mode operation.  The SS pin is also usually a logical choice
// to use to control the first SPI slave device, and is the only one you'll
// need to use if you're talking to just one slave.  It's possible to use
// another digital output to control a SPI slave, however.  If there are
// multiple slaves, you'll need to use a different output pin for each
// of them.  All that is required is that the output pin to be used be
// initialized for output, and that you take the pin for the device you
// want to talk to low before talking.  The example given at the top of
// this file could change to look like this:
/*
//   #define MY_SPI_SLAVE_1_SELECT_INIT() \
//     SPI_SS_INIT(DIO_OUTPUT, DIO_DONT_CARE, HIGH)
//   #define MY_SPI_SLAVE_1_SELECT_SET_LOW SPI_SS_SET_LOW
//   #define MY_SPI_SLAVE_1_SELECT_SET_HIGH SPI_SS_SET_HIGH
//
//   #define MY_SPI_SLAVE_2_SELECT_INIT() \
//     DIO_INIT_DIGITAL_4(DIO_OUTPUT, DIO_DONT_CARE, HIGH)
//   #define MY_SPI_SLAVE_2_SELECT_SET_LOW DIO_SET_DIGITAL_4_LOW
//   #define MY_SPI_SLAVE_2_SELECT_SET_HIGH DIO_SET_DIGITAL_4_HIGH
*/
//   SPI_SLAVE_1_SELECT_INIT ();
//   SPI_SLAVE_2_SELECT_INIT ();
//
//   spi_init ();
//   spi_set_data_order (SPI_BIT_ORDER_LSB_FIRST);
//   spi_set_data_mode (SPI_DATA_MODE_0);
//   spi_set_clock_divider (SPI_CLOCK_DIVIDER_DIV4);
//
//   // Talk to first slave device
//   SPI_SLAVE_1_SELECT_SET_LOW ();
//   uint8_t input_byte1 = spi_transfer (output_byte1);
//   uint8_t input_byte2 = spi_transfer (output_byte2);
//   //...
//   SPI_SLAVE_1_SELECT_SET_HIGH ();
//
//   // Talk to second slave device
//   SPI_SLAVE_2_SELECT_SET_LOW ();
//   uint8_t input_byte1 = spi_transfer (output_byte1);
//   uint8_t input_byte2 = spi_transfer (output_byte2);
//   //...
//   SPI_SLAVE_2_SELECT_SET_HIGH ();
//
//   spi_shutdown ();   // Possibly
//
// Of course, it might also be necessary to change SPI data order, data
// mode, and/or clock rate settings between different slaves (which should
// be possible).

/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@bug.st>
 * SPI Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */

// Bit order expected by the connected device
typedef enum {
  SPI_DATA_ORDER_LSB_FIRST,
  SPI_DATA_ORDER_MSB_FIRST
} spi_data_order_t;

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

// The SS pin (aka PB2, aka DIGITAL_10) will *always* be initialized for
// output with a HIGH value, even it it isn't used as a slave select line
// (usually its reasonable to use it as a slave select line).  The ATMega
// requires this in order for SPI master mode to operate correctly.
// See comments at the top of this file for details on how to use other
// pins instead of or in addition to SS as slave select pins.
#define SPI_SS_PIN DIO_PIN_DIGITAL_10
#define SPI_SS_INIT DIO_INIT_DIGITAL_10
#define SPI_SS_SET_LOW DIO_SET_DIGITAL_10_LOW
#define SPI_SS_SET_HIGH DIO_SET_DIGITAL_10_HIGH

// This interface assumes that the SCK and MOSI pins are always used as the
// clock and Master Out Slave In pins.  I've never tried anything else,
// though I have a vague idea that it might be possible.  The MISO pin
// (aka PB4, aka DIGITAL_12) will automatically override to act as an input
// when spi_init() is called, but we still control the status of the MISO
// internal pull-up resistor.  This interface always disables this pull-up.
#define SPI_SCK_INIT DIO_INIT_DIGITAL_13
#define SPI_MOSI_INIT DIO_INIT_DIGITAL_11
#define SPI_MISO_INIT DIO_INIT_DIGITAL_12

// Initialize hardware SPI interface.  This function initializes the SS (aka
// PB2, aka DIGITAL_10) pin for output, which is always required for correct
// SPI master mode operation regardless of which pin is actually used for
// slave selection.  See the comments at the top of this file for information
// on how to use different or multiple pins for SPI slave selection.
//
// The default SPI hardware configuration is as follows:
//
//   * Interrupts are disabled
//
//   * Master mode is enabled
//
//   * Data order is MSB first
//
//   * Data mode is 0 (~CPOL and ~CPHA), meaning the the clock is active-high
//     (~CPOL) and sampled at the leading edge of the clock cycle
//
//   * A SPI clock frequency of F_CPU / 4 is used (SPR1, SPR0, and ~SPI2X)
//
// These are the default setting for the SPCR and SPSR registers, except that
// that SPI is enabled (SPE) and set to master mode (MSTR).  Its possible
// to change the data order, data mode, and SPI clock frequency using other
// methods in this interface.
//
void
spi_init (void);

// Set data (bit) order to use.
void
spi_set_data_order (spi_data_order_t data_order);

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

// Transfer data (in both directions, either of which might be meaningless).
uint8_t
spi_transfer (uint8_t data);

// Shut down hardware SPI interface.
void
spi_shutdown (void);

#endif // SPI_H
