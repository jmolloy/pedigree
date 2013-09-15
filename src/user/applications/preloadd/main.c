/*
 * Copyright (c) 2013 Matthew Iselin
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

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>

const char *g_FilesToPreload[] = {
    "/applications/bash",
    "/applications/ls",
    "/applications/cat",
    "/applications/make",
    "/applications/gcc",
    "/applications/ld",
    "/support/gcc/libexec/gcc/i686-pedigree/4.5.1/cc1",
    "/support/gcc/libexec/gcc/i686-pedigree/4.5.1/cc1plus",
    "/support/gcc/libexec/gcc/i686-pedigree/4.5.1/collect2",
    "/libraries/libc.so",
    "/libraries/libm.so",
    "/libraries/libgmp.so",
    "/libraries/libmpfr.so",
    "/libraries/libmpc.so",
    0
};

#define BLOCK_READ_SIZE     0x10000

int main(int argc, char **argv)
{
  syslog(LOG_INFO, "preloadd: starting...");

  size_t n = 0;
  const char *s = g_FilesToPreload[n++];
  char *buf = (char *) malloc(BLOCK_READ_SIZE);
  do
  {
    struct stat st;
    int e = stat(s, &st);
    if(e == 0)
    {
      syslog(LOG_INFO, "preloadd: preloading %s...", s);
      FILE *fp = fopen(s, "rb");
      for(off_t off = 0; off < st.st_size; off += BLOCK_READ_SIZE);
        fread(buf, BLOCK_READ_SIZE, 1, fp);
      fclose(fp);
      syslog(LOG_INFO, "preloadd: preloading %s complete!", s);
    }
    else
    {
      syslog(LOG_INFO, "preloadd: %s probably does not exist", s);
    }
    s = g_FilesToPreload[n++];
  } while(s);

  free(buf);

  /// \todo Fix bug with orphaned processes exiting.
  while(1) sched_yield();
  return 0;
}
