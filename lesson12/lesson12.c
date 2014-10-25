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
#include <avr/wdt.h>
#include <util/delay.h>

// WARNING WARNING WARNING: you really shouldn't be depending on the
// watchdog timer for anything without a really careful reading of Atmel
// document AVR132 "Using the Enhanced Watchdog Timer".  And while you're
// at it re-read the above disclaimer as well.

// This lesson demonstrates the simnplest kind of use of the watchdog
// timer system: resetting the system if a the watchdog timer isn't reset
// frequently.  Other techniques exist (see the above mentioned document).

// Storage for the contents of the MCUSR (which must be cleared during system
// initialization to ensure that continuous watchdog reset doesn't occur; see
// http://www.nongnu.org/avr-libc/user-manual/group__avr__watchdog.html for
// details).  This can be used to investigate the cause of a reset on reboot.
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));

// Backup and clear the MCUSR regester early in the AVR boot process (to
// ensure we don't enter a continual reset loop; see above comment).
void
fetch_and_clear_mcusr (void)
  __attribute__((naked))
  __attribute__((section(".init3")));
void
fetch_and_clear_mcusr (void)
{
  mcusr_mirror = MCUSR;
  MCUSR = 0x0;
  wdt_disable ();
}

// Quickly blink the LED that is hopefully attached to PORTB5 (assuming
// the port is correctly set up for output).
static void
quick_portb5_blink_sequence (void)
{
  int ii;
  const int blink_count = 5;
  const int blink_time_ms = 50;

  for ( ii = 0 ; ii < blink_count ; ii++ ) {
    PORTB |= _BV (PORTB5);
    _delay_ms (blink_time_ms);
    PORTB &= ~(_BV (PORTB5));
    _delay_ms (blink_time_ms);
  }
}

int
main (void) {

  // Set PORTB5 for output.
  DDRB = _BV (PORTB5);
  PORTB = _BV (PORTB5);

  // Make sure we can tell when a watchdog reset has occurred.
  quick_portb5_blink_sequence ();

  // Enable the watchdog timer.  Note that if the WDTON fuse is programmed,
  // watchdog resets will be enabled (and watchdog interrupts disabled)
  // and calling wdt_enable() is not needed.
  wdt_enable (WDTO_2S);

  // This delay doesn't cause a problem, since it's shorter than the watchdog
  // timeout value set above.
  _delay_ms (1500);

  // Reset the watchdog timer.
  wdt_reset ();

  // Now we can safely do some more work, since the timer has been reset.
  _delay_ms (1500);

  // Here we simulate a software hangup.  Since the resulting delay is longer
  // than the timeout period, a reset will be triggered.  Note that using
  // the watchdog timer to wake from sleep mode via an WDT interrupt (without
  // a reset) is also a common practice, but that method is not covered here.
  while ( 1 ) {
    ;
  }

}
