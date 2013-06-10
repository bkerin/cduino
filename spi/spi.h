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
#include <Arduino.h>
#include <avr/pgmspace.h>

typedef enum {
  SPI_BIT_ORDER_LSB_FIRST,
  SPI_BIT_ORDER_MSB_FIRST
} spi_bit_order_t;

typedef enum {
  SPI_CLOCK_DIVIDER_DIV4 = 0x00,
  SPI_CLOCK_DIVIDER_DIV16 = 0x01,
  SPI_CLOCK_DIVIDER_DIV64 = 0x02,
  SPI_CLOCK_DIVIDER_DIV128 = 0x03,
  SPI_CLOCK_DIVIDER_DIV2 = 0x04,
  SPI_CLOCK_DIVIDER_DIV8 = 0x05,
  SPI_CLOCK_DIVIDER_DIV32 = 0x06
} spi_clock_divider_t;

typedef enum {
  SPI_MODE_0 = 0x00,
  SPI_MODE_1 = 0x04,
  SPI_MODE_2 = 0x08,
  SPI_MODE_3 = 0x0C,
} spi_mode_t;

#define SPI_MODE_MASK 0x0C  // CPOL = bit 3, CPHA = bit 2 on SPCR
#define SPI_CLOCK_MASK 0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR

// Initialize hardware SPI interface.
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

void
spi_attach_interrupt (void);

void
spi_detach_interrupt (void);

// Transfer a byte to/from/both?FIXME: WHICH? the slave.
uint8_t
spi_transfer (uint8_t data);

// Shut down hardware SPI interface.
void
spi_shutdown (void);

class SPIClass {
public:
  inline static byte transfer(byte _data);

  // SPI Configuration methods

  inline static void attachInterrupt();
  inline static void detachInterrupt(); // Default

  static void begin(); // Default
  static void end();

  static void setBitOrder(uint8_t);
  static void setDataMode(uint8_t);
  static void setClockDivider(uint8_t);
};

extern SPIClass SPI;

byte SPIClass::transfer(byte _data) {
  SPDR = _data;
  while (!(SPSR & _BV(SPIF)))
    ;
  return SPDR;
}

void SPIClass::attachInterrupt() {
  SPCR |= _BV(SPIE);
}

void SPIClass::detachInterrupt() {
  SPCR &= ~_BV(SPIE);
}

#endif
