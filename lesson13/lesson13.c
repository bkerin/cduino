/* $CSK: lesson13.c,v 1.1 2010/01/09 21:54:37 ckuethe Exp $ */
/*
 * Copyright (c) 2010 Chris Kuethe <chris.kuethe@gmail.com>
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
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/sleep.h>

ISR(WDT_vect, ISR_NAKED){
	_WD_CONTROL_REG = _BV(WDIE); // must reset interrupt after trigger
	/* do other work inside the watchdog interrupt? */
	PINB = _BV(PB5); /* toggle the pin */
	reti();
}

int main(void){
	DDRB = _BV(PB5);
	PORTB = _BV(PB5);

	/* watchdog setup goo */
	wdt_enable(WDTO_1S);
	_WD_CONTROL_REG = _BV(WDIE); /* generate watchdog interrupts */

	sei(); /* enable interrupts */

	while(1){
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_mode();

		/* Do stuff after the watchdog wakes us up */
	}
}

