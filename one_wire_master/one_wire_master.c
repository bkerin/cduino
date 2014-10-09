
#include <stdlib.h>
#include <string.h>  // FIXME: for memcpy only, might want to do away with it
#include <util/crc16.h>
#include <util/delay.h>

#include "dio.h"
#include "one_wire_master.h"
#include "util.h"

///////////////////////////////////////////////////////////////////////////////
//
// Line Drive, Sample, and Delay Routines
//
// These macros correspond to the uses of the inp and outp and tickDelay
// functions of Maxim application note AN126.  We use macros to avoid
// function call time overhead, which can be significant: Maxim application
// note AN148 states that the most common programming error in 1-wire
// programmin involves late sampling, which given that some samples occur
// after proscribed waits of only 9 us requires some care, especially at
// slower processor frequencies.

// Release (tri-state) the one wire master pin.  Note that this does
// not enable the internal pullup.  See the commends near omw_init()
// in one_wire_master.h.
#define RELEASE_LINE()     \
  DIO_INIT (               \
      ONE_WIRE_MASTER_PIN, \
      DIO_INPUT,           \
      DIO_DISABLE_PULLUP,  \
      DIO_DONT_CARE )

// Drive the line of the one wire master pin low.
#define DRIVE_LINE_LOW()   \
  DIO_INIT (               \
      ONE_WIRE_MASTER_PIN, \
      DIO_OUTPUT,          \
      DIO_DONT_CARE,       \
      LOW )

#define SAMPLE_LINE() DIO_READ (ONE_WIRE_MASTER_PIN)

// We support only standard speed, not overdrive speed, so we make our tick
// 1 us.
#define TICK_TIME_IN_US 1.0

// WARNING: the argument to this macro must be a double expression that the
// compiler knows is constant at compile time.  Pause for exactly ticks ticks.
#define TICK_DELAY(ticks) _delay_us (TICK_TIME_IN_US * ticks)

///////////////////////////////////////////////////////////////////////////////

// Tick delays for various parts of the standard speed one-wire protocol,
// as described in Table 2 in Maxim application note AN126.
#define TICK_DELAY_A   6.0
#define TICK_DELAY_B  64.0
#define TICK_DELAY_C  60.0
#define TICK_DELAY_D  10.0
#define TICK_DELAY_E   9.0
#define TICK_DELAY_F  55.0
#define TICK_DELAY_G   0.0
#define TICK_DELAY_H 480.0
#define TICK_DELAY_I  70.0
#define TICK_DELAY_J 410.0

void
owm_init (void)
{
  RELEASE_LINE ();
}

uint8_t
owm_touch_reset (void)
{
  TICK_DELAY (TICK_DELAY_G);
  DRIVE_LINE_LOW ();
  TICK_DELAY (TICK_DELAY_H);
  RELEASE_LINE ();
  TICK_DELAY (TICK_DELAY_I);
  // Look for presence pulse from slave
  uint8_t result = ! SAMPLE_LINE ();
  TICK_DELAY (TICK_DELAY_J); // Complete the reset sequence recovery

  return result; // Return sample presence pulse result
}

void
owm_write_bit (uint8_t value)
{
  // Send a 1-Wire write bit. Provide 10us recovery time.

  if ( value ) {
    // Write '1' bit
    DRIVE_LINE_LOW ();
    TICK_DELAY (TICK_DELAY_A);
    RELEASE_LINE ();
    TICK_DELAY (TICK_DELAY_B); // Complete the time slot and 10us recovery
  }
  else {
    // Write '0' bit
    DRIVE_LINE_LOW ();
    TICK_DELAY (TICK_DELAY_C);
    RELEASE_LINE ();
    TICK_DELAY (TICK_DELAY_D);
  }
}

uint8_t
owm_read_bit (void)
{
  // Read a bit from the 1-Wire bus and return it. Provide 10us recovery time.

  DRIVE_LINE_LOW ();
  TICK_DELAY (TICK_DELAY_A);
  RELEASE_LINE ();
  TICK_DELAY (TICK_DELAY_E);
  uint8_t result = SAMPLE_LINE ();   // Sample bit value from slave
  TICK_DELAY (TICK_DELAY_F); // Complete the time slot and 10us recovery

  return result;
}

// Global search state
static uint8_t ROM_NO[OWM_ID_BYTE_COUNT];
static int     LastDiscrepancy;
static int     LastFamilyDiscrepancy;
static int     LastDeviceFlag;
static uint8_t crc8;

//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
int OWSearch(void);   // FIXME: change to match API prototype
int OWSearch(void)
{
   int id_bit_number;
   int last_zero, rom_byte_number, search_result;
   int id_bit, cmp_id_bit;
   unsigned char rom_byte_mask, search_direction;
   // initialize for search
   id_bit_number = 1;
   last_zero = 0;
   rom_byte_number = 0;
   rom_byte_mask = 1;
   search_result = 0;
   crc8 = 0;
   // if the last call was not the last one
   if (!LastDeviceFlag)
   {
      // 1-Wire reset
      if (!owm_touch_reset ())
      {
         // reset the search
         LastDiscrepancy = 0;
         LastDeviceFlag = FALSE;
         LastFamilyDiscrepancy = 0;
         return FALSE;
      }
      // issue the search command  FIXME: remove this comment?
      owm_write_byte (SEARCH_ROM_COMMAND);
      // loop to do the search
      do
      {
         // read a bit and its complement
         id_bit = owm_read_bit ();
         cmp_id_bit = owm_read_bit ();
         // check for no devices on 1-wire
         if ((id_bit == 1) && (cmp_id_bit == 1))
            break;
         else
         {
            // all devices coupled have 0 or 1
            if (id_bit != cmp_id_bit)
               search_direction = id_bit; // bit write value for search
            else
            {
               // if this discrepancy if before the Last Discrepancy
               // on a previous next then pick the same as last time
               if (id_bit_number < LastDiscrepancy)
                   search_direction
                     = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
               else
                   // if equal to last pick 1, if not then pick 0
                   search_direction = (id_bit_number == LastDiscrepancy);
               // if 0 was picked then record its position in LastZero
               if (search_direction == 0)
               {
                   last_zero = id_bit_number;
                   // check for Last discrepancy in family
                   if (last_zero < 9)
                      LastFamilyDiscrepancy = last_zero;
               }
            }
            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1)
              ROM_NO[rom_byte_number] |= rom_byte_mask;
            else
              ROM_NO[rom_byte_number] &= ~rom_byte_mask;
            // serial number search direction write bit
            owm_write_bit (search_direction);
            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            id_bit_number++;
            rom_byte_mask <<= 1;
            // if the mask is 0 then go to new SerialNum byte rom_byte_number
            // and reset mask
            if (rom_byte_mask == 0)
            {
                 // Incrementally update CRC
                 crc8 = _crc_ibutton_update (crc8, ROM_NO[rom_byte_number]);
                 rom_byte_number++;
                 rom_byte_mask = 1;
            }
         }
      }
      while(rom_byte_number < 8);   // loop until through all ROM bytes 0-7
      // if the search was successful then
      if (!((id_bit_number < 65) || (crc8 != 0)))
      {
        // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
        LastDiscrepancy = last_zero;
        // check for last device
        if (LastDiscrepancy == 0)
           LastDeviceFlag = TRUE;
        search_result = TRUE;
      }
    }
    // if no device found then reset counters so next 'search' will be like
    // a first
    if (!search_result || !ROM_NO[0])
    {
      LastDiscrepancy = 0;
      LastDeviceFlag = FALSE;
      LastFamilyDiscrepancy = 0;
      search_result = FALSE;
    }

    return search_result;
}

// FIXME: rename these functions to match the API presentation

int OWFirst(void);   // FIXME: change to use API prototype
int OWFirst(void)
{
  // Reset the search state
  LastDiscrepancy = 0;
  LastDeviceFlag = FALSE;
  LastFamilyDiscrepancy = 0;
  return OWSearch();
}


//--------------------------------------------------------------------------
// Find the 'next' devices on the 1-Wire bus
// Return TRUE : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
int OWNext(void);  // FIXME: change to use API prototype
int OWNext(void)
{
   // leave the search state alone
   return OWSearch();
}

//--------------------------------------------------------------------------
// Verify the device with the ROM number in ROM_NO buffer is present.
// Return TRUE : device verified present
//         FALSE : device not present
//
int
OWVerify(void);
int
OWVerify(void)
{
  unsigned char rom_backup[8];
  int i,rslt,ld_backup,ldf_backup,lfd_backup;

  // keep a backup copy of the current state
  for (i = 0; i < 8; i++)
     rom_backup[i] = ROM_NO[i];
  ld_backup = LastDiscrepancy;
  ldf_backup = LastDeviceFlag;
  lfd_backup = LastFamilyDiscrepancy;

  // set search to find the same device
  LastDiscrepancy = 64;
  LastDeviceFlag = FALSE;

  if (OWSearch())
  {
     // check if same device found
     rslt = TRUE;
     for (i = 0; i < 8; i++)
     {
        if (rom_backup[i] != ROM_NO[i])
        {
            rslt = FALSE;
            break;
        }
     }
  }
  else
    rslt = FALSE;

  // restore the search state
  for (i = 0; i < 8; i++)
     ROM_NO[i] = rom_backup[i];
  LastDiscrepancy = ld_backup;
  LastDeviceFlag = ldf_backup;
  LastFamilyDiscrepancy = lfd_backup;

  // return the result of the verify
  return rslt;
}

uint8_t
owm_first (uint8_t *id_buf)
{
  int result = OWFirst ();

  if ( result ) {
    memcpy (id_buf, ROM_NO, OWM_ID_BYTE_COUNT);
  }

  return result;
}

uint8_t
owm_next (uint8_t *id_buf)
{
  int result = OWNext ();
  if ( result ) {
    memcpy (id_buf, ROM_NO, OWM_ID_BYTE_COUNT);
  }

  return result;
}

uint8_t
owm_verify (uint8_t *id_buf)
{
  memcpy (ROM_NO, id_buf, OWM_ID_BYTE_COUNT);
  uint8_t result = OWVerify ();
  return result;
}

void
owm_write_byte (uint8_t data)
{
  // Loop to write each bit in the byte, LS-bit first
  for ( uint8_t ii = 0; ii < BITS_PER_BYTE; ii++ )
  {
    owm_write_bit (data & B00000001);
    data >>= 1;   // Shift to get to next bit
  }
}

uint8_t
owm_read_byte (void)
{
  uint8_t result = 0;
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
