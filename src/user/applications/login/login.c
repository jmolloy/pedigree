/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

#include <pwd.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

// PID of the process we're running
int g_RunningPid = -1;

// Pedigree function, defined in glue.c
extern int login(int uid, char *password);

// SIGINT handler
void sigint(int sig)
{
    // If we're in the background...
    if(g_RunningPid != -1)
    {
        // Ignore, but don't log (running program)
    }
    else
    {
        // Do not kill us! CTRL-C does not do anything while the login prompt
        // is active
        syslog(LOG_NOTICE, "SIGINT ignored");
    }
}

int main(int argc, char **argv)
{
#ifdef INSTALLER
  // For the installer, just run Python
  printf("Loading installer, please wait...\n");

  static const char *app_argv[] = {"root»/applications/python", "root»/code/installer/install.py", 0};
  static const char *app_env[] = { "TERM=xterm", "PATH=/applications", "PYTHONHOME=/", 0 };
  execve("root»/applications/python", (char * const *) app_argv, (char * const *) app_env);

  printf("FATAL: Couldn't load Python!\n");

  return 0;
#endif

  // New process group for job control. We'll ignore SIGINT for now.
  signal(SIGINT, sigint);
  setsid();

  const char *TERM = getenv("TERM");
  if(!TERM)
  {
    TERM = "vt100";
  }

  // Turn on output processing if it's not already on (we depend on it)
  struct termios curt;
  tcgetattr(1, &curt);
  if(!(curt.c_oflag & OPOST))
      curt.c_oflag |= OPOST;
  tcsetattr(1, TCSANOW, &curt);

  // Grab the greeting if one exists.
  FILE *stream = fopen("root»/config/greeting", "r");
  if (stream)
  {
    char msg[1025];
    int nread = fread(msg, 1, 1024, stream);
    msg[nread] = '\0';
    printf("%s", msg);

    fclose(stream);
  }
  else
  {
    // Some default greeting
    printf("Welcome to Pedigree\n");
  }

  while (1)
  {
    // Set terminal title, if we can.
    if(!strcmp(TERM, "xterm"))
      printf("\033]0;Pedigree Login\007");

    // Not running anything
    g_RunningPid = -1;

    // This handles the case where a bad character goes into the stream and is
    // impossible to get out. Everything else I've tried does not work...
    fclose(stdin);
    int fd = open("/dev/tty", 0);
    if(fd != 0)
        dup2(fd, 0);
    stdin = fdopen(fd, "r");

    // Get username
    printf("Username: ");

    char buffer[256];
    char *username = fgets(buffer, 256, stdin);
    if(!username)
    {
      printf("\nUnknown user: `%s'\n", username);
      continue;
    }

    // Knock off the newline character
    username[strlen(username)-1] = '\0';

    struct passwd *pw = getpwnam(username);
    if (!pw)
    {
      printf("\nUnknown user: `%s'\n", username);
      continue;
    }

    // Get password

    // Use own way - display *

    printf("Password: ");
    fflush(stdout);
    char password[256], c;
    int i = 0;

    tcgetattr(0, &curt); curt.c_lflag &= ~(ECHO|ICANON); tcsetattr(0, TCSANOW, &curt);
    while ( i < 256 && (c=getchar()) != '\n' )
    {
        if(!c)
        {
            continue;
        }
        else if(c == '\b')
        {
            if(i > 0)
            {
                password[--i] = '\0';
                printf("\b \b");
            }
        }
        else if (c != '\033')
        {
            password[i++] = c;
            if(!strcmp(TERM, "xterm"))
              printf("•");
            else
              printf("*");
        }
    }
    tcgetattr(0, &curt); curt.c_lflag |= (ECHO|ICANON); tcsetattr(0, TCSANOW, &curt);
    printf("\n");

    password[i] = '\0';

    // Perform login - this function is in glue.c.
    if(login(pw->pw_uid, password) != 0)
    {
      printf("Password incorrect.\n");
      continue;
    }
    else
    {
      // Terminal title -> shell name.
      if(!strcmp(TERM, "xterm"))
        printf("\033]0;%s\007", pw->pw_shell);

      // Logged in successfully - launch the shell.
      int pid;
      if ( (g_RunningPid=pid=fork()) == 0)
      {
        // Child...
        g_RunningPid = -1;

        // Environment:
        char *newenv[2];
        newenv[0] = (char*)malloc(256);
        newenv[1] = 0;

        sprintf(newenv[0], "HOME=%s", pw->pw_dir);

        // Child.
        execle(pw->pw_shell, pw->pw_shell, 0, newenv);

        // If we got here, the exec failed.
        printf("Unable to launch default shell: `%s'\n", pw->pw_shell);
        exit(1);
      }
      else
      {
        // Parent.
        int stat;
        waitpid(pid, &stat, 0);

        g_RunningPid = -1;

        continue;
      }

    }

  }

  return 0;
}
