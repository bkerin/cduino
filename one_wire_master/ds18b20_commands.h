// This header contains some of the slave-specific command byte codes used by
// the Maxim DS18B20 one-wire temperature sensor.  Only the commands required
// by the test programs for the one_wire_master_test.c and one_wire_slave.c
// programs are defined here.

#ifndef DS18B20_COMMANDS_H
#define DS18B20_COMMANDS_H

#define DS18B20_COMMANDS_CONVERT_T_COMMAND       0x44
#define DS18B20_COMMANDS_READ_SCRATCHPAD_COMMAND 0xBE

#endif // DS18B20_COMMANDS_H
