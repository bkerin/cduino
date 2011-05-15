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
// 	  with potentionmeter tap connected to pin PC0 (ADC0).
//
// 	- Note that there are a variety of hardware techniques that can be used
// 	  to improve the resolution and noise resistance of the ADC; the
// 	  ATMega328P datasheet discusses these.  For simplicity, we assume 
// 	  that they aren't needed here.


#include <avr/io.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

/* let the compiler do some of this, to avoid malloc */
static int cput (char, FILE *);
static FILE O = FDEV_SETUP_STREAM (cput, NULL, _FDEV_SETUP_WRITE);

static int
cput (char c, FILE *f)
{
        f = f;   // Compiler reassurance.
  
	loop_until_bit_is_set (UCSR0A, UDRE0);
	UDR0 = c;
	return 0;
}

static void
adc_init (void)
{
        // Internal pull-ups interfere with the ADC. disable the pull-up on the
        // pin if it's being used for ADC. either writing 0 to the port
        // register or setting it to output should be enough to disable
        // pull-ups.
        PORTC = 0x00;
        DDRC = 0x00;

        // Unless otherwise configured, arduinos use the internal Vcc
        // reference. MUX 0x0f samples the ground (0.0V) (we'll change this
        // before each actual ADC read).
	ADMUX = _BV(REFS0) | 0x0f;

        // Enable the ADC system, use 128 as the clock divider on a 16MHz
        // arduino (ADC needs a 50 - 200kHz clock) and start a sample.  The AVR
        // needs to do some set-up the first time the ADC is used; this first,
        // discarded, sample primes the system for later use.
	ADCSRA |= _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0) | _BV(ADSC);

	// Wait for the ADC to return a sample.
	loop_until_bit_is_clear (ADCSRA, ADSC);
}

static unsigned short
adc_read (unsigned char pin)
{
	unsigned char l, h, r;

	r = (ADMUX & 0xf0) | (pin & 0x0f);

        // Select the input channel.
	ADMUX = r;
	ADCSRA |= _BV(ADSC);
	loop_until_bit_is_clear (ADCSRA, ADSC);

        // It is required to read the low ADC byte before the high byte.
	l = ADCL;
	h = ADCH;

	return ((unsigned short)h << 8) | l;
}

int main (void)
{
	short raw;
	PGM_P pot_fmtstr = "Potentiometer tap voltage: %f (%d raw)\r\n";

        // Set up stdio.
#define BAUD 9600
#include <util/setbaud.h>
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);
	stdout = &O;

	adc_init ();

	while ( 1 )
        {
		raw = adc_read (0);

                // Print tap voltage and raw ADC value.
                const uint16_t a2d_steps = 1024;
                const float supply_voltage = 5.0;
                float tap_voltage = ((float) raw / a2d_steps) * supply_voltage;
                printf (pot_fmtstr, tap_voltage, raw);

		_delay_ms (500);
	}
	return 0;
}

