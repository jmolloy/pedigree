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

#include <unistd.h>
#include <stdio.h>
#include <pwd.h>
#include <sys/wait.h>

// Pedigree function, defined in glue.c
extern int login(int uid, char *password);

int main(int argc, char **argv)
{
  // Grab the greeting if one exists.
  FILE *stream = fopen("root:/config/greeting", "r");
  if (stream)
  {
    char msg[1025];
    int nread = fread(msg, 1, 1024, stream);
    msg[nread] = '\0';
    printf("%s", msg);
  }
  else
  {
    printf("penis\n");
  }

  while (1)
  {
    printf("Username: ");
    fflush(stdout);
    char c;
    char username[256];
    int i = 0;
    while ( i < 256 && (c=getchar()) != '\n' )
      username[i++] = c;

    username[i] = '\0';

    struct passwd *pw = getpwnam(username);

    if (!pw)
    {
      printf("Unknown user: `%s'\n", username);
      continue;
    }

    char *password = getpass("Password: ");
    fflush(stdout);

    // Perform login - this function is in glue.c.
    if (login(pw->pw_uid, password) != 0)
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
        // Environment:
        char *newenv[2];
        newenv[0] = malloc(256);
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

        continue;
      }

    }

  }

  return 0;
}
