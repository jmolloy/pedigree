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

#include <pwd.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <syslog.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <sys/fb.h>

#include <input/Input.h>

// PID of the process we're running
pid_t g_RunningPid = -1;

// File descriptor for our PTY master.
int g_MasterPty;

#define ALT_KEY (1ULL << 60)
#define SHIFT_KEY (1ULL << 61)
#define CTRL_KEY (1ULL << 62)
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
    pedigree_fb_mode current_mode;
    int fb = open("/dev/fb", O_RDWR);
    if(fb >= 0)
    {
        int result = ioctl(fb, PEDIGREE_FB_GETMODE, &current_mode);
        close(fb);

        if(!result)
        {
            if(current_mode.width && current_mode.height && current_mode.depth)
            {
                syslog(LOG_INFO, "ttyterm: dropping input, currently in graphics mode!");
                return;
            }
        }
    }

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
        else if(c & CTRL_KEY)
        {
            // CTRL-key = unprintable (ie, CTRL-C, CTRL-U)
            c &= 0x1F;
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
            uint32_t utf32 = c & 0xFFFFFFFF;

            // UTF32 -> UTF8
            char buf[4];
            size_t nbuf = 0;
            if (utf32 <= 0x7F)
            {
                buf[0] = utf32&0x7F;
                nbuf = 1;
            }
            else if (utf32 <= 0x7FF)
            {
                buf[0] = 0xC0 | ((utf32>>6) & 0x1F);
                buf[1] = 0x80 | (utf32 & 0x3F);
                nbuf = 2;
            }
            else if (utf32 <= 0xFFFF)
            {
                buf[0] = 0xE0 | ((utf32>>12) & 0x0F);
                buf[1] = 0x80 | ((utf32>>6) & 0x3F);
                buf[2] = 0x80 | (utf32 & 0x3F);
                nbuf = 3;
            }
            else if (utf32 <= 0x10FFFF)
            {
                buf[0] = 0xE0 | ((utf32>>18) & 0x07);
                buf[1] = 0x80 | ((utf32>>12) & 0x3F);
                buf[2] = 0x80 | ((utf32>>6) & 0x3F);
                buf[3] = 0x80 | (utf32 & 0x3F);
                nbuf = 4;
            }

            // UTF8 conversion complete.
            write(g_MasterPty, buf, nbuf);
        }
    }
}

int main(int argc, char **argv)
{
    syslog(LOG_INFO, "ttyterm: starting up...");

    // Create ourselves a lock file so we don't end up getting run twice.
    int fd = open("runtime»/ttyterm.lck", O_WRONLY | O_EXCL | O_CREAT);
    if(fd < 0)
    {
        fprintf(stderr, "ttyterm: lock file exists, terminating.\n");
        return 1;
    }
    close(fd);

    // New process group for job control. We'll ignore SIGINT for now.
    signal(SIGINT, sigint);
    setsid();

    // Ensure we are in fact in text mode.
    int fb = open("/dev/fb", O_RDWR);
    if(fb != 0)
    {
        /// \todo error handling?
        syslog(LOG_INFO, "ttyterm: forcing text mode");
        pedigree_fb_modeset mode = {0, 0, 0};
        int fd = ioctl(fb, PEDIGREE_FB_SETMODE, &mode);
        close(fb);
    }

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

        // Text UI is only vt100-compatible (not an xterm)
        setenv("TERM", "vt100", 1);

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
        FD_SET(tty, &fds);

        int nReady = select(g_MasterPty + 1, &fds, NULL, NULL, NULL);
        if(nReady > 0)
        {
            // Handle incoming data from the PTY.
            if(FD_ISSET(g_MasterPty, &fds))
            {
                size_t len = read(g_MasterPty, buffer, maxBuffSize);
                buffer[len] = 0;
                write(tty, buffer, len);
            }

            // Handle incoming data from the TTY.
            if(FD_ISSET(tty, &fds))
            {
                size_t len = read(tty, buffer, maxBuffSize);
                buffer[len] = 0;
                write(g_MasterPty, buffer, len);
            }
        }
    }

    return 0;
}
