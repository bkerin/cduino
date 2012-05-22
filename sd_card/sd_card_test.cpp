#include <Sd2Card.h>

#ifdef __cplusplus
extern "C" {
  #include <lcd_keypad.h>
}
#endif

int
main (void)
{
  Sd2Card card;

  double card_size = (double) card.cardSize ();

  lcd_keypad_show_value ("card_size", &card_size);
}
