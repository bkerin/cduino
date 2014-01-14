
// Initialize the interface to the XBee.  Currently this interface only
// supports talking to XBee devices over the hardware serial port at 9600
// Baud, with eight data bits, no parity, and one stop bit (8-N-1 format).
// So the serial port is initialized with those parameters, and that's it
// all this routine does.
void
wx_init (void);

// Check if the XBee network ID (ID parameter) is set to id, and if not,
// set it to id and save the settings.  Valid id values are 0x00 - 0xffff.
// NOTE: this command may permanently alter the XBee configuration (it can
// be restored using wx_restore_defaults().
uint8_t
wx_ensure_network_id_set_to (uint16_t id);

// Check if the XBee channel (CH parameter) is set to channel, and if not,
// set it to channel and save the settings.  Valid channel values are 0x0b
// - 0x1a.  NOTE: this command may permanently alter the XBee configuration
// (it can be restored using wx_restore_defaults()).
uint8_t
wx_ensure_channel_set_to (uint8_t channel);

// Restore the XBee factory default configuration and save the settings.
uint8_t
wx_restore_defaults (void);

// I don't think the (FIXME: shield name) gives us any connection to the
// SLEEP_RQ pin of the XBee module, so FIXXME: this is unimplemented.
// However, hibernating is probably the first thing you'll want to do for
// a battery operated device, so its too bad we can't easily prototype it
// using the Arduino.  A few hints:
//
//   * Setting the SM parameter to 1 (using wx_com_expect_ok() once to set
//     the parameter and again to save the parameters) and then asserting
//     the SLEEP_RQ should give a very simple way to cut Xbee module power
//     consumption to about 10 uA, with the only disadvantage being that
//     the sleepy node will have to wake itself up (it cannot be called
//     awake from a coordinator).  Perhaps just soldering a lead to the
//     top of the SLEEP_RQ on the XBee module and plugging it into a DIO
//     line would be a good way to test this out.
//
//   * An all-software solution which reduces power consumption to about 50 uA
//     is also possible, but it requires significantly more module
//     confiruation in order to establish a coordinator node, end devices
//     nodes, etc. 
//
//   * The XBee Product manual version v1.xEx (a copy is in this module's
//     directory) has a description of the sleep mode options on page 23.
//
//void
//wx_hibernate (void);

///////////////////////////////////////////////////////////////////////////////
//
// Low Level/Extension Interface
//
// The remaining functions in this header are only useful if you need to
// change the XBee module configuration significantly.

// Maximum Command Output String Length (in bytes).  This includes any
// trailing carriage return ('\r') or NUL bytes that may be involved,
// and so is a safe size of buffer to use with wx_com().
#define WX_MCOSL 15

// Enter command mode, execute the given AT command with an "AT" prefix
// and "\r" postfix implicitly added (e.g. "BD9600" becomes "ATBD9600"),
// place the command output in output, stip the trailing carriage return
// ("\r") from output, and finally return TRUE if all that succeeded.
// Both command and output should be pointers to at least WX_MCOSL bytes
// of storage.  Its ok to pass the same pointer for both command and output,
// in which case the command string is overwritten with the command output
// (saving a few bytes of RAM).  The command string should be NUL-terminated,
// and the output will be NUL-terminated.
uint8_t
wx_com (char *command, char *output);

// Convenience function.  Like wx_com(), but the given command is expected
// to ouput "OK\r" (note that wx_com() would strip the trailing "\r").
// Return true iff everything wx_com() would do works and we get an OK back.
uint8_t
wx_com_expect_ok (char const *command);
