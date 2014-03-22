// Test/demo for the term_io.h interface.

#include "term_io.h"

// This program repeatedly prompts for a line of input, then prints it
// back out.
//
// There are no external hardware requirements other than an arduino and a USB
// cable to connect it to the computer.  It should be possible to run
//
//   make -rR run_screen
//
// or so from the module directory to see it do its thing.

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
