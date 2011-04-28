/* $Id: lesson7b.c,v 1.2 2009/02/09 03:30:03 ckuethe Exp $ */
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
#include <util/delay.h>

/*
 * Assumptions:
 * 	- LEDs connected to PORTD.5 and PORTD.6
 */

int main (void)
{
	/* Waveform Generation Mode 3 - Fast PWM */
	TCCR0A |= _BV(WGM01) | _BV(WGM00);

	/*
	 * Compare Output Mode - fast PWM
	 * Non-inverting mode drives the output high while the counter
	 * is greater than OCRNx. Inverting mode drives the output low
	 * while the counter is greater than OCRNx.
	 */
	TCCR0A |= _BV(COM0A1) | _BV(COM0A0); /* inverting: fade down */
	TCCR0A |= _BV(COM0B1); /* non-inverting: fade up */

	/* reset all the timers and comparators */
	OCR0A = 0;
	OCR0B = 0;
	TCNT0 = 0;

	/*
	 * Clock Source 1 - CLK. Setting this bit late allows us to
	 * initialize the registers before the clocks start ticking
	 */
	TCCR0B |= _BV(CS00);

	/*
	 * Arduino pins 5 & 6 (PORTD.5 and PORTD.6) are PWM driven by TIMER0
	 * "The setup of the OC0x should be performed before setting the Data
	 * Direction Register for the port pin to output." -- S14.5.3
	 */
	DDRD |= _BV(PORTD5) | _BV(PORTD6);

	while (1){
		/*
		 * slowly crank up the compare register. since one output
		 * is inverting, the net result is to fade from one channel
		 * to the other.
		 */
		OCR0A++;
		OCR0B++;
		_delay_ms(10); /* busy wait. could be done with timers too. */
	}
	return 0;
}
