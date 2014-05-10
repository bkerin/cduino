// Control one or two DC motors of up to 2 amps each using the Arduino
// Motor Shield Model R3 (http://arduino.cc/en/Main/ArduinoMotorShieldR3).
//
// Test driver: dc_motor_test.c    Implementation: dc_motor.c
//
// The Arduino motor shield is a fairly thin wrapper around the underlying
// L298P H-Bridge controller chip.

#ifndef DC_MOTOR_H
#define DC_MOTOR_H

//  These pins are used by the shield for direction and brake controls
#define DC_MOTOR_CHANNEL_A_DIRECTION_DIO_PIN DIO_PIN_DIGITAL_12
#define DC_MOTOR_CHANNEL_B_DIRECTION_DIO_PIN DIO_PIN_DIGITAL_13
#define DC_MOTOR_CHANNEL_A_BRAKE_DIO_PIN DIO_PIN_DIGITAL_9
#define DC_MOTOR_CHANNEL_B_BRAKE_DIO_PIN DIO_PIN_DIGITAL_8

// These pins are use by the shield for adc inputs which give load current.
// These are the pins as the Arduino numbers them:
//
//   0 -> Arduino A0 -> ATmega pin PC0
//   1 -> Arduino A1 -> ATmega pin PC1
//
#define DC_MOTOR_CHANNEL_A_CURRENT_SENSE_ADC_PIN 0
#define DC_MOTOR_CHANNEL_B_CURRENT_SENSE_ADC_PIN 1

// NOTE: In addition to the above pins, the motor shield uses pins Digital
// 3 and Digital 11 (aka PD3 and PB3 respectively in ATmega328P-speak)
// as PWM outputs to control the motor speed.

// NOTE: For some reason, the Arduino motor shield uses the OC2B pin (which
// is in turn affected by the OCR2B register) to control what it calls
// motor channel A, and the OC2A pin (and OCR2A register) to control what
// it calls channel B.  In theory clients don't need to know about this,
// but it seems to have enough potential for confusion that I'm documenting
// it here in the interface.
#define DC_MOTOR_CHANNEL_A_OCR_REGISTER OCR2B
#define DC_MOTOR_CHANNEL_B_OCR_REGISTER OCR2A

typedef enum {
  DC_MOTOR_CHANNEL_A,
  DC_MOTOR_CHANNEL_B
} dc_motor_channel_t;

// The Arduino connects its internal VCC to AVCC, so this is the reference
// source we'll use to measure the current sensing outputs.
#define DC_MOTOR_ADC_REFERENCE ADC_REFERENCE_AVCC

// Initialize the direction control pins, brake pins, PWM pins and their
// associated timer hardware, and current sensing pins, and set the motor
// speeds to 0.  If the timer/counter0 hardward is shut down to save power,
// this routine wakes it up.  FIXME: should wake up the ADC hardware too
// if required, or else adc interface should do it (probably the latter).
void
dc_motor_init (void);

// Set motor target speed for channel.  The speed argument must be in
// [-100, 100] and is interpreted as follows: -100 => full speed reverse,
// 0 => off, 100 => full speed ahead).  WARNING: it's possible for sudden
// changes to the motor speed or direction to place significant inertial
// loads on the motor.  You might want to call this routine multiple times
// over time, or perhaps implement dc_motor_ramp_to_speed() (see below).
void
dc_motor_set_speed (dc_motor_channel_t channel, int8_t speed);

// FIXXME: something like this might be useful in some applications to slowly
// ramp the speed to a given setting and thereby avoid high acceleration
// loading.  It could be implemented easily in terms of dc_motor_set_speed().
// It's unimplemented because I've only used heavily geared gear motors,
// and so haven't needed this.
//void
//dc_motor_ramp_to_speed (
//    dc_motor_channel_t channel, uint8_t target_speed, uint8_t rate );

// FIXXME: For reaons similar to those discussed above for the prospective
// dc_motor_ramp_to_speed() function, this is currently unimplemented
// (though the break lines are initialized by dc_motor_init()).
//void
//dc_motor_brake (dc_motor_channel_t channel);

// For the ADC-based load current calculations, we assume that the ADC
// reference voltage has this value.  Note that even if a high-voltage power
// supply is being used (to drive a more powerful motor), the Arduino will
// still have its internal voltage-regulator-generated VCC connected to AVCC.
// Its possible for this to be 3.3V under some circumstances, in which case
// this assumption will be wrong (FIXXME: arduino has a jumper or something
// to detect this, read it?).  But if you're driving motors chances are
// you have at least a 5V supply lurking somewhere, and hopefully you're
// running the CPU off 5V as well as the motor...
#define DC_MOTOR_ADC_REFERENCE_VOLTAGE 5.0

// The Arduino Motor Shield Model R3 features a current sensing resistor
// and amplifier arrangement such than we get this many volts per amp of
// load current.
#define DC_MOTOR_CURRENT_SENSE_AMPS_PER_VOLT (2.0 / 3.3)

// Return the load current in amps for motor on channel
// channel.  Note that this is the current value as computed using
// DC_MOTOR_CURRENT_SENSE_AMPS_PER_VOLT, not the voltage reading at the
// sensor output.
float 
dc_motor_load_current (dc_motor_channel_t channel);

// FIXXME: if we wanted to entirely shut down the timer/counter2 hardware to
// save power or something (and perhaps deconfigure direction/brake/current
// sense lines?, make sure OCR2A/B end up low by strobing FOC2X from
// normal mode like the ATmega328P datasheed babbles about? ), we would
// have implemented this function :)
//void
//dc_motor_shutdown (void)

#endif // DC_MOTOR_H
