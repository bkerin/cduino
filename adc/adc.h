// Simple Analog to Digital Converter Interface
//
// Test driver: adc_test.c    Implementation: adc.c
//
// This interface always uses a 125 kHz ADC clock.
//
// See the ATMega328P datasheet for details of other options.

#ifndef ADC_H
#define ADC_H

#include <stdint.h>

// Possible reference sources for the ADC.  See section 23.5.2 of ATMegs328P
// datasheet Rev. 8271C.  Note that Arduinos connect AVCC to VCC, so both
// ADC_REFERENCE_AVCC and ADC_REFERENCE_INTERNAL are pretty easy to use
// (but see the warning below regarding ADC_REFERENCE_INTERNAL).  FIXME:
// ADC_REFERENCE_AREF hasn't been tested (though its a dead-simple difference
// from the tested paths).
typedef enum {
  ADC_REFERENCE_AREF,
  ADC_REFERENCE_AVCC,
  ADC_REFERENCE_INTERNAL
} adc_reference_source_t;

// WARNING: using ADC_REFERENCE_INTERNAL when the AREF pin is connected to
// an external DC voltage can destroy the ADC.
//
// Prepare port C pins for use by the ADC, and ready the ADC.  If the
// ADC hardware is shut down to save power (i.e. if the PRADC bit of PRR
// register is set), this routine wakes it up.
//
// The ADC is initialized for polling operation with a 125 kHz ADC clock
// using reference_source.  Note that after this function is called,
// adc_pin_init() must still be called on the pin to be read.
void
adc_init (adc_reference_source_t reference_source);

// ADC pins available (corresponding to ADC0 .. ADC5).
#define ADC_LOWEST_PIN 0
#define ADC_HIGHEST_PIN 5

// Initialize a single pin for use as an ADC input.  Note that the pin
// argument must be an integer in the range [ADC_LOWEST_PIN, ADC_HIGHEST_PIN]
// (NOT a bit field specifying multiple pins at once).  The internal pull-up
// on the pin is disabled, the pin is set as an input, and the appropriate bit
// of the DIDR0 register is set to disable the digital input buffer on the pin
// (saving power).  Note that if you're trying to do something odd and use
// the pin for both analog and digital input, this might cause you trouble.
void
adc_pin_init (uint8_t pin);

// The adc_read_raw() function returns values between 0 and
// ADC_RAW_READING_STEPS - 1.
#define ADC_RAW_READING_STEPS ((uint16_t) 1024)

// Read a raw sample value from pin (which must be on of 0 through 5).
uint16_t
adc_read_raw (uint8_t pin);

// Read a voltage value from pin (which must be one of 0 through 5),
// assuming reference_voltage.  Note that if ADC_REFERENCE_INTERNAL was
// used with adc_init(), reference_voltage should be 1.1 for most (all?) AVR
// microcontrollers.
float
adc_read_voltage (uint8_t pin, float reference_voltage);

// The ADC hardware is not automatically disabled when entering power-saving
// sleep modes.  (see ATmega328P datasheet Rev. 8271C, section 23.6).
// The ADC hardware does not consume power when the ADEN bit of ADCSRA
// is cleared (ATmega328P datasheet Rev. 8271C, section 23.2).

// Disable the ADC to save power.  NOTE: the ADC hardware is NOT automatically
// disabled when entering power-saving sleep modes.  (see ATmega328P
// datasheet Rev. 8271C, sections, 23.2 and 23.6).
#define ADC_DISABLE()                       \
  do {                                      \
    ADCSRA &= ~(_BV (ADEN));                \
    loop_until_bit_is_clear (ADCSRA, ADEN); \
  } while ( 0 );

// Re-enable ADC hardware.
#define ADC_ENABLE()                      \
  do {                                    \
    ADCSRA |= _BV (ADEN);                 \
    loop_until_bit_is_set (ADCSRA, ADEN); \
  } while ( 0 );

#endif // ADC_H
