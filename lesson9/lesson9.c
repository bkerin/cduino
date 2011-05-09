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
 */

#include <avr/io.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

/* let the compiler do some of this, to avoid malloc */
static int cput(char, FILE *);
static FILE O = FDEV_SETUP_STREAM(cput, NULL, _FDEV_SETUP_WRITE);

static int cput(char c, FILE *f)
{
        f = f;   // Compiler reassurance.
  
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
	return 0;
}

/*
 * This lookup table and the adc_convert function are used to map raw
 * sensor values into more meaningful measurements. It's not necessary
 * to do analog input, but it sure make the output prettier. This table
 * is only valid for my input device - it was generated with RepRap's
 * createTemperatureLookup.py.
 */
#define NUMTEMPS 21
short T[NUMTEMPS][2] = { /* raw:temp (in oC * 10) */
	{1, 8041},	{52, 2514},	{103, 2061},
	{154, 1817},	{205, 1648},	{256, 1518},
	{307, 1410},	{358, 1317},	{409, 1233},
	{460, 1156},	{511, 1084},	{562, 1014},
	{613, 946},	{664, 877},	{715, 806},
	{766, 731},	{817, 650},	{868, 557},
	{919, 441},	{970, 274},	{1021, -259}
};

static short adc_convert(short in)
{
	uint8_t i;
	short s, r = 0x8000;

	/* clamp the input range */
	if (in < T[0][0])
		in = T[1][0];
	if (in > T[NUMTEMPS-1][0])
		in = T[NUMTEMPS-1][0];

	/* walk the table */
	for(i = 0; i < NUMTEMPS-1; i++)
		/* to find the conversion range to use */
		if ((in >= T[i][0]) && (in < T[i+1][0])){
			/* calculate the slope of this segment */
			s = (T[i+1][1] - T[i][1]) / (T[i+1][0] - T[i][0]);
			/* y = m x + b */
			r = (in - T[i][0]) * s + T[i][1] ;
		}
	return r;
}

static void adc_init()
{
	/* internal pull-ups interfere with the ADC. disable the
	 * pull-up on the pin if it's being used for ADC. either
	 * writing 0 to the port register or setting it to output
	 * should be enough to disable pull-ups. */
	PORTC = 0x00;
	DDRC = 0x00;
	/* unless otherwise configured, arduinos use the internal Vcc
	 * reference. MUX 0x0f samples the ground (0.0V). */
	ADMUX = _BV(REFS0) | 0x0f;
	/*
	 * Enable the ADC system, use 128 as the clock divider on a 16MHz
	 * arduino (ADC needs a 50 - 200kHz clock) and start a sample. the
	 * AVR needs to do some set-up the first time the ADC is used; this
	 * first, discarded, sample primes the system for later use.
	 */
	ADCSRA |= _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0) | _BV(ADSC);
	/* wait for the ADC to return a sample */
	loop_until_bit_is_clear(ADCSRA, ADSC);
}

unsigned short adc_read(unsigned char pin)
{
	unsigned char l, h, r;

	r = (ADMUX & 0xf0) | (pin & 0x0f);
	ADMUX = r; /* select the input channel */
	ADCSRA |= _BV(ADSC);
	loop_until_bit_is_clear(ADCSRA, ADSC);

	/* must read the low ADC byte before the high ADC byte */
	l = ADCL;
	h = ADCH;

	return ((unsigned short)h << 8) | l;
}

int main (void)
{
	short raw, tc, tf;
	PGM_P fmtstr = "thermistor output: %d raw %d oC %d oF\r\n";

	/* set up stdio */
#define BAUD 9600
#include <util/setbaud.h>
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);
	stdout = &O;

	adc_init(0);
	DDRB = 0xff; /* there are some LEDs connected to PORTB */

	while (1) {
		raw = adc_read(0);

		/* convert raw readings into more meaningful measurement */
		tc = adc_convert(raw);
		tf = ((tc * 9) / 5) +320;
		printf(fmtstr, raw, tc/10, tf/10);

		/* light up some LED depending on the temperature */
		if (raw > 970)
			PORTB = _BV(PORTB1);
		else if ((raw <= 970) && (raw > 955))
			PORTB = _BV(PORTB2);
		else if (raw <= 955)
			PORTB = _BV(PORTB3);

		_delay_ms(500);
	}
	return 0;
}

