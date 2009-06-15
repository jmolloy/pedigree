#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#include <sys/termios.h>

_BEGIN_STD_C

/* If you change this, please change file-syscalls.cc. */

#define __IOCTL_FIRST 0x1000

#define TIOCGWINSZ  0x1000  /* Get console window size. */
#define TIOCSWINSZ  0x1001  /* Set console window size. */

#define FIONREAD    0x2000  /* Number of bytes available to read */
#define FIONBIO     0x2001  /* Non-blocking? */

#define SIOCATMARK  0x3000  /* Socket at the OOB mark? */

#define __IOCTL_LAST  0x3000

int ioctl(int fildes, int request, void *buff);

_END_STD_C

#endif
