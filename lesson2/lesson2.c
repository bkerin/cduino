/* $CSK: lesson2.c,v 1.3 2009/05/17 06:22:44 ckuethe Exp $ */
/*
 * Copyright (c) 2008 Chris Kuethe <chris.kuethe@gmail.com>
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
 *
 * 	- LEDs connected to PORTB bins B1, B2, and B3 (to ground).  They
 * 	  should in theory have current-limiting resistors but the 
 * 	  microcontroller output current it limited so you can get away without
 * 	  them.  The B5 output pin blink the onboard LED L on the arduino.
 * 	  If nothing else is hooked up all that you'll get is a the L LED :)
 *
 * 	- F_CPU is defined to be your cpu speed (preprocessor define)
 */

#define B0 0x01
#define B1 0x02
#define B2 0x04
#define B3 0x08
#define B4 0x10
#define B5 0x20
#define B6 0x40
#define B7 0x80
#define D 100 /* ms */

int main (void)
{
	/* set PORTB for output*/
	DDRB = 0xFF;

	while (1) {
		PORTB = B1;
		_delay_ms(D);
		PORTB = B1 | B2;
		_delay_ms(D);
		PORTB = B2;
		_delay_ms(D);
		PORTB = B2 | B3;
		_delay_ms(D);
		PORTB = B3;
		_delay_ms(D);
		PORTB = B3 | B5;
		_delay_ms(D);
		PORTB = B5;
		_delay_ms(D);
		PORTB = B5 | B1;
		_delay_ms(D);
	}
	return 0;
}

