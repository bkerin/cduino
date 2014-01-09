// Implementation of the interface described in dc_motor.h.

#include <avr/io.h>
#include <assert.h>
// FIXME: remove this if all its needed for is assert once assert.h is fixed
#include <stdlib.h>  

#include "adc.h"
#include "dc_motor.h"
#include "dio.h"

#define DC_MOTOR_DIRECTION_FORWARD HIGH
#define DC_MOTOR_DIRECTION_REVERSE LOW

#define DC_MOTOR_CHANNEL_A_SET_DIRECTION(dir) \
  DIO_SET (DC_MOTOR_CHANNEL_A_DIRECTION_DIO_PIN, dir)
#define DC_MOTOR_CHANNEL_B_SET_DIRECTION(dir) \
  DIO_SET (DC_MOTOR_CHANNEL_B_DIRECTION_DIO_PIN, dir)

#define DC_MOTOR_BREAK_OFF LOW
#define DC_MOTOR_BREAK_ON  HIGH

void
dc_motor_init (void)
{
  // Configure direction line control registers
  {
    DIO_INIT (
        DC_MOTOR_CHANNEL_A_DIRECTION_DIO_PIN,
        DIO_OUTPUT,
        DIO_DONT_CARE,
        DC_MOTOR_DIRECTION_FORWARD );
    DIO_INIT (
        DC_MOTOR_CHANNEL_B_DIRECTION_DIO_PIN,
        DIO_OUTPUT,
        DIO_DONT_CARE,
        DC_MOTOR_DIRECTION_FORWARD );
  }

  // Configure brake line control registers
  {
    DIO_INIT (
        DC_MOTOR_CHANNEL_A_BRAKE_DIO_PIN,
        DIO_OUTPUT,
        DIO_DONT_CARE,
        DC_MOTOR_BREAK_OFF );
    DIO_INIT (
        DC_MOTOR_CHANNEL_B_BRAKE_DIO_PIN,
        DIO_OUTPUT,
        DIO_DONT_CARE,
        DC_MOTOR_BREAK_OFF );
  }

  // Configure the ADC and ADC input pins
  {
    adc_init (DC_MOTOR_ADC_REFERENCE);
    adc_pin_init (DC_MOTOR_CHANNEL_A_CURRENT_SENSE_ADC_PIN);
    adc_pin_init (DC_MOTOR_CHANNEL_B_CURRENT_SENSE_ADC_PIN);
  }

  // Configure timer/counter2 hardware, with clocking stopped
  {
    PRR &= ~(_BV (PRTIM2));   // Ensure timer2 not shut down to save power

    // Clear OC2A/B on compare match when up-counting, set OC2A/B on
    // compare match when down-counting.  Count to TOP before reversing.
    // See ATmega328P datasheet Table 17-4.
    TCCR2A =  0x00;
    TCCR2A |= _BV (COM2A1) | _BV (COM2B1) | _BV (WGM20);
    TCCR2A &= ~(_BV (COM2A0) | _BV (COM2B0));
    // This assignment sets WMG22 as desired to specify phase-correct
    // operation with the direction change at TOP, and also set bits CS22:0
    // all to zero which disconnects the clock and stops the timer (as
    // desired at this point).
    TCCR2B =  0x00;

    // We don't use timer interrupts in this application, or
    // asynchronout clocking. 
    TIMSK2 = 0x00;
    ASSR = 0x00;
  
    // FIXXME: The ATmega328P datasheet Section 17.5.3 contains some
    // mumbo-jumbo about how OC2x setup should be performed using a FOC2X
    // strobe from Normal mode, which would have to be done before the mode
    // is set (it says OC2x keep their value accross modes.  But that sounds
    // like a pain and it seems that OC2x should be low at the start anyway.
    // I guess if we changed out of the mode, OC2A/B might stay high,
    // so we might need to worry about this on shutdown (which we haven't
    // implemented yet)?
  }

  TCNT2 = 0;   // Set counter value to 0

  // Set the data direction of the PWM pins to output as the ATmega requires
  {
    // Probably these loop_until_is_set() calls could be replaced with a
    // single no-op, which the newer avr-libc's have.

    PORTB &= ~(_BV (PORTB3));   // Make sure output is initialized low.
    loop_until_bit_is_clear (PORTB, PORTB3);
    DDRB |= _BV (DDB3);
    loop_until_bit_is_set (DDRB, DDB3);   

    PORTD &= ~(_BV (PORTD3));   // Make sure output is initialized low.
    loop_until_bit_is_clear (PORTD, PORTD3);
    DDRD |= _BV (DDD3);
    loop_until_bit_is_set (DDRD, DDD3);   
  }

  // Start the clock.  We don't use any prescaler here, so the counter runs
  // at full F_CPU speed.  Given that the phase-correct PWM output ends up
  // generating one pulse per full count-up-count-down cycle, then ends up
  // amounting to a PWM frequency of 16 MHz / ((256 - 1) * 2) = ~31.37 kHz
  // at the pins, which should be fast enough to prevent motor hum but not
  // so fast that the transistors in the motor driver can't switch along.
  TCCR2B |= _BV (CS20);
}

// Map an argument in the range [0, 100] onto the range [0, 255].
#define DC_MOTOR_SPEED_MAP(arg) (((uint16_t) arg * 255) / 100)

void
dc_motor_set_speed (dc_motor_channel_t channel, int8_t speed)
{
  assert (-100 <= speed);
  assert (speed <= 100);

  switch ( channel ) {
    case DC_MOTOR_CHANNEL_A:
      if ( speed >= 0 ) {
        DC_MOTOR_CHANNEL_A_SET_DIRECTION (DC_MOTOR_DIRECTION_FORWARD);
        DC_MOTOR_CHANNEL_A_OCR_REGISTER = DC_MOTOR_SPEED_MAP (speed);
      }
      else { // ( speed < 0 )
        DC_MOTOR_CHANNEL_A_SET_DIRECTION (DC_MOTOR_DIRECTION_REVERSE);
        DC_MOTOR_CHANNEL_A_OCR_REGISTER = DC_MOTOR_SPEED_MAP (-speed);
      }
      break;
    case DC_MOTOR_CHANNEL_B:
      if ( speed >= 0 ) {
        DC_MOTOR_CHANNEL_B_SET_DIRECTION (DC_MOTOR_DIRECTION_FORWARD);
        DC_MOTOR_CHANNEL_B_OCR_REGISTER = DC_MOTOR_SPEED_MAP (speed);
      }
      else { // ( speed < 0 )
        DC_MOTOR_CHANNEL_B_SET_DIRECTION (DC_MOTOR_DIRECTION_REVERSE);
        DC_MOTOR_CHANNEL_B_OCR_REGISTER = DC_MOTOR_SPEED_MAP (-speed);
      }
      break;
    default:
      assert (0);   // Shouldn't be here
      break;
  }
}

float 
dc_motor_load_current (dc_motor_channel_t channel)
{
  uint8_t adc_pin;

  switch ( channel ) {
    case DC_MOTOR_CHANNEL_A:
      adc_pin = DC_MOTOR_CHANNEL_A_CURRENT_SENSE_ADC_PIN;
      break;
    case DC_MOTOR_CHANNEL_B:
      adc_pin = DC_MOTOR_CHANNEL_B_CURRENT_SENSE_ADC_PIN;
      break;
    default:
      assert (0);   // Shouldn't be here
      break;
  }

  return
    DC_MOTOR_CURRENT_SENSE_AMPS_PER_VOLT *
    adc_read_voltage (adc_pin, DC_MOTOR_ADC_REFERENCE_VOLTAGE);
}
