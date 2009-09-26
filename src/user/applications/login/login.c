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
        // Pass it down to the running program
        kill(g_RunningPid, SIGINT);
    }
    else
    {
        // Do not kill us
        /// \todo Can't be killed because the console read request still exists - exit() needs
        ///       to actually cancel all pending requests as well! In the meantime this will have
        ///       to suffice...
        printf("foff");
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

#if 0
  int pid;
  if ( (pid=fork()) == 0)
  {
      // Environment:
      char *newenv[1];
      newenv[0] = 0;

      // Child.
      execle("/applications/TUI", "/applications/TUI", 0, newenv);
      execle("/applications/tui", "/applications/TUI", 0, newenv);
      // If we got here, the exec failed.
      printf("Unable to launch /applications/TUI: `%s'\n");
      exit(1);
  }
#endif

  // New process group for job control. We'll ignore SIGINT for now
  /// \todo Perhaps restart the process on SIGINT?
  signal(SIGINT, SIG_IGN);
  setsid();

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
    // Not running anything
    g_RunningPid = -1;

    // Get username

    printf("Username: ");
    fflush(stdout);
    char c;
    char username[256];
    int i = 0;

    struct termios curt;
    tcgetattr(0, &curt); curt.c_lflag &= ~ECHO; tcsetattr(0, TCSANOW, &curt);

    while ( i < 256 && (c=getchar()) != '\n' )
    {
      if(c == '\b')
      {
        if(i > 0)
        {
            username[--i] = '\0';
            printf("\b \b");
        }
      }
      else if (c != '\033')
      {
        username[i++] = c;
        printf("%c",c);
      }
    }
    tcgetattr(0, &curt); curt.c_lflag |= ECHO; tcsetattr(0, TCSANOW, &curt);
    printf("\n");

    username[i] = '\0';

    struct passwd *pw = getpwnam(username);

    if (!pw)
    {
      printf("Unknown user: `%s'\n", username);
      continue;
    }

    // Get password

    /*
    char *password = getpass("Password: ");
    fflush(stdout);
    */

    // Use own way - display *

    printf("Password: ");
    fflush(stdout);
    char password[256];
    i = 0;

    tcgetattr(0, &curt); curt.c_lflag &= ~ECHO; tcsetattr(0, TCSANOW, &curt);

    while ( i < 256 && (c=getchar()) != '\n' )
    {
        if(c == '\b')
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
            printf("*");
        }
    }
    tcgetattr(0, &curt); curt.c_lflag |= ECHO; tcsetattr(0, TCSANOW, &curt);
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
      // Logged in successfully - launch the shell.
      int pid;
      if ( (pid=fork()) == 0)
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
