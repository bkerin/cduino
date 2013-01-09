/* $CSK: lesson8.c,v 1.3 2009/05/17 06:22:44 ckuethe Exp $ */
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
static int cput(char c, FILE *f);
static int cget(FILE *f);
static FILE O = FDEV_SETUP_STREAM(cput, NULL, _FDEV_SETUP_WRITE);
static FILE I = FDEV_SETUP_STREAM(NULL, cget, _FDEV_SETUP_READ );

static int cput(char c, FILE *f)
{
        f = f;   // Compiler reassurance (we don't need to use f in this case).

	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
	return 0;
}

static int cget(FILE *f)
{
        f = f;   // Compiler reassurance (we don't need to use f in this case).

	loop_until_bit_is_set(UCSR0A, RXC0);
	return UDR0;
}

int main (void)
{
	uint8_t c = 0;
        // FIXME: this array should get a better name than 's'
	char s[80];
	int n;
	/* use program space (flash) to store these. don't waste RAM */
	PGM_P pn = "please enter (blind type) a number: ";
	PGM_P ps = "please enter (blind type) a string: ";
	PGM_P rn = "twice %d is %d\r\n";
	PGM_P rs = "changed case: %s\r\n";

	/* set up stdio */
#define BAUD 9600
#include <util/setbaud.h>
	UBRR0H = UBRRH_VALUE;
	UBRR0L = UBRRL_VALUE;
	UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);
	stdout = &O;
	stdin  = &I;

	while (1) {
		/* prompt for a number, double it and print it back */
		printf(pn);
                scanf("%d", &n);
                printf(rn, n, 2*n);

		/* prompt for a string, and swap the case of the letters */
		printf(ps);
		scanf("%s", s); /* yeah, unbounded string ops suck */
		c = 0;
		/* change the case of the input string */
		while(s[c] != '\0'){
			if (s[c] >= 'a' && s[c] <= 'z')
				s[c] -= 0x20;
			else if (s[c] >= 'A' && s[c] <= 'Z')
				s[c] += 0x20;
			c++;
		}
		printf(rs, s);
	}
	return 0;
}

