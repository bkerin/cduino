// Implementation of the interface described in adc.h.

#include <assert.h>
#include <avr/io.h>
#include <stdlib.h>   // FIXME: remove when have latest avr libc

#include "adc.h"

void
adc_init (adc_reference_source_t reference_source)
{
  // FIXME: disable PRADC to disable power management as we do for other
  // modules?

  // Restore the default settings for ADMUX.
  ADMUX = 0x00;

  switch ( reference_source ) {
    case ADC_REFERENCE_AREF:
      // Nothing to set since ADMUX bits default to 0.
      // FIXME: this path hasn't been tested.
      break;
    case ADC_REFERENCE_AVCC:
      ADMUX |= _BV (REFS0);
      break;
    case ADC_REFERENCE_INTERNAL:
      ADMUX |= _BV (REFS0) | _BV (REFS1);
      break;
    default:
      // Shouldn't be here.
      break;
  }

  // Sample the ground for now (we'll change this before taking real samples). 
  ADMUX |= _BV (MUX3) | _BV (MUX2) | _BV (MUX1) | _BV (MUX0);

  // Restore the default settings for ADC status register A.
  ADCSRA = 0x00;

  // Restore the default settings for ADC status register B.
  ADCSRB = 0x00;

  // Enable the ADC system, use 128 as the clock divider on a 16MHz arduino
  // (ADC needs a 50 - 200kHz clock) and start a sample.  The ATmega328P
  // datasheet specifies that the first sample taken after the voltage
  // reference is changed should be discarded.
  ADCSRA |= _BV (ADEN) | _BV (ADPS2) | _BV (ADPS1) | _BV (ADPS0) | _BV (ADSC);

  // Wait for the ADC to return a sample (and discard it).
  loop_until_bit_is_clear (ADCSRA, ADSC);
}

void
adc_pin_init (uint8_t pin)
{
  // Conceptual assertion (unsigned datatype prevents it actually happening :)
  // assert (pin >= ADC_LOWEST_PIN);
  assert (pin <= ADC_HIGHEST_PIN);

  PORTC &= ~(0x01 << pin);   // Disable pull-up on pin
  DDRC &= ~(0x01 << pin);    // Ensure pin is set as an input

  // Save power: See the ATmega328P datasheet section 9.10.6.
  DIDR0 |= 0x01 << pin;      // Disable digital input buffer on pin
}

uint16_t
adc_read_raw (uint8_t pin)
{
  // Select the input channel.  Table 23-4 of the ATmega328P datasheet
  // effectively specifies that the pin selection bits in the lower nibble
  // of ADMUX are interpreted as an integer specifying the channel.
  uint8_t admux_byte = (ADMUX & 0xf0) | (pin & 0x0f);
  ADMUX = admux_byte;

  // Start a sample and wait until its done.
  ADCSRA |= _BV (ADSC);
  loop_until_bit_is_clear (ADCSRA, ADSC);

  // It is required to read the low ADC byte before the high byte.
  uint8_t low_byte = ADCL;
  uint8_t high_byte = ADCH;

  return (((uint16_t) high_byte) << 8) | low_byte;
}

float
adc_read_voltage (uint8_t pin, float reference_voltage)
{
    uint16_t raw = adc_read_raw (pin);

    return ((float) raw / ADC_RAW_READING_STEPS) * reference_voltage;
}
