/* app.ld - move applications away from the kernel location on hosted systems */

MEMORY
{
  base (rwxil) : ORIGIN = 0x20000000, LENGTH = 1024M
}

INSERT BEFORE .text;
