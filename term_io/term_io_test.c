// Test/demo for the term_io.h interface.

#include "term_io.h"

int
main (void)
{
  term_io_init ();

  char buffer[TERM_IO_LINE_BUFFER_MIN_SIZE];

  for ( ; ; ) {
    printf ("Enter something: ");
    int line_length = term_io_getline (buffer);
    if ( line_length == -1 ) {
      printf ("Error reading line\n");
    }
    else {
      if ( buffer[line_length - 1] == '\n' ) {
        buffer[line_length - 1] = '\0';
        printf (
            "You entered %d characters: '%s' (followed by newline)\n", 
            line_length, buffer);
      }
      else {
        printf ("BUG: shouldn't be here\n");
      }
    }
  }

}
