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
#include <util/delay.h>

#define soft_reset()        \
do                          \
{                           \
    wdt_enable(WDTO_15MS);  \
    for(;;)                 \
    {                       \
    }                       \
} while(0)

// Storage for the contents of the MCURS (which must be cleared during system
// initialization to ensure that continuous watchdog reset doesn't occur;
// see http://www.nongnu.org/avr-libc/user-manual/group__avr__watchdog.html
// for details).
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));

// Backup and clear the MCUSR regester (to ensure we don't enter a continual
// reset loop; see above comment).
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

//ISR (WDT_vect, ISR_NAKED)
//{
  // FIXME: docs for WDIE say shouldnt do this in interrupt routing itself
  // but later, to avoid busting the safety feature by which WDIE is cleared
  // the first time, and a reset triggered the next time.

  // We're not supposed to do this here, according to one part of spec sheet,
  // since it might compromize the escalation trick whereby WDIE is cleared
  // st the next wd timeout causes a reset.  But elsewhere it says WDT can
  // be used a general interrupt mechanism, in which case it seemt that one
  // might want to do this in the handler, idk.  Maybe it can always be done
  // after the handler as well if done right.

  // _WD_CONTROL_REG = _BV (WDIE); // Must reset interrupt after trigger

  // FIXME: WHY does this seem to toggle the light rather than just turning
  // it on over and over?  Or is the reset interaction jus making it look
  // like this is what happens?
  //PINB = _BV (PORTB5); /* toggle the pin */

  //quick_portb5_blink_sequence ();

  //reti();   // Enable interrupts and return from (naked handler).
//}

int
main (void) {

  // Set PORTB5 for output.
  DDRB = _BV (PORTB5);
  PORTB = _BV (PORTB5);

  // Make sure we can tell when a watchdog reset has occurred.
  quick_portb5_blink_sequence ();

  // Enable the watchdog timer.  Note that if the WDTON fuse is
  // programmed, watchdog resets will be enabled (and watchdog
  // interrupts disabled) and calling wdt_enable() is needed.
  wdt_enable (WDTO_4S);

  // Generate interrupts for watchdog timer expiration events.  Note that
  // since we haven't disabled watchdog timer resets, we'll get a reset
  // after the interrupt handler completes.
  // FIXME: why does disabling this cause the led to just blink crazily after
  // the first reset (which looks like continual reseting due to non-reset
  // MCUSR as discussed in spec sheet and avr libc page).
  //_WD_CONTROL_REG = _BV (WDIE);

  //sei ();   // Enable interrupts

  while ( 1 ) {

    set_sleep_mode (SLEEP_MODE_PWR_DOWN);
    sleep_mode ();

    // Here is where we would do things after the watchdog wakes us up.
    // _WD_CONTROL_REG = _BV (WDIE); // Must reset interrupt after trigger
  }
}

