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
#include <sys/wait.h>
#include <utmpx.h>
#include <libgen.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef NOGFX
#define MAIN_PROGRAM "/applications/ttyterm"
#else
#define MAIN_PROGRAM "/applications/ttyterm"
#endif

pid_t start(const char *proc)
{
  pid_t f = fork();
  if(f == -1)
  {
    syslog(LOG_ALERT, "init: fork failed %s", strerror(errno));
    exit(errno);
  }
  if(f == 0)
  {
    syslog(LOG_INFO, "init: starting %s...", proc);
    execl(proc, proc, 0);
    syslog(LOG_INFO, "init: loading %s failed: %s", proc, strerror(errno));
    exit(errno);
  }
  syslog(LOG_INFO, "init: %s running with pid %d", proc, f);

  // Avoid calling basename() on the given parameter, as basename is non-const.
  char basename_buf[PATH_MAX];
  strcpy(basename_buf, proc);

  // Add a utmp entry.
  setutxent();
  struct utmpx init;
  struct timeval tv;
  memset(&init, 0, sizeof(init));
  gettimeofday(&tv, NULL);
  init.ut_type = INIT_PROCESS;
  init.ut_pid = f;
  init.ut_tv = tv;
  strcpy(init.ut_id, basename(basename_buf));
  pututxline(&init);
  endutxent();

  return f;
}

void startAndWait(const char *proc)
{
  pid_t f = start(proc);
  waitpid(f, 0, 0);
}

extern void pedigree_reboot();

int main(int argc, char **argv)
{
  syslog(LOG_INFO, "init: starting...");

  // Make sure we have a utmp file.
  int fd = open(UTMP_FILE, O_CREAT | O_RDWR);
  if(fd >= 0)
    close(fd);

  // Set up utmp.
  setutxent();

  // Boot time (for uptime etc).
  struct timeval tv;
  gettimeofday(&tv, NULL);

  struct utmpx boot;
  memset(&boot, 0, sizeof(boot));
  boot.ut_type = BOOT_TIME;
  boot.ut_tv = tv;
  pututxline(&boot);

  // All done with utmp.
  endutxent();

#ifdef HOSTED
  // Reboot the system instead of starting up.
  syslog(LOG_INFO, "init: hosted build, triggering a reboot");
  pedigree_reboot();
#else
  // Fork out and run startup programs.
  start("/applications/preloadd");
  start("/applications/python");
  start(MAIN_PROGRAM);
#endif

  // Done, enter PID reaping loop.
  syslog(LOG_INFO, "init: complete!");
  while(1) {
    /// \todo Do we want to eventually recognise that we have no more
    ///       children, and terminate/shutdown/restart?
    int status = 0;
    pid_t changer = waitpid(-1, &status, 0);
    if(changer > 0)
      syslog(LOG_INFO, "init: child %d exited with status %d", changer, WEXITSTATUS(status));

    // Register the dead process now.
    struct utmpx *p = 0;
    setutxent();
    do {
      p = getutxent();
      if(p && (p->ut_type == INIT_PROCESS && p->ut_pid == changer))
        break;
    } while(p);

    if(!p)
    {
      endutxent();
      continue;
    }

    setutxent();
    struct utmpx dead;
    memset(&dead, 0, sizeof(dead));
    gettimeofday(&tv, NULL);
    dead.ut_type = DEAD_PROCESS;
    dead.ut_pid = changer;
    dead.ut_tv = tv;
    strcpy(dead.ut_id, p->ut_id);
    pututxline(&dead);
    endutxent();
  }
  return 0;
}
