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
#include <errno.h>
#include <string.h>

// Pedigree function, defined in glue.c
extern int login(int uid, char *password);

int main(int argc, char *argv[])
{
    /// \todo Support further arguments
    int iRunShell = 0, error = 0, help = 0, nStart = 0, i = 0;
    for(i = 1; i < argc; i++)
    {
        if(!strcmp(argv[i], "-s"))
            iRunShell = 1;
        else if(!strcmp(argv[i], "-h"))
            help = 1;
        else if(!nStart)
            nStart = i;
    }

    // If there was an error, or if the help string needs to be printed, do so
    if(error || help || (!nStart && !iRunShell))
    {
        fprintf(stderr, "Usage: sudo [-h] [-s|<command>]\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "    -s: Access root shell\n");
        fprintf(stderr, "    -h: Show this help text\n");
        return error;
    }

    // Verify that the command to run actually exists
    if(0) // !iRunShell)
    {
        struct stat st;
        error = stat(argv[nStart], &st);
        if(error == -1)
        {
            fprintf(stderr, "sudo: can't run command: %s\n", strerror(errno));
            return errno;
        }
    }

    // Grab the root user's pw structure
    struct passwd *pw = getpwnam("root");
    if(!pw)
    {
        fprintf(stderr, "sudo: user 'root' doesn't exist!\n");
        return 1;
    }

    // Request the root password
    char password[256], c;
    i = 0;

    struct termios curt;
    tcgetattr(0, &curt); curt.c_lflag &= ~(ECHO|ICANON); tcsetattr(0, TCSANOW, &curt);

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
            printf("•");
        }
    }
    tcgetattr(0, &curt); curt.c_lflag |= (ECHO|ICANON); tcsetattr(0, TCSANOW, &curt);
    printf("\n");

    password[i] = '\0';

    // Attempt to log in as that user
    if(login(pw->pw_uid, password) != 0)
    {
        fprintf(stderr, "sudo: password is incorrect\n");
        return 1;
    }

    // Begin a new session so SIGINT is properly handled here
    setsid();

    // We're now running as root, so execute whatever we're supposed to execute
    if(iRunShell)
    {
        // Execute root's shell
        int pid;
        if((pid = fork()) == 0)
        {
            // Run the command
            execlp(pw->pw_shell, pw->pw_shell, 0);

            // Command not found!
            fprintf(stderr, "sudo: couldn't run shell: %s\n", strerror(errno));
            exit(errno);
        }
        else
        {
            // Wait for it to complete
            int status;
            waitpid(pid, &status, 0);

            // Did it exit with a non-zero status?
            if(status)
            {
                // Return error
                exit(status);
            }
        }
    }
    else
    {
        // Run the command
        int pid;
        if((pid = fork()) == 0)
        {
            // Run the command
            execvp(argv[nStart], &argv[nStart]);

            // Command not found!
            fprintf(stderr, "sudo: couldn't run command '%s': %s\n", argv[nStart], strerror(errno));
            exit(errno);
        }
        else
        {
            // Wait for it to complete
            int status;
            waitpid(pid, &status, 0);

            // Did it exit with a non-zero status?
            if(status)
            {
                // Return error
                exit(status);
            }
        }
    }

    // All done!
    return 0;
}
