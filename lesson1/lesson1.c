/* $CSK: lesson1.c,v 1.3 2009/05/17 06:22:44 ckuethe Exp $ */
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

//
// Assumptions:
// 	- LED connected to PORTB (arduino boards have LED L onboard)
// 	- F_CPU is defined to be your cpu speed (preprocessor define)
//
// NOTE: the file in blink/blink.c is a better place for look for an example of 
// how to address individual IO pins.
//
// WARNING: this technique doesn't translate to all the other IO pins on a
// typical arduino, because the arduino bootloader uses some of them for its
// own purposes (e.g. PD0 is set up as the RX pin for serial communication,
// which precludes its use as an output).  The unconnected IO pins are
// presumably ok to use, or you can just nuke the bootloader with an
// AVRISPmkII or similar device.

int main (void)
{
	/* set all pins of PORTB for output*/
	DDRB = 0xFF;

	while (1) {
		/* set all pins of PORTB high */
		PORTB = 0xFF;
		_delay_ms(500);

		/* set all pins of PORTB low */
		PORTB = 0x00;
		_delay_ms(500);
	}
	return 0;
}
