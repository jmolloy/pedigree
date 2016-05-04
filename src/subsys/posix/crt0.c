/*
 * Copyright (c) 2008-2014, Pedigree Developers
 *
 * Please see the CONTRIB file in the root of the source tree for a full
 * list of contributors.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <newlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

extern int main(int, char**, char**);
extern void _init_signals(void);

void _start(char **argv, char **env)  _ATTRIBUTE((noreturn)) __attribute__((section(".text.crt0")));
void _start(char **argv, char **env)
{
  _init_signals();

  if (write(2,0,0) == -1)
  {
    open("/dev/tty", O_RDONLY, 0);
    open("/dev/tty", O_WRONLY, 0);
    open("/dev/tty", O_WRONLY, 0);
  }

  // Count how many args we have.
  int argc;
  if (argv == 0)
  {
    char *p = 0;
    argv = &p;
    argc = 0;
    environ = &p;
  }
  else
  {
    char **i = argv;
    argc = 0;
    while (*i++)
      argc++;
    if(!env)
      env = environ;
    i  = env;
    while (env && (*i))
    {
      // Save the key.
      char *key = *i;
      char *value = *i;
      // Iterate until we see the end of the string or an '='.
      while (*value && *value != '=') value++;
      // If we found a '=', change it to a NULL terminator (for the key) and increment position.
      if (*value == '=') *value++ = '\0';
      // Set the env var.
      setenv(key, value, 1);
      i++;
    }
  }

  exit(main(argc, argv, env));

  // Unreachable.
  for (;;);
}
