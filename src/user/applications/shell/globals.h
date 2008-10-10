#ifndef MYGLOBALS_H
#define MYGLOBALS_H

typedef struct cmd
{
  char argv[32][256];
  int nargs;
  char in[256];
  char out[256];
  char err[256];
  char background;
} cmd_t;

#endif

