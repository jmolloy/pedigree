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

#ifndef CONSOLE_SYSCALLS_H
#define CONSOLE_SYSCALLS_H

#include <vfs/File.h>

typedef unsigned int cc_t;
typedef unsigned int speed_t;
typedef unsigned int tcflag_t;


#define NCCS 13 // The number of control characters.
#define VEOF   4       /* also VMIN -- thanks, AT&T */
#define VEOL   5       /* also VTIME -- thanks again */
#define VERASE 2
#define VINTR  0
#define VKILL  3
#define VMIN   4       /* also VEOF */
#define VQUIT  1
#define VSUSP  10
#define VTIME  5       /* also VEOL */
#define VSTART 11
#define VSTOP  12

# define TCSAFLUSH	0
# define TCSANOW	1
# define TCSADRAIN	2
# define TCSADFLUSH	3

# define IGNBRK	000001
# define BRKINT	000002
# define IGNPAR	000004
# define INPCK	000020
# define ISTRIP	000040
# define INLCR	000100
# define IGNCR	000200
# define ICRNL	000400
# define IXON	002000
# define IXOFF	010000

# define OPOST	000001
# define OCRNL	000004
# define ONLCR	000010
# define ONOCR	000020
# define TAB3	014000

# define CLOCAL	004000
# define CREAD	000200
# define CSIZE	000060
# define CS5	0
# define CS6	020
# define CS7	040
# define CS8	060
# define CSTOPB	000100
# define HUPCL	002000
# define PARENB	000400
# define PAODD	001000

// Defines for the c_lflag attribute of termios.
#define ECHO   0000010
#define ECHOE  0000020
#define ECHOK  0000040
#define ECHONL 0000100
#define ICANON 0000002
#define IEXTEN 0000400 /* anybody know *what* this does?! */
#define ISIG   0000001
#define NOFLSH 0000200
#define TOSTOP 0001000

typedef struct termios
{
  tcflag_t c_iflag;		///< Has useless properties - ignored.
  tcflag_t c_oflag;		///< Has useless properties - ignored.
  tcflag_t c_cflag;		///< Has useless properties - ignored.
  tcflag_t c_lflag;		///< ECHO,ECHOE,ECHONL recognised.
  cc_t     c_cc[NCCS];
} termios_t;

typedef struct winsize
{
  unsigned short ws_row;
  unsigned short ws_col;
} winsize_t;


int posix_tcgetattr(int fd, struct termios *p);
int posix_tcsetattr(int fd, int optional_actions, struct termios *p);
int console_getwinsize(File file, winsize_t *buf);

#endif
