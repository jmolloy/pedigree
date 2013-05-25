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
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

#include <sched.h>
int main(int argc, char **argv)
{
  syslog(LOG_INFO, "init: starting...");

  // Fork out and run preloadd.
  pid_t f = fork();
  if(f == 0)
  {
    syslog(LOG_INFO, "init: starting preloadd...");
    execl("/applications/preloadd", "/applications/preloadd", 0);
    syslog(LOG_INFO, "init: loading preloadd failed: %s", strerror(errno));
    exit(errno);
  }
  syslog(LOG_INFO, "init: preloadd running with pid %d", f);

  // Fork out and run the window manager
  /// \todo Need some sort of init script that specifies what we should
  ///       actually load and do here!
  f = fork();
  if(f == 0)
  {
    syslog(LOG_INFO, "init: starting winman...");
    execl("/applications/winman", "/applications/winman", 0);
    syslog(LOG_INFO, "init: loading winman failed: %s", strerror(errno));
    exit(errno);
  }
  syslog(LOG_INFO, "init: winman running with pid %d", f);

  syslog(LOG_INFO, "init: complete!");
  while(1);
  return 0;
}
