// Human-readable names for the various cryptic avr libc macros, AVR
// registers, etc.  FIXME: this module hasn't een kep up-to-date and I think
// it ends up being a bad way to do it, since the spec sheets and everything
// insist on the cryptic register names, and if this is used they don't show
// up directly in the source.  Maybe a vim module to support looking up the
// current word as a register in the datasheets or something would be better.

#ifndef HUMAN_READABLE_NAMES_H
#define HUMAN_READABLE_NAMES_H

#define STRING_IN_ROM PSTR

#define USART0_CONTROL_AND_STATUS_REGISTER_A UCSR0A 
#define USART0_DOUBLE_SPEED_OPERATION_BIT U2X0
#define USART0_DATA_REGISTER_EMPTY_BIT UDRE0
#define USART0_RECEIVE_COMPLETE_BIT RXC0
#define USART0_FRAME_ERROR_BIT FE0
#define USART0_DATA_OVERRUN_BIT DOR0

#define USART0_CONTROL_AND_STATUS_REGISTER_B UCSR0B 
#define USART0_TRANSMITTER_ENABLE_BIT TXEN0
#define USART0_RECEIVER_ENABLE_BIT RXEN0

#define USART0_CONTROL_AND_STATUS_REGISTER_C UCSR0C 

#define USART0_BAUD_RATE_REGISTER_HIGH UBRR0H

#define USART0_BAUD_RATE_REGISTER_LOW UBRR0L

#define USART0_DATA_REGISTER UDR0

#define PROGRAM_SPACE_STATIC_CHAR_POINTER PSTR 

#endif // HUMAN_READABLE_NAMES_H
