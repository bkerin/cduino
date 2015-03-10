// Test/demo for the debug_led.h interface.

#include <avr/wdt.h>

#include "debug_led.h"
#include "util.h"

int
main (void)
{
  DBL_INIT ();

#ifdef ENABLE_WDT
  wdt_enable (WDTO_15MS);
#endif

#ifdef TEST_CASE_MULTIBLINK
  double const time_per_cycle = 4000;
  uint16_t const blink_count = 42;
  dbl_multiblink (time_per_cycle, blink_count);
#endif

#ifdef TEST_CASE_CHKP
  DBL_CHKP ();
#endif

#ifdef TEST_CASE_TRAP
  DBL_TRAP ();
#endif

#ifdef TEST_CASE_ASSERT
  DBL_ASSERT (FALSE);
#endif

#ifdef TEST_CASE_DISPLAY_UINT32
  uint32_t test_int = 42;
  dbl_display_uint32 (test_int);
#endif

#ifdef TEST_CASE_ASSERT_SHOW_POINT
  DBL_ASSERT_SHOW_POINT (FALSE);
#endif

}
