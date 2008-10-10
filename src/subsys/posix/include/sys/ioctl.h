#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#include <sys/termios.h>

/* If you change this, please change file-syscalls.cc. */

#define __IOCTL_FIRST 0x1000

#define TIOCGWINSZ  0x1000  /* Get console window size. */
#define TIOCSWINSZ  0x1001  /* Set console window size. */

#define __IOCTL_LAST  0x1001  

#endif
