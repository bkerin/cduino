/* $CSK: lesson9.c,v 1.7 2009/05/17 06:22:44 ckuethe Exp $ */
/*
 * Copyright (c) 2009 Chris Kuethe <chris.kuethe@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

// Assumptions:
//
// 	- 10 kohm (or so) potentiometer connected between 5V supply and ground,
// 	  with potentionmeter tap connected to pin A0 (aka PC0 aka ADC0).
//
// 	- Note that there are a variety of hardware techniques that can be used
// 	  to improve the resolution and noise resistance of the ADC; the
// 	  ATMega328P datasheet discusses these.  For simplicity, we assume 
// 	  that they aren't needed here.

#include <avr/io.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "term_io.h"

// Initialize the ADC unit, without initialized any particular pins (see
// initialize_adc_pin() for that).  See the implementatation for details.
static void
initialize_adc (void)
{
  ADMUX = 0x00;   // Restore the default settings for ADMUX.

  // Unless otherwise configured, arduinos use the internal Vcc reference.
  // Thats what we're going to do as well, so we set bit REFS0 to specify
  // this (in combination with the already set default value of 0 for bit
  // REFS1). The MUX[3:0] bit setting used here indicate that we should
  // sample the ground (0.0V) (we'll change this before each actual ADC read).
  ADMUX |= _BV (REFS0) | _BV (MUX3) | _BV (MUX2) | _BV (MUX1) | _BV (MUX0);
  
  // Restore the default settings for ADC status register A.
  ADCSRA = 0x00;
  
  // Restore the default settings for ADC status register B.
  ADCSRB = 0x00;
  
  // Enable the ADC system, use 128 as the clock divider on a 16MHz arduino
  // (ADC needs a 50 - 200kHz clock) and start a sample.  The ATmega329P
  // datasheet specifies that the first sample taken after the voltage
  // reference is changed should be discarded.
  ADCSRA |= _BV (ADEN) | _BV (ADPS2) | _BV (ADPS1) | _BV (ADPS0) | _BV (ADSC);
  
  // Wait for the ADC to return a sample (and discard it).
  loop_until_bit_is_clear (ADCSRA, ADSC);
}

// ADC pins available (corresponding to ADC0 .. ADC5).
#define ADC_LOWEST_PIN 0
#define ADC_HIGHEST_PIN 5

// Initialize a particular ADC pin (ADC_LOWEST_PIN up to ADC_HIGHEST_PIN)
// for use as an ADC input.
static void
initialize_adc_pin (uint8_t pin)
{
  PORTC &= ~(0x01 << pin);   // Disable pull-up on pin
  DDRC &= ~(0x01 << pin);    // Ensure pin is set as an input
  DIDR0 |= 0x01 << pin;      // Disable digital input buffer on pin
}

// Read an initialized ADC pin (ADC_LOWEST_PIN up to ADC_HIGHEST_PIN).
static uint16_t
adc_read (uint8_t pin)
{
  // Select the input channel.
  uint8_t admux_value = (ADMUX & 0xf0) | (pin & 0x0f);
  ADMUX = admux_value;

  // Start a sample and wait until its done.
  ADCSRA |= _BV (ADSC);
  loop_until_bit_is_clear (ADCSRA, ADSC);

  // It is required to read the low ADC byte before the high byte.
  uint8_t lb = ADCL;   // Low Byte
  uint8_t hb = ADCH;   // High Byte

  return (((uint16_t) hb) << 8) | lb;
}

int
main (void)
{
  term_io_init ();   // Set up terminal communications.

  uint8_t const aip = 0;   // Analog Input Pin being used

  initialize_adc ();
  initialize_adc_pin (aip);

  while ( 1 )
  {
    uint16_t raw = adc_read (aip);

    // Print tap voltage and raw ADC value.
    uint16_t const a2d_steps = 1024;
    float const reference_voltage = 5.0;
    float tap_voltage = ((float) raw / a2d_steps) * reference_voltage;
    printf_P (
        PSTR ("Potentiometer tap voltage: %f (%d raw)\r\n"),
        tap_voltage,
        raw );

    float const mspr = 500;   // Milliseconds Per Reading
    _delay_ms (mspr);
  }
}
