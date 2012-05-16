// Implementation of the interface described in adc.h.

#include <avr/io.h>

#include "adc.h"

void
adc_init (adc_reference_source_t reference_source)
{
  // Internal pull-ups interfere with the ADC. We need to disable the pull-up
  // on the pin if it's being used for ADC. either writing 0 to the port
  // register or setting it to output should be enough to disable pull-ups.
  DDRC = 0x00;

  // NOTE: ADMUX is full of zeros by default.

  switch ( reference_source ) {
    case ADC_REFERENCE_AREF:
      // Nothing to set since ADMUX bits default to 0.
      // FIXME: this path hasn't been tested.
      // FIXME: shouldn't we explicitly clear the bits then??
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

  // Enable the ADC system, use 128 as the clock divider on a 16MHz arduino
  // (ADC needs a 50 - 200kHz clock) and start a sample.  The AVR needs to
  // do some set-up the first time the ADC is used; this first, discarded,
  // sample primes the system for later use.
  ADCSRA |= _BV (ADEN) | _BV (ADPS2) | _BV (ADPS1) | _BV (ADPS0) | _BV (ADSC);
  
  // Wait for the ADC to return a sample.
  loop_until_bit_is_clear (ADCSRA, ADSC);
}

uint16_t
adc_read_raw (uint8_t pin)
{
  uint8_t low_byte, high_byte, channel_select_byte;

  // Select the input channel.
  channel_select_byte = (ADMUX & 0xf0) | (pin & 0x0f);
  ADMUX = channel_select_byte;
  ADCSRA |= _BV (ADSC);
  loop_until_bit_is_clear (ADCSRA, ADSC);

  // It is required to read the low ADC byte before the high byte.
  low_byte = ADCL;
  high_byte = ADCH;

  return (((uint16_t) high_byte) << 8) | low_byte;
}

float
adc_read_voltage (uint8_t pin, float reference_voltage)
{
    uint16_t raw = adc_read_raw (pin);

    return ((float) raw / ADC_RAW_READING_STEPS) * reference_voltage;
}
