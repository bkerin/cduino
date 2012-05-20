// Simple Analog to Digital Converter Interface
//
// WARNING: this interface is currently not very generic.  In particular,
// it always:
//
//   * monopolizes port C pins (sets DDRC to 0x00)
//   * uses a 125 kHz ADC clock
//
// See the ATMega328P datasheet for details of other options.

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
// an extern DC voltage can destroy the ADC.  Prepare port C pins for use
// by the ADC, and ready the ADC.  See the warning above for more details.
void
adc_init (adc_reference_source_t reference_source);

// The adc_read_raw() function returns values between 0 and
// ADC_RAW_READING_STEPS - 1.
#define ADC_RAW_READING_STEPS ((uint16_t) 1024)

// Read a raw sample value from pin (which must be on of 0 through 5).
uint16_t
adc_read_raw (uint8_t pin);

// Read a voltage value from pin (which must be on of 0 through 5),
// assuming reference_voltage.  Note that if ADC_REFERENCE_INTERNAL was
// used with adc_init(), reference_voltage should be 1.1 for most (all?) AVR
// microcontrollers.
float
adc_read_voltage (uint8_t pin, float reference_voltage);
