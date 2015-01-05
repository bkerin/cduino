// Implementation of the interface described in one_wire_master.h.

#include <assert.h>
#include <string.h>
#include <util/crc16.h>

#include "dio.h"
#include "one_wire_common.h"
#include "one_wire_master.h"
// FIXME: remove this debug goop
#define TERM_IO_POLLUTE_NAMESPACE_WITH_DEBUGGING_GOOP
#include "term_io.h"
#include "util.h"

#ifdef OWM_BUILD_RESULT_DESCRIPTION_FUNCTION

char *
owm_result_as_string (owm_result_t result, char *buf)
{
  switch ( result ) {

#  define X(result_code)                                                      \
    case result_code:                                                         \
      PFP_ASSERT (strlen (#result_code) < OWM_RESULT_DESCRIPTION_MAX_LENGTH); \
      strcpy_P (buf, PSTR (#result_code));                                    \
      break;
    OWM_RESULT_CODES
#  undef X

    default:
      PFP_ASSERT_NOT_REACHED ();
      break;
  }

  return buf;
}

#endif

// Aliases for some operations from one_wire_commoh.h (for readability).
#define RELEASE_LINE()    OWC_RELEASE_LINE (OWM_PIN)
#define DRIVE_LINE_LOW()  OWC_DRIVE_LINE_LOW (OWM_PIN)
#define SAMPLE_LINE()     OWC_SAMPLE_LINE (OWM_PIN)
#define TICK_DELAY(ticks) OWC_TICK_DELAY (ticks)

void
owm_init (void)
{
  RELEASE_LINE ();
}

owm_result_t
owm_start_transaction (uint8_t rom_cmd, uint8_t *rom_id, uint8_t function_cmd)
{
  if ( ! OWC_IS_TRANSACTION_INITIATING_ROM_COMMAND (rom_cmd) ) {
    return OWM_RESULT_ERROR_GOT_INVALID_TRANSACTION_INITIATION_COMMAND;
  }
  if ( OWC_IS_ROM_COMMAND (function_cmd) ) {
    return OWM_RESULT_ERROR_GOT_ROM_COMMAND_INSTEAD_OF_FUNCTION_COMMAND;
  }

  if ( ! owm_touch_reset () ) {
    return OWM_RESULT_DID_NOT_GET_PRESENCE_PULSE;
  }

  owm_write_byte (rom_cmd);

  switch ( rom_cmd ) {

    case OWC_READ_ROM_COMMAND:
      {
        uint8_t crc = 0;
        for ( uint8_t ii = 0 ; ii < OWC_ID_SIZE_BYTES ; ii++ ) {
          rom_id[ii] = owm_read_byte ();
          uint8_t const ncb = OWC_ID_SIZE_BYTES - 1;   // Non-CRC Bytes (in ID)
          if ( LIKELY (ii < ncb) ) {
            crc = _crc_ibutton_update (crc, rom_id[ii]);
          }
          else {
            if ( crc != rom_id[ii] ) {
              return OWM_RESULT_ERROR_GOT_ROM_ID_WITH_INCORRECT_CRC_BYTE;
            }
          }
        }
        break;
      }

    case OWC_MATCH_ROM_COMMAND:
      for ( uint8_t ii = 0 ; ii < OWC_ID_SIZE_BYTES ; ii++ ) {
        owm_write_byte (rom_id[ii]);
      }
      break;

    case OWC_SKIP_ROM_COMMAND:
      break;

    default:
      assert (0);   // Shouldn't be here
      break;
  }

  owm_write_byte (function_cmd);

  return OWM_RESULT_SUCCESS;
}

uint8_t
owm_touch_reset (void)
{
  TICK_DELAY (OWC_TICK_DELAY_G);
  DRIVE_LINE_LOW ();
  TICK_DELAY (OWC_TICK_DELAY_H);
  RELEASE_LINE ();
  TICK_DELAY (OWC_TICK_DELAY_I);
  // Look for presence pulse from slave
  uint8_t result = ! SAMPLE_LINE ();
  TICK_DELAY (OWC_TICK_DELAY_J); // Complete the reset sequence recovery

  return result; // Return sample presence pulse result
}

void
owm_write_bit (uint8_t value)
{
  // Send a 1-Wire write bit. Provide recovery time.

  if ( value ) {
    // Write '1' bit
    DRIVE_LINE_LOW ();
    TICK_DELAY (OWC_TICK_DELAY_A);
    RELEASE_LINE ();
    TICK_DELAY (OWC_TICK_DELAY_B); // Complete the time slot and recovery
  }
  else {
    // Write '0' bit
    DRIVE_LINE_LOW ();
    TICK_DELAY (OWC_TICK_DELAY_C);
    RELEASE_LINE ();
    TICK_DELAY (OWC_TICK_DELAY_D);
  }
}

uint8_t
owm_read_bit (void)
{
  // Read a bit from the 1-Wire bus and return it. Provide recovery time.

  DRIVE_LINE_LOW ();
  TICK_DELAY (OWC_TICK_DELAY_A);
  RELEASE_LINE ();
  TICK_DELAY (OWC_TICK_DELAY_E);
  uint8_t result = SAMPLE_LINE ();   // Sample bit value from slave
  TICK_DELAY (OWC_TICK_DELAY_F); // Complete the time slot and recovery

  return result;
}

owm_result_t
owm_read_id (uint8_t *id_buf)
{
  uint8_t slave_presence = owm_touch_reset ();
  if ( ! slave_presence ) {
    return OWM_RESULT_DID_NOT_GET_PRESENCE_PULSE;
  }

  uint8_t const read_rom_command = OWC_READ_ROM_COMMAND;
  owm_write_byte (read_rom_command);
  uint8_t crc = 0;
  uint8_t const lnbo = OWC_ID_SIZE_BYTES - 1 - 1;   // Last Non-crc Byte Offset
  for ( uint8_t ii = 0 ; ii < OWC_ID_SIZE_BYTES ; ii++ ) {
    id_buf[ii] = owm_read_byte ();
    if ( ii <= lnbo ) {
      crc = _crc_ibutton_update (crc, id_buf[ii]);
    }
    else {
      if ( crc != id_buf[ii] ) {
        return OWM_RESULT_ERROR_GOT_ROM_ID_WITH_INCORRECT_CRC_BYTE;
      }
    }
  }

  return OWM_RESULT_SUCCESS;
}

// These are errors that can occur in the search() funtion.
#define SEARCH_ERROR_NONE    0   // No error
#define SEARCH_ERROR_NPP     1   // No Presence Pulse
#define SEARCH_ERROR_GBAC    2   // Got 1s for Bit and Compliment
#define SEARCH_ERROR_BAD_CRC 3   // Got bad ROM ID CRC value
#define SEARCH_ERROR_SPLD    4   // Searched Past Last Device
#define SEARCH_ERROR_RID0IS0 5   // rom_id[0] is zero

// Global search state
static uint8_t rom_id[OWC_ID_SIZE_BYTES];   // Current ROM device ID
static uint8_t last_discrep;                // Bit position of last discrepancy
static uint8_t last_family_discrep;         // FIXXME: currently unused
static uint8_t last_device_flag;            // True iff we got last slave
static uint8_t search_error;                // One of the SEARCH_ERROR_* values
static uint8_t gbac_bit_number;             // Bit number of SEARCH_ERROR_GBAC
static uint8_t crc8;                        // For most recent search() result

// Length of slave ROM IDs, in bits
#define ID_BIT_COUNT 64

// This many bits of each slave ROM ID form a so-called family code.
#define FAMILY_ID_BIT_COUNT 8

// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.  If alarmed_slaves_only is TRUE, use OWC_ALARM_SEARCH_COMMAND
// instead of OWC_SEARCH_ROM_COMMAND to find only slaves with an active
// alarm condition.
//
// Return TRUE  : device found, ROM number in rom_id buffer
//        FALSE : device not found, end of search
//
static uint8_t
search (uint8_t alarmed_slaves_only)
{
  uint8_t id_bit_number;
  uint8_t last_zero, rom_byte_number, search_result;
  uint8_t id_bit, cmp_id_bit;
  uint8_t rom_byte_mask, search_direction;

  // Initialize for search
  id_bit_number = 1;
  last_zero = 0;
  rom_byte_number = 0;
  rom_byte_mask = 1;
  search_result = FALSE;
  search_error = SEARCH_ERROR_NONE;
  gbac_bit_number = 0;
  crc8 = 0;

  // If the last call was not the last one
  if ( last_device_flag ) {
    search_error = SEARCH_ERROR_SPLD;
  }

  else {

    // 1-Wire reset
    if ( ! owm_touch_reset () ) {
      // Reset the search
      last_discrep = 0;
      last_device_flag = FALSE;
      last_family_discrep = 0;
      search_error = SEARCH_ERROR_NPP;
      return FALSE;
    }

    // Issue the appropriate search command
    owm_write_byte (
        alarmed_slaves_only ?
          OWC_ALARM_SEARCH_COMMAND :
          OWC_SEARCH_ROM_COMMAND );
    // Loop to do the search
    do {
      // Read a bit and its complement
      id_bit = owm_read_bit ();
      cmp_id_bit = owm_read_bit ();
      // Check for no elligible devices on 1-wire.  I would think this can
      // only happen from noise or when doing an alarm search, since we only
      // make it here if a presence pulse is received above.
      if ( (id_bit == 1) && (cmp_id_bit == 1) ) {
        search_error = SEARCH_ERROR_GBAC;
        gbac_bit_number = id_bit_number;
        break;
      }
      else {
        // All devices coupled have 0 or 1
        if ( id_bit != cmp_id_bit ) {
           search_direction = id_bit;   // Bit write value for search
        }
        else {
          // If this discrepancy is before the last discrepancy on a previous
          // next then pick the same as last time
          if ( id_bit_number < last_discrep ) {
            search_direction = ((rom_id[rom_byte_number] & rom_byte_mask) > 0);
          }
          else {
            // If equal to last pick 1, otherwire pick 0
            search_direction = (id_bit_number == last_discrep);
          }
          // If 0 was picked then record its position
          if ( search_direction == 0 ) {
            last_zero = id_bit_number;
            // Check for Last discrepancy in family
            if ( last_zero <= FAMILY_ID_BIT_COUNT ) {
              last_family_discrep = last_zero;
            }
          }
        }
        // Set or clear the bit in rom_byte_number with mask rom_byte_mask
        if ( search_direction == 1 ) {
          rom_id[rom_byte_number] |= rom_byte_mask;
        }
        else {
          rom_id[rom_byte_number] &= ~rom_byte_mask;
        }
        // Serial number search direction write bit
        owm_write_bit (search_direction);
        // Increment the byte counter id_bit_number and shift the mask
        // rom_byte_mask
        id_bit_number++;
        rom_byte_mask <<= 1;
        // If the mask is 0 then go to next byte
        // and reset mask
        if ( rom_byte_mask == 0 ) {
          // Incrementally update CRC
          crc8 = _crc_ibutton_update (crc8, rom_id[rom_byte_number]);
          rom_byte_number++;
          rom_byte_mask = 1;
        }
      }
    }
    while ( rom_byte_number < OWC_ID_SIZE_BYTES );

    if ( rom_id[0] == 0 ) {
      // The most likely way to end up here is by having a data line that's
      // got a ground fault (or a slave holding the line low).  A careful
      // analysis of the above portion of this function indicates this,
      // and I've tested it with a slave that holds the line low.  Note that
      // in this case no CRC error is generated (because the CRC sum keeps
      // on being zero when it starts zero and is fed an endless series
      // of zeros).
      search_error = SEARCH_ERROR_RID0IS0;
    }

    if ( crc8 != 0 ) {
      search_error = SEARCH_ERROR_BAD_CRC;
    }

    if ( ! search_error ) {
      // Search was successful
      last_discrep = last_zero;
      // If this was the last device...
      if ( last_discrep == 0 ) {
         last_device_flag = TRUE;
      }
      search_result = TRUE;
    }
  }

  // If no device found, then reset counters so next 'search' will be like
  // a first
  if ( ! search_result ) {
    last_discrep = 0;
    last_device_flag = FALSE;
    last_family_discrep = 0;
    search_result = FALSE;
  }

  return search_result;
}

// Find the 'first' device on the one-wire bus.  If alarmed_slaves_only is
// true, only slaves with an active alarm condition are found.  On success,
// the discovered ROM ID is placed in rom_id and OWM_RESULT_SUCCESS is
// returned, otherwise a non-zero result code is returned.
static owm_result_t
first (uint8_t alarmed_slaves_only)
{
  // Reset the search state
  last_discrep = 0;
  last_device_flag = FALSE;
  last_family_discrep = 0;

  uint8_t search_result = search (alarmed_slaves_only);

  if ( search_result ) {
    return OWM_RESULT_SUCCESS;
  }
  else {
    switch ( search_error ) {
      case SEARCH_ERROR_NPP:
        return OWM_RESULT_DID_NOT_GET_PRESENCE_PULSE;
        break;
      case SEARCH_ERROR_GBAC:
        if ( alarmed_slaves_only && gbac_bit_number == 1 ) {
          return OWM_RESULT_NO_SUCH_SLAVE;
        }
        else {
          return
            OWM_RESULT_ERROR_UNEXPECTEDLY_GOT_ONES_FOR_BIT_AND_ITS_COMPLIMENT;
        }
        break;
      case SEARCH_ERROR_BAD_CRC:
        return OWM_RESULT_ERROR_GOT_ROM_ID_WITH_INCORRECT_CRC_BYTE;
        break;
      case SEARCH_ERROR_SPLD:
        return OWM_RESULT_NO_SUCH_SLAVE;
        break;
      case SEARCH_ERROR_RID0IS0:
        return
          OWM_RESULT_ERROR_GOT_ROM_ID_WITH_BYTE_0_OF_0_PROBABLE_GROUNDED_LINE;
        break;
      default:
        return OWM_RESULT_ERROR_UNKNOWN_PROBLEM;
        break;
    }
  }
}

// Find the 'next' device on the one-wire bus.  If alarmed_slaves_only is
// true, only slaves ith an active alarm condition are found.  On success,
// th ediscovered ROM ID is placed in rom_id and and OWM_RESULT_SUCCESS IS
// returned, otherwise a non-zero result code is returned.
static owm_result_t
next (uint8_t alarmed_slaves_only)
{
  uint8_t search_result = search (alarmed_slaves_only);

  if ( search_result ) {
    return OWM_RESULT_SUCCESS;
  }
  else {
    switch ( search_error ) {
      case SEARCH_ERROR_NPP:
        return OWM_RESULT_DID_NOT_GET_PRESENCE_PULSE;
        break;
      case SEARCH_ERROR_GBAC:
        return
          OWM_RESULT_ERROR_UNEXPECTEDLY_GOT_ONES_FOR_BIT_AND_ITS_COMPLIMENT;
        break;
      case SEARCH_ERROR_BAD_CRC:
        return OWM_RESULT_ERROR_GOT_ROM_ID_WITH_INCORRECT_CRC_BYTE;
        break;
      case SEARCH_ERROR_SPLD:
        return OWM_RESULT_NO_SUCH_SLAVE;
        break;
      case SEARCH_ERROR_RID0IS0:
        return
          OWM_RESULT_ERROR_GOT_ROM_ID_WITH_BYTE_0_OF_0_PROBABLE_GROUNDED_LINE;
        break;
      default:
        return OWM_RESULT_ERROR_UNKNOWN_PROBLEM;
        break;
    }
  }
}

// Verify that the device with the ROM number in rom_id buffer is present.
// Return OWM_RESULT_SUCCESS if it is, or a non-zero result code otherwise.
static owm_result_t
verify (void)
{
  owm_result_t result;
  unsigned char rom_backup[OWC_ID_SIZE_BYTES];
  uint8_t ld_backup, ldf_backup, lfd_backup;

  // Keep a backup copy of the current state
  for ( uint8_t ii = 0 ; ii < OWC_ID_SIZE_BYTES ; ii++ ) {
     rom_backup[ii] = rom_id[ii];
  }
  ld_backup = last_discrep;
  ldf_backup = last_device_flag;
  lfd_backup = last_family_discrep;

  // Set globals st the next search will look for the device with id in rom_id
  last_discrep = ID_BIT_COUNT;
  last_device_flag = FALSE;

  if ( search (FALSE) ) {
     // Check if same device found
     result = OWM_RESULT_SUCCESS;
     for ( uint8_t ii = 0 ; ii < OWC_ID_SIZE_BYTES ; ii++ ) {
        if ( rom_backup[ii] != rom_id[ii] ) {
            result = OWM_RESULT_NO_SUCH_SLAVE;
            break;
        }
     }
  }
  else {
    switch ( search_error ) {
      case SEARCH_ERROR_NPP:
        result = OWM_RESULT_DID_NOT_GET_PRESENCE_PULSE;
        break;
      case SEARCH_ERROR_GBAC:
        result
          = OWM_RESULT_ERROR_UNEXPECTEDLY_GOT_ONES_FOR_BIT_AND_ITS_COMPLIMENT;
        break;
      case SEARCH_ERROR_BAD_CRC:
        result = OWM_RESULT_ERROR_GOT_ROM_ID_WITH_INCORRECT_CRC_BYTE;
        break;
      case SEARCH_ERROR_SPLD:
        // We should never get SEARCH_ERROR_SPLD in this context, since we
        // reset globals to avoid that before calling search().
        result = OWM_RESULT_ERROR_UNKNOWN_PROBLEM;
        break;
      case SEARCH_ERROR_RID0IS0:
        return
          OWM_RESULT_ERROR_GOT_ROM_ID_WITH_BYTE_0_OF_0_PROBABLE_GROUNDED_LINE;
        break;
      default:
        result = OWM_RESULT_ERROR_UNKNOWN_PROBLEM;
        break;
    }
  }

  // Restore the search state
  for ( uint8_t ii = 0 ; ii < OWC_ID_SIZE_BYTES ; ii++ ) {
     rom_id[ii] = rom_backup[ii];
  }
  last_discrep = ld_backup;
  last_device_flag = ldf_backup;
  last_family_discrep = lfd_backup;

  return result;
}

owm_result_t
owm_first (uint8_t *id_buf)
{
  owm_result_t result = first (FALSE);

  if ( result == OWM_RESULT_SUCCESS ) {
    memcpy (id_buf, rom_id, OWC_ID_SIZE_BYTES);
  }

  return result;
}

owm_result_t
owm_next (uint8_t *id_buf)
{
  owm_result_t result = next (FALSE);

  if ( result == OWM_RESULT_SUCCESS ) {
    memcpy (id_buf, rom_id, OWC_ID_SIZE_BYTES);
  }

  return result;
}

owm_result_t
owm_verify (uint8_t *id_buf)
{
  memcpy (rom_id, id_buf, OWC_ID_SIZE_BYTES);

  owm_result_t result = verify ();

  return result;
}

owm_result_t
owm_first_alarmed (uint8_t *id_buf)
{
  owm_result_t result = first (TRUE);

  if ( result == OWM_RESULT_SUCCESS ) {
    memcpy (id_buf, rom_id, OWC_ID_SIZE_BYTES);
  }

  return result;
}

owm_result_t
owm_next_alarmed (uint8_t *id_buf)
{
  owm_result_t result = first (TRUE);

  if ( result == OWM_RESULT_SUCCESS ) {
    memcpy (id_buf, rom_id, OWC_ID_SIZE_BYTES);
  }

  return result;
}

void
owm_write_byte (uint8_t data)
{
  // Loop to write each bit in the byte, LSB first
  for ( uint8_t ii = 0; ii < BITS_PER_BYTE; ii++ )
  {
    owm_write_bit (data & B00000001);
    data >>= 1;   // Shift to get to next bit
  }
}

// FIXXME: Like the other functions, these come from Maxim Application
// Note AN187.  But they didn't seem to work right for me.  Actually
// maybe they did, because I recenty read that if they can't find one
// of the targetted family of slaves, it just ends up finding another.
// With our new improved search function that can signal errors, we could
// perhaps detect this and propagate a real error, if it seemed worth it.
// These functions seem sort of pointless: surely clients can just remember
// things by family for themsleves after the initial scan if the need to?
// I guess it could make things a tiny bit faster in the presence of hot-plug
// devices or something but I have difficulty imagining caring.
//void
//owm_target_setup (uint8_t family_code)
//{
//  rom_id[0] = family_code;
//  for ( uint8_t ii = 1; ii < FAMILY_ID_BIT_COUNT ; ii++ ) {
//    rom_id[ii] = 0;
//  }
//  last_discrep = ID_BIT_COUNT;
//  last_family_discrep = 0;
//  last_device_flag = FALSE;
//}
//
//void
//owm_skip_setup (void)
//{
//  last_discrep = last_family_discrep;
//  last_family_discrep = 0;
//
//  // If there are no devices or other families left...
//  if ( last_discrep == 0 ) {
//     last_device_flag = TRUE;
//  }
//}

uint8_t
owm_read_byte (void)
{
  uint8_t result = 0;

  // Loop to read each bit in the byte, LSB first.
  for ( uint8_t ii = 0; ii < BITS_PER_BYTE; ii++ ) {
    result >>= 1;  // Shift the result to get ready for the next bit
    // If result is one, then set MS bit
    if ( owm_read_bit () ) {
      result |= B10000000;
    }
  }

  return result;
}

uint8_t
owm_touch_byte (uint8_t data)
{
  uint8_t result = 0;
  for ( uint8_t ii = 0; ii < BITS_PER_BYTE; ii++ ) {
    // Shift the result to get it ready for the next bit
    result >>= 1;
    // If sending a '1' then read a bit, otherwise write a '0'
    if ( data & B00000001 ) {
      if ( owm_read_bit () ) {
        result |= B10000000;
      }
    }
    else {
      owm_write_bit (0);
    }
    // Shift the data byte for the next bit
    data >>= 1;
  }
  return result;
}
