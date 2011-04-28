/* $CSK: lesson7.c,v 1.4 2009/02/08 09:00:43 ckuethe Exp $ */
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

/*
 * This lesson will use software PWM to gradually brighten the onboard led L.
 * Of course, if you have hardware PWM available (as we do on the arduino, you
 * want that instead (see lesson7b).
 *
 * The area filled with '#' is the time when the output is driven high. From
 * this, we can see that we need a timer (oscillator) whose period is as short
 * as each of the modulation steps.
 *
 * 0%   20%  20%  40%  40%  60%  60%  80%  80%  100% 100% |    |    |    |    |
 * |    |    |    |    |    |    | #    #    ##   ##   ###  ###  #### ####
 * ########## - 1 #    #    ##   ##   ###  ###  #### #### ##########
 * _____#____#____##___##___###__###__####_####_########## _ 0
 * +----+----+----+----+----+----+----+----+----+----+----+ 0    1    2    3 4
 * 5    6    7    8    9    10   11
 *
 * Speed is just an extra knob to control the speed of the fade up.  Ctr, when
 * graphed, forms a ramp wave. It counts from 0 to some maximum value and then
 * it resets to 0. To generate a pwm output, the brightness knob is compared
 * against the current counter value.  If counter is less than brightness,
 * drive the output high. if the brightness knob is small, the counter can only
 * grow a small amount before it crosses the threshold and the output is driven
 * low.
 *
 * Later tutorials will use the hardware PWM facilities.
 */

int main (void)
{
	unsigned char ctr, brightness, speed;
	/* set PORTB for output*/
	DDRB = 0xFF;

	ctr = 0;
	for(brightness = 0; ; brightness++){ // brightness
		for(ctr = 0; ctr < 255; ctr++){
			for(speed = 0; speed < 128; speed++){ // ramp-up speed
				if (ctr < brightness)
					PORTB = 0xff;
				else
					PORTB = 0x00;
			}
		}
	}
	return 0;
}
