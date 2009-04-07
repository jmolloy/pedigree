#ifndef NEWLIB_H
#define NEWLIB_H

#ifndef COMPILING_SUBSYS
#define COMPILING_SUBSYS
#endif

#ifdef ssize_t
#undef ssize_t
#endif

#ifdef _EXFUN
#undef _EXFUN
#define _EXFUN(x)
#endif

#include <stdio.h>
#include <pwd.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/termios.h>

#endif
