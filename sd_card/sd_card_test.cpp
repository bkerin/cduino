#include <Sd2Card.h>

#ifdef __cplusplus
  extern "C" {
#   include <term_io.h>
  }
#endif

int
main (void)
{
  term_io_init ();
  printf ("LCD Initialized\n");

  Sd2Card card;

  printf ("cp1\n");

  double card_size = (double) card.cardSize ();

  printf ("card_size: %f\n", card_size);
}
