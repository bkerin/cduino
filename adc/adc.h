// Simple Analog to Digital Converter Interface
//
// WARNING: this interface is currently not very generic.  In particular,
// it always:
//
//   * monopolizes port C pins (sets DDRC to 0x00)
//   * uses the internal Vcc reference
//   * uses a 125 kHz clock
//
// See the ATMega328P datasheet for details of other options.

// Prepare port C pins for use by the ADC, and ready the ADC.  See the
// warning above for more details.
void
adc_init (void);

// Read a raw sample value from pin (which must be on of 0 through 5).
uint16_t
adc_read_raw (uint8_t pin);

// Read a voltage value from pin (which must be on of 0 through 5), assuming
// that the supply voltage is supply_voltage.
float
adc_read_voltage (uint8_t pin, float supply_voltage);
