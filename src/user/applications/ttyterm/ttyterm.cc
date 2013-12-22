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
#include <sys/ioctl.h>

#include <input/Input.h>

// PID of the process we're running
pid_t g_RunningPid = -1;

// File descriptor for our PTY master.
int g_MasterPty;

#define ALT_KEY (1ULL << 60)
#define SHIFT_KEY (1ULL << 61)
#define SPECIAL_KEY (1ULL << 63)

enum ActualKey
{
    None,
    Left,
    Right,
    Up,
    Down,
};

// Pedigree function, defined in glue.c
extern int login(int uid, char *password);

// SIGINT handler
void sigint(int sig)
{
    // Ignore, but don't log (running program)
}

void handle_input(Input::InputNotification &note)
{
    syslog(LOG_INFO, "ttyterm: system input (type=%d)", note.type);

    if(note.type & Input::Key)
    {
        uint64_t c = note.data.key.key;

        ActualKey realKey = None;
        if(c & SPECIAL_KEY)
        {
            uint32_t k = c & 0xFFFFFFFFULL;
            char str[5];
            memcpy(str, reinterpret_cast<char*>(&k), 4);
            str[4] = 0;

            if(!strcmp(str, "left"))
            {
                realKey = Left;
            }
            else if(!strcmp(str, "righ"))
            {
                realKey = Right;
            }
            else if(!strcmp(str, "up"))
            {
                realKey = Up;
            }
            else if(!strcmp(str, "down"))
            {
                realKey = Down;
            }
        }

        if(c == '\n')
            c = '\r'; // Enter key (ie, return) - CRtoNL.

        if(realKey != None)
        {
            switch(realKey)
            {
                case Left:
                    write(g_MasterPty, "\e[D", 3);
                    break;
                case Right:
                    write(g_MasterPty, "\e[C", 3);
                    break;
                case Up:
                    write(g_MasterPty, "\e[A", 3);
                    break;
                case Down:
                    write(g_MasterPty, "\e[B", 3);
                    break;
                default:
                    break;
            }
        }
        else if(c & ALT_KEY)
        {
            // ALT escaped key
            c &= 0x7F;
            write(g_MasterPty, "\e", 1);
            write(g_MasterPty, &c, 1);
        }
        else if(c)
        {
            write(g_MasterPty, &c, 1);
        }
    }
}

int main(int argc, char **argv)
{
    syslog(LOG_INFO, "ttyterm: starting up...");

    // New process group for job control. We'll ignore SIGINT for now.
    signal(SIGINT, sigint);
    setsid();

    // Text UI is only vt100-compatible (not an xterm)
    setenv("TERM", "ansi", 1);

    // Get a PTY and the main TTY.
    int tty = open("/dev/textui", O_WRONLY);
    if(tty < 0)
    {
        syslog(LOG_ALERT, "ttyterm: couldn't open /dev/textui: %s", strerror(errno));
        return 1;
    }

    g_MasterPty = posix_openpt(O_RDWR);
    if(g_MasterPty < 0)
    {
        close(tty);
        syslog(LOG_ALERT, "ttyterm: couldn't get a pseudo-terminal to use: %s", strerror(errno));
        return 1;
    }

    /// \todo Assumption of size here.
    struct winsize ptySize;
    ptySize.ws_col = 80;
    ptySize.ws_row = 25;
    ioctl(g_MasterPty, TIOCSWINSZ, &ptySize);

    char slavename[16] = {0};
    strcpy(slavename, ptsname(g_MasterPty));

    // Clear the screen.
    write(tty, "\e[2J", 5);

    // Install our input callback so we can hook that in to the pty as well.
    Input::installCallback(Input::Key, handle_input);

    // Start up child process.
    g_RunningPid = fork();
    if(g_RunningPid == 0)
    {
        close(0);
        close(1);
        close(2);
        close(tty);
        close(g_MasterPty);

        // Open the slave ready for the child.
        int slave = open(slavename, O_RDWR);
        dup2(slave, 1);
        dup2(slave, 2);

        syslog(LOG_INFO, "Starting up 'login' on pty %s", slavename);
        execl("/applications/login", "/applications/login", 0);
        syslog(LOG_ALERT, "Launching login failed (next line is the error in errno...)");
        syslog(LOG_ALERT, strerror(errno));
        exit(1);
    }

    // Main loop - read from PTY master, write to TTY.
    const size_t maxBuffSize = 32768;
    char buffer[maxBuffSize];
    while(1)
    {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(g_MasterPty, &fds);

        int nReady = select(g_MasterPty + 1, &fds, NULL, NULL, NULL);
        if(nReady > 0)
        {
            if(FD_ISSET(g_MasterPty, &fds))
            {
                size_t len = read(g_MasterPty, buffer, maxBuffSize);
                buffer[len] = 0;
                write(tty, buffer, len);
            }
        }
    }

    return 0;
}
