/* $Id: lesson10.c,v 1.2 2009/02/08 15:55:47 ckuethe Exp $ */
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
#include <avr/interrupt.h>

volatile uint8_t intrs;

ISR(TIMER0_OVF_vect) {
	/* this ISR is called when TIMER0 overflows */
	intrs++;
#if 1
	/* strobe PORTB.5 - the LED on arduino boards */
	if (intrs >= 61){
		PORTB ^= _BV(5);
		intrs = 0;
	}
#else
	/* color cycle RGB LED connected to pins 9,10,11 */
	PORTB = ((intrs >> 2) & 0x0e);
#endif
}


int
main(void) {
	/* 
	 * set up cpu clock divider. the TIMER0 overflow ISR toggles the
	 * output port after enough interrupts have happened.
	 * 16MHz (FCPU) / 1024 (CS0 = 5) -> 15625 incr/sec
	 * 15625 / 256 (number of values in TCNT0) -> 61 overflows/sec
	 */
	TCCR0B |= _BV(CS02) | _BV(CS00);

	/* Enable Timer Overflow Interrupts */
	TIMSK0 |= _BV(TOIE0);

	/* other set up */
	DDRB = 0xff;
	TCNT0 = 0;
	intrs = 0;

	/* Enable Interrupts */
	sei();

	while (1)
		; /* empty loop */
}

